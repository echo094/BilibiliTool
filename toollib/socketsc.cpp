#include "stdafx.h"
#include "socketsc.h"
#include <ws2tcpip.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
using namespace toollib;

// 每一个处理器上产生多少个线程(为了最大限度的提升服务器性能
#define WORKER_THREADS_PER_PROCESSOR	2
// 最小的Worker线程数量
#define WORKER_THREADS_MINIMUN	4
// 同时投递的Accept请求的数量(这个要根据实际的情况灵活设置)
#define MAX_POST_ACCEPT	10
// 传递给Worker线程的退出信号
#define EXIT_CODE	NULL
// 释放指针和句柄资源的宏
// 释放指针宏
#define RELEASE(x)	{if(x != NULL ){delete x;x=NULL;}}
// 释放句柄宏
#define RELEASE_HANDLE(x)	{if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
// 释放Socket宏
#define RELEASE_SOCKET(x)	{if(x !=INVALID_SOCKET) { closesocket(x);x=INVALID_SOCKET;}}
/////////////////////////////////////////////////////////////////
PER_IO_CONTEXT::PER_IO_CONTEXT()
{
	ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
	ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
	m_sockAccept = INVALID_SOCKET;
	m_wsaBuf.buf = m_szBuffer;
	m_wsaBuf.len = VALID_BUFFER_LEN;
	m_OpType = NULL_POSTED;
	m_occupy = 0;
}

PER_IO_CONTEXT::~PER_IO_CONTEXT()
{
	if (m_sockAccept != INVALID_SOCKET)
	{
		closesocket(m_sockAccept);
		m_sockAccept = INVALID_SOCKET;
	}
}

void PER_IO_CONTEXT::ResetBuffer()
{
	ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
}

/////////////////////////////////////////////////////////////////

PER_SOCKET_CONTEXT::PER_SOCKET_CONTEXT()
{
	issending = false;
	isdroped = false;
	label = 0;
	heartcount = 0;
	m_Socket = INVALID_SOCKET;
	memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
}

PER_SOCKET_CONTEXT::~PER_SOCKET_CONTEXT()
{
	if (m_Socket != INVALID_SOCKET)
	{
		/*
		struct linger opt;
		opt.l_onoff = 1;
		opt.l_linger = 0;
		setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&opt, sizeof(linger));
		*/
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}
	// 释放掉所有的IO上下文数据
	std::list<PER_IO_CONTEXT*>::iterator itor;
	for (itor = m_arrayIoContext.begin(); itor!= m_arrayIoContext.end();)
	{
		delete *itor;
		itor = m_arrayIoContext.erase(itor);
	}
}

PER_IO_CONTEXT* PER_SOCKET_CONTEXT::GetNewIoContext()
{
	PER_IO_CONTEXT* p = new PER_IO_CONTEXT;
	m_arrayIoContext.push_back(p);
	return p;
}

int PER_SOCKET_CONTEXT::RemoveContext(PER_IO_CONTEXT* pContext)
{
	if (pContext == NULL)
		return -1;
	std::list<PER_IO_CONTEXT*>::iterator itor;
	for (itor = m_arrayIoContext.begin(); itor != m_arrayIoContext.end(); itor++)
	{
		if (pContext == *itor)
		{
			delete pContext;
			pContext = NULL;
			itor = m_arrayIoContext.erase(itor);
			return 0;
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////

CIOCPBase::CIOCPBase()
{
	m_hShutdownEvent = NULL;
	m_hIOCompletionPort = NULL;
	m_phWorkerThreads = NULL;
	m_nThreads = 0;
	isinitialized = false;

	_msgthread = 0;
	memset(_isworking, 0, sizeof(_isworking));
	_isneedheart = false;
}

CIOCPBase::~CIOCPBase()
{

}

int CIOCPBase::LoadSocketLib()
{
	WSADATA wsaData;
	int ret;
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 错误(一般都不可能出现)
	if (NO_ERROR != ret)
	{
#ifdef LOG_STREAM
		printf("[CIOCPBase][INFO] Initialize WinSock 2.2 failed! \n");
#endif
		return -1;
	}
	return 0;
}

int CIOCPBase::TransAddrInfo(SOCKADDR_IN* addr, char *str, int len, int &port)
{
	str[0] = 0;
	port = ntohs(addr->sin_port);
	const char *ptr = inet_ntop(AF_INET, &(addr->sin_addr), str, len);
	if (ptr == NULL) {
		str[0] = 0;
		return -1;
	}
	return 0;
}

// 获得本机中处理器的数量
int CIOCPBase::_GetNoOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

// 初始化完成端口
int CIOCPBase::_InitializeIOCP(int maxthreadnum)
{
	if (isinitialized)
		return 0;
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] Initialize start... \n");
#endif
	// 初始化线程互斥量
	InitializeCriticalSection(&m_csContextList);
	// 建立系统退出的事件通知
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//建立第一个完成端口
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIOCompletionPort){
#ifdef LOG_STREAM
		printf("[CIOCPBase][ERROR]Create IoCompletionPort failed! \n");
#endif
		//关闭系统退出事件句柄
		RELEASE_HANDLE(m_hShutdownEvent);
		// 删除客户端列表的互斥量
		DeleteCriticalSection(&m_csContextList);
		return -1;
	}
	// 根据本机中的处理器数量，建立对应的线程数
	m_nThreads = WORKER_THREADS_PER_PROCESSOR * _GetNoOfProcessors();
	if (m_nThreads < WORKER_THREADS_MINIMUN)
		m_nThreads = WORKER_THREADS_MINIMUN;
	//是否有额外限制
	if (maxthreadnum)
		m_nThreads = maxthreadnum;
	// 为工作者线程初始化句柄
	m_phWorkerThreads = new HANDLE[m_nThreads];

	// 根据计算出来的数量建立工作者线程
	DWORD nThreadID;
	for (int i = 0; i < m_nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->pIOCPModel = this;
		pThreadParams->nThreadNo = i + 1;
		m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThreadEntrance, (void *)pThreadParams, 0, &nThreadID);
	}
	//开启心跳模块
	if (_isneedheart) {
		_lphandle[0] = CreateThread(NULL, 0, Thread_IOCPMain, this, 0, &_msgthread);
		while (!_isworking[0])
			Sleep(100);
	}
	isinitialized = true;
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] Initialize end and create %d _WorkerThread. \n", m_nThreads);
#endif
	return 0;
}

