#include "../../../common/data_mysql.hpp"
#include "../../../odb/user.hxx"
#include "user-odb.hxx"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");


void insert(crin_lc::UserTable &user)
{
    
    auto usr1 = std::make_shared<crin_lc::User>("uid1", "nick1", "111");
    user.insert(usr1);

    auto usr2 = std::make_shared<crin_lc::User>("uid2", "15377584596");
    user.insert(usr2);
}

void updata_by_id(crin_lc::UserTable &user)
{
    auto usr1 = user.select_by_id("uid1");
    usr1->description("呜呼呜呼");
    user.update(usr1);
}

void updata_by_phone(crin_lc::UserTable &user)
{
    auto usr1 = user.select_by_phone("15377584596");
    usr1->password("468");
    user.update(usr1);
}

void updata_by_nickname(crin_lc::UserTable &user)
{
    auto usr1 = user.select_by_nickname("uid2");
    usr1->nickname("hfdjsuik");
    user.update(usr1);
}

void selece_users(crin_lc::UserTable &user, std::vector<std::string> &id_list)
{
    auto ret = user.select_multi_users(id_list);
    for(auto usr : ret)
    {
        if(!usr.nickname().empty())
        {
            std::cout << usr.nickname() << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto db = crin_lc::ODBFactory::create("root", "123456", "127.0.0.1", "crin_lc", "utf8", 0, 1);
    crin_lc::UserTable user(db);

    //insert(user);
    //updata_by_id(user);
    //updata_by_phone(user);
    //updata_by_nickname(user);
    std::vector<std::string> vv = {"uid1", "uid2"};
    selece_users(user, vv);
    return 0;
}