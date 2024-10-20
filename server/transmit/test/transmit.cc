// speech_server的测试客户端实现
// 1. 进行服务发现--发现speech_server的服务器节点地址信息并实例化的通信信道
// 2. 读取语音文件数据
// 3. 发起语音识别RPC调用

#include "etcd.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include "transmit.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(transmite_service, "/service/transmite_service", "服务监控根目录");

crin_lc::ServiceManager::ptr sm;

void string_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgTransmitService_Stub stub(channel.get());
    crin_lc::NewMessageReq req;
    crin_lc::GetTransmitTargetRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(crin_lc::MessageType::STRING);
    req.mutable_message()->mutable_string_message()->set_content(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    std::cout << "msg_id: " << rsp.message().message_id() << std::endl;
    std::cout << "csid: " << rsp.message().chat_session_id() << std::endl;
    std::cout << "time: " << rsp.message().timestamp() << std::endl;
    std::cout << "sender: " << rsp.message().sender().nickname() << std::endl;
    std::cout << "message: " << rsp.message().message().string_message().content() << std::endl;

    for (int i = 0; i < rsp.target_id_list_size(); i++)
    {
        std::cout << "target" << rsp.target_id_list(i) << std::endl;
    }
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}
void image_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgTransmitService_Stub stub(channel.get());
    crin_lc::NewMessageReq req;
    crin_lc::GetTransmitTargetRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(crin_lc::MessageType::IMAGE);
    req.mutable_message()->mutable_image_message()->set_image_content(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void speech_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgTransmitService_Stub stub(channel.get());
    crin_lc::NewMessageReq req;
    crin_lc::GetTransmitTargetRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(crin_lc::MessageType::SPEECH);
    req.mutable_message()->mutable_speech_message()->set_file_contents(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void file_message(const std::string &uid, const std::string &sid,
                  const std::string &filename, const std::string &content)
{
    auto channel = sm->choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgTransmitService_Stub stub(channel.get());
    crin_lc::NewMessageReq req;
    crin_lc::GetTransmitTargetRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(crin_lc::MessageType::FILE);
    req.mutable_message()->mutable_file_message()->set_file_contents(content);
    req.mutable_message()->mutable_file_message()->set_file_name(filename);
    req.mutable_message()->mutable_file_message()->set_file_size(content.size());
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1. 先构造Rpc信道管理对象
    sm = std::make_shared<crin_lc::ServiceManager>();
    sm->declared(FLAGS_transmite_service);
    auto put_cb = std::bind(&crin_lc::ServiceManager::onserviceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&crin_lc::ServiceManager::onserviceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    // 2. 构造服务发现对象
    crin_lc::Discovery::ptr dclient = std::make_shared<crin_lc::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);

    // 3. 通过Rpc信道管理对象，获取提供Echo服务的信道
    string_message("fcc4-de2ce44b-0001", "会话ID2", "呜呼呜呼");
    string_message("608b-a92b2f12-0002", "会话ID2", "呜呼呜呼2222");
    
    // image_message("608b-a92b2f12-0002", "会话ID2", "呜呼呼图片");
    // image_message("fcc4-de2ce44b-0001", "会话ID2", "呜呼呼图片啊啊");

    // speech_message("608b-a92b2f12-0002", "会话ID2", "呜呼呜呼音频");
    // file_message("608b-a92b2f12-0002", "0d90-755571d8-0003", "文件名称", "文件数据");
    return 0;
}