int CIOCPBase::_AssociateWithIOCP(PER_SOCKET_CONTEXT *pContext)
{
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pContext->m_Socket, m_hIOCompletionPort, (DWORD)pContext, 0);
	if (NULL == hTemp){
		return WSAGetLastError();
	}
	return 0;
}

//关闭心跳线程
int CIOCPBase::_CloseHeartThread()
{
	//关闭心跳模块
	if (_isworking[0]) {
		CloseHandle(_lphandle[0]);
		PostThreadMessage(_msgthread, MSG_IOCPTHREADCLOSE, WPARAM(0), LPARAM(0));
		while (_isworking[0])
			Sleep(100);
	}
	_isneedheart = false;
	return 0;
}

//释放资源在所有连接关闭后执行
int CIOCPBase::_DeInitialize() {
	if (!isinitialized) {
		return 0;
	}

	//先关闭心跳线程 如果在之前已调用会直接返回
	this->_CloseHeartThread();

	// 激活关闭消息通知
	SetEvent(m_hShutdownEvent);
	for (int i = 0; i < m_nThreads; i++) {
		// 通知所有的完成端口操作退出
		PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
	}
	// 等待所有的客户端资源退出
	WaitForMultipleObjects(m_nThreads, m_phWorkerThreads, TRUE, INFINITE);
	// 释放工作者线程句柄指针
	for (int i = 0; i<m_nThreads; i++) {
		RELEASE_HANDLE(m_phWorkerThreads[i]);
	}
	RELEASE(m_phWorkerThreads);

	// 清除客户端列表信息
	_ClearContextList();

	//关闭IOCP句柄
	RELEASE_HANDLE(m_hIOCompletionPort);
	//关闭系统退出事件句柄
	RELEASE_HANDLE(m_hShutdownEvent);
	// 删除客户端列表的互斥量
	DeleteCriticalSection(&m_csContextList);
	
	isinitialized = false;

#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] DeInitialize end. \n");
#endif
	return 0;
}

//调用shutdown关闭以connect方式建立的连接
int CIOCPBase::_DisConnectSocketSoft(PER_SOCKET_CONTEXT* pSocketContext) {
	pSocketContext->isdroped = true;
	shutdown(pSocketContext->m_Socket, SD_BOTH);
	return 0;
}

//调用closehandle关闭以connect方式建立的连接并标记为丢弃
int CIOCPBase::_DisConnectSocketHard(std::list<PER_SOCKET_CONTEXT*>::iterator &pitor) {
	(*pitor)->isdroped = true;
	closesocket((*pitor)->m_Socket);
	(*pitor)->m_Socket = INVALID_SOCKET;

	return 0;
}

//投递接收数据请求
int CIOCPBase::_PostRecv(PER_IO_CONTEXT* pIoContext)
{
	//初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	pIoContext->m_wsaBuf.buf = &pIoContext->m_szBuffer[pIoContext->m_occupy];
	pIoContext->m_wsaBuf.len = VALID_BUFFER_LEN - pIoContext->m_occupy;
	WSABUF *p_wbuf = &pIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pIoContext->m_Overlapped;

	//pIoContext->ResetBuffer();
	pIoContext->m_OpType = RECV_POSTED;

	//初始化完成后，，投递WSARecv请求
	int nBytesRecv = WSARecv(pIoContext->m_sockAccept, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL);
	//如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if (SOCKET_ERROR == nBytesRecv)
	{
		DWORD err = WSAGetLastError();
		if (WSA_IO_PENDING != err) {
#ifdef LOG_STREAM
			printf("[CIOCPBase][ERROR]%d Post RECV IO failed. \n", err);
#endif
			return err;
		}
	}
	return 0;
}

//处理和转发接收到的数据
int CIOCPBase::_DoRecv(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen)
{
	OnReceive(pSocketContext, pIoContext, byteslen);
	//开始投递下一个WSARecv请求
	return _PostRecv(pIoContext);
}

//投递发送数据请求
int CIOCPBase::_PostSend(PER_SOCKET_CONTEXT *pSocket, const char *buf, int len)
{
	int ret;
	//有消息正在发送则直接退出
	if (pSocket->issending)
		return SOCKETERR_SEND_PENDING;
	//套接字失效则不发送
	if (pSocket->m_Socket == INVALID_SOCKET)
		return SOCKETERR_SEND_FAILED;
	pSocket->issending = true;

	//寻找是否存在标记为SEND_POSTED的IO
	PER_IO_CONTEXT* pContext = NULL;
	std::list<PER_IO_CONTEXT*>::iterator itor;
	for (itor = pSocket->m_arrayIoContext.begin(); itor != pSocket->m_arrayIoContext.end(); itor++)
	{
		if ((*itor)->m_OpType == SEND_POSTED)
		{
			pContext = *itor;
			break;
		}
	}
	if (pContext == NULL) {
		pContext = pSocket->GetNewIoContext();
		pContext->m_OpType = SEND_POSTED;
		pContext->m_sockAccept = pSocket->m_Socket;
	}
	//拷贝数据
	pContext->ResetBuffer();
	memcpy_s(pContext->m_szBuffer, MAX_BUFFER_LEN, buf, len);
	//初始化变量
	WSABUF *p_wbuf = &pContext->m_wsaBuf;
	p_wbuf->buf = pContext->m_szBuffer;
	p_wbuf->len = len;
	DWORD dwBytes = 0;//发送的字节数
	DWORD dwFlags = 0;
	OVERLAPPED *p_ol = &pContext->m_Overlapped;
	//发送数据
	ret = WSASend(pContext->m_sockAccept, p_wbuf, 1, &dwBytes, 0, p_ol, NULL);
	//如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == ret) && (WSA_IO_PENDING != WSAGetLastError()))
	{
#ifdef LOG_STREAM
		DWORD err = WSAGetLastError();
		printf("[CIOCPBase][ERROR]%d Post SEND IO failed. \n", err);
#endif
		return SOCKETERR_SEND_FAILED;
	}
	return 0;
}

