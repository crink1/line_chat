#pragma once
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#include "logger.hpp"

namespace crin_lc
{

    class MQClient
    {
    public:
        using ptr = std::shared_ptr<MQClient>;
        using MessageCallback = std::function<void(const char *, size_t)>;
        MQClient(const std::string &user, const std::string passwd, const std::string host)
        {
            _loop = EV_DEFAULT;
            _handler = std::make_unique<AMQP::LibEvHandler>(_loop);
            std::string url = "amqp://" + user + ":" + passwd + "@" + host + "/";
            AMQP::Address address(url);
            _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
            _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

            _loop_thread = std::thread([this]()
                                       { ev_run(_loop, 0); });
        }

        ~MQClient()
        {

            ev_async_init(&_async_watcher, watcher_callback);
            ev_async_start(_loop, &_async_watcher);
            ev_async_send(_loop, &_async_watcher);
            _loop_thread.join();
            _loop = nullptr;
            // ev_loop_destroy(_loop);
        }

        void declareComponents(const std::string &exchange, const std::string &queue,
                               const std::string &routing_key = "routing_key",
                               AMQP::ExchangeType exchange_type = AMQP::ExchangeType::direct)
        {
            // 4.声明交换机
            _channel->declareExchange(exchange, exchange_type)
                .onError([](const char *message)
                         {
                LOG_ERROR( "声明交换机失败: {}", message);
                exit(0); })
                .onSuccess([exchange]()
                           { LOG_INFO("{} 交换机创建成功", exchange); });

            // 5.声明队列
            _channel->declareQueue(queue)
                .onError([](const char *message)
                         {
                    LOG_ERROR( "声明队列失败: {}", message);
                exit(0); })
                .onSuccess([queue]()
                           { LOG_INFO("{} 队列创建成功", queue); });

            // 6.绑定交换机和队列
            _channel->bindQueue(exchange, queue, routing_key)
                .onError([exchange, queue](const char *message)
                         {
                    LOG_ERROR( "{} - {} 绑定失败：{}",exchange, queue, message);
                exit(0); })
                .onSuccess([exchange, queue]()
                           { LOG_INFO("{} - {} 绑定成功", exchange, queue); });
        }

        bool publish(const std::string &exchange,
                     const std::string &msg,
                     const std::string &routing_key = "routing_key")
        {
            bool ret = _channel->publish(exchange, routing_key, msg);
            if (ret == false)
            {
                LOG_ERROR("{} 发布消息失败！：{}", exchange, msg);
                return false;
            }
            return true;
        }

        void consume(const std::string &queue, const MessageCallback &cb)
        {
            _channel->consume(queue, "consume-tag")
                .onReceived([this, cb](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
                            {
            cb(message.body(), message.bodySize());
            this->_channel->ack(deliveryTag); })
                .onError([queue](const char *message)
                         { LOG_ERROR("订阅 {} 队列失败: {}", queue, message); exit(0); });
        }

    private:
        static void watcher_callback(struct ev_loop *loop, ev_async *watcher, int32_t revents)
        {
            ev_break(loop, EVBREAK_ALL);
        }

    private:
        struct ev_async _async_watcher;
        struct ev_loop *_loop;
        std::unique_ptr<AMQP::LibEvHandler> _handler;
        std::unique_ptr<AMQP::TcpConnection> _connection;
        std::unique_ptr<AMQP::TcpChannel> _channel;
        std::thread _loop_thread;
    };
}