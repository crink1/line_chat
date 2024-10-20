
#include <brpc/server.h>
#include <butil/logging.h>

#include "data_es.hpp"                   // es数据管理客户端封装
#include "mysql_chat_session_member.hpp" // mysql数据管理客户端封装
#include "mysql_chat_session.hpp"        // mysql数据管理客户端封装
#include "mysql_relation.hpp"            // mysql数据管理客户端封装
#include "mysql_apply.hpp"               // mysql数据管理客户端封装
#include "etcd.hpp"                      // 服务注册模块封装
#include "logger.hpp"                    // 日志模块封装
#include "utils.hpp"                     // 基础工具接口
#include "channel.hpp"                   // 信道管理模块封装

#include "friend.pb.h"  // protobuf框架代码
#include "base.pb.h"    // protobuf框架代码
#include "user.pb.h"    // protobuf框架代码
#include "message.pb.h" // protobuf框架代码

namespace crin_lc
{
    class FriendServiceImpl : public crin_lc::FriendService
    {

    public:
        FriendServiceImpl(
            const std::shared_ptr<elasticlient::Client> &es_client,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const ServiceManager::ptr &channel_manager,
            const std::string &user_service_name,
            const std::string &message_service_name)
            : _es_user(std::make_shared<ESUser>(es_client)),
              _mysql_apply(std::make_shared<FriendApplyTable>(mysql_client)),
              _mysql_chat_session(std::make_shared<ChatSessionTable>(mysql_client)),
              _mysql_chat_session_member(std::make_shared<ChatSessionMemberTable>(mysql_client)),
              _mysql_relation(std::make_shared<RelationTable>(mysql_client)),
              _user_service_name(user_service_name),
              _message_service_name(message_service_name),
              _mm_channels(channel_manager)

        {
        }

        ~FriendServiceImpl() {}

        virtual void GetFriendList(::google::protobuf::RpcController *controller,
                                   const ::crin_lc::GetFriendListReq *request,
                                   ::crin_lc::GetFriendListRsp *response,
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

            // 1.解析响应
            std::string uid = request->user_id();
            std::string rid = request->request_id();

            // 2.数据库查询好友ID
            auto friend_id_list = _mysql_relation->friends(uid);
            std::unordered_set<std::string> user_id_lists;
            for (auto &id : friend_id_list)
            {
                user_id_lists.insert(id);
            }
            // 3.通过用户子服务获取好友信息
            std::unordered_map<std::string, UserInfo> user_list;
            bool ret = GetUserInfo(rid, user_id_lists, user_list);
            if (ret == false)
            {
                LOG_ERROR("{} - 批量获取用户信息失败!", rid);
                return err_response(rid, "批量获取用户信息失败!");
            }
            // 4. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (const auto &user_it : user_list)
            {
                auto user_info = response->add_friend_list();
                user_info->CopyFrom(user_it.second);
            }
        }

        virtual void FriendRemove(::google::protobuf::RpcController *controller,
                                  const ::crin_lc::FriendRemoveReq *request,
                                  ::crin_lc::FriendRemoveRsp *response,
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
            // 1.解析请求
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string pid = request->peer_id();
            // 2.删除好友关系表、会话信息表里面的内容
            bool ret = _mysql_relation->remove(uid, pid);
            if (!ret)
            {
                LOG_ERROR("{} - 从数据库删除好友信息失败", rid);
                return err_response(rid, "从数据库删除好友信息失败");
            }

            ret = _mysql_chat_session->remove(uid, pid);
            if (!ret)
            {
                LOG_ERROR("{} - 从数据库删除好友信息失败", rid);
                return err_response(rid, "从数据库删除好友信息失败");
            }
            // 3.组织响应
            response->set_request_id(rid);
            response->set_success(true);
        }