//处理发送操作的响应
int CIOCPBase::_DoSend(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen)
{
	OnSend(pSocketContext, byteslen);
	pSocketContext->issending = false;
	return 0;
}

int CIOCPBase::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen)
{
#ifdef LOG_STREAM
	char str[INET_ADDRSTRLEN] = "";
	int port; 
	TransAddrInfo(&pSocketContext->m_ClientAddr, str, INET_ADDRSTRLEN, port);
	printf("[CIOCPBase][INFO] RECV %s:%d MSG:%s \n", str, port, pIoContext->m_wsaBuf.buf);
#endif
	return 0;
}

int CIOCPBase::OnSend(PER_SOCKET_CONTEXT* pSocketContext, int byteslen)
{
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] SEND RES \n");
#endif
	return 0;
}

//添加新的Socket连接信息到列表m_arrayClientContext
//在Accept和Connect函数中被调用
int CIOCPBase::_AddToContextList(PER_SOCKET_CONTEXT *pHandleData) {
	EnterCriticalSection(&m_csContextList);
	m_arrayClientContext.push_back(pHandleData);
	LeaveCriticalSection(&m_csContextList);
	return 0;
}

//从列表m_arrayClientContext中移除传入的连接信息成员
//delete操作触发的析构函数会自动清理socket句柄
//目前只被Worker线程调用
int CIOCPBase::_RemoveContextByMember(PER_SOCKET_CONTEXT *pSocketContext) {
	//列表的end()函数会指向一个无效对象
	//当列表为空时begin()函数会同样指向该无效对象
	EnterCriticalSection(&m_csContextList);
	std::list<PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end(); m_itor++) {
		if (pSocketContext == *m_itor) {
			RELEASE(pSocketContext);
			m_itor = m_arrayClientContext.erase(m_itor);
			break;
		}
	}
	LeaveCriticalSection(&m_csContextList);
	return 0;
}

//清空连接信息列表
//delete操作触发的析构函数会自动清理socket句柄
//在关闭函数中被调用
int CIOCPBase::_ClearContextList() {
	EnterCriticalSection(&m_csContextList);
	// 释放掉所有的IO上下文数据
	std::list<PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end(); m_itor++) {
		RELEASE(*m_itor);
	}
	m_arrayClientContext.clear();
	LeaveCriticalSection(&m_csContextList);
	return 0;
}

DWORD WINAPI CIOCPBase::_WorkerThreadEntrance(LPVOID lpParam)
{
	THREADPARAMS_WORKER* pParam = (THREADPARAMS_WORKER*)lpParam;
	CIOCPBase* pIOCPModel = (CIOCPBase*)pParam->pIOCPModel;
	int nThreadNo = (int)pParam->nThreadNo;
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] _WorkerThread %d start successfully. \n", nThreadNo);
#endif
	//进入派生类的处理函数
	int ret;
	ret = pIOCPModel->_WorkerThread(pParam);
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] _WorkerThread %d exit successfully. \n", nThreadNo);
#endif
	// 释放线程参数
	RELEASE(lpParam);
	return 0;
}

//监视线程
DWORD CIOCPBase::Thread_IOCPMain(PVOID lpParameter)
{
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] Main Thread Start!\n");
#endif
	CIOCPBase *pclass = (CIOCPBase *)lpParameter;
	pclass->_isworking[0] = true;
	int bRet = 0;
	MSG msg;
	//开启心跳定时器
	DWORD hearttimer = SetTimer(NULL, 10, pclass->_heartinterval, NULL);
	while (pclass->_isworking[0])
	{
		if ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if (msg.message == MSG_IOCPTHREADCLOSE)
			{
				KillTimer(NULL, hearttimer);
				hearttimer = 0;
				//等待心跳线程退出
				while (pclass->_isworking[1])
					Sleep(100);
				pclass->_isworking[0] = false;
#ifdef LOG_STREAM
				printf("[CIOCPBase][INFO] Main Thread Stop!\n");
#endif
				return 0;
			}
			if (msg.message == WM_TIMER)
			{
				if ((msg.wParam == hearttimer) && (!pclass->_isworking[1])) {
					pclass->_lphandle[1] = CreateThread(NULL, 0, pclass->Thread_IOCPHeart, pclass, 0, 0);
					CloseHandle(pclass->_lphandle[1]);
				}
			}
		}
		Sleep(0);
	}
	return 0;
}

//心跳处理线程
DWORD CIOCPBase::Thread_IOCPHeart(PVOID lpParameter)
{
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] Heart Thread Start!\n");
#endif
	CIOCPBase *pclass = (CIOCPBase *)lpParameter;
	pclass->_isworking[1] = true;
	pclass->OnHeart();
	pclass->_isworking[1] = false;
