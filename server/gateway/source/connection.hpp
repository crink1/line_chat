#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "logger.hpp"

namespace crin_lc
{

    // 定义server类型
    typedef websocketpp::server<websocketpp::config::asio> server_t;

    class Connection
    {
    public:
        struct Client
        {
            Client(const std::string &u, const std::string &s)
                : uid(u),
                  ssid(s)
            {
            }
            std::string uid;
            std::string ssid;
        };
        using ptr = std::shared_ptr<Connection>;
        Connection() {}
        ~Connection() {}
        void insert(const server_t::connection_ptr &conn,
                    const std::string &uid,
                    const std::string &ssid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _uid_connections.insert(std::make_pair(uid, conn));
            _conn_clients.insert(std::make_pair(conn, Client(uid, ssid)));
            LOG_DEBUG("新增长连接 {}-{}", uid, ssid);
        }

        server_t::connection_ptr connection(const std::string &uid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _uid_connections.find(uid);
            if (it == _uid_connections.end())
            {
                LOG_ERROR("没有找到 {} 客户端长连接", uid);
                return server_t::connection_ptr();
            }
            LOG_DEBUG("找到 {} 客户端长连接", uid);
            return it->second;
        }

        bool client(const server_t::connection_ptr &conn,
                    std::string &uid,
                    std::string &ssid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conn_clients.find(conn);
            if (it == _conn_clients.end())
            {
                LOG_ERROR("获取--没有找到长连接对应的客户端");
                return false;
            }
            uid = it->second.uid;
            ssid = it->second.ssid;
            LOG_DEBUG("获取长连接对应的客户端成功");
            return true;
        }

        void remove(const server_t::connection_ptr &conn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conn_clients.find(conn);
            if (it == _conn_clients.end())
            {
                LOG_ERROR("删除--没有找到长连接对应的客户端");
            }
            _uid_connections.erase(it->second.uid);
            _conn_clients.erase(it);
            LOG_DEBUG("删除长连接对应的客户端成功");
        }
        private:

    private:
        std::mutex _mutex;
        std::unordered_map<std::string, server_t::connection_ptr> _uid_connections;
        std::unordered_map<server_t::connection_ptr, Client> _conn_clients;
    };
}
