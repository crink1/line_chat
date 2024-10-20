#include "mysql_message.hpp"
#include <gflags/gflags.h>


DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

void insert_test(crin_lc::MessageTable &tb) {
    crin_lc::Message m1("msg1", "session1", "user1", 0, boost::posix_time::time_from_string("1999-09-10 23:59:59.000"));
    tb.insert(m1);
    crin_lc::Message m2("msg2", "session1", "user2", 0, boost::posix_time::time_from_string("1999-09-11 23:59:59.000"));
    tb.insert(m2);
    crin_lc::Message m3("msg3", "session2", "user3", 0, boost::posix_time::time_from_string("1999-09-12 23:59:59.000"));
    tb.insert(m3);

    crin_lc::Message m4("msg4", "session3", "user4", 0, boost::posix_time::time_from_string("1999-09-10 23:59:59.000"));
    tb.insert(m4);
    crin_lc::Message m5("msg5", "session2", "user5", 0, boost::posix_time::time_from_string("1999-09-11 23:59:59.000"));
    tb.insert(m5);
}
void remove_test(crin_lc::MessageTable &tb) {
    tb.remove("session2");
}

void recent_test(crin_lc::MessageTable &tb) {
    auto res = tb.recent("session1", 2);
    auto begin = res.rbegin();
    auto end = res.rend();
    for (; begin != end; ++begin) {
        std::cout << begin->message_id() << std::endl;
        std::cout << begin->session_id() << std::endl;
        std::cout << begin->user_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(begin->create_time()) << std::endl;
    }
}

void range_test(crin_lc::MessageTable &tb) {
    boost::posix_time::ptime stime(boost::posix_time::time_from_string("1999-09-10 23:59:59.000"));
    boost::posix_time::ptime etime(boost::posix_time::time_from_string("1999-09-12 23:59:59.000"));
    auto res = tb.range("session3", stime, etime);
    for (const auto &m : res) {
        std::cout << m.message_id() << std::endl;
        std::cout << m.session_id() << std::endl;
        std::cout << m.user_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(m.create_time()) << std::endl;
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    crin_lc::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto db = crin_lc::ODBFactory::create("root", "123456", "127.0.0.1", "crin_lc", "utf8", 0, 1);
    crin_lc::MessageTable tb(db);
     //insert_test(tb);
     //remove_test(tb);
     //recent_test(tb);
     range_test(tb);
    return 0;
}