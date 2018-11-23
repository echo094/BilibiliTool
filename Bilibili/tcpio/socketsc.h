/*
在原作者基础上去掉了对MFC库的依赖 将CArray替换为STL列表
添加了客户端类用于建立大量的Socket连接
服务器类CIOCPServer 客户端类CIOCPClient
运行流程：
  LoadSocketLib() 加载SOCKET库
  _InitializeIOCP() 初始化资源
  建立连接
    服务器 _InitializeListenSocket() 打开一个监听端口 然后自动连接客户
    客户端 _InitializeClientSocket() 每次建立一个指定的连接
  执行操作
  关闭连接
  _DeInitialize() 清理所有资源
  UnloadSocketLib() 卸载SOCKET库

-----------------------------------重要-----------------------------------

本文件中的类包含纯虚函数无法直接实例化相应的使用方法可以参考demo中的项目

---------------------------------注意事项---------------------------------

目前所有对连接列表的迭代读和修改都无法多线程同时进行。也就是说添加、删除
连接，发送消息（需要查找列表）以及处理心跳（遍历列表）这几项操作互斥。

服务器可对接入用户进行验证，通过GetAcceptExSockAddrs返回的第一组数据。
目前该数据包限定为不超过100个字符。在连接完成后可调用OnResponse对用户
回应。

使用心跳模块时在初始化前调用SetHeart函数开启功能，然后重载OnHeart函数，该
函数在独立线程中运行。当新的心跳消息触发而上一个OnHeart未处理完成时会丢弃
一次心跳处理。

目前客户端类在连接时若使用ConnectEX方式会出现一些问题。

目前服务器类的监听套接字需要接收到数据后才能返回事件。_CleanOccupiedAcceptIO
方法用于检测监听端口的连接时间，可在心跳验证时调用，当前超时时间设置为16秒。
若改为连接建立后立即返回事件能彻底解决该问题。但是需要新增一个队列处理已连接
但未通过验证的套接字。

----------------------------------分隔线----------------------------------

参考资料：
http://blog.csdn.net/piggyxp/article/details/6922277
http://blog.csdn.net/ylj135cool/article/details/46875799
*/

// 日志输出
#ifdef _DEBUG
#define LOG_STREAM
#endif

#pragma once
#ifndef _TOOLLIB_SOCKETSC_
#define _TOOLLIB_SOCKETSC_

#include <winsock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
#include <assert.h>
#include <list> 

// 缓冲区长度 (1024*8)
// 如果确实客户端发来的每组数据都比较少，那么就设置得小一些，省内存
#define MAX_BUFFER_LEN        8192
// 是否留下一位用于填结束符
#ifdef NONE_ZERO_FLAG
#define VALID_BUFFER_LEN  MAX_BUFFER_LEN
#else
#define VALID_BUFFER_LEN  MAX_BUFFER_LEN - 1
#endif
// 默认端口
#define DEFAULT_PORT          12345    
// 默认IP地址
#define DEFAULT_IP            "127.0.0.1"

#define MSG_IOCPTHREADCLOSE WM_USER + 600

