#include "../../../common/data_mysql.hpp"
#include "../../../odb/chat_session_member.hxx"
#include "chat_session_member-odb.hxx"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式， false-调试； true-发布;");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

void append_test(crin_lc::ChatSessionMemberTable &tb)
{
    crin_lc::ChatSessionMember csm("会话ID1", "用户ID1");
    tb.append(csm);
    crin_lc::ChatSessionMember csm2("会话ID2", "用户ID2");
    tb.append(csm2);
    crin_lc::ChatSessionMember csm3("会话ID1", "用户ID3");
    tb.append(csm3);
}

void multi_append_test(crin_lc::ChatSessionMemberTable &tb)
{
    crin_lc::ChatSessionMember csm("会话ID3", "用户ID5");
    crin_lc::ChatSessionMember csm2("会话ID3", "用户ID6");
    crin_lc::ChatSessionMember csm3("会话ID2", "用户ID7");
    std::vector<crin_lc::ChatSessionMember> v = {csm, csm2, csm3};
    tb.append(v);
}

void remove_append_test(crin_lc::ChatSessionMemberTable &tb)
{
    crin_lc::ChatSessionMember csm("会话ID3", "用户ID5");
   
    tb.remove(csm);
}
using namespace std;
void select(crin_lc::ChatSessionMemberTable &tb)
{
    auto res = tb.members("会话ID1");
    for(auto &i : res)
    {
        cout << i << endl;
    }
}

void remove_all__test(crin_lc::ChatSessionMemberTable &tb)
{
    tb.remove("会话ID1");
}


int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto db = crin_lc::ODBFactory::create("root", "123456", "127.0.0.1", "crin_lc", "utf8", 0, 1);
    crin_lc::ChatSessionMemberTable csmt(db);
    //append_test(csmt);
    //multi_append_test(csmt);
    //remove_append_test(csmt);
    //select(csmt);
    remove_all__test(csmt);
    return 0;
}