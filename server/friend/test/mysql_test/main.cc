#include "mysql_chat_session.hpp"
#include "mysql_apply.hpp"
#include "mysql_relation.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

void r_insert_test(crin_lc::RelationTable &tb)
{
    tb.insert("userID1", "userID2");
    tb.insert("userID1", "userID3");
}
void r_select_test(crin_lc::RelationTable &tb)
{
    auto res = tb.friends("userID1");
    for (auto &uid : res)
    {
        std::cout << uid << std::endl;
    }
}
void r_remove_test(crin_lc::RelationTable &tb)
{
    tb.remove("userID2", "userID1");
}

void r_exists_test(crin_lc::RelationTable &tb)
{
    std::cout << tb.exists("userID2", "userID1") << std::endl;
    std::cout << tb.exists("userID3", "userID1") << std::endl;
}

void a_insert_test(crin_lc::FriendApplyTable &tb)
{
    crin_lc::FriendApply fa1("uuid1", "userID1", "userID2");
    tb.insert(fa1);

    crin_lc::FriendApply fa2("uuid2", "userID1", "userID3");
    tb.insert(fa2);

    crin_lc::FriendApply fa3("uuid3", "userID2", "userID3");
    tb.insert(fa3);
}
void a_remove_test(crin_lc::FriendApplyTable &tb)
{
    tb.remove("userID2", "userID3");
}

void a_select_test(crin_lc::FriendApplyTable &tb)
{
    // crin_lc::FriendApply fa3("uuid3", "userID2", "userID3");
    // tb.insert(fa3);

    auto res = tb.applyUsers("userID2");
    for (auto &uid : res)
    {
        std::cout << uid << std::endl;
    }
}
void a_exists_test(crin_lc::FriendApplyTable &tb)
{
    std::cout << tb.exists("userID1", "userID2") << std::endl;
    std::cout << tb.exists("userID1", "userID3") << std::endl;
    std::cout << tb.exists("userID2", "userID3") << std::endl;
}

void c_insert_test(crin_lc::ChatSessionTable &tb)
{
    crin_lc::ChatSession cs1("sessionID1", "session_name1", crin_lc::ChatSessionType::SINGLE);
    tb.insert(cs1);
    crin_lc::ChatSession cs2("sessionID2", "session_name2", crin_lc::ChatSessionType::GROUP);
    tb.insert(cs2);
}

void c_select_test(crin_lc::ChatSessionTable &tb)
{
    auto res = tb.select("sessionID1");
    std::cout << res->chat_session_id() << std::endl;
    std::cout << res->chat_session_name() << std::endl;
    std::cout << (int)res->chat_session_type() << std::endl;
}

void c_single_test(crin_lc::ChatSessionTable &tb)
{
    auto res = tb.singleChatSession("用户ID6");
    for (auto &info : res)
    {
        std::cout << info.chat_session_id << std::endl;
        std::cout << info.friend_id << std::endl;
    }
}
void c_group_test(crin_lc::ChatSessionTable &tb)
{
    auto res = tb.groupChatSession("d789-8c481279-0000");
    for (auto &info : res)
    {
        std::cout << info.chat_session_id << std::endl;
        std::cout << info.chat_session_name << std::endl;
    }
}
void c_remove_test(crin_lc::ChatSessionTable &tb)
{
    tb.remove("sessionID2");
}
void c_remove_test2(crin_lc::ChatSessionTable &tb)
{
    tb.remove("d789-8c481279-0000", "fcc4-de2ce44b-0001");
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto db = crin_lc::ODBFactory::create("root", "123456", "127.0.0.1", "crin_lc", "utf8", 0, 1);
    crin_lc::RelationTable rtb(db);
    crin_lc::FriendApplyTable ftb(db);
    crin_lc::ChatSessionTable ctb(db);
    // r_insert_test(rtb);
    // r_select_test(rtb);
    // r_remove_test(rtb);
    // r_exists_test(rtb);
    //  a_insert_test(ftb);
    //  a_remove_test(ftb);
    //  a_select_test(ftb);
    //  a_exists_test(ftb);

    // c_insert_test(ctb);
    // c_select_test(ctb);
    //  c_single_test(ctb);
    // c_group_test(ctb);
    // c_remove_test(ctb);
    c_remove_test2(ctb);
    return 0;
}