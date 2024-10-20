#include <brpc/server.h>
#include <butil/logging.h>
#include "main.pb.h"

// 1.创建子服务继承于EchoService，并实现prc调用的业务
class EchoServiceImpl : public example::EchoService
{
public:
    EchoServiceImpl() {};
    ~EchoServiceImpl() {};

public:
    void Echo(google::protobuf::RpcController *controller,
              const ::example::EchoRequest *request,
              ::example::EchoResponse *response,
              ::google::protobuf::Closure *done)
    {
        brpc::ClosureGuard rpc_guard(done);
        std::cout << "收到消息" << request->message() << "\n";

        std::string str = request->message() + "--这是响应!!";
        response->set_message(str);
    }
};

int main()
{
    // 关闭brpc默认日志输出
    logging::LoggingSettings settings;
    settings.logging_dest = logging::LoggingDestination::LOG_TO_NONE;
    logging::InitLogging(settings);
    // 2.构造服务器对象
    brpc::Server server;

    // 3.向服务器对象中，增加子服务
    EchoServiceImpl echo_server;
    int ret = server.AddService(&echo_server, brpc::ServiceOwnership::SERVER_DOESNT_OWN_SERVICE);
    if (ret == -1)
    {
        std::cout << "添加RPC服务失败！\n";
        return -1;
    }

    // 4.启动服务器
    brpc::ServerOptions options;
    options.idle_timeout_sec = -1; // 空闲超时事件，超时断开连接
    options.num_threads = 2;       // io线程数量

    ret = server.Start(8080, &options);
    if (ret == -1)
    {
        std::cout << "服务器启动失败！\n";
        return -1;
    }
    server.RunUntilAskedToQuit(); // 服务器启动，等待运行结果
    return 0;
}
