#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

/// @brief //////////
/// @param mode - 运行模式：true-发布模式；false-调试模式
/// @param  //
std::shared_ptr<spdlog::logger> g_default_logger_;

void init_logger(bool mode, const std::string &file, int level)
{
    // 调试模式，创建标准输出日志器，输出等级为最低
    if (false == mode)
    {
        g_default_logger_ = spdlog::stderr_color_mt("defaulr-logger");
        g_default_logger_->set_level(spdlog::level::level_enum::trace);
        g_default_logger_->flush_on(spdlog::level::level_enum::trace);
    }
    else
    {
        g_default_logger_ = spdlog::basic_logger_mt("default-logger", file);
        g_default_logger_->set_level(static_cast<spdlog::level::level_enum>(level));
        g_default_logger_->flush_on(static_cast<spdlog::level::level_enum>(level));
    }
    g_default_logger_->set_pattern("[%n][%H:%M:%S][%t][%-8l]%v");
    // 发布模式，创建文件输出日志器， 输出等级根据参数
}

#define LOG_TRACE(format, ...) g_default_logger_->trace(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##_VA_ARGS__)
#define LOG_DEBUG(format, ...) g_default_logger_->debug(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) g_default_logger_->info(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(format, ...) g_default_logger_->warn(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) g_default_logger_->error(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) g_default_logger_->critical(std::string("[{}:{}] ") + format, __FILE__, __LINE__, ##__VA_ARGS__)