#ifdef LOG_STREAM
	printf("[CIOCPBase][INFO] Heart Thread Stop!\n");
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////
CIOCPServer::CIOCPServer() 
{
	m_strIP = DEFAULT_IP;
	m_nPort = DEFAULT_PORT;
	m_lpfnAcceptEx = NULL;
	m_pListenContext = NULL;
}

CIOCPServer::~CIOCPServer() {
	Stop();
}

//初始化Socket监听句柄
int CIOCPServer::_InitializeListenSocket(int port, const char *addr)
{
#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] Initialize: Creating Listen PORT... \n");
#endif

	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;

	int ret;
	char errstr[100] = "";
	// 生成用于监听的Socket的信息
	m_pListenContext = new PER_SOCKET_CONTEXT;
	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	m_pListenContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket) {
		sprintf_s(errstr, "[CIOCPServer][ERROR]%d Initialize failed! \n", WSAGetLastError());
		goto errhandle1;
	}
	// 将Listen Socket绑定至完成端口中
	ret = _AssociateWithIOCP(m_pListenContext);
	if (ret) {
		sprintf_s(errstr, "[CIOCPServer][ERROR]%d Listen CreateIoCompletionPort failed! \n", ret);
		goto errhandle1;
	}

	// 服务器地址信息，用于绑定Socket
	struct sockaddr_in taddrs;
	// 填充地址信息
	ZeroMemory((char *)&taddrs, sizeof(taddrs));
	taddrs.sin_family = AF_INET;
	// 这里可以绑定任何可用的IP地址，或者绑定一个指定的IP地址 
	// ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);   
	m_strIP = addr;
	inet_pton(AF_INET, m_strIP.c_str(), (void *)&(taddrs.sin_addr));
	m_nPort = port;
	taddrs.sin_port = htons(m_nPort);
	// 绑定地址和端口
	ret = bind(m_pListenContext->m_Socket, (struct sockaddr *) &taddrs, sizeof(taddrs));
	if (SOCKET_ERROR == ret) {
		strcpy_s(errstr, "[CIOCPServer][ERROR] Bind failed! \n");
		goto errhandle1;
	}
	// 开始进行监听
	ret = listen(m_pListenContext->m_Socket, SOMAXCONN);
	if (SOCKET_ERROR == ret) {
		strcpy_s(errstr, "[CIOCPServer][ERROR] Listen failed! \n");
		goto errhandle1;
	}

	// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
	// 所以需要额外获取一下函数的指针，
	// 获取AcceptEx函数指针
	dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		sprintf_s(errstr, "[CIOCPServer][ERROR]%d WSAIoctl get AcceptEx pointer failed! \n", WSAGetLastError());
		goto errhandle1;
	}
	// 获取GetAcceptExSockAddrs函数指针，也是同理
	if (SOCKET_ERROR == WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		sprintf_s(errstr, "[CIOCPServer][ERROR]%d WSAIoctl get GuidGetAcceptExSockAddrs pointer failed! \n", WSAGetLastError());
		goto errhandle1;
	}

	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	for (int i = 0; i<MAX_POST_ACCEPT; i++)
	{
		// 新建一个IO_CONTEXT
		PER_IO_CONTEXT* pAcceptIoContext = m_pListenContext->GetNewIoContext();
		if (_PostAccept(pAcceptIoContext)) {
			// 投递IO失败
			goto errhandle1;
		}
	}

#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] Initialize end and Post AcceptEx request succeeeded. \n");
#endif
	return 0;

errhandle1:
	//关闭监听Socket句柄
	RELEASE(m_pListenContext);
	if (strlen(errstr))
		printf(errstr);
	return -1;
}

//释放资源在所有连接关闭后执行
int CIOCPServer::_DeInitialize()
{
#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] DeInitialize start... \n");
#endif
	// 调用基类的清理函数
	CIOCPBase::_DeInitialize();
	// 关闭监听Socket句柄
	RELEASE(m_pListenContext);

#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] DeInitialize end. \n");
#endif
	return 0;
}

//重置已连接但未发送数据的连接
//没有连接时时间为-1
int CIOCPServer::_CleanOccupiedAcceptIO()
{
	int time = 0;
	int res, len = sizeof(int);
	std::list<PER_IO_CONTEXT*>::iterator itor;
	for (itor = m_pListenContext->m_arrayIoContext.begin(); itor != m_pListenContext->m_arrayIoContext.end(); itor++)
	{
		res = getsockopt((*itor)->m_sockAccept, SOL_SOCKET, SO_CONNECT_TIME, (char*)&time, &len);
		if (time > 16) {
			//超时则直接断开
#ifdef LOG_STREAM
			printf("[CIOCPServer][WARN] Close listening socket of no data. \n");
#endif
			closesocket((*itor)->m_sockAccept);
			_PostAccept((*itor));
		}
	}
	return 0;
}

//投递Accept请求
//为该IO类成员创建一个新的SOCKET句柄
//将该句柄投递到AcceptEx中等待客户端接入
int CIOCPServer::_PostAccept(PER_IO_CONTEXT* pAcceptIoContext)
{
	assert(INVALID_SOCKET != m_pListenContext->m_Socket);
	// 准备参数
	DWORD dwBytes = 0;
	pAcceptIoContext->m_OpType = ACCEPT_POSTED;
	WSABUF *p_wbuf = &pAcceptIoContext->m_wsaBuf;
	OVERLAPPED *p_ol = &pAcceptIoContext->m_Overlapped;

	// 为以后新连入的客户端先准备好Socket( 这个是与传统accept最大的区别 ) 
	pAcceptIoContext->m_sockAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoContext->m_sockAccept) {
#ifdef LOG_STREAM
		printf("[CIOCPServer][ERROR]%d Accept socket handle Initialize failed! \n", WSAGetLastError());
#endif
		return -1;
	}
	// 投递AcceptEx
	if (FALSE == m_lpfnAcceptEx(m_pListenContext->m_Socket, pAcceptIoContext->m_sockAccept, p_wbuf->buf, p_wbuf->len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, p_ol))
	{
		int err = WSAGetLastError();
		if (WSA_IO_PENDING != err)
		{
#ifdef LOG_STREAM
			printf("[CIOCPServer][ERROR]%d Post acceptEx request failed! \n", err);
#endif
			return -1;
		}
	}

	return 0;
}

