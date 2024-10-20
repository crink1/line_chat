
#include "data_redis.hpp"
#include "data_es.hpp" // es数据管理客户端封装
#include "etcd.hpp"    // 服务注册模块封装
#include "logger.hpp"  // 日志模块封装
#include "channel.hpp" // 信道管理模块封装
#include "connection.hpp"
#include "httplib.h"

#include "friend.pb.h"   // protobuf框架代码
#include "base.pb.h"     // protobuf框架代码
#include "user.pb.h"     // protobuf框架代码
#include "gateway.pb.h"  // protobuf框架代码
#include "notify.pb.h"   // protobuf框架代码
#include "speech.pb.h"   // protobuf框架代码
#include "transmit.pb.h" // protobuf框架代码
#include "message.pb.h"  // protobuf框架代码
#include "file.pb.h"     // protobuf框架代码

namespace crin_lc
{
#define GET_PHONE_VERIFY_CODE "/service/user/get_phone_verify_code"
#define USERNAME_REGISTER "/service/user/username_register"
#define USERNAME_LOGIN "/service/user/username_login"
#define PHONE_REGISTER "/service/user/phone_register"
#define PHONE_LOGIN "/service/user/phone_login"
#define GET_USERINFO "/service/user/get_user_info"
#define SET_USER_AVATAR "/service/user/set_avatar"
#define SET_USER_NICKNAME "/service/user/set_nickname"
#define SET_USER_DESC "/service/user/set_description"
#define SET_USER_PHONE "/service/user/set_phone"
#define FRIEND_GET_LIST "/service/friend/get_friend_list"
#define FRIEND_APPLY "/service/friend/add_friend_apply"
#define FRIEND_APPLY_PROCESS "/service/friend/add_friend_process"
#define FRIEND_REMOVE "/service/friend/remove_friend"
#define FRIEND_SEARCH "/service/friend/search_friend"
#define FRIEND_GET_PENDING_EV "/service/friend/get_pending_friend_events"
#define CSS_GET_LIST "/service/friend/get_chat_session_list"
#define CSS_CREATE "/service/friend/create_chat_session"
#define CSS_GET_MEMBER "/service/friend/get_chat_session_member"
#define MSG_GET_RANGE "/service/message_storage/get_history"
#define MSG_GET_RECENT "/service/message_storage/get_recent"
#define MSG_KEY_SEARCH "/service/message_storage/search_history"
#define NEW_MESSAGE "/service/message_transmit/new_message"
#define FILE_GET_SINGLE "/service/file/get_single_file"
#define FILE_GET_MULTI "/service/file/get_multi_file"
#define FILE_PUT_SINGLE "/service/file/put_single_file"
#define FILE_PUT_MULTI "/service/file/put_multi_file"
#define SPEECH_RECOGNITION "/service/speech/recognition"
    class GatewayServer
    {
    public:
        using ptr = std::shared_ptr<GatewayServer>;
        GatewayServer(int websocket_port,
                      int http_port,
                      const std::shared_ptr<sw::redis::Redis> &redis_client,
                      const ServiceManager::ptr &channels,
                      const Discovery::ptr &service_discoverer,
                      const std::string &user_service,
                      const std::string &file_service,
                      const std::string &speech_service,
                      const std::string &message_service,
                      const std::string &transmit_service,
                      const std::string &friend_service)
            : _redis_session(std::make_shared<Session>(redis_client)),
              _redis_status(std::make_shared<Status>(redis_client)),
              _mm_channels(channels),
              _service_discoverer(service_discoverer),
              _user_service(user_service),
              _file_service(file_service),
              _speech_service(speech_service),
              _message_service(message_service),
              _transmit_service(transmit_service),
              _friend_service(friend_service),
              _connections(std::make_shared<Connection>())

