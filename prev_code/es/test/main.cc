#include <iostream>
#include <elasticlient/client.h>
#include<cpr/cpr.h>

int main()
{
    //1.构造es客户端
    elasticlient::Client client({"http://127.0.0.1:9200/"});
    //2.发起搜索
    try
    {
        auto rsp = client.search("user", "_doc", "{\"query\":{\"match_all\":{}}}");
        //3.打印相应状态码和正文
        std::cout << rsp.status_code << std::endl;
        std::cout << rsp.text << std::endl;
    }
    catch(std::exception &e)
    {
        std::cout << "请求失败:" << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}