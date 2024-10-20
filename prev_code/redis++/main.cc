#include <sw/redis++/redis.h>
#include <gflags/gflags.h>
#include <iostream>
#include <thread>

DEFINE_string(ip, "127.0.0.1", "监听地址");
DEFINE_int32(port, 6379, "监听端口");
DEFINE_int32(db, 0, "库的编号：默认0");
DEFINE_bool(keep_alive, true, "是否长连接保活");


void print(sw::redis::Redis &client)
{
    auto user1 = client.get("会话ID0");
    if(user1)
    {
        std::cout << *user1 << std::endl;
    }

    auto user2 = client.get("会话ID1");
    if(user2)
    {
        std::cout << *user2 << std::endl;
    }

    auto user3 = client.get("会话ID2");
    if(user3)
    {
        std::cout << *user3 << std::endl;
    }

    auto user4 = client.get("会话ID3");
    if(user4)
    {
        std::cout << *user4 << std::endl;
    }
}

void add_string(sw::redis::Redis &client)
{
    client.set("会话ID0", "用户ID0");
    client.set("会话ID1", "用户ID1");
    client.set("会话ID2", "用户ID2");
    client.set("会话ID3", "用户ID3");

    client.del("会话ID2");

    client.set("会话ID3", "用户ID335666666");

    print(client);
    
}

void expired_test(sw::redis::Redis &client)
{
    client.set("会话ID0", "用户0000000", std::chrono::milliseconds(1000));
    print(client);
    std::cout << "休眠" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    print(client);

}

void list_test(sw::redis::Redis &client)
{
    client.rpush("群聊1", "成员1");
    client.rpush("群聊1", "成员2");
    client.rpush("群聊1", "成员3");
    client.rpush("群聊1", "成员4");
    client.lpush("群聊1", "成员0");


    std::vector<std::string> users;
    client.lrange("群聊1", 0, -1, std::back_inserter(users));

    for(auto &i : users)
    {
        std::cout << i << std::endl;
    }
}



int main(int argc, char *argv[])
{
    //1.实例化redis对象，构造连接选项， 连接服务器
    sw::redis::ConnectionOptions opts;
    opts.host = FLAGS_ip;
    opts.port = FLAGS_port;
    opts.db = FLAGS_db;
    opts.keep_alive = FLAGS_keep_alive;

    sw::redis::Redis client(opts);
    //2.添加字符串键值对，删除字符串键值对，获取键值对
    add_string(client);
    //3.实践控制数据的有效时间
    expired_test(client);
    //4.列表的操作，主要实现数据的尾插，获取
    std::cout << "-----------" << std::endl;
    list_test(client);
    return 0;
}