        {
            _ws_server.set_access_channels(websocketpp::log::alevel::none);

            // 初始化asio框架
            _ws_server.init_asio();

            // 设置消息、连接握手、连接关闭的回调函数
            _ws_server.set_open_handler(std::bind(&GatewayServer::onOpen, this, std::placeholders::_1));
            _ws_server.set_close_handler(std::bind(&GatewayServer::onClose, this, std::placeholders::_1));

            auto wscb = std::bind(&GatewayServer::onMessage, this, std::placeholders::_1, std::placeholders::_2);
            _ws_server.set_message_handler(wscb);

            // 设置地址重用
            _ws_server.set_reuse_addr(true);
            // 设置监听端口
            _ws_server.listen(websocket_port);
            // 开始监听
            _ws_server.start_accept();

            _http_server.Post(GET_PHONE_VERIFY_CODE, (httplib::Server::Handler)std::bind(&GatewayServer::GetPhoneVerifyCode, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(USERNAME_REGISTER, (httplib::Server::Handler)std::bind(&GatewayServer::UserRegister, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(USERNAME_LOGIN, (httplib::Server::Handler)std::bind(&GatewayServer::UserLogin, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(PHONE_REGISTER, (httplib::Server::Handler)std::bind(&GatewayServer::PhoneRegister, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(PHONE_LOGIN, (httplib::Server::Handler)std::bind(&GatewayServer::PhoneLogin, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(GET_USERINFO, (httplib::Server::Handler)std::bind(&GatewayServer::GetUserInfo, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(SET_USER_AVATAR, (httplib::Server::Handler)std::bind(&GatewayServer::SetUserAvatar, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(SET_USER_NICKNAME, (httplib::Server::Handler)std::bind(&GatewayServer::SetUserNickname, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(SET_USER_DESC, (httplib::Server::Handler)std::bind(&GatewayServer::SetUserDescription, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(SET_USER_PHONE, (httplib::Server::Handler)std::bind(&GatewayServer::SetUserPhoneNumber, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_GET_LIST, (httplib::Server::Handler)std::bind(&GatewayServer::GetFriendList, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_APPLY, (httplib::Server::Handler)std::bind(&GatewayServer::FriendAdd, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_APPLY_PROCESS, (httplib::Server::Handler)std::bind(&GatewayServer::FriendAddProcess, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_REMOVE, (httplib::Server::Handler)std::bind(&GatewayServer::FriendRemove, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_SEARCH, (httplib::Server::Handler)std::bind(&GatewayServer::FriendSearch, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FRIEND_GET_PENDING_EV, (httplib::Server::Handler)std::bind(&GatewayServer::GetPendingFriendEventList, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(CSS_GET_LIST, (httplib::Server::Handler)std::bind(&GatewayServer::GetChatSessionList, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(CSS_CREATE, (httplib::Server::Handler)std::bind(&GatewayServer::ChatSessionCreate, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(CSS_GET_MEMBER, (httplib::Server::Handler)std::bind(&GatewayServer::GetChatSessionMember, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(MSG_GET_RANGE, (httplib::Server::Handler)std::bind(&GatewayServer::GetHistoryMsg, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(MSG_GET_RECENT, (httplib::Server::Handler)std::bind(&GatewayServer::GetRecentMsg, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(MSG_KEY_SEARCH, (httplib::Server::Handler)std::bind(&GatewayServer::MsgSearch, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(NEW_MESSAGE, (httplib::Server::Handler)std::bind(&GatewayServer::NewMessage, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FILE_GET_SINGLE, (httplib::Server::Handler)std::bind(&GatewayServer::GetSingleFile, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FILE_GET_MULTI, (httplib::Server::Handler)std::bind(&GatewayServer::GetMultiFile, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FILE_PUT_SINGLE, (httplib::Server::Handler)std::bind(&GatewayServer::PutSingleFile, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(FILE_PUT_MULTI, (httplib::Server::Handler)std::bind(&GatewayServer::PutMultiFile, this, std::placeholders::_1, std::placeholders::_2));

            _http_server.Post(SPEECH_RECOGNITION, (httplib::Server::Handler)std::bind(&GatewayServer::SpeechRecognition, this, std::placeholders::_1, std::placeholders::_2));

            _http_thread = std::thread([this, http_port]()
                                       { _http_server.listen("0.0.0.0", http_port); });
            _http_thread.detach();
        }
        void start()
        {
            _ws_server.run();
        }

    private:
        void onOpen(websocketpp::connection_hdl hdl)
        {
            std::cout << "websocket长连接建立成功！\n";
        }

        void onClose(websocketpp::connection_hdl hdl)
        {

            // 通过连接对象，获取对应的用户ID与登录会话ID
            auto conn = _ws_server.get_con_from_hdl(hdl);
            std::string uid, ssid;
            bool ret = _connections->client(conn, uid, ssid);
            if (ret == false)
            {
                LOG_WARN("长连接断开，未找到长连接对应的客户端信息！");
                return;
            }
            // 移除登录会话信息
            _redis_session->remove(ssid);
            // 移除登录状态信息
            _redis_status->remove(uid);
            // 移除长连接管理数据
            _connections->remove(conn);
            LOG_DEBUG("{} {} {} 长连接断开，清理缓存数据!", ssid, uid, (size_t)conn.get());
        }

        void keepAlive(server_t::connection_ptr conn)
        {
            if (!conn || conn->get_state() != websocketpp::session::state::value::open)
            {
                LOG_DEBUG("结束连接保活");
                return;
            }
            conn->ping("");
            _ws_server.set_timer(60000, std::bind(&GatewayServer::keepAlive, this, conn));
        }

        void onMessage(websocketpp::connection_hdl hdl, server_t::message_ptr msg)
        {
            // 取出长连接对象
            auto conn = _ws_server.get_con_from_hdl(hdl);
            // 解析请求
            ClientAuthenticationReq request;
            bool ret = request.ParseFromString(msg->get_payload());
            if (!ret)
            {
                LOG_ERROR("长连接建立失败,反序列化失败");
                _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "反序列化失败");
                return;
            }
            // redis查找会话消息
            std::string ssid = request.session_id();
            auto uid = _redis_session->uid(ssid);
            // 会话不存在就关闭连接
            if (!uid)
            {
                LOG_ERROR("长连接建立失败,未找到会话信息 {}!", ssid);
                _ws_server.close(hdl, websocketpp::close::status::unsupported_data, "反序列化失败");
                return;
            }
            // 存在就添加长连接
            _connections->insert(conn, *uid, ssid);
            LOG_DEBUG("新增长连接管理：{}-{}-{}", ssid, *uid, (uint64_t)conn.get());
            keepAlive(conn);
        }

        void GetPhoneVerifyCode(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            PhoneVerifyCodeReq req;
            PhoneVerifyCodeRsp resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取短信验证码请求正文反序列化失败");
                return err_response("获取短信验证码请求正文反序列化失败");
            }
            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetPhoneVerifyCode(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void UserRegister(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            UserRegisterReq req;
            UserRegisterRsp resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("用户名注册正文反序列化失败");
                return err_response("用户名注册正文反序列化失败");
            }
            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.UserRegister(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void UserLogin(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            UserLoginReq req;
            UserLoginRsp resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("用户名登录正文反序列化失败");
                return err_response("用户名登录正文反序列化失败");
            }
            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.UserLogin(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void PhoneRegister(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            PhoneRegisterReq req;
            PhoneRegisterRsp resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("手机号注册正文反序列化失败");
                return err_response("手机号注册正文反序列化失败");
            }
            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.PhoneRegister(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void PhoneLogin(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            PhoneLoginReq req;
            PhoneLoginRsp resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("手机号登录正文反序列化失败");
                return err_response("手机号登录正文反序列化失败");
            }
            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.PhoneLogin(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetUserInfo(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            GetUserInfoReq req;
            GetUserInfoRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取用户信息正文反序列化失败");
                return err_response("获取用户信息正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetUserInfo(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void SetUserAvatar(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            SetUserAvatarReq req;
            SetUserAvatarRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("设置头像正文反序列化失败");
                return err_response("设置头像正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.SetUserAvatar(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void SetUserNickname(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            SetUserNicknameReq req;
            SetUserNicknameRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("设置用户昵称正文反序列化失败");
                return err_response("设置用户昵称正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.SetUserNickname(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void SetUserDescription(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            SetUserDescriptionReq req;
            SetUserDescriptionRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("设置用户签名正文反序列化失败");
                return err_response("设置用户签名正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.SetUserDescription(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void SetUserPhoneNumber(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            SetUserPhoneNumberReq req;
            SetUserPhoneNumberRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("设置用户手机号正文反序列化失败");
                return err_response("设置用户手机号正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.SetUserPhoneNumber(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return err_response("用户子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetFriendList(const httplib::Request &request, httplib::Response &response)
        {
            // 1. 取出http请求正文，将正文进行反序列化
            GetFriendListReq req;
            GetFriendListRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取好友列表正文反序列化失败");
                return err_response("获取好友列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            // 2. 将请求转发给用户子服务进行业务处理
            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetFriendList(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            // 3. 得到用户子服务的响应后，将响应内容进行序列化作为http响应正文
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        std::shared_ptr<GetUserInfoRsp> _GetUserInfo(const std::string &rid, const std::string &uid)
        {

            GetUserInfoReq req;
            std::shared_ptr<GetUserInfoRsp> resp = std::make_shared<GetUserInfoRsp>();
            req.set_request_id(rid);
            req.set_user_id(uid);

            auto channel = _mm_channels->choose(_user_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return std::shared_ptr<GetUserInfoRsp>();
            }
            crin_lc::UserService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetUserInfo(&cntl, &req, resp.get(), nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 用户子服务调用失败", req.request_id());
                return std::shared_ptr<GetUserInfoRsp>();
            }
            return resp;
        }

        void FriendAdd(const httplib::Request &request, httplib::Response &response)
        {
            // 申请完还要通知被申请用户

            // 正文反序列化，解析请求
            FriendAddReq req;
            FriendAddRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("好友申请正文反序列化失败");
                return err_response("好友申请正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.FriendAdd(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            auto conn = _connections->connection(req.respondent_id());
            if (resp.success() && conn)
            {

                auto user_rsp = _GetUserInfo(req.request_id(), *uid);
                if (!user_rsp)
                {
                    LOG_ERROR("{} 获取当前客户端用户信息失败!", req.request_id());
                    return err_response("获取当前客户端用户信息失败");
                }
                NotifyMessage notify;
                notify.set_notify_type(NotifyType::FRIEND_ADD_APPLY_NOTIFY);
                notify.mutable_friend_add_apply()->mutable_user_info()->CopyFrom(user_rsp->user_info());
                conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void FriendAddProcess(const httplib::Request &request, httplib::Response &response)
        {
            FriendAddProcessReq req;
            FriendAddProcessRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("好友申请处理正文反序列化失败");
                return err_response("好友申请处理正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的用户服务子节点", req.request_id());
                return err_response("没有可用的用户服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.FriendAddProcess(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }

            if (resp.success())
            {
                auto process_user_rsp = _GetUserInfo(req.request_id(), *uid);
                if (!process_user_rsp)
                {
                    LOG_ERROR("{} 获取用户信息失败！", req.request_id());
                    return err_response("获取用户信息失败！");
                }
                auto apply_user_rsp = _GetUserInfo(req.request_id(), req.apply_user_id());
                if (!process_user_rsp)
                {
                    LOG_ERROR("{} 获取用户信息失败！", req.request_id());
                    return err_response("获取用户信息失败！");
                }
                auto process_conn = _connections->connection(*uid);
                if (process_conn)
                    LOG_DEBUG("找到处理人的长连接！");
                else
                    LOG_DEBUG("未找到处理人的长连接！");
                auto apply_conn = _connections->connection(req.apply_user_id());
                if (apply_conn)
                    LOG_DEBUG("找到申请人的长连接！");
                else
                    LOG_DEBUG("未找到申请人的长连接！");
                // 将处理结果给申请人进行通知
                if (apply_conn)
                {
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::FRIEND_ADD_PROCESS_NOTIFY);
                    auto process_result = notify.mutable_friend_process_result();
                    process_result->mutable_user_info()->CopyFrom(process_user_rsp->user_info());
                    process_result->set_agree(req.agree());
                    apply_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                    LOG_DEBUG("对申请人进行申请处理结果通知！");
                }
                // 若处理结果是同意 --- 会伴随着单聊会话的创建 -- 因此需要对双方进行会话创建的通知
                if (req.agree() && apply_conn)
                { // 对申请人的通知---会话信息就是处理人信息
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                    auto chat_session = notify.mutable_new_chat_session_info();
                    chat_session->mutable_chat_session_info()->set_single_chat_friend_id(*uid);
                    chat_session->mutable_chat_session_info()->set_chat_session_id(resp.new_session_id());
                    chat_session->mutable_chat_session_info()->set_chat_session_name(process_user_rsp->user_info().nickname());
                    chat_session->mutable_chat_session_info()->set_avatar(process_user_rsp->user_info().avatar());
                    apply_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                    LOG_DEBUG("对申请人进行会话创建通知！");
                }
                if (req.agree() && process_conn)
                { // 对处理人的通知 --- 会话信息就是申请人信息
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                    auto chat_session = notify.mutable_new_chat_session_info();
                    chat_session->mutable_chat_session_info()->set_single_chat_friend_id(req.apply_user_id());
                    chat_session->mutable_chat_session_info()->set_chat_session_id(resp.new_session_id());
                    chat_session->mutable_chat_session_info()->set_chat_session_name(apply_user_rsp->user_info().nickname());
                    chat_session->mutable_chat_session_info()->set_avatar(apply_user_rsp->user_info().avatar());
                    process_conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                    LOG_DEBUG("对处理人进行会话创建通知！");
                }
            }
            // 对客户端进行响应
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void FriendRemove(const httplib::Request &request, httplib::Response &response)
        {
            FriendRemoveReq req;
            FriendRemoveRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("删除好友正文反序列化失败");
                return err_response("删除好友正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.FriendRemove(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            auto conn = _connections->connection(req.peer_id());
            if (resp.success() && conn)
            {
                LOG_ERROR("对被删除人 {} 进行好友删除通知！", req.peer_id());
                NotifyMessage notify;
                notify.set_notify_type(NotifyType::FRIEND_REMOVE_NOTIFY);
                notify.mutable_friend_remove()->set_user_id(*uid);
                conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void FriendSearch(const httplib::Request &request, httplib::Response &response)
        {
            FriendSearchReq req;
            FriendSearchRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("好友搜索正文反序列化失败");
                return err_response("好友搜索正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.FriendSearch(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetPendingFriendEventList(const httplib::Request &request, httplib::Response &response)
        {
            GetPendingFriendEventListReq req;
            GetPendingFriendEventListRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取好友申请列表正文反序列化失败");
                return err_response("获取好友申请列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetPendingFriendEventList(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetChatSessionList(const httplib::Request &request, httplib::Response &response)
        {
            GetChatSessionListReq req;
            GetChatSessionListRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取聊天会话列表正文反序列化失败");
                return err_response("获取聊天会话列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetChatSessionList(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void ChatSessionCreate(const httplib::Request &request, httplib::Response &response)
        {
            ChatSessionCreateReq req;
            ChatSessionCreateRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("创建会话正文反序列化失败");
                return err_response("创建会话正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.ChatSessionCreate(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }

            // 创建成功通知所有在线的群员
            if (resp.success())
            {
                for (int i = 0; i < req.member_id_list_size(); i++)
                {
                    auto conn = _connections->connection(req.member_id_list(i));
                    if (!conn)
                    {
                        continue;
                    }
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
                    auto chat_session = notify.mutable_new_chat_session_info();
                    chat_session->mutable_chat_session_info()->CopyFrom(resp.chat_session_info());
                    conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                }
            }
            resp.clear_chat_session_info();
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetChatSessionMember(const httplib::Request &request, httplib::Response &response)
        {
            GetChatSessionMemberReq req;
            GetChatSessionMemberRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取聊天会话成员正文反序列化失败");
                return err_response("获取聊天会话成员正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_friend_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的好友服务子节点", req.request_id());
                return err_response("没有可用的好友服务子节点");
            }
            crin_lc::FriendService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetChatSessionMember(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 好友子服务调用失败", req.request_id());
                return err_response("好友子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetHistoryMsg(const httplib::Request &request, httplib::Response &response)
        {
            GetHistoryMsgReq req;
            GetHistoryMsgRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取区间历史消息列表正文反序列化失败");
                return err_response("获取区间历史消息列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_message_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的消息服务子节点", req.request_id());
                return err_response("没有可用的消息服务子节点");
            }
            crin_lc::MsgStorageService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetHistoryMsg(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 消息子服务调用失败", req.request_id());
                return err_response("消息子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetRecentMsg(const httplib::Request &request, httplib::Response &response)
        {
            GetRecentMsgReq req;
            GetRecentMsgRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取区间历史消息列表正文反序列化失败");
                return err_response("获取区间历史消息列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_message_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的消息服务子节点", req.request_id());
                return err_response("没有可用的消息服务子节点");
            }
            crin_lc::MsgStorageService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetRecentMsg(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 消息子服务调用失败", req.request_id());
                return err_response("消息子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void MsgSearch(const httplib::Request &request, httplib::Response &response)
        {
            MsgSearchReq req;
            MsgSearchRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("获取消息搜索列表正文反序列化失败");
                return err_response("获取消息搜索列表正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_message_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的消息服务子节点", req.request_id());
                return err_response("没有可用的消息服务子节点");
            }
            crin_lc::MsgStorageService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.MsgSearch(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 消息子服务调用失败", req.request_id());
                return err_response("消息子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void NewMessage(const httplib::Request &request, httplib::Response &response)
        {
            NewMessageReq req;
            NewMessageRsp resp;
            GetTransmitTargetRsp target_resp;
            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("新消息正文反序列化失败");
                return err_response("新消息正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_transmit_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的消息转发子节点", req.request_id());
                return err_response("没有可用的消息转发子节点");
            }
            crin_lc::MsgTransmitService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetTransmitTarget(&cntl, &req, &target_resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 消息转发子服务调用失败", req.request_id());
                return err_response("消息转发子服务调用失败");
            }

            // 发送成功通知所有在线的群员
            if (target_resp.success())
            {
                for (int i = 0; i < target_resp.target_id_list_size(); i++)
                {
                    std::string notify_uid = target_resp.target_id_list(i);
                    if (notify_uid == *uid)
                        continue;
                    auto conn = _connections->connection(notify_uid);
                    if (!conn)
                    {
                        continue;
                    }
                    NotifyMessage notify;
                    notify.set_notify_type(NotifyType::CHAT_MESSAGE_NOTIFY);
                    auto msg_info = notify.mutable_new_message_info();
                    msg_info->mutable_message_info()->CopyFrom(target_resp.message());
                    conn->send(notify.SerializeAsString(), websocketpp::frame::opcode::value::binary);
                }
            }
            resp.set_request_id(req.request_id());
            resp.set_success(target_resp.success());
            resp.set_errmsg(target_resp.errmsg());
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetSingleFile(const httplib::Request &request, httplib::Response &response)
        {
            GetSingleFileReq req;
            GetSingleFileRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("单文件下载正文反序列化失败");
                return err_response("单文件下载正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_file_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的文件存储服务子节点", req.request_id());
                return err_response("没有可用的文件存储服务子节点");
            }
            crin_lc::FileService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetSingleFile(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 文件存储子服务调用失败", req.request_id());
                return err_response("文件存储子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void GetMultiFile(const httplib::Request &request, httplib::Response &response)
        {
            GetMultiFileReq req;
            GetMultiFileRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("多文件下载正文反序列化失败");
                return err_response("多文件下载正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_file_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的文件存储服务子节点", req.request_id());
                return err_response("没有可用的文件存储服务子节点");
            }
            crin_lc::FileService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.GetMultiFile(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 文件存储子服务调用失败", req.request_id());
                return err_response("文件存储子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void PutSingleFile(const httplib::Request &request, httplib::Response &response)
        {
            PutSingleFileReq req;
            PutSingleFileRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("单文件上传正文反序列化失败");
                return err_response("单文件上传正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_file_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的文件存储服务子节点", req.request_id());
                return err_response("没有可用的文件存储服务子节点");
            }
            crin_lc::FileService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.PutSingleFile(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 文件存储子服务调用失败", req.request_id());
                return err_response("文件存储子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void PutMultiFile(const httplib::Request &request, httplib::Response &response)
        {
            PutMultiFileReq req;
            PutMultiFileRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("多文件上传正文反序列化失败");
                return err_response("多文件上传正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_file_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的文件存储服务子节点", req.request_id());
                return err_response("没有可用的文件存储服务子节点");
            }
            crin_lc::FileService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.PutMultiFile(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 文件存储子服务调用失败", req.request_id());
                return err_response("文件存储子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

        void SpeechRecognition(const httplib::Request &request, httplib::Response &response)
        {
            SpeechRecognitionReq req;
            SpeechRecognitionRsp resp;

            auto err_response = [&req, &resp, &response](const std::string &errmsg) -> void
            {
                resp.set_success(false);
                resp.set_errmsg(errmsg);
                response.set_content(resp.SerializeAsString(), "application/x-protbuf");
            };
            bool ret = req.ParseFromString(request.body);
            if (!ret)
            {
                LOG_ERROR("语音识别正文反序列化失败");
                return err_response("语音识别正文反序列化失败");
            }
            // 身份识别与鉴权
            std::string ssid = req.session_id();
            auto uid = _redis_session->uid(ssid);
            if (!uid)
            {
                LOG_ERROR("{} 获取登录会话关联用户信息失败！", ssid);
                return err_response("获取登录会话关联用户信息失败！");
            }
            req.set_user_id(*uid);

            auto channel = _mm_channels->choose(_speech_service);
            if (!channel)
            {
                LOG_ERROR("{} 没有可用的语音服务子节点", req.request_id());
                return err_response("没有可用的语音服务子节点");
            }
            crin_lc::SpeechService_Stub stub(channel.get());
            brpc::Controller cntl;
            stub.SpeechRecognition(&cntl, &req, &resp, nullptr);
            if (cntl.Failed())
            {
                LOG_ERROR("{} 语音识别子服务调用失败", req.request_id());
                return err_response("语音识别子服务调用失败");
            }
            response.set_content(resp.SerializeAsString(), "application/x-protbuf");
        }

    private:
        Status::ptr _redis_status;
        Session::ptr _redis_session;

        std::string _user_service;
        std::string _file_service;
        std::string _speech_service;
        std::string _message_service;
        std::string _transmit_service;
        std::string _friend_service;
        ServiceManager::ptr _mm_channels;
        Discovery::ptr _service_discoverer;

        Connection::ptr _connections;
        server_t _ws_server;
        httplib::Server _http_server;
        std::thread _http_thread;
    };

    class GatewayServerBuilder
    {
    public:
        // 构造redis客户端对象
        void make_redis_object(const std::string &host,
                               int port,
                               int db,
                               bool keep_alive)
        {
            _redis_client = RedisClientFactory::create(host, port, db, keep_alive);
        }
        // 用于构造服务发现客户端&信道管理对象
        void make_discovery_object(const std::string &reg_host,
                                   const std::string &base_service_name,
                                   const std::string &file_service_name,
                                   const std::string &speech_service_name,
                                   const std::string &message_service_name,
                                   const std::string &friend_service_name,
                                   const std::string &user_service_name,
                                   const std::string &transmite_service_name)
        {
            _file_service_name = file_service_name;
            _speech_service_name = speech_service_name;
            _message_service_name = message_service_name;
            _friend_service_name = friend_service_name;
            _user_service_name = user_service_name;
            _transmite_service_name = transmite_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->declared(file_service_name);
            _mm_channels->declared(speech_service_name);
            _mm_channels->declared(message_service_name);
            _mm_channels->declared(friend_service_name);
            _mm_channels->declared(user_service_name);
            _mm_channels->declared(transmite_service_name);
            auto put_cb = std::bind(&ServiceManager::onserviceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::onserviceOffline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            _service_discoverer = std::make_shared<Discovery>(reg_host, base_service_name, put_cb, del_cb);
        }
        void make_server_object(int websocket_port, int http_port)
        {
            _websocket_port = websocket_port;
            _http_port = http_port;
        }
        // 构造RPC服务器对象
        GatewayServer::ptr build()
        {
            if (!_redis_client)
            {
                LOG_ERROR("还未初始化Redis客户端模块！");
                abort();
            }
            if (!_service_discoverer)
            {
                LOG_ERROR("还未初始化服务发现模块！");
                abort();
            }
            if (!_mm_channels)
            {
                LOG_ERROR("还未初始化信道管理模块！");
                abort();
            }
            GatewayServer::ptr server = std::make_shared<GatewayServer>(
                _websocket_port, _http_port, _redis_client, _mm_channels,
                _service_discoverer, _user_service_name, _file_service_name,
                _speech_service_name, _message_service_name,
                _transmite_service_name, _friend_service_name);
            return server;
        }

    private:
        int _websocket_port;
        int _http_port;

        std::shared_ptr<sw::redis::Redis> _redis_client;

        std::string _file_service_name;
        std::string _speech_service_name;
        std::string _message_service_name;
        std::string _friend_service_name;
        std::string _user_service_name;
        std::string _transmite_service_name;
        ServiceManager::ptr _mm_channels;
        Discovery::ptr _service_discoverer;
    };
}