#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

//定义server类型
typedef websocketpp::server<websocketpp::config::asio> server_t;

void onOpen(websocketpp::connection_hdl hdl)
{
    std::cout << "websocket长连接建立成功！\n";
}

void onClose(websocketpp::connection_hdl hdl)
{
    std::cout << "websocket长连接断开！\n";

}

void onMessage(server_t *server, websocketpp::connection_hdl hdl, server_t::message_ptr msg)
{
    //1.获取有效消息载荷，进行业务处理
    std::string body = msg->get_payload();
    std::cout << "收到消息: " << body << std::endl;
    //2.对客户端响应
    //获取通信连接
    auto conn = server->get_con_from_hdl(hdl);
    //发送数据
    conn->send(body + "-sendEcho", websocketpp::frame::opcode::value::text);
    
}

int main()
{
    
    //实例化服务器对象
    server_t server;

    //关闭日志输出
    server.set_access_channels(websocketpp::log::alevel::none);
    
    //初始化asio框架
    server.init_asio();

    //设置消息、连接握手、连接关闭的回调函数
    server.set_open_handler(onOpen);
    server.set_close_handler(onClose);

    auto msg_handler = std::bind(onMessage, &server, std::placeholders::_1, std::placeholders::_2);
    server.set_message_handler(msg_handler);

    //设置地址重用
    server.set_reuse_addr(true);
    //设置监听端口
    server.listen(9090);
    //开始监听
    server.start_accept();
    //启动服务器
    server.run();

    return 0;
}