        // 发生好友申请
        virtual void FriendAdd(::google::protobuf::RpcController *controller,
                               const ::crin_lc::FriendAddReq *request,
                               ::crin_lc::FriendAddRsp *response,
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
            // 解析请求
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string pid = request->respondent_id();
            // 判断是否是好友
            bool ret = _mysql_relation->exists(uid, pid);
            if (ret)
            {
                LOG_ERROR("{} -- {}与{}已经是好友", rid, uid, pid);
                return err_response(rid, "双方已经是好友");
            }
            // 判断是否申请过好友
            ret = _mysql_apply->exists(uid, pid);
            if (ret)
            {
                LOG_ERROR("{} -- {}与{}已存在好友申请", rid, uid, pid);
                return err_response(rid, "已存在好友申请");
            }
            // 新增好友表
            std::string eid = uuid();
            FriendApply fa(eid, uid, pid);
            ret = _mysql_apply->insert(fa);
            if (!ret)
            {
                LOG_ERROR("{} - 从数据库新增好友申请失败", rid);
                return err_response(rid, "从数据库新增好友申请失败");
            }

            // 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            response->set_notify_event_id(eid);
        }

        // 收到好友申请进行处理
        virtual void FriendAddProcess(::google::protobuf::RpcController *controller,
                                      const ::crin_lc::FriendAddProcessReq *request,
                                      ::crin_lc::FriendAddProcessRsp *response,
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
            // 解析请求
            std::string rid = request->request_id();
            std::string eid = request->notify_event_id();
            std::string uid = request->user_id();
            std::string pid = request->apply_user_id();
            bool agree = request->agree();
            // 判断是否申请过
            bool ret = _mysql_apply->exists(pid, uid);
            if (!ret)
            {
                LOG_ERROR("{} -- 用户未申请过好友添加{} -{}", rid, pid, uid);
                return err_response(rid, "用户未申请过好友添加");
            }
            // 申请过-进行处理(删除申请，新增好友关系、会话信息、会话成员)
            ret = _mysql_apply->remove(pid, uid);
            if (!ret)
            {
                LOG_ERROR("{} -- 数据库删除申请事件{} -{} 失败", rid, pid, uid);
                return err_response(rid, "数据库删除申请事件失败");
            }

            std::string cssid;

            if (agree)
            {
                ret = _mysql_relation->insert(uid, pid);
                if (!ret)
                {
                    LOG_ERROR("{} -- 新增好友关系失败-{}-{}", rid, uid, pid);
                    return err_response(rid, "新增好友关系失败");
                }
                cssid = uuid();
                ChatSession cs(cssid, "", ChatSessionType::SINGLE);
                ret = _mysql_chat_session->insert(cs);
                if (!ret)
                {
                    LOG_ERROR("{} -- 新增单聊会话信息失败-{}", rid, cssid);
                    return err_response(rid, "新增单聊会话信息失败");
                }
                ChatSessionMember csm1(cssid, uid);
                ChatSessionMember csm2(cssid, pid);
                std::vector<ChatSessionMember> v = {csm1, csm2};
                ret = _mysql_chat_session_member->append(v);
                if (!ret)
                {
                    LOG_ERROR("{} --新增单聊会话成员信息失败失败", rid);
                    return err_response(rid, "新增单聊会话成员信息失败失败");
                }
            }
            // 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            response->set_new_session_id(cssid);
        }

        virtual void FriendSearch(::google::protobuf::RpcController *controller,
                                  const ::crin_lc::FriendSearchReq *request,
                                  ::crin_lc::FriendSearchRsp *response,
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

            // 解析请求，搜索关键字（用户ID、手机号、昵称）
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string skey = request->search_key();
            // 获取用户好友ID列表
            std::vector<std::string> friend_id_list = _mysql_relation->friends(uid);
            friend_id_list.push_back(uid);
            // ES进行用户搜索,把好友过滤掉
            std::vector<User> search_es = _es_user->search(skey, friend_id_list);

            std::unordered_set<std::string> user_id_hash;
            for (auto &i : search_es)
            {
                user_id_hash.insert(i.user_id());
            }
           
            // 根据es获取的用户id，从用户子服务进行信息获取
            std::unordered_map<std::string, UserInfo> user_list;
            bool ret = GetUserInfo(rid, user_id_hash, user_list);
            if (ret == false)
            {
                LOG_ERROR("{} - 批量获取用户信息失败!", rid);
                return err_response(rid, "批量获取用户信息失败!");
            }
            // 5. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (const auto &user_it : user_list)
            {
                auto user_info = response->add_user_info();
                user_info->CopyFrom(user_it.second);
            }
        }