// 在回调函数中区别消息类型
typedef enum _CALLBACK_TYPE
{
	SOCKET_CLOSE,   
	SOCKET_RECEIVE, 
	SOCKET_SEND,    
}CALLBACK_TYPE;
// 方法未执行成功时的返回代码
typedef enum _SOCKET_ERROR_TYPE
{
	SOCKETERR_ERRORCODE_START = -200,
	SOCKETERR_INIT_FAILED,
	SOCKETERR_CONNECT_FAILED,
	SOCKETERR_SEND_PENDING,
	SOCKETERR_SEND_FAILED
}SOCKET_ERROR_TYPE;

	// 在完成端口上投递的I/O操作的类型
	typedef enum _OPERATION_TYPE
	{
		ACCEPT_POSTED,                     // 标志投递的Accept操作
		CONNECT_POSTED,                    // 标志投递的CONNECT操作
		SEND_POSTED,                       // 标志投递的是发送操作
		RECV_POSTED,                       // 标志投递的是接收操作
		NULL_POSTED                        // 用于初始化，无意义

	}OPERATION_TYPE;
	/////////////////////////////////////////////////////////////////
	//单IO数据结构体定义(用于每一个重叠操作的参数)
	//每一个句柄在被投递到接收列表前都会被指定操作类型
	//ACCEPT_POSTED 连接操作调用WSASocket以及AcceptEx
	//SEND_POSTED 发送操作
	//RECV_POSTED 接收操作调用WSARecv
	//IO句柄由SSOCKET类PER_SOCKET_CONTEXT管理
	//归属于监听成员对象的句柄拥有独立的套接字用于连接新客户
	//归属于其它成员对象的句柄只处理所属套接字的收发操作
	//该类在析构时会自动关闭套接字句柄m_sockAccept
	//在普通模式下套接字由PER_SOCKET_CONTEXT先行关闭
	class PER_IO_CONTEXT
	{
	public:
		// 标识网络操作的类型(对应上面的枚举)
		OPERATION_TYPE m_OpType; 
		// 这个网络操作所使用的Socket套接字
		SOCKET         m_sockAccept; 
		// 每一个重叠网络操作的重叠结构(针对每一个Socket的每一个操作，都要有一个)
		OVERLAPPED     m_Overlapped; 
		// WSA类型的缓冲区，用于给重叠操作传参数的
		WSABUF         m_wsaBuf; 
		// 这个是WSABUF里具体存字符的缓冲区
		char           m_szBuffer[MAX_BUFFER_LEN]; 
		// 在一次接收后需要留到下一次处理的字节数
		int            m_occupy; 
	public:
		// 初始化
		PER_IO_CONTEXT();
		// 释放掉Socket
		~PER_IO_CONTEXT();
		// 重置缓冲区内容
		void ResetBuffer();
	};
	/////////////////////////////////////////////////////////////////
	//单套接字句柄成员数据结构体定义
	//一个成员表示一个活跃的Socket连接
	//拥有一个PER_IO_CONTEXT成员列表
	//监听模式下会常驻多个ACCEPT_POSTED操作IO句柄
	//普通模式下会常驻1个RECV_POSTED操作IO句柄
	//该类在析构时先关闭套接字句柄m_Socket然后清空IO列表
	//在连接之后不能删除特定的IO上下文这会使连接失效
	class PER_SOCKET_CONTEXT
	{
	public:
		// 一个连接同一时间只能有一个消息被发送
		bool issending; 
		// 是否需要被清理
		bool isdroped; 
		// 派生类传入的标签
		int label; 
		// 心跳未响应计数
		int heartcount; 
		// 每一个客户端连接的Socket句柄
		SOCKET m_Socket; 
		// 对方的地址
		SOCKADDR_IN m_ClientAddr; 
		// 该连接的交互数据
		std::list<PER_IO_CONTEXT*> m_arrayIoContext; 
		// 也就是说对于每一个客户端Socket，是可以在上面同时投递多个IO请求的
	public:
		// 初始化
		PER_SOCKET_CONTEXT();
		// 释放资源
		~PER_SOCKET_CONTEXT();
		// 获取一个新的IoContext
		PER_IO_CONTEXT* GetNewIoContext();
		// 根据成员指针从列表中移除一个指定的IoContext
		int RemoveContext(PER_IO_CONTEXT* pContext);
	};
	/////////////////////////////////////////////////////////////////
	// 工作者线程的线程参数
	class CIOCPBase;
	typedef struct _tagThreadParams_WORKER
	{
		// 类指针用于调用类中的函数
		CIOCPBase* pIOCPModel; 
		// 线程编号
		int nThreadNo; 

	} THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;
	/////////////////////////////////////////////////////////////////
	//基于IOCP的Socket客户端及服务器的基类无法实例化
	//基类中不含有监听操作相关的函数方法
	//基类中实现了客户端及服务器模式所共有的操作
	class CIOCPBase
	{
	protected:
		// 用来通知线程系统退出的事件使Worker线程快速退出
		HANDLE m_hShutdownEvent; 
		// 用于Worker线程同步的互斥量
		CRITICAL_SECTION m_csContextList; 
		// 完成端口的句柄
		HANDLE m_hIOCompletionPort; 
		// 工作者线程的句柄指针
		HANDLE* m_phWorkerThreads; 
		// 生成的线程数量
		int  m_nThreads; 
		// 各连接中的Socket的Context信息
		std::list<PER_SOCKET_CONTEXT*> m_arrayClientContext; 
		// 是否初始化成功
		bool isinitialized;
		// 线程运行标志
		bool _isworking[2]; 
		// 当前主消息循环
		DWORD _msgthread; 
		// 消息句柄与定时器句柄
		HANDLE _lphandle[2]; 
		// 是否开启心跳检测及应答 默认不开启
		bool _isneedheart; 
		int _heartinterval;

	public:
		CIOCPBase();
		virtual ~CIOCPBase();

	public:
		// 加载Socket库
		int LoadSocketLib();
		// 卸载Socket库
		void UnloadSocketLib() { WSACleanup(); }
		// 配置心跳检测
		void SetHeart(bool value, int interval) {
			_isneedheart = value;
			_heartinterval = interval;
		}
		// 从sockaddr_in结构体读取网络地址信息
		int TransAddrInfo(SOCKADDR_IN* addr, char *str, int len, int &port);

	protected:
		// 获得本机的处理器数量
		int _GetNoOfProcessors();
		// 初始化IOCP
		// maxthreadnum为0表示不限制Worker线程数
		// 若返回失败无需调用清理函数
		int _InitializeIOCP(int maxthreadnum = 0);
		// 将句柄绑定到完成端口中
		int _AssociateWithIOCP(PER_SOCKET_CONTEXT *pContext);
		// 关闭心跳线程
		int _CloseHeartThread();
		// 释放资源在所有连接关闭后执行 会释放初始化操作中所有创建的元素
		virtual int _DeInitialize();
		// 调用shutdown关闭以connect方式建立的连接
		int _DisConnectSocketSoft(PER_SOCKET_CONTEXT* pSocketContext);
		// 调用closehandle关闭以connect方式建立的连接并标记为丢弃
		int _DisConnectSocketHard(std::list<PER_SOCKET_CONTEXT*>::iterator &pitor);

	// IO操作处理函数
	protected:
		// 投递接收数据请求
		int _PostRecv(PER_IO_CONTEXT* pIoContext);
		// 处理和转发接收到的数据
		int _DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen);
		// 投递发送数据请求
		int _PostSend(PER_SOCKET_CONTEXT *pSocket, const char *buf, int len);
		// 处理发送操作的响应
		int _DoSend(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen);
		// 传递给派生类的处理函数
		virtual int OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen);
		virtual int OnSend(PER_SOCKET_CONTEXT* pSocketContext, int byteslen);
		virtual int OnHeart() = 0;

	// 连接信息列表处理函数
	protected:
		// 含迭代器写 添加新的Socket连接信息到列表
		int _AddToContextList(PER_SOCKET_CONTEXT *pSocketContext);
		// 含迭代器写 从列表中移除传入的连接信息成员
		int _RemoveContextByMember(PER_SOCKET_CONTEXT *pSocketContext);
		// 含迭代器写 清空连接信息列表
		int _ClearContextList();
	// Worker线程与函数入口
	protected:
		// 线程函数，为IOCP请求服务的工作者线程
		static DWORD WINAPI _WorkerThreadEntrance(LPVOID lpParam);
		// 工作者线程处理函数
		virtual int _WorkerThread(THREADPARAMS_WORKER* lpParam) = 0;
		// 主线程
		static DWORD WINAPI Thread_IOCPMain(PVOID lpParameter);
		// 心跳处理线程
		static DWORD WINAPI Thread_IOCPHeart(PVOID lpParameter);
	};


	//基于IOCP的Socket服务器的派生类
	//使用AcceptEx和GetAcceptExSockaddrs优化了新客户的连接过程
	class CIOCPServer: public CIOCPBase
	{
	public:
		CIOCPServer();
		virtual ~CIOCPServer();
	private:
		// 用于监听的Socket的Context信息
		PER_SOCKET_CONTEXT*  m_pListenContext;
		// 服务器端的IP地址
		std::string m_strIP;
		// 服务器端的监听端口
		int m_nPort;
		// AcceptEx的函数指针
		LPFN_ACCEPTEX m_lpfnAcceptEx;
		// GetAcceptExSockaddrs的函数指针
		LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockAddrs;

	protected:
		// 初始化Socket监听句柄 
		int _InitializeListenSocket(int port = DEFAULT_PORT, const char *addr = DEFAULT_IP);
		// 释放资源在所有连接关闭后执行
		int _DeInitialize();
		// 重置已连接但未发送数据的连接
		int _CleanOccupiedAcceptIO();
	// IO连接操作处理函数
	protected:
		// 投递Accept请求
		int _PostAccept(PER_IO_CONTEXT* pAcceptIoContext);
		// 在有客户端连入的时候，进行处理
		int _DoAccpet(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);
		// 验证操作 若需要将数据保留到回应阶段 可用len返回需要保留的数据长度
		virtual int OnAccpet(PER_SOCKET_CONTEXT* pSocketContext, char *str, int &len) = 0;
		// 对于新连接的回应
		virtual int OnResponse(PER_SOCKET_CONTEXT* pSocketContext, char *str, int len) = 0;
		virtual int OnClose(PER_SOCKET_CONTEXT* pSocketContext);
		virtual int OnSend(PER_SOCKET_CONTEXT* pSocketContext, int byteslen) { return 0; };
		virtual int OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen);
		virtual int OnHeart() = 0;

	protected:
		// 工作者线程处理函数
		int _WorkerThread(THREADPARAMS_WORKER* lpParam);
		// 判断客户端Socket是否已经断开
		int _IsSocketAlive(SOCKET s);

	public:
		// 启动服务器
		int Start(int port = DEFAULT_PORT, const char *addr = DEFAULT_IP, u_int workerlimit = 0);
		// 停止服务器
		int Stop();
		// 获得本机的IP地址
		std::string GetLocalIP();
		// 设置监听端口
		void SetPort(const int& nPort) { m_nPort = nPort; }

	};

	//客户端收到消息后向非派生类发送处理通知的方法
	class CIOCPClientCallback
	{
	public:
		// 第一个整数表示消息类型为枚举量_CALLBACK_TYPE
		virtual int OnNotify(int, PER_SOCKET_CONTEXT* , PER_IO_CONTEXT* ) = 0;
	};

	//基于IOCP的Socket客户端的派生类
	//对于并发连接复数个服务器做了优化
	//采用IOCP模式必须使用ConnectEx进行连接
	class CIOCPClient: public CIOCPBase
	{
	public:
		CIOCPClient();
		virtual ~CIOCPClient();

	private:
		// 外部类消息回调
		CIOCPClientCallback * m_IOCPClientCallback;
	// 各类型操作的自定义处理
	protected:
		virtual int OnClose(PER_SOCKET_CONTEXT* pSocketContext);
		virtual int OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen) override;
		virtual int OnSend(PER_SOCKET_CONTEXT* pSocketContext, int byteslen) override { return 0; };
		virtual int OnHeart() override = 0;

	protected:
		// 释放资源在所有连接关闭后执行
		int _DeInitialize();
		// 以connect方式建立一个连接 连接成功则传出Socket上下文供发送第一次数据
		int _ConnectSocket(PER_SOCKET_CONTEXT **psocketout, int pport, const char *paddr, int plabel = 0);
		// 以ConnectEX方式建立一个连接
		int _PostConnect(int pport, const char *paddr, int plabel = 0);
		// 在有客户端连入的时候，进行处理
		int _DoConnect(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext);
		// 工作者线程处理函数
		int _WorkerThread(THREADPARAMS_WORKER* lpParam);

	public:
		// 设置回调函数指针
		int SetCallback(CIOCPClientCallback *);
		// 初始化资源环境 该函数非线程安全
		int Initialize(int maxthreadnum);
		// 清理所有资源 该函数非线程安全
		int Destory();
		// 开启一个连接 该函数线程安全
		int Connect(int pport, const char *paddr, int plabel = 0, const char * str = NULL, int len = 0);
		// 含迭代器读 软断开一个特定的连接
		int CloseConnectionByLabel(int plabel);
		// 含迭代器读 软断开所有连接
		int CloseConnections();
	};

#endif
