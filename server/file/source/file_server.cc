//参数解析
//日志
//构造服务器
#include "file_server.hpp"

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务中心注册地址");
DEFINE_string(base_service, "/service", "服务监控的根目录");
DEFINE_string(instance_name, "/file_service/instance", "当前实例名称");
DEFINE_string(access_host, "127.0.0.1:10002", "当前实例的外部访问地址");
DEFINE_string(storage_path, "./data/", "文件默认存储位置");


DEFINE_int32(listen_port, 10002, "Rpc监听端口");
DEFINE_int32(rpc_timeout, -1, "Rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "Rpc的I/O线程数量");


int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    crin_lc::FileServerBuilder fsb;
    fsb.make_rpc_server(FLAGS_listen_port, FLAGS_rpc_timeout, FLAGS_rpc_threads, FLAGS_storage_path);
    fsb.make_reg_object(FLAGS_registry_host , FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);

    auto server = fsb.build();
    server->start();
    return 0;
}