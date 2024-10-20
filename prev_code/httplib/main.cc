#include "../common/httplib.h"



int main()
{
    //1.实例化服务对象
    httplib::Server server;
    //2.注册回调函数
    server.Get("/hi", [](const httplib::Request &req,  httplib::Response &resp){
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        
        for(auto &i : req.headers)
        {
            std::cout << i.first << ":" << i.second << std::endl;
        }

        resp.status = 200;
        std::string body = "<html lang=\"zh-CN\"><head><meta charset=\"UTF-8\"><title>DarkOrLight</title></head><body><h1>芜湖</h1></body></html>";
        resp.set_content(body, "text/html");
    });
    //3.启动服务器
    server.listen("0.0.0.0", 9090);
    return 0;
}