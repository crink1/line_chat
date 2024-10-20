#include <iostream>
#include <gflags/gflags.h>
DEFINE_string(ip, "127.0.0.1", "监听地址");
DEFINE_int32(port, 8080, "监听端口");
DEFINE_bool(debug_enable, true, "是否启用调试信息");

using namespace std;

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << FLAGS_ip << endl;
    cout << FLAGS_port << endl;
    cout << FLAGS_debug_enable << endl;
    return 0;
}