//传入的PER_IO_CONTEXT属于ListenSocket
//需要将Socket套接字等信息复制到新建的PER_SOCKET_CONTEXT中并建立自己的PER_IO_CONTEXT
//最后调用_PostAccept给传入的Context申请一个新的Socket套接字继续投递下一个Accept请求
int CIOCPServer::_DoAccpet(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext)
{
	int ret;
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
	//首先用GetAcceptExSockAddrs取得连入客户端的地址信息和第一组数据
	m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);

	//将pIoContext中已建立连接的套接字转移给新的PER_SOCKET_CONTEXT成员
	PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
	pNewSocketContext->m_Socket = pIoContext->m_sockAccept;
	memcpy(&(pNewSocketContext->m_ClientAddr), ClientAddr, sizeof(SOCKADDR_IN));
	//将Buffer拷贝出来用于客户端验证
	int bufflen = 100;
	char recvstr[100];
	memcpy(recvstr, pIoContext->m_szBuffer, bufflen);

	//重新初始化传入的IoContext
	pIoContext->ResetBuffer();
	ret = _PostAccept(pIoContext);

	//进行额外的验证
	ret = OnAccpet(pNewSocketContext, recvstr, bufflen);
	//如果验证失败则关闭连接
	if (ret){
		RELEASE(pNewSocketContext);
		return -1;
	}

	//验证成功则建立连接
	//将这个Socket句柄和完成端口绑定
	ret = _AssociateWithIOCP(pNewSocketContext);
	if (ret){
#ifdef LOG_STREAM
		printf("[CIOCPServer][ERROR]%d Add client to CreateIoCompletionPort failed! \n", ret);
#endif
		//绑定失败的情况
		RELEASE(pNewSocketContext);
		return -1;
	}

	//建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
	//将新IoContext的套接字设为该Socket连接的套接字
	PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_sockAccept = pNewSocketContext->m_Socket;

	// 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
	ret = _PostRecv(pNewIoContext);
	if (ret){
		pNewSocketContext->RemoveContext(pNewIoContext);
		return -1;
	}
	//投递成功则将该连接信息添加到连接列表
	ret = _AddToContextList(pNewSocketContext);

	//连接成功后的回应
	ret = OnResponse(pNewSocketContext, recvstr, bufflen);

	return 0;
}

int CIOCPServer::OnClose(PER_SOCKET_CONTEXT* pSocketContext)
{
#ifdef LOG_STREAM
	char str[INET_ADDRSTRLEN] = "";
	int port;
	TransAddrInfo(&pSocketContext->m_ClientAddr, str, INET_ADDRSTRLEN, port);
	printf("[CIOCPServer][INFO] Close %s:%d \n", str, port);
#endif
	return 0;
}

int CIOCPServer::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen)
{
	CIOCPBase::OnReceive(pSocketContext, pIoContext, byteslen);
	_PostSend(pSocketContext, pIoContext->m_wsaBuf.buf, pIoContext->m_wsaBuf.len);
	return 0;
}

//工作者线程处理函数
int CIOCPServer::_WorkerThread(THREADPARAMS_WORKER* lpParam)
{
	OVERLAPPED           *pOverlapped = NULL;
	PER_SOCKET_CONTEXT   *pSocketContext = NULL;
	DWORD                dwBytesTransfered = 0;
	// 循环处理请求，知道接收到Shutdown信息为止
	while (WAIT_OBJECT_0 != WaitForSingleObject(m_hShutdownEvent, 0))
	{
		//获取一个消息
		//pSocketContext为产生消息的Socket句柄
		//pOverlapped归属于一个已有的PER_IO_CONTEXT成员
		BOOL bReturn = GetQueuedCompletionStatus(
			m_hIOCompletionPort,
			&dwBytesTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);

		// 如果收到的是退出命令则直接退出
		if (EXIT_CODE == (DWORD)pSocketContext){
			break;
		}
		// 判断是否出现了错误
		if (!bReturn){
			DWORD err = GetLastError();
#ifdef LOG_STREAM
			printf("[CIOCPServer][ERROR]%d WORKER Get Queued MSG Failed. \n", err);
#endif

			//释放掉对应的资源
			if (err == 64) {
				OnClose(pSocketContext);
				_RemoveContextByMember(pSocketContext);
			}
			continue;
		}
		//读取传入的参数
		//由pOverlapped的内存地址获取所属类PER_IO_CONTEXT的指针
		PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT, m_Overlapped);
		//连接断开的情况
		if ((0 == dwBytesTransfered) && (RECV_POSTED == pIoContext->m_OpType || SEND_POSTED == pIoContext->m_OpType))
		{
			OnClose(pSocketContext);
			// 释放掉对应的资源
			_RemoveContextByMember(pSocketContext);
			continue;
		}
		//其它情况
		switch (pIoContext->m_OpType)
		{
		case ACCEPT_POSTED:{
			_DoAccpet(pSocketContext, pIoContext);
			break;
		}
		case RECV_POSTED:{
			_DoRecv(pSocketContext, pIoContext, dwBytesTransfered);
			break;
		}
		case SEND_POSTED:{
			_DoSend(pSocketContext, pIoContext, dwBytesTransfered);
			break;
		}
		default:
			// 不应该执行到这里
#ifdef LOG_STREAM
			printf("[CIOCPServer][ERROR] Worker unknown OpType %d. \n", pIoContext->m_OpType);
#endif
			break;
		}
	}//while

	return 0;
}

