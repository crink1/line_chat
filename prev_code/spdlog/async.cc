#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

int main()
{
    // 设置全局刷新策略（每个日志带自己的刷新策略）
    spdlog::flush_every(std::chrono::seconds(1));       // 每秒刷新
    spdlog::flush_on(spdlog::level::level_enum::debug); // 遇到debug以上的立即刷新

    //初始化异步日志输出线程配置
    spdlog::init_thread_pool(3072, 2);

    // 创建异步日志器（标准输出/文件）
    auto logger = spdlog::stdout_color_mt<spdlog::async_factory>("default-logger");

    // 设置日志器的刷新策略， 设置日志器的输出等级
    logger->set_pattern("[%n] [%H:%M:%S] [%t][%-8l] - [%v]");

    // 进行日志输出
    logger->trace("你好 {}", "芜湖");
    logger->debug("你好 {}", "芜湖");
    logger->info("你好 {}", "芜湖");
    logger->warn("你好 {}", "芜湖");
    logger->error("你好 {}", "芜湖");
    logger->critical("你好 {}", "芜湖");
    std::cout << "日志输出完毕" << std::endl;

    return 0;
}