        virtual void GetChatSessionList(::google::protobuf::RpcController *controller,
                                        const ::crin_lc::GetChatSessionListReq *request,
                                        ::crin_lc::GetChatSessionListRsp *response,
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
            // 数据库查询用户单聊列表和群聊列表
            auto sf_list = _mysql_chat_session->singleChatSession(uid);
            //  1. 从单聊会话列表中，取出所有的好友ID，从用户子服务获取用户信息
            std::unordered_set<std::string> users_id_list;
            for (const auto &f : sf_list)
            {
                users_id_list.insert(f.friend_id);
            }
            std::unordered_map<std::string, UserInfo> user_list;
            bool ret = GetUserInfo(rid, users_id_list, user_list);
            if (ret == false)
            {
                LOG_ERROR("{} - 批量获取用户信息失败！", rid);
                return err_response(rid, "批量获取用户信息失败!");
            }
            //  2. 设置响应会话信息：会话名称就是好友名称；会话头像就是好友头像
            // 3. 从数据库中查询出用户的群聊会话列表
            auto gc_list = _mysql_chat_session->groupChatSession(uid);

            // 4. 根据所有的会话ID，从消息存储子服务获取会话最后一条消息
            // 5. 组织响应
            for (const auto &f : sf_list)
            {
                auto chat_session_info = response->add_chat_session_info_list();
                chat_session_info->set_single_chat_friend_id(f.friend_id);
                chat_session_info->set_chat_session_id(f.chat_session_id);
                chat_session_info->set_chat_session_name(user_list[f.friend_id].nickname());
                chat_session_info->set_avatar(user_list[f.friend_id].avatar());
                MessageInfo msg;
                ret = GetRecentMsg(rid, f.chat_session_id, msg);
                if (ret == false)
                {
                    continue;
                }
                chat_session_info->mutable_prev_message()->CopyFrom(msg);
            }
            for (const auto &f : gc_list)
            {
                auto chat_session_info = response->add_chat_session_info_list();
                chat_session_info->set_chat_session_id(f.chat_session_id);
                chat_session_info->set_chat_session_name(f.chat_session_name);
                MessageInfo msg;
                ret = GetRecentMsg(rid, f.chat_session_id, msg);
                if (ret == false)
                {
                    continue;
                }
                chat_session_info->mutable_prev_message()->CopyFrom(msg);
            }
            response->set_request_id(rid);
            response->set_success(true);
        }

        virtual void ChatSessionCreate(::google::protobuf::RpcController *controller,
                                       const ::crin_lc::ChatSessionCreateReq *request,
                                       ::crin_lc::ChatSessionCreateRsp *response,
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

            // 创建会话，主要是群聊会话
            // 1.解析请求（会话名称，成员）
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string cssname = request->chat_session_name();
            std::string cssid = uuid();

            // 生成会话ID，向数据库添加会话信息，添加成员信息
            ChatSession cs(cssid, cssname, ChatSessionType::GROUP);
            bool ret = _mysql_chat_session->insert(cs);
            if (ret == false)
            {
                LOG_ERROR("{} -向数据库添加会话信息失败：{}", rid, cssname);
                return err_response(rid, "向数据库添加会话信息失败!");
            }

            std::vector<ChatSessionMember> member_list;
            for (int i = 0; i < request->member_id_list_size(); i++)
            {
                ChatSessionMember csm(cssid, request->member_id_list(i));
                member_list.push_back(csm);
            }

            ret = _mysql_chat_session_member->append(member_list);
            if (ret == false)
            {
                LOG_ERROR("{} - 向数据库添加会话成员信息失败: {}", rid, cssname);
                return err_response(rid, "向数据库添加会话成员信息失败!");
            }
            // 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            response->mutable_chat_session_info()->set_chat_session_id(cssid);
            response->mutable_chat_session_info()->set_chat_session_name(cssname);
        }

