#include "../../common/rabbitmq.hpp"
#include <gflags/gflags.h>
DEFINE_string(user, "root", "rbmq服务器用户名");
DEFINE_string(pswd, "123456", "rbmq服务器密码");
DEFINE_string(host, "127.0.0.1:5672", "rbmq服务器地址");


DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");




int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    MQClient client(FLAGS_user, FLAGS_pswd, FLAGS_host);
    client.declareComponents("test-exchange", "test-queue");
    for (int i = 0; i < 10; i++)
    {
        std::string msg = "hello crin" + std::to_string(i);
        bool ret = client.publish("test-exchange", msg);

        if (ret == false)
        {
            std::cout << "publish失败！\n";
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}