// 判断客户端Socket是否已经断开，否则在一个无效的Socket上投递WSARecv操作会出现异常
// 使用的方法是尝试向这个socket发送数据，判断这个socket调用的返回值
// 因为如果客户端网络异常断开(例如客户端崩溃或者拔掉网线等)的时候，服务器端是无法收到客户端断开的通知的
int CIOCPServer::_IsSocketAlive(SOCKET s)
{
	int nByteSent = send(s, "", 0, 0);
	if (-1 == nByteSent)
		return -1;
	return 0;
}

int CIOCPServer::Start(int port, const char *addr, u_int workerlimit)
{
	if (isinitialized)
		return 0;
	int ret;
	// 初始化IOCP
	ret = _InitializeIOCP(workerlimit);
	if (ret){
		return -1;
	}
	// 初始化Socket
	ret = _InitializeListenSocket(port, addr);
	if (ret){
#ifdef LOG_STREAM
		printf("[CIOCPServer][ERROR] CIOCPServer start failed! \n");
#endif
		_DeInitialize();
		return -1;
	}
#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] CIOCPServer Ready... \n");
#endif
	return 0;
}

int CIOCPServer::Stop()
{
	if (!isinitialized)
		return 0;
	// 关闭心跳线程
	this->_CloseHeartThread();
	// 释放资源
	_DeInitialize();
#ifdef LOG_STREAM
	printf("[CIOCPServer][INFO] CIOCPServer Stop. \n");
#endif

	return 0;
}

std::string CIOCPServer::GetLocalIP()
{
	// 获得本机主机名
	char hostname[MAX_PATH] = { 0 };
	gethostname(hostname, MAX_PATH);
	// 取得IP结构体
	int ret;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo(hostname, NULL, &hints, &res);
	if (ret) {
		return DEFAULT_IP;
	}
	struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
	// 将IP地址转化成字符串形式
	char str[INET_ADDRSTRLEN] = "";
	int port;
	TransAddrInfo(addr, str, INET_ADDRSTRLEN, port);
	m_strIP = str;

	return m_strIP;
}

/////////////////////////////////////////////////////////////////
CIOCPClient::CIOCPClient()
{
	m_IOCPClientCallback = NULL;
}

CIOCPClient::~CIOCPClient()
{
	Destory();
}

int CIOCPClient::OnClose(PER_SOCKET_CONTEXT* pSocketContext)
{
	if (m_IOCPClientCallback)
		m_IOCPClientCallback->OnNotify(SOCKET_CLOSE, pSocketContext, NULL);
	else {
#ifdef LOG_STREAM
		char str[INET_ADDRSTRLEN] = "";
		int port;
		TransAddrInfo(&pSocketContext->m_ClientAddr, str, INET_ADDRSTRLEN, port);
		printf("[CIOCPClient][INFO] Close %s:%d Label:%d \n", str, port, pSocketContext->label);
#endif
	}

	return 0;
}

int CIOCPClient::OnReceive(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext, int byteslen)
{
	if (m_IOCPClientCallback)
		m_IOCPClientCallback->OnNotify(SOCKET_RECEIVE, pSocketContext, pIoContext);
	else {
#ifdef LOG_STREAM
		char str[INET_ADDRSTRLEN] = "";
		int port;
		TransAddrInfo(&pSocketContext->m_ClientAddr, str, INET_ADDRSTRLEN, port);
		printf("[CIOCPClient][INFO] RECV %s:%d MSG:%s \n", str, port, pIoContext->m_wsaBuf.buf);
#endif
	}

	return 0;
}

//释放资源在所有连接关闭后执行
int CIOCPClient::_DeInitialize()
{
#ifdef LOG_STREAM
	printf("[CIOCPClient][INFO] DeInitialize Start... \n");
#endif
	//调用基类的清理函数
	CIOCPBase::_DeInitialize();

#ifdef LOG_STREAM
	printf("[CIOCPClient][INFO] DeInitialize ended. \n");
#endif
	return 0;
}

//以connect方式建立一个连接
int CIOCPClient::_ConnectSocket(PER_SOCKET_CONTEXT **psocketout, int pport, const char *paddr, int plabel)
{
	int ret;
	*psocketout = NULL;
	//获取IP地址
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = 0;
	hints.ai_family = AF_INET;//IPV4
	hints.ai_socktype = SOCK_STREAM;//TCP
	ret = getaddrinfo(paddr, NULL, &hints, &res);
	if (ret) {
		return -1;
	}

	// 生成用于连接的Socket的信息
	PER_SOCKET_CONTEXT * pConnectContext = new PER_SOCKET_CONTEXT;
	pConnectContext->label = plabel;
	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	pConnectContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pConnectContext->m_Socket) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]%d Create socket failed! \n", WSAGetLastError());
