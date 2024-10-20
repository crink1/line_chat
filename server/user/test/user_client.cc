// user测试客户端
// 1.服务发现
// 2.读取语音文件
// 3.发起rpc请求
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include "channel.hpp"
#include "etcd.hpp"
#include "utils.hpp"
#include "user.pb.h"
#include "base.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "127.0.0.1:2379", "服务中心注册地址");
DEFINE_string(base_service, "/service", "服务监控的根目录");
DEFINE_string(user_service, "/service/user_service", "服务监控的根目录");

crin_lc::ServiceManager::ptr _user_channels;

crin_lc::UserInfo user_info;
std::string login_ssid;
std::string avatar = "头像";
std::string newname = "大电风吹";


// TEST(用户子服务测试, 用户注册)
// {
//     auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
//     ASSERT_TRUE(channel);
//     user_info.set_nickname("超级龙卷风");

//     crin_lc::UserRegisterReq req;
//     req.set_request_id(crin_lc::uuid());
//     req.set_nickname(user_info.nickname());
//     req.set_password("666666");

//     crin_lc::UserRegisterRsp resp;
//     brpc::Controller cntl;
//     crin_lc::UserService_Stub stub(channel.get());
//     stub.UserRegister(&cntl, &req, &resp, nullptr); // sync
//     ASSERT_FALSE(cntl.Failed());
//     ASSERT_TRUE(resp.success());
// }

TEST(用户子服务测试, 用户登录)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::UserLoginReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_nickname(user_info.nickname());
    req.set_password("123456");

    crin_lc::UserLoginRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.UserLogin(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
    login_ssid = resp.login_session_id();
}

TEST(用户子服务测试, 用户头像)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::SetUserAvatarReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_avatar(user_info.avatar());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);

    crin_lc::SetUserAvatarRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.SetUserAvatar(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
}

TEST(用户子服务测试, 用户签名)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::SetUserDescriptionReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    req.set_description(user_info.description());

    crin_lc::SetUserDescriptionRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.SetUserDescription(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
}

TEST(用户子服务测试, 用户昵称)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::SetUserNicknameReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    req.set_nickname(newname);
    
    crin_lc::SetUserNicknameRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.SetUserNickname(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
}

TEST(用户子服务测试, 用户信息获取)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::GetUserInfoReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_user_id(user_info.user_id());
    req.set_session_id(login_ssid);
    
    crin_lc::GetUserInfoRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.GetUserInfo(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(user_info.user_id(), resp.user_info().user_id());
    ASSERT_EQ(newname, resp.user_info().nickname());
    ASSERT_EQ(user_info.description(), resp.user_info().description());
    ASSERT_EQ("", resp.user_info().phone());
    ASSERT_EQ(user_info.avatar(), resp.user_info().avatar());
}

std::string get_code()
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    
    

    crin_lc::PhoneVerifyCodeReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_phone_number("15377059362");

    crin_lc::PhoneVerifyCodeRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.GetPhoneVerifyCode(&cntl, &req, &resp, nullptr); // sync
    

    return resp.verify_code_id();
}

void set_user_avarar(const string &uid, const string &avatar)
{
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);
    

    crin_lc::SetUserAvatarReq req;
    req.set_request_id(crin_lc::uuid());
    req.set_avatar(avatar);
    req.set_user_id(uid);
    req.set_session_id("_test_1");

    crin_lc::SetUserAvatarRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.SetUserAvatar(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
}

TEST(用户子服务测试, 批量用户信息获取)
{
    //set_user_avarar();
    auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
    ASSERT_TRUE(channel);

    crin_lc::GetMultiUserInfoReq req;
    req.set_request_id(crin_lc::uuid());
    req.add_users_id("用户ID1");
    req.add_users_id("d789-8c481279-0000");
    req.add_users_id("fcc4-de2ce44b-0001");
    
    crin_lc::GetMultiUserInfoRsp resp;
    brpc::Controller cntl;
    crin_lc::UserService_Stub stub(channel.get());
    stub.GetMultiUserInfo(&cntl, &req, &resp, nullptr); // sync
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(resp.success());
    auto map = resp.mutable_users_info();
    crin_lc::UserInfo user = (*map)["d789-8c481279-0000"];
    ASSERT_EQ(user.user_id(), "d789-8c481279-0000");
    ASSERT_EQ(user.nickname(), "大电风吹");
    ASSERT_EQ(user.description(), "呜呼呜呼我");
    ASSERT_EQ(user.phone(), "");
    ASSERT_EQ(user.avatar(), "头像");

    crin_lc::UserInfo user2 = (*map)["用户ID1"];
    ASSERT_EQ(user2.user_id(), "用户ID1");
    ASSERT_EQ(user2.nickname(), "小猪佩奇");
    ASSERT_EQ(user2.description(), "这是一只小猪");
    ASSERT_EQ(user2.phone(), "手机号1");
    ASSERT_EQ(user2.avatar(), "小猪头像1");

    crin_lc::UserInfo user3 = (*map)["fcc4-de2ce44b-0001"];
    ASSERT_EQ(user3.user_id(), "fcc4-de2ce44b-0001");
    ASSERT_EQ(user3.nickname(), "超级龙卷风");
    ASSERT_EQ(user3.description(), "");
    ASSERT_EQ(user3.phone(), "");
    ASSERT_EQ(user3.avatar(), "");

}

