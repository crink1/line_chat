#include "../../../common/data_redis.hpp"

#include <gflags/gflags.h>
#include <thread>

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(ip, "127.0.0.1", "监听地址");
DEFINE_int32(port, 6379, "监听端口");
DEFINE_int32(db, 0, "库的编号：默认0");
DEFINE_bool(keep_alive, true, "是否长连接保活");

void session_test(const std::shared_ptr<sw::redis::Redis> &redis_client)
{
    crin_lc::Session client(redis_client);
    client.append("sessionID-1", "userID-1");
    client.append("sessionID-2", "userID-2");
    client.append("sessionID-3", "userID-3");

    client.remove("sessionID-2");

    auto ret = client.uid("sessionID-1");
    if(ret)
    {
        std::cout << *ret << std::endl;
    }
    auto ret1 = client.uid("sessionID-2");
    if(ret1)
    {
        std::cout << *ret1 << std::endl;
    }
    auto ret2 = client.uid("sessionID-3");
    if(ret2)
    {
        std::cout << *ret2 << std::endl;
    }

}

void status(const std::shared_ptr<sw::redis::Redis> &redis_client)
{
    crin_lc::Status st(redis_client);
    st.append("userID-1");
    st.append("userID-2");

    st.remove("userID-2");

    if(st.exists("userID-1"))
    {
        std::cout << "用户1在线" << std::endl;
    }

    if(st.exists("userID-2"))
    {
        std::cout << "用户2在线" << std::endl;
    }

}

void code_test(const std::shared_ptr<sw::redis::Redis> &redis_client)
{
    crin_lc::Codes st(redis_client);
    st.append("验证码ID-1", "验证码1");
    st.append("验证码ID-2", "验证码2");

    st.remove("验证码ID-2");



    if(st.code("验证码ID-1"))
    {
        std::cout << "验证码1存在" << std::endl;
    }

    if(st.code("验证码ID-2"))
    {
        std::cout << "验证码2存在" << std::endl;
    }

    std::cout << "开始睡眠" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));

    if(!st.code("验证码ID-1"))
    {
        std::cout << "验证码1不存在" << std::endl;
    }

    if(!st.code("验证码ID-2"))
    {
        std::cout << "验证码2不存在" << std::endl;
    }

}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    //crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);


    auto client = crin_lc::RedisClientFactory::create(FLAGS_ip, FLAGS_port, FLAGS_db, FLAGS_keep_alive);
    //session_test(client);
    //status(client);
    code_test(client);
    return 0;
}