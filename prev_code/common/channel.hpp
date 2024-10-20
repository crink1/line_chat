#pragma once

#include <brpc/channel.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include "logger.hpp"
namespace crin_lc
{
    

// 1.封装单个服务的信道管理
class ServiceChannel
{
public:
    using ptr = std::shared_ptr<ServiceChannel>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

    ServiceChannel(const std::string &name)
        : _service_name(name), _index(0)
    {
    }

    ~ServiceChannel()
    {
    }

    // 服务新上线了一个节点就调用append新增信道
    void append(const std::string &host)
    {
        std::shared_ptr<brpc::Channel> channel = std::make_shared<brpc::Channel>();
        brpc::ChannelOptions options;
        options.connect_timeout_ms = -1; // 设置连接超时等待时间， -1是一直等待
        options.timeout_ms = -1;         // 设置RPC请求超时等待时间， -1是一直等待
        options.max_retry = 3;           // 请求重试次数
        options.protocol = "baidu_std";  // 序列化协议，默认使用百度的

        int ret = channel->Init(host.c_str(), &options);
        if (ret == -1)
        {
            LOG_ERROR("初始化{}-{}信道失败", _service_name, host);
            return;
        }

        std::unique_lock<std::mutex> lock(_mutex);
        _hosts.insert(std::make_pair(host, channel));
        _channels.push_back(channel);
    }

    // 服务下线了一个节点，调用remove释放信道
    void remove(const std::string &host)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _hosts.find(host);
        if (it == _hosts.end())
        {
            LOG_WARN("{}-{}节点删除信道，没有找到信道信息", _service_name, host);
            return;
        }
        for (auto i = _channels.begin(); i != _channels.end(); i++)
        {
            if (*i == it->second)
            {
                _channels.erase(i);
                break;
            }
        }
        _hosts.erase(it);
    }

    // 通过rr轮转策略获取一个channel用于发起对应服务的rpc调用
    ChannelPtr choose()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (0 == _channels.size())
        {
            LOG_ERROR("当前没有提供 {} 服务的节点", _service_name);
            return ChannelPtr();
        }
        int32_t index = _index++ % _channels.size();
        return _channels[index];
    }

private:
    std::mutex _mutex;
    int32_t _index;                                     // 当前轮转下标计数器
    std::string _service_name;                          // 服务名称
    std::vector<ChannelPtr> _channels;                  // 当前服务对应的信道集合
    std::unordered_map<std::string, ChannelPtr> _hosts; // 主机地址与信道映射关系
};

// 2.总体的服务信道管理类
class ServiceManager
{
public:
    using ptr = std::shared_ptr<ServiceManager>;

    // 先声明我关注那些服务的上下线，不关心的就不管理
    void declared(const std::string &service_name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _follow_services.insert(service_name);
    }

    // 获取指定服务的节点信道
    ServiceChannel::ChannelPtr choose(const std::string &service_name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _services.find(service_name);
        if (it == _services.end())
        {
            LOG_ERROR("当前没有提供 {} 服务的节点", service_name);
            return ServiceChannel::ChannelPtr();
        }
        return it->second->choose();
    }

    // 服务上线时回调的接口，将服务节点管理起来
    void onserviceOnline(const std::string &service_instance, const std::string &host)
    {
        std::string service_name = getServiceName(service_instance);
        ServiceChannel::ptr service;
        {
            std::unique_lock<std::mutex> lock(_mutex);

            // 先判断一下是否关心上线的节点
            auto fit = _follow_services.find(service_name);
            if (fit == _follow_services.end())
            {
                LOG_DEBUG("{}-{}服务上线了， 但是当前并不关心!", service_name, host);
                return;
            }

            // 获取管理对象，没有就创建完再添加，有就直接添加
            auto it = _services.find(service_name);
            if (it == _services.end())
            {
                service = std::make_shared<ServiceChannel>(service_name);
                _services.insert(std::make_pair(service_name, service));
            }
            else
            {
                service = it->second;
            }
        }
        if (!service)
        {
            LOG_ERROR("新增 {} 管理服务节点失败！", service_name);
            return;
        }
        service->append(host);
        LOG_DEBUG("{}-{} 服务上线，进行添加管理", service_name, host);
    }

    // 服务下线时回调的接口，从服务信道管理中，删除指定节点信道
    void onserviceOffline(const std::string &service_instance, const std::string &host)
    {
        std::string service_name = getServiceName(service_instance);
        ServiceChannel::ptr service;
        {
            std::unique_lock<std::mutex> lock(_mutex);

            // 先判断一下是否关心下线的节点
            auto fit = _follow_services.find(service_name);
            if (fit == _follow_services.end())
            {
                LOG_DEBUG("{}-{}服务下线了， 但是当前并不关心!", service_name, host);
                return;
            }

            // 获取管理对象，没有就创建完再添加，有就直接添加
            auto it = _services.find(service_name);
            if (it == _services.end())
            {
                LOG_WARN("删除 {} 服务节点时，没有找到管理对象", service_name);
                return;
            }
            service = it->second;
        }
        service->remove(host);
        LOG_DEBUG("{}-{} 服务下线，进行删除管理", service_name, host);

    }

private:
    std::string getServiceName(const std::string &service_instance)
    {
        auto pos = service_instance.find_last_of('/');
        if(pos == std::string::npos)
        {
            return service_instance;
        }
        return service_instance.substr(0, pos);
    }

private:
    std::mutex _mutex;
    std::unordered_set<std::string> _follow_services;
    std::unordered_map<std::string, ServiceChannel::ptr> _services;
};
}