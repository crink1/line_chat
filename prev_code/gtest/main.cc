#include <iostream>
#include <gtest/gtest.h>
using namespace std;
int add(int x, int y)
{
    return x + y;
}

TEST(测试名称, 加法用例)
{
    ASSERT_EQ(add(10, 20), 30);
    ASSERT_LT(add(10, 10), 30);
}

TEST(测试名称, 字符串用例)
{
    string str = "hello";
    ASSERT_EQ(str, "hello");
    ASSERT_EQ(str, "Hello");
}

int main(int argc, char *argv[])
{
    // 单元测试初始化
    testing::InitGoogleTest(&argc, argv);
    // 开始所有的单元测试
    return RUN_ALL_TESTS();
}