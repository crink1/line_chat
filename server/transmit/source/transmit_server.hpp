#pragma once
// 语音识别子模块

#include <brpc/server.h>
#include <butil/logging.h>
#include "rabbitmq.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include "mysql_chat_session_member.hpp"
#include "etcd.hpp"   //服务注册封装
#include "logger.hpp" //日志模块封装

#include "base.pb.h"  //protobuf框架代码
#include "transmit.pb.h"
#include "user.pb.h"

namespace crin_lc
{
    class TransmitServiceImpl : public crin_lc::MsgTransmitService
    {
    public:
        TransmitServiceImpl(const std::string &user_service_name,
                            const ServiceManager::ptr &channels,
                            const std::shared_ptr<odb::core::database> &member_client,
                            const std::string &exchange_name,
                            const std::string &routing_key,
                            const MQClient::ptr &mq_client)
            : _user_service_name(user_service_name),
              _mm_channels(channels),
              _mysql_session_member_table(std::make_shared<ChatSessionMemberTable>(member_client)),
              _exchange_name(exchange_name),
              _routing_key(routing_key),
              _mq_client(mq_client)
        {
        }

        ~TransmitServiceImpl() {};

    public:
        void GetTransmitTarget(google::protobuf::RpcController *controller,
                               const ::crin_lc::NewMessageReq *request,
                               ::crin_lc::GetTransmitTargetRsp *response,
                               ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);

            auto err_response = [this, response](const std::string &rid,
                                                 const std::string &errmsg) -> void
            {
                response->set_request_id(rid);
                response->set_success(false);
                response->set_errmsg(errmsg);
                return;
            };
            // 解析响应
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string chat_ssid = request->chat_session_id();
            const MessageContent &content = request->message();

            auto channel = _mm_channels->choose(_user_service_name);
            if (!channel)
            {
                LOG_ERROR("{}-{} 没有可访问的用户子服务节点", rid, _user_service_name);
                return err_response(rid, "没有可访问的用户子服务节点!");
            }
            // 请求用户信息
            UserService_Stub stub(channel.get());
            GetUserInfoReq req;
            GetUserInfoRsp rsp;
            req.set_request_id(rid);
            req.set_user_id(uid);
            brpc::Controller cntl;
            stub.GetUserInfo(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                LOG_ERROR("{} - 用户子服务调用失败：{}！", request->request_id(), cntl.ErrorText());
                return err_response(rid, "用户子服务调用失败!");
            }
            MessageInfo msg;
            msg.set_message_id(uuid());
            msg.set_chat_session_id(chat_ssid);
            msg.set_timestamp(time(nullptr));
            msg.mutable_sender()->CopyFrom(rsp.user_info());
            msg.mutable_message()->CopyFrom(content);

            // 获取转发客户端列表
            auto target_list = _mysql_session_member_table->members(chat_ssid);

            // 把消息给消息队列进行持久化
            bool ret = _mq_client->publish(_exchange_name, msg.SerializeAsString(), _routing_key);
            if (ret == false)
            {
                LOG_ERROR("{} - 消息队列持久化消息失败：{}！", request->request_id(), cntl.ErrorText());
                return err_response(rid, "消息队列持久化消息失败：!");
            }
            // 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            response->mutable_message()->CopyFrom(msg);
            for (const auto &i : target_list)
            {
                response->add_target_id_list(i);
            }
        }

    private:
        // 用户子服务
        std::string _user_service_name;
        ServiceManager::ptr _mm_channels;

        // 聊天会话成员表的操作句柄
        ChatSessionMemberTable::ptr _mysql_session_member_table;

        // mq持久化
        std::string _exchange_name;
        std::string _routing_key;
        MQClient::ptr _mq_client;
    };

    class TransmitServer
    {
    public:
        using ptr = std::shared_ptr<TransmitServer>;
        TransmitServer(const std::shared_ptr<odb::core::database> &mysql_client,
                       const Discovery::ptr &server_discovery,
                       const Registry::ptr &reg_client,
                       const std::shared_ptr<brpc::Server> &server)
            : _mysql_client(mysql_client),
              _service_discover(server_discovery),
              _reg_client(reg_client),
              _rpc_server(server)
        {
        }
        ~TransmitServer() {}
        // rpc服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit(); // 服务器启动，等待运行结果
        }

    private:
        std::shared_ptr<odb::core::database> _mysql_client;
        Discovery::ptr _service_discover; // 服务发现
        Registry::ptr _reg_client;        // 服务注册
        std::shared_ptr<brpc::Server> _rpc_server;
    };

    class TransmitServerBuilder
    {
    public:
        void make_mysql_object(const std::string &user,
                               const std::string &pswd,
                               const std::string &host,
                               const std::string &db,
                               const std::string &cset,
                               int port,
                               int conn_pool_count)
        {
            _mysql_client = ODBFactory::create(user, pswd, host, db, cset, port, conn_pool_count);
        }

        // 构造服务发现和信道
        void make_discovery_object(const std::string &reg_host,
                                   const std::string &base_service_name,
                                   const std::string &user_service_name)
        {
            _user_service_name = user_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->declared(user_service_name);
            auto put_cb = std::bind(&ServiceManager::onserviceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onserviceOffline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            _service_discover = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
        }

        // 构造服务注册客户端
        void make_registry_object(const std::string &reg_host, const std::string &service_name, const std::string &access_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, access_host);
        }

        void make_mq_object(const std::string &user,
                            const std::string &passwd,
                            const std::string &host,
                            const std::string &exchange,
                            const std::string &queue,
                            const std::string &banding_key)
        {
            _routing_key = banding_key;
            _exchange_name = exchange;
            _mq_client = std::make_shared<MQClient>(user, passwd, host);
            _mq_client->declareComponents(exchange, queue, banding_key);
        }

        // 构造RPC服务对象
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads)
        {
            if (!_mq_client)
            {
                LOG_ERROR("还未初始化消息队列模块！");
                abort();
            }
            if (!_mm_channels)
            {
                LOG_ERROR("还未初始化信道管理模块！");
                abort();
            }

            if (!_mysql_client)
            {
                LOG_ERROR("还未初始化MySql模块！");
                abort();
            }

            _rpc_server = std::make_shared<brpc::Server>();
            TransmitServiceImpl *transmit_server = new TransmitServiceImpl(_user_service_name, _mm_channels, _mysql_client, _exchange_name, _routing_key, _mq_client);
            int ret = _rpc_server->AddService(transmit_server, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1)
            {
                LOG_ERROR("添加RPC服务失败！");
                abort();
            }

            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;
            options.num_threads = num_threads;

            ret = _rpc_server->Start(port, &options);
            if (ret == -1)
            {
                LOG_ERROR("服务器启动失败！");
                abort();
            }
        }
        TransmitServer::ptr build()
        {
            if (!_service_discover)
            {
                LOG_ERROR("还未初始化服务发现模块！");
                abort();
            }

            if (!_reg_client)
            {
                LOG_ERROR("还未初始化服务注册模块！");
                abort();
            }

            if (!_rpc_server)
            {
                LOG_ERROR("还未初始化rpc服务器模块！");
                abort();
            }
            TransmitServer::ptr server = std::make_shared<TransmitServer>(_mysql_client, _service_discover, _reg_client, _rpc_server);
            return server;
        }

    private:
        std::string _user_service_name;
        ServiceManager::ptr _mm_channels;
        Discovery::ptr _service_discover; // 服务发现
        Registry::ptr _reg_client;        // 服务注册
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        std::string _exchange_name;
        std::string _routing_key;
        MQClient::ptr _mq_client;
    };
}