        virtual void GetChatSessionMember(::google::protobuf::RpcController *controller,
                                          const ::crin_lc::GetChatSessionMemberReq *request,
                                          ::crin_lc::GetChatSessionMemberRsp *response,
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

            // 1. 提取关键要素：聊天会话ID
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            std::string cssid = request->chat_session_id();
            // 2. 从数据库获取会话成员ID列表
            auto member_id_lists = _mysql_chat_session_member->members(cssid);
            std::unordered_set<std::string> uid_list;
            for (const auto &id : member_id_lists)
            {
                uid_list.insert(id);
            }
            // 3. 从用户子服务批量获取用户信息
            std::unordered_map<std::string, UserInfo> user_list;
            bool ret = GetUserInfo(rid, uid_list, user_list);
            if (ret == false)
            {
                LOG_ERROR("{} - 从用户子服务获取用户信息失败！", rid);
                return err_response(rid, "从用户子服务获取用户信息失败!");
            }
            // 4. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (const auto &uit : user_list)
            {
                auto user_info = response->add_member_info_list();
                user_info->CopyFrom(uit.second);
            }
        }

        virtual void GetPendingFriendEventList(::google::protobuf::RpcController *controller,
                                               const ::crin_lc::GetPendingFriendEventListReq *request,
                                               ::crin_lc::GetPendingFriendEventListRsp *response,
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
            // 解析请求
            std::string rid = request->request_id();
            std::string uid = request->user_id();
            // 数据库获取列表
            auto ret = _mysql_apply->applyUsers(uid);
            // 获取用户信息
            std::unordered_map<std::string, UserInfo> user_list;
            if (!ret.empty())
            {
                std::unordered_set<std::string> user_id_lists;
                for (auto &id : ret)
                {
                    user_id_lists.insert(id);
                }
                bool res = GetUserInfo(rid, user_id_lists, user_list);
                if (!res)
                {
                    LOG_ERROR("{} - 批量获取用户信息失败!", rid);
                    return err_response(rid, "批量获取用户信息失败");
                }
            }
            // 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (const auto &i : user_list)
            {
                auto ev = response->add_event();
                ev->mutable_sender()->CopyFrom(i.second);
            }
        }

    private:
        bool GetRecentMsg(const std::string &rid,
                          const std::string &cssid, MessageInfo &msg)
        {
            auto channel = _mm_channels->choose(_message_service_name);
            if (!channel)
            {
                LOG_ERROR("{} - 获取消息子服务信道失败！！", rid);
                return false;
            }
            GetRecentMsgReq req;
            GetRecentMsgRsp rsp;
            req.set_request_id(rid);
            req.set_chat_session_id(cssid);
            req.set_msg_count(1);
            brpc::Controller cntl;
            crin_lc::MsgStorageService_Stub stub(channel.get());
            stub.GetRecentMsg(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true)
            {
                LOG_ERROR("{} - 消息存储子服务调用失败: {}", rid, cntl.ErrorText());
                return false;
            }
            if (rsp.success() == false)
            {
                LOG_ERROR("{} - 获取会话 {} 最近消息失败: {}", rid, cssid, rsp.errmsg());
                return false;
            }
            if (rsp.msg_list_size() > 0)
            {
                msg.CopyFrom(rsp.msg_list(0));
                return true;
            }
            return false;
        }

        bool GetUserInfo(const std::string &rid, const std::unordered_set<std::string> &uid_list, std::unordered_map<std::string, UserInfo> &user_list)
        {

            auto channel = _mm_channels->choose(_user_service_name);
            if (!channel)
            {
                LOG_ERROR("{} - 获取用户子服务信道失败!", rid);
                return false;
            }

            GetMultiUserInfoReq req;
            GetMultiUserInfoRsp resp;
            req.set_request_id(rid);
            for (auto &i : uid_list)
            {
                req.add_users_id(i);
            }

            brpc::Controller cntl;
            crin_lc::UserService_Stub stub(channel.get());
            stub.GetMultiUserInfo(&cntl, &req, &resp, nullptr);
            if (cntl.Failed() == true)
            {
                LOG_ERROR("{} - 获取用户子服务调用失败: {}", rid, cntl.ErrorText());
                return false;
            }
            if (resp.success() == false)
            {
                LOG_ERROR("{} -批量获取用户信息响应失败: {}", rid, resp.errmsg());
                return false;
            }

            for (const auto &i : resp.users_info())
            {
                user_list.insert(std::make_pair(i.first, i.second));
            }
            return true;
        }