#endif
		RELEASE(pConnectContext);
		return -1;
	}
	// 服务器地址信息，用于连接
	ZeroMemory((char *)&(pConnectContext->m_ClientAddr), sizeof(sockaddr_in));
	pConnectContext->m_ClientAddr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	pConnectContext->m_ClientAddr.sin_family = AF_INET;
	pConnectContext->m_ClientAddr.sin_port = htons(pport);
	//开始连接
	ret = connect(pConnectContext->m_Socket, (sockaddr *)( &pConnectContext->m_ClientAddr), sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]SOCKET connect failed! \n");
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	//将Socket绑定至完成端口中
	ret = _AssociateWithIOCP(pConnectContext);
	if (ret) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]%d Add connect to CreateIoCompletionPort failed! \n", ret);
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	//建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
	//将新IoContext的套接字设为该Socket连接的套接字
	PER_IO_CONTEXT* pNewIoContext = pConnectContext->GetNewIoContext();
	pNewIoContext->m_sockAccept = pConnectContext->m_Socket;
	//绑定完毕之后，就可以开始在这个Socket上投递完成请求了
	ret = _PostRecv(pNewIoContext);
	if (ret) {
		RELEASE(pConnectContext);
		return -1;
	}
	//投递成功则将该连接信息添加到连接列表
	ret = _AddToContextList(pConnectContext);
	*psocketout = pConnectContext;

	return 0;
}

//以ConnectEX方式建立一个连接
int CIOCPClient::_PostConnect(int pport, const char *paddr, int plabel)
{
	int ret;
	//获取IP地址
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = 0;
	hints.ai_family = AF_INET;//IPV4
	hints.ai_socktype = SOCK_STREAM;//TCP
	ret = getaddrinfo(paddr, NULL, &hints, &res);
	if (ret) {
		return -1;
	}

	// 生成用于连接的Socket的信息
	PER_SOCKET_CONTEXT * pConnectContext = new PER_SOCKET_CONTEXT;
	pConnectContext->label = plabel;
	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	pConnectContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pConnectContext->m_Socket) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]%d Create socket failed! \n", WSAGetLastError());
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	//将Socket绑定至完成端口中
	ret = _AssociateWithIOCP(pConnectContext);
	if (ret) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]%d Add connectex to CreateIoCompletionPort failed! \n", ret);
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	// 以下的绑定很重要，也是容易漏掉的。（如果少了绑定，在 ConnextEx 时将得到错误代码：10022 — 提供了一个无效的参数
	sockaddr_in local_addr;
	ZeroMemory(&local_addr, sizeof(sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(ADDR_ANY);
	local_addr.sin_port = htons(0);
	ret = bind(pConnectContext->m_Socket, (sockaddr *)(&local_addr), sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]bind failed! \n");
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	//获取CONNECTEX指针
	LPFN_CONNECTEX m_lpfnConnectEx;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(pConnectContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidConnectEx,
		sizeof(GuidConnectEx),
		&m_lpfnConnectEx,
		sizeof(m_lpfnConnectEx),
		&dwBytes,
		NULL,
		NULL))
	{
		unsigned int err = WSAGetLastError();
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]Get CONNECTEX pointer failed! \n");
#endif
		RELEASE(pConnectContext);
		return -1;
	}

	// 服务器地址信息，用于连接
	sockaddr_in addrPeer;
	ZeroMemory(&addrPeer, sizeof(sockaddr_in));
	addrPeer.sin_family = AF_INET;
	addrPeer.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	addrPeer.sin_port = htons(pport);
	int nLen = sizeof(addrPeer);

	// 新建一个IO_CONTEXT
	PER_IO_CONTEXT* pConnectIoContext = pConnectContext->GetNewIoContext();
	pConnectIoContext->m_OpType = CONNECT_POSTED;
	pConnectIoContext->m_sockAccept = pConnectContext->m_Socket;

	OVERLAPPED *p_ol = &pConnectIoContext->m_Overlapped;
	PVOID lpSendBuffer = NULL;
	DWORD dwSendDataLength = 0;
	DWORD dwBytesSent = 0;

	BOOL bResult = m_lpfnConnectEx(pConnectIoContext->m_sockAccept,
		(sockaddr *)&addrPeer,  // [in] 对方地址
		nLen,               // [in] 对方地址长度
		lpSendBuffer,       // [in] 连接后要发送的内容，这里不用
		dwSendDataLength,   // [in] 发送内容的字节数 ，这里不用
		&dwBytesSent,       // [out] 发送了多少个字节，这里不用
		(OVERLAPPED *)p_ol); // [in] 这东西复杂，下一篇有详解
	if (!bResult)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)   // 调用失败
		{
			//pConnectContext->RemoveContext(pConnectIoContext);
			RELEASE(pConnectContext);
#ifdef LOG_STREAM
			printf("[CIOCPClient][ERROR]Post ConnectEx failed! \n");
#endif
			return -1;
		}
	}
	//投递成功则将该连接信息添加到连接列表
	ret = _AddToContextList(pConnectContext);

	return 0;
}

