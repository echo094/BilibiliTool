# WSS相关说明

这里使用Websocketpp进行连接。



## 显示devel级别日志的方法

正常情况下只有使用debug系列配置文件才能显示，其它配置文件需要手动修改变量 alog_level 的初始值：

```c++
// websocketpp\core\core_client.hpp or websocketpp\core\core.hpp
struct core_client {
    static const websocketpp::log::level alog_level =
        websocketpp::log::alevel::all ^ websocketpp::log::alevel::devel;
}
```



## SSL证书验证

SSL连接使用了Boost中Asio的SSL加密组件，调用了OpenSSL库。

OpenSSL自身不包含可信证书库，因此如果需要验证证书，需要将所有需要使用的证书、其中间证书以及根证书都导入进来。

参照Boost网站对[SSL](https://www.boost.org/doc/libs/1_68_0/doc/html/boost_asio/overview/ssl.html)的介绍，以及Websocketpp中的例子**print_client_tls**，可以建立一个简化的初始化和验证步骤：

```c++
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);

        ctx->set_verify_mode(boost::asio::ssl::verify_peer);
		ctx->set_verify_callback(boost::asio::ssl::rfc2818_verification(hostname));
        // Here we load the CA certificates of all CA's that this client trusts.
		ctx->load_verify_file("bilibili-chain.pem");
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}

```

由于证书可能更换，在软件中图方便不进行验证，如果需要验证可以将目录中的pem文件放到程序所在目录。

需要注意的是子域名 *.chat.bilibili.com 与 *.bilibili.com 使用的不是同一张证书，签发机构也不同，前者的有效期只有一年。当前的证书有效期到2019年9月29日。

在导出证书时选择Base64格式导出，然后合并到同一个文件并将后缀改为pem即可，也可以使用在线工具。



## TLS数据包发送异常的原因

根据[issue 424](https://github.com/zaphoyd/websocketpp/issues/424)以及[issue 626](https://github.com/zaphoyd/websocketpp/issues/626)的描述，这个问题可能只出现在Windows环境下。

在Websocketpp中，为了减少数据的复制次数，同一个数据包的Header以及Payload被分别加入到了一个列表中。当使用Boost的Asio发送时，理论上这两部分会被整合为一个TCP数据包。

通过Wireshark抓包可以发现，在Windows环境下，进行非加密连接时这一切都正常。但是使用TLS加密连接时，Asio并没有将数据合并，而是发送了两个数据包。这就导致了只要发送数据连接就被服务器关闭。

既然找到了问题，那么手动将两部分合并成一组数据传递给Asio就能解决问题了。在这里只需修改一个文件：

```c++
// websocketpp\impl\connection_impl.hpp
// write_frame line 1830 
template <typename config>
void connection<config>::write_frame() {
	// ...
    typename std::vector<message_ptr>::iterator it;
    for (it = m_current_msgs.begin(); it != m_current_msgs.end(); ++it) {
        std::string const & header = (*it)->get_header();
		(*it)->get_raw_payload().insert(0, header);
        std::string const & payload = (*it)->get_payload();

		// m_send_buffer.push_back(transport::buffer(header.c_str(), header.size()));
        m_send_buffer.push_back(transport::buffer(payload.c_str(),payload.size()));
    }
	// ...
}

```

也就是在这里将Header的数据插入到Payload的头部，然后仅把修改后的Payload添加到待发送列表。

