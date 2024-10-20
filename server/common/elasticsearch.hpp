#pragma once
#include <elasticlient/client.h>
#include <cpr/cpr.h>
#include <iostream>
#include <memory>
#include "json/json.h"
#include "logger.hpp"

namespace crin_lc
{
    
bool Serialize(const Json::Value &val, std::string &dst)
{
    // 定义json::streamwriter 工厂类json::streamwriterbuilder
    Json::StreamWriterBuilder swb;
    swb.settings_["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    // 通过json::streamwriter中的write接口进行序列化
    std::stringstream ss;
    int ret = sw->write(val, &ss);
    if (ret != 0)
    {
        std::cout << "json反序列化失败!\n";
        return false;
    }
    dst = ss.str();
    return true;
}

bool UnSerialize(const std::string &src, Json::Value &val)
{
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    std::string err;
    bool ret = cr->parse(src.c_str(), src.c_str() + src.size(), &val, &err);
    if (ret == false)
    {
        std::cout << "json反序列化失败: " << err << std::endl;
        return false;
    }
    return true;
}

class ESIndex
{
public:
    ESIndex(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc")
        : _name(name), _type(type), _client(client)
    {
        Json::Value analyzer;
        Json::Value analysis;
        Json::Value ik;
        Json::Value tokenizer;
        tokenizer["tokenizer"] = "ik_max_word";
        ik["ik"] = tokenizer;
        analyzer["analyzer"] = ik;
        analysis["analysis"] = analyzer;
        _index["settings"] = analysis;
    }

    ESIndex &append(const std::string &key,
                    const std::string &type = "text",
                    const std::string &analyzer = "ik_max_word",
                    bool enabled = true)
    {
        Json::Value fields;
        fields["type"] = type;
        fields["analyzer"] = analyzer;
        if (enabled == true)
        {
            fields["enabled"] = enabled;
        }
        _properties[key] = fields;

        return *this;
    }

    bool create(const std::string &index_id = "default_index_id")
    {
        Json::Value mappings;
        mappings["dynamic"] = true;
        mappings["properties"] = _properties;
        _index["mappings"] = mappings;

        std::string body;
        bool ret = Serialize(_index, body);
        if (ret == false)
        {
            LOG_ERROR("索引序列化失败！");
            return false;
        }

        try
        {
            auto rsp = _client->index(_name, _type, index_id, body);
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                LOG_ERROR("创建ES索引 {} 失败，响应状态码异常: {}", _name, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {

            LOG_ERROR("创建ES索引 {} 失败: {}", _name, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;
    std::string _type;
    Json::Value _properties;
    Json::Value _index;
    std::shared_ptr<elasticlient::Client> _client;
};

class ESInsert
{
public:
    ESInsert(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc")
        : _name(name), _type(type), _client(client)
    {
    }

    template<typename T>
    ESInsert &append(const std::string &key, const T &val)
    {
        _item[key] = val;
        return *this;
    }

    bool insert(const std::string _id = "")
    {
        std::string body;
        bool ret = Serialize(_item, body);
        if (ret == false)
        {
            LOG_ERROR("索引序列化失败！");
            return false;
        }

        try
        {
            auto rsp = _client->index(_name, _type, _id, body);
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                LOG_ERROR("新增数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {

            LOG_ERROR("新增数据 {} 失败: {}", body, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;
    std::string _type;
    std::string _id;
    Json::Value _item;
    std::shared_ptr<elasticlient::Client> _client;
};

class ESRemove
{
public:
    ESRemove(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc")
        : _name(name), _type(type), _client(client)
    {
    }

    bool remove(const std::string &id)
    {
        try
        {
            auto rsp = _client->remove(_name, _type, id);
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                LOG_ERROR("删除数据 {} 失败，响应状态码异常: {}", id, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {
            LOG_ERROR("删除数据 {} 失败: {}", id, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;
    std::string _type;
    std::shared_ptr<elasticlient::Client> _client;
};

class ESSearch
{
public:
    ESSearch(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc")
        : _name(name), _type(type), _client(client)
    {
    }

    ESSearch &append_must_not_term(const std::string &key, const std::vector<std::string> &vals)
    {
        Json::Value fields;
        for (auto &val : vals)
        {
            fields[key].append(val);
        }
        Json::Value terms;
        terms["terms"] = fields;
        _must_not.append(terms);
        return *this;
    }

    ESSearch &append_should_match(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;
        _should.append(match);
        return *this;
    }

     ESSearch &append_must_term(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value trem;
        trem["term"] = field;
        _must.append(trem);
        return *this;
    }

     ESSearch &append_must_match(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;
        _must.append(match);
        return *this;
    }

    Json::Value search()
    {
        Json::Value cond;
        if (!_must_not.empty())
        {
            cond["must_not"] = _must_not;
        }

        if (!_should.empty())
        {
            cond["should"] = _should;
        }

         if (!_must.empty())
        {
            cond["must"] = _must;
        }

        Json::Value query;
        query["bool"] = cond;

        Json::Value root;
        root["query"] = query;
        

        std::string body;
        bool ret = Serialize(root, body);
        if (ret == false)
        {
            LOG_ERROR("索引序列化失败！");
            return Json::Value();
        }

        //LOG_DEBUG("检索正文 [{}]", body);
        cpr::Response resp;
        try
        {
            resp = _client->search(_name, _type, body);
            if (resp.status_code < 200 || resp.status_code >= 300)
            {
                LOG_ERROR("检索数据 {} 失败，响应状态码异常: {}", body, resp.status_code);
                return Json::Value();
            }
            

        }
        catch (std::exception &e)
        {

            LOG_ERROR("检索数据 {} 失败: {}", body, e.what());
            return Json::Value();
        }
        //对响应正文反序列化
        //LOG_DEBUG("检索响应正文: [{}]", resp.text);

        Json::Value json_res;
        ret = UnSerialize(resp.text, json_res);
        if(ret == false)
        {
            LOG_ERROR("检索数据 {} 反序列化失败", resp.text);
            return Json::Value();
        }
        return json_res["hits"]["hits"];
    }

private:
    std::string _name;
    std::string _type;
    Json::Value _must_not;
    Json::Value _should;
    Json::Value _must;


    std::shared_ptr<elasticlient::Client> _client;
};
}