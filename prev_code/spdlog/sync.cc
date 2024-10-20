#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

int main()
{
    // 设置全局刷新策略（每个日志带自己的刷新策略）
    spdlog::flush_every(std::chrono::seconds(1));       // 每秒刷新
    spdlog::flush_on(spdlog::level::level_enum::debug); // 遇到debug以上的立即刷新

    // 设置全局日志等级
    spdlog::set_level(spdlog::level::level_enum::debug);
    // 创建同步日志器（标准输出/文件）
    // auto logger = spdlog::stdout_color_mt("default-logger");
    auto logger = spdlog::basic_logger_mt("file-logger", "sync.log");

    // 设置日志器的刷新策略， 设置日志器的输出等级
    // logger->flush_on(spdlog::level::level_enum::debug);
    // logger->set_level(spdlog::level::level_enum::debug);;
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