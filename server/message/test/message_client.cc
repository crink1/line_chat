#include "etcd.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "message.pb.h"
#include "base.pb.h"
#include "user.pb.h"


DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(message_service, "/service/message_service", "服务监控根目录");

crin_lc::ServiceManager::ptr sm;

void range_test(const std::string &ssid, 
    const boost::posix_time::ptime &stime,
    const boost::posix_time::ptime &etime) {
    auto channel = sm->choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgStorageService_Stub stub(channel.get());
    crin_lc::GetHistoryMsgReq req;
    crin_lc::GetHistoryMsgRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_chat_session_id(ssid);
    req.set_start_time(boost::posix_time::to_time_t(stime));
    req.set_over_time(boost::posix_time::to_time_t(etime));
    brpc::Controller cntl;
    stub.GetHistoryMsg(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "-----------------------获取区间消息--------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == crin_lc::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}

void recent_test(const std::string &ssid, int count) {
    auto channel = sm->choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgStorageService_Stub stub(channel.get());
    crin_lc::GetRecentMsgReq req;
    crin_lc::GetRecentMsgRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_chat_session_id(ssid);
    req.set_msg_count(count);
    brpc::Controller cntl;
    stub.GetRecentMsg(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "----------------------获取最近消息---------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == crin_lc::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}


void search_test(const std::string &ssid, const std::string &key) {
    auto channel = sm->choose(FLAGS_message_service);
    if (!channel) {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    crin_lc::MsgStorageService_Stub stub(channel.get());
    crin_lc::MsgSearchReq req;
    crin_lc::MsgSearchRsp rsp;
    req.set_request_id(crin_lc::uuid());
    req.set_chat_session_id(ssid);
    req.set_search_key(key);
    brpc::Controller cntl;
    stub.MsgSearch(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.msg_list_size(); i++) {
        std::cout << "----------------------关键字搜索消息---------------------------\n";
        auto msg = rsp.msg_list(i);
        std::cout << msg.message_id() << std::endl;
        std::cout << msg.chat_session_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(boost::posix_time::from_time_t(msg.timestamp())) << std::endl;
        std::cout << msg.sender().user_id() << std::endl;
        std::cout << msg.sender().nickname() << std::endl;
        std::cout << msg.sender().avatar() << std::endl;
        if (msg.message().message_type() == crin_lc::MessageType::STRING) {
            std::cout << "文本消息：" << msg.message().string_message().content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::IMAGE) {
            std::cout << "图片消息：" << msg.message().image_message().image_content() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::FILE) {
            std::cout << "文件消息：" << msg.message().file_message().file_contents() << std::endl;
            std::cout << "文件名称：" << msg.message().file_message().file_name() << std::endl;
        }else if (msg.message().message_type() == crin_lc::MessageType::SPEECH) {
            std::cout << "语音消息：" << msg.message().speech_message().file_contents() << std::endl;
        }else {
            std::cout << "类型错误！！\n";
        }
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    
    //1. 先构造Rpc信道管理对象
    sm = std::make_shared<crin_lc::ServiceManager>();
    sm->declared(FLAGS_message_service);
    auto put_cb = std::bind(&crin_lc::ServiceManager::onserviceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&crin_lc::ServiceManager::onserviceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    //2. 构造服务发现对象
    crin_lc::Discovery::ptr dclient = std::make_shared<crin_lc::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);
    
    boost::posix_time::ptime stime(boost::posix_time::time_from_string("2024-10-10 00:00:00"));
    boost::posix_time::ptime etime(boost::posix_time::time_from_string("2024-10-16 00:00:00"));
    range_test("会话ID2", stime, etime);
    recent_test("会话ID2", 2);
    search_test("会话ID2", "呜呼");
    return 0;
}