#include <iostream>
#include <gflags/gflags.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include "../common/etcd.hpp"
#include "main.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");
DEFINE_string(etcd_host, "127.0.0.1:2379", "服务中心注册地址");
DEFINE_string(base_service, "/service", "服务监控的根目录");
DEFINE_string(instance_name, "/echo/instance", "当前实例名称");
DEFINE_string(access_host, "127.0.0.1:7070", "当前实例的外部访问地址");
DEFINE_int32(listen_port, 7070, "Rpc监听端口");

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

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1.关闭brpc默认日志输出
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

    ret = server.Start(FLAGS_listen_port, &options);
    if (ret == -1)
    {
        std::cout << "服务器启动失败！\n";
        return -1;
    }
    // 5.注册服务
    Registry::ptr rclient = std::make_shared<Registry>(FLAGS_etcd_host);
    rclient->registry(FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);

    server.RunUntilAskedToQuit(); // 服务器启动，等待运行结果

    return 0;
}