int CIOCPClient::_DoConnect(PER_SOCKET_CONTEXT* pSocketContext, PER_IO_CONTEXT* pIoContext)
{
	int ret;
	ret = setsockopt(pSocketContext->m_Socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	// 建立其下的IoContext，用于在这个Socket上投递第一个Recv数据请求
	// 将新IoContext的套接字设为该Socket连接的套接字
	// 连接后发送第一组数据 这里为测试数据
	ret = _PostSend(pSocketContext, "OK", strlen("OK"));
	PER_IO_CONTEXT* pNewIoContext = pSocketContext->GetNewIoContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_sockAccept = pSocketContext->m_Socket;
	// 绑定完毕之后，就可以开始在这个Socket上投递完成请求了
	ret = _PostRecv(pNewIoContext);
	if (ret) {
		pSocketContext->RemoveContext(pNewIoContext);
		return -1;
	}

	return 0;
}

//工作者线程处理函数
int CIOCPClient::_WorkerThread(THREADPARAMS_WORKER* lpParam)
{
	OVERLAPPED           *pOverlapped = NULL;
	PER_SOCKET_CONTEXT   *pSocketContext = NULL;
	DWORD                dwBytesTransfered = 0;
	// 循环处理请求，知道接收到Shutdown信息为止
	while (WAIT_OBJECT_0 != WaitForSingleObject(m_hShutdownEvent, 0))
	{
		//获取一个消息
		//pSocketContext为产生消息的Socket句柄
		//pOverlapped归属于一个已有的PER_IO_CONTEXT成员
		BOOL bReturn = GetQueuedCompletionStatus(
			m_hIOCompletionPort,
			&dwBytesTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);

		// 如果收到的是退出命令则直接退出
		if (EXIT_CODE == (DWORD)pSocketContext) {
			break;
		}
		// 判断是否出现了错误
		if (!bReturn) {
			DWORD err = GetLastError();
#ifdef LOG_STREAM
			printf("[CIOCPClient][ERROR]%d WORKER Get Queued MSG Failed. \n", err);
#endif
			//释放掉对应的资源
			if (err == 64) {
				OnClose(pSocketContext);
				_RemoveContextByMember(pSocketContext);
			}
			continue;
		}
		//读取传入的参数
		//由pOverlapped的内存地址获取所属类PER_IO_CONTEXT的指针
		PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT, m_Overlapped);
		//连接断开的情况
		if ((0 == dwBytesTransfered) && (RECV_POSTED == pIoContext->m_OpType || SEND_POSTED == pIoContext->m_OpType))
		{
			OnClose(pSocketContext);
			//释放掉对应的资源
			_RemoveContextByMember(pSocketContext);
			continue;
		}
		//其它情况
		switch (pIoContext->m_OpType)
		{
		case CONNECT_POSTED: {
			_DoConnect(pSocketContext, pIoContext);
			break;
		}
		case RECV_POSTED: {
			_DoRecv(pSocketContext, pIoContext, dwBytesTransfered);
			break;
		}
		case SEND_POSTED: {
			_DoSend(pSocketContext, pIoContext, dwBytesTransfered);
			break;
		}
		default:
			// 不应该执行到这里
#ifdef LOG_STREAM
			printf("[CIOCPClient][ERROR] Worker unknown OpType %d. \n", pIoContext->m_OpType);
#endif
			break;
		}
	}//while

	return 0;
}

//设置回调函数指针
int CIOCPClient::SetCallback(CIOCPClientCallback *pcallback)
{
	m_IOCPClientCallback = pcallback;
	return 0;
}

//初始化资源环境 该函数非线程安全
int CIOCPClient::Initialize(int maxthreadnum)
{
	if (isinitialized)
		return 0;
	int ret;
	// 初始化IOCP
	ret = _InitializeIOCP(maxthreadnum);
	if (ret) {
		return ret;
	}
#ifdef LOG_STREAM
	printf("[CIOCPClient][INFO] Initialized. \n");
#endif
	return 0;
}

//清理所有资源 该函数非线程安全
int CIOCPClient::Destory()
{
	if (!isinitialized)
		return 0;
	// 先关闭心跳线程
	this->_CloseHeartThread();
	int i = 3;
	// 先尝试正常方式断开Socket
	CloseConnections();
	while (i && !m_arrayClientContext.empty()) {
		i--;
		Sleep(200);
	}
	// 释放其他资源
	_DeInitialize();
#ifdef LOG_STREAM
	printf("[CIOCPClient][INFO] Stop. \n");
#endif
	return 0;
}

// 开启一个连接 该函数线程安全
int CIOCPClient::Connect(int pport, const char *paddr, int plabel, const char * str, int len)
{
	//如果没有初始化IOCP则自动按默认值初始化
	int ret;
	ret = _InitializeIOCP(0);
	if (ret) {
		return ret;
	}
	//建立连接
	PER_SOCKET_CONTEXT* psocket = NULL;
	ret = _ConnectSocket(&psocket, pport, paddr, plabel);
	if (ret) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]Failed to connect to %s:%d %d. \n", paddr, pport, plabel);
#endif
		return SOCKETERR_CONNECT_FAILED;
	}
	//发送第一次数据
	if (len == 0)
		return 0;
	ret = _PostSend(psocket, str, len);
	//发送失败则关闭该连接
	if (ret) {
		//将label置为错误值
		psocket->label = -1;
		_DisConnectSocketSoft(psocket);
		return SOCKETERR_SEND_FAILED;
	}

	return 0;
}

//软断开一个特定的连接
int CIOCPClient::CloseConnectionByLabel(int plabel) {
	if (m_arrayClientContext.empty()) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]No connection. \n");
#endif
		return 0;
	}
	//查找连接
	EnterCriticalSection(&m_csContextList);
	std::list<PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end(); m_itor++) {
		if ((*m_itor)->label == plabel) {
			_DisConnectSocketSoft(*m_itor);
			break;
		}
	}
	LeaveCriticalSection(&m_csContextList);

	return 0;
}

//软断开所有连接
//正常情况下Worker线程会收到所有连接的断开消息然后清空Socket列表
int CIOCPClient::CloseConnections() {
	if (m_arrayClientContext.empty()) {
#ifdef LOG_STREAM
		printf("[CIOCPClient][ERROR]No connection. \n");
#endif
		return 0;
	}
	//发送Socket关闭信息
	EnterCriticalSection(&m_csContextList);
	std::list<PER_SOCKET_CONTEXT*>::iterator m_itor;
	for (m_itor = m_arrayClientContext.begin(); m_itor != m_arrayClientContext.end(); m_itor++) {
		_DisConnectSocketSoft(*m_itor);
	}
	LeaveCriticalSection(&m_csContextList);
#ifdef LOG_STREAM
	printf("[CIOCPClient][INFO] List reseting soft... \n");
#endif
	return 0;
}