// TEST(用户子服务测试, 手机号注册)
// {
//     std::string code_id = get_code();
//     auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
//     ASSERT_TRUE(channel);

//     crin_lc::PhoneRegisterReq req;
//     req.set_request_id(crin_lc::uuid());
//     req.set_phone_number("15377059362");
//     req.set_verify_code_id(code_id);
//     std::string code;
//     std::cin >> code;
//     req.set_verify_code(code);
//     crin_lc::PhoneRegisterRsp resp;
//     brpc::Controller cntl;
//     crin_lc::UserService_Stub stub(channel.get());
//     stub.PhoneRegister(&cntl, &req, &resp, nullptr); // sync
//     ASSERT_FALSE(cntl.Failed());
//     ASSERT_TRUE(resp.success());
    
// }


// TEST(用户子服务测试, 手机号登录)
// {
//     std::string code_id = get_code();
//     auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
//     ASSERT_TRUE(channel);

//     crin_lc::PhoneLoginReq req;
//     req.set_request_id(crin_lc::uuid());
//     req.set_phone_number("15377059362");
//     req.set_verify_code_id(code_id);
//     LOG_INFO("输入登录验证码：");
//     std::string code;
//     std::cin >> code;
//     req.set_verify_code(code);
//     crin_lc::PhoneLoginRsp resp;
//     brpc::Controller cntl;
//     crin_lc::UserService_Stub stub(channel.get());
//     stub.PhoneLogin(&cntl, &req, &resp, nullptr); // sync
//     ASSERT_FALSE(cntl.Failed());
//     ASSERT_TRUE(resp.success());
//     LOG_DEBUG("手机号登录会话ID：{}", resp.login_session_id());   
    
// }

// TEST(用户子服务测试, 手机号设置)
// {
//     std::this_thread::sleep_for(std::chrono::seconds(10));
//     std::string code_id = get_code();
//     auto channel = _user_channels->choose(FLAGS_user_service); // 获取信道
//     ASSERT_TRUE(channel);

//     crin_lc::SetUserPhoneNumberReq req;
//     req.set_request_id(crin_lc::uuid());
//     LOG_INFO("输入用户ID：");
//     std::string code;
//     std::cin >> code;
//     req.set_user_id(code);
//     req.set_phone_number("18688854641");
//     req.set_phone_verify_code_id(code_id);
//     LOG_INFO("输入换手机号验证码：");
//     std::cin >> code;
//     req.set_phone_verify_code(code);
//     crin_lc::SetUserPhoneNumberRsp resp;
//     brpc::Controller cntl;
//     crin_lc::UserService_Stub stub(channel.get());
//     stub.SetUserPhoneNumber(&cntl, &req, &resp, nullptr); // sync
//     ASSERT_FALSE(cntl.Failed());
//     ASSERT_TRUE(resp.success());
    
// }
int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1.构造rpc信道管理对象

    _user_channels = std::make_shared<crin_lc::ServiceManager>();
    _user_channels->declared(FLAGS_user_service);
    auto put_cb = std::bind(&crin_lc::ServiceManager::onserviceOnline, _user_channels.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&crin_lc::ServiceManager::onserviceOffline, _user_channels.get(), std::placeholders::_1, std::placeholders::_2);

    // 2.构造服务发现服务
    crin_lc::Discovery::ptr dclient = std::make_shared<crin_lc::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);
    // user_info.set_nickname("大电风吹");
    // user_info.set_user_id("d789-8c481279-0000");
    // user_info.set_description("呜呼呜呼我");
    // user_info.set_phone("15377635498");
    // user_info.set_avatar(avatar);
    set_user_avarar("fcc4-de2ce44b-0001", "龙卷风头像");
    set_user_avarar("608b-a92b2f12-0002", "测试机头像");
    return 0;
}