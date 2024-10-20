// speech测试客户端
// 1.服务发现
// 2.读取语音文件
// 3.发起rpc请求
#include "aip-cpp-sdk/speech.h"
#include "channel.hpp"
#include "etcd.hpp"
#include "speech.pb.h"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "127.0.0.1:2379", "服务中心注册地址");
DEFINE_string(base_service, "/service", "服务监控的根目录");
DEFINE_string(speech_service, "/service/speech_service", "服务监控的根目录");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1.构造rpc信道管理对象
    auto sm = std::make_shared<crin_lc::ServiceManager>();
    sm->declared(FLAGS_speech_service);
    auto put_cb = std::bind(&crin_lc::ServiceManager::onserviceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&crin_lc::ServiceManager::onserviceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);

    // 2.构造服务发现服务
    crin_lc::Discovery::ptr dclient = std::make_shared<crin_lc::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    // 3.通过rpc信道管理对象，获取提供echo的服务
    auto channel = sm->choose(FLAGS_speech_service);
    if (!channel)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return -1;
    }

    std::string file_content;
    aip::get_file_content("./16k.pcm", &file_content);
    
    // 4.发起echoRpc调用
    crin_lc::SpeechService_Stub stub(channel.get());
    crin_lc::SpeechRecognitionReq req;
    req.set_speech_content(file_content);
    req.set_request_id("4653345");
    brpc::Controller *cntl = new brpc::Controller();
    crin_lc::SpeechRecognitionRsp *resp = new crin_lc::SpeechRecognitionRsp();
    stub.SpeechRecognition(cntl, &req, resp, nullptr); // sync
    if (cntl->Failed() == true)
    {
        std::cout << "Rpc调用失败：" << cntl->ErrorText() << std::endl;
        delete cntl;
        delete resp;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if(resp->success() == false)
    {
        std::cout << resp->errmsg() << std::endl;
    }

    std::cout << "收到响应：" << resp->request_id() << std::endl;
    std::cout << "收到响应：" << resp->recognition_result() << std::endl;


    return 0;
}