    private:
        ESUser::ptr _es_user;

        FriendApplyTable::ptr _mysql_apply;
        ChatSessionTable::ptr _mysql_chat_session;
        ChatSessionMemberTable::ptr _mysql_chat_session_member;
        RelationTable::ptr _mysql_relation;

        std::string _user_service_name;
        std::string _message_service_name;
        ServiceManager::ptr _mm_channels;
    };

    class FriendServer
    {
    public:
        using ptr = std::shared_ptr<FriendServer>;
        FriendServer(const Discovery::ptr &service_discover,
                     const Registry::ptr &registry_client,
                     const std::shared_ptr<elasticlient::Client> &es_client,
                     const std::shared_ptr<odb::core::database> &mysql_client,
                     const std::shared_ptr<brpc::Server> &server)
            : _service_discover(service_discover),
              _registry_client(registry_client),
              _es_client(es_client),
              _mysql_client(mysql_client),
              _rpc_server(server)
        {
        }
        ~FriendServer() {}
        // rpc服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit(); // 服务器启动，等待运行结果
        }

    private:
        Discovery::ptr _service_discover;
        Registry::ptr _registry_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
    };

    class FriendServerBuilder
    {
    public:
        // 构造es
        void make_es_object(const std::vector<std::string> &host_list)
        {
            _es_client = ESClientFactory::create(host_list);
        }

        // 构造mysql
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
                                   const std::string &user_service_name,
                                   const std::string &message_service_name)
        {
            _user_service_name = user_service_name;
            _message_service_name = message_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->declared(user_service_name);
            _mm_channels->declared(message_service_name);
            LOG_DEBUG("添加管理用户子服务：{}", user_service_name);
            LOG_DEBUG("添加管理消息子服务：{}", message_service_name);
            auto put_cb = std::bind(&ServiceManager::onserviceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onserviceOffline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            _service_discover = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
        }

        // 构造服务注册客户端
        void make_registry_object(const std::string &reg_host, const std::string &service_name, const std::string &access_host)
        {
            _registry_client = std::make_shared<Registry>(reg_host);
            _registry_client->registry(service_name, access_host);
        }

        // 构造RPC服务对象
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads)
        {

            if (!_es_client)
            {
                LOG_ERROR("还未初始化搜索引擎模块！");
                abort();
            }

            if (!_mysql_client)
            {
                LOG_ERROR("还未初始化MySql模块！");
                abort();
            }

            if (!_mm_channels)
            {
                LOG_ERROR("还未初始化信道管理模块！");
                abort();
            }

            _rpc_server = std::make_shared<brpc::Server>();
            FriendServiceImpl *friend_service = new FriendServiceImpl(
                _es_client,
                _mysql_client,
                _mm_channels,
                _user_service_name,
                _message_service_name);
            int ret = _rpc_server->AddService(friend_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        FriendServer::ptr build()
        {

            if (!_service_discover)
            {
                LOG_ERROR("还未初始化服务发现模块！");
                abort();
            }

            if (!_registry_client)
            {
                LOG_ERROR("还未初始化服务注册模块！");
                abort();
            }

            if (!_rpc_server)
            {
                LOG_ERROR("还未初始化rpc服务器模块！");
                abort();
            }
            FriendServer::ptr server = std::make_shared<FriendServer>(_service_discover, _registry_client, _es_client, _mysql_client, _rpc_server);
            return server;
        }

    private:
        std::string _user_service_name;
        std::string _message_service_name;
        Registry::ptr _registry_client;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        Discovery::ptr _service_discover;
        ServiceManager::ptr _mm_channels;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
}