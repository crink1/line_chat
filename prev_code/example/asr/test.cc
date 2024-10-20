#include "../../common/asr.hpp"
#include <gflags/gflags.h>

DEFINE_string(app_id, "115756046", "语言APP_ID");
DEFINE_string(api_key, "m2RjE54aDEbDg1rKGHzHG61n", "语音API_ID");
DEFINE_string(api_secret, "R1XGfk2HnWVZIjBZZqo5xO0CmzAjLTEl", "语音API_secret");

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    ASRClient client(FLAGS_app_id, FLAGS_api_key, FLAGS_api_secret);
    std::string file_content;
    aip::get_file_content("./16k.pcm", &file_content);
    std::string res = client.recognize(file_content);
    if(res.empty())
    {
        return -1;
    }
    std::cout << res << std::endl;
    return 0;
}