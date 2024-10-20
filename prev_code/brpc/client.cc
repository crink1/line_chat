#include <brpc/channel.h>
#include <thread>
#include "main.pb.h"

void callback(brpc::Controller *cntl, ::example::EchoResponse *response)
{
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<::example::EchoResponse> response_guard(response);

    if (cntl_guard->Failed() == true)
    {
        std::cout << "Rpc调用失败：" << cntl_guard->ErrorText() << std::endl;
        return;
    }
    std::cout << "收到响应：" << response_guard->message() << std::endl;
    
    
}

int main(int argc, char *argv[])
{
    // 1.构造channel信道，连接服务器
    brpc::ChannelOptions options;
    options.connect_timeout_ms = -1; // 设置连接超时等待时间， -1是一直等待
    options.timeout_ms = -1;         // 设置RPC请求超时等待时间， -1是一直等待
    options.max_retry = 3;           // 请求重试次数
    options.protocol = "baidu_std";  // 序列化协议，默认使用百度的
    brpc::Channel channel;
    int ret = channel.Init("127.0.0.1:8080", &options);
    if (ret == -1)
    {
        std::cout << "初始化信道失败！\n";
    }

    // 2.构造echoServer_Stub对象，用于进行rpc调用
    example::EchoService_Stub stub(&channel);
    // 3.进行prc调用
    example::EchoRequest req;
    req.set_message("呜呼呜呼");

    brpc::Controller *cntl = new brpc::Controller();

    example::EchoResponse *resp = new example::EchoResponse();

    // stub.Echo(cntl, &req, resp, nullptr); // sync

    // if (cntl->Failed() == true)
    // {
    //     std::cout << "Rpc调用失败：" << cntl->ErrorText() << std::endl;
    //     return -1;
    // }
    // std::cout << "收到响应：" << resp->message() << std::endl;

    auto clusure = google::protobuf::NewCallback(callback, cntl, resp);
    stub.Echo(cntl, &req, resp, clusure); // async
    std::cout << "异步调用结束！\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // delete cntl;
    // delete resp;
    return 0;
}
