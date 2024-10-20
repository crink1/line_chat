#include "../common/elasticsearch.hpp"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1.构造es客户端

    std::shared_ptr<elasticlient::Client> client(new elasticlient::Client({"http://127.0.0.1:9200/"}));
    bool ret;
    ESIndex index(client, "test_user");
    ret = index.append("nickname")
                   .append("phone", "keyword", "standard", true)
                   .create();
    if (ret == false)
    {
        LOG_INFO("索引创建失败");
        return -1;

    }
    else
    {
        LOG_INFO("索引创建成功");

    }

    //数据的新增
    ret = ESInsert(client, "test_user")
    .append("nickname", "张三")
    .append("phone", "15566667777")
    .insert("00001");
    if(ret == false)
    {
        LOG_ERROR("数据插入失败！");
        return -1;

    }
    else
    {
        LOG_INFO("数据插入成功");

    }

    //数据的修改
    ret = ESInsert(client, "test_user")
    .append("nickname", "张三")
    .append("phone", "13344445555")
    .insert("00001");
    if(ret == false)
    {
        LOG_ERROR("数据更新失败！");
        return -1;

    }
    else
    {
        LOG_INFO("数据更新成功！");
    }

    Json::Value user = ESSearch(client, "test_user")
    .append_should_match("phone.keyword", "13344445555")
    //.append_must_not_trem("nickname.keyword", {"张三"})
    .search();

    if(user.empty() || user.isArray() == false)
    {
        LOG_ERROR("结果为空或者结果不是数组类型");
        return -1;
    }
    else
    {
        LOG_INFO("数据检索成功！");
        
    }


    int sz = user.size();
    for(int i = 0; i < sz; i++)
    {
        LOG_INFO(user[i]["_source"]["nickname"].asString());
    }

    ret = ESRemove(client, "test_user").remove("00001");
    if(ret == false)
    {
        LOG_ERROR("删除数据失败！");
        return -1;
    }
    else
    {
        LOG_ERROR("删除数据成功！");
    }

    return 0;
}