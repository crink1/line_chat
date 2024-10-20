#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "logger.hpp"
#include "mysql.hpp"
#include "chat_session_member.hxx"
#include "chat_session_member-odb.hxx"


namespace crin_lc
{
    class ChatSessionMemberTable
    {
    public:
        using ptr = std::shared_ptr<ChatSessionMemberTable>;
        ChatSessionMemberTable(const std::shared_ptr<odb::core::database> &db)
            : _db(db)
        {
        }

        bool append(ChatSessionMember &csm) // 新增会话成员
        {
            try
            {
                odb::transaction trans(_db->begin());
                _db->persist(csm);
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("新增1会话成员失败: {}-{}-{}", csm.session_id(), csm.user_id(), e.what());
                return false;
            }
            return true;
        }

        bool append(std::vector<ChatSessionMember> &csm_list) // 新增会话成员
        {
            try
            {
                odb::transaction trans(_db->begin());
                for (auto &csm : csm_list)
                {
                    _db->persist(csm);
                }
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("新增N会话成员失败：{}-{}", csm_list.size(), e.what());
                return false;
            }
            return true;
        }

        // 删除指定成员
        bool remove( ChatSessionMember &csm)
        {
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query;
                typedef odb::result<ChatSessionMember> result;
                _db->erase_query<ChatSessionMember>(query::session_id == csm.session_id() && query::user_id == csm.user_id());
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("删除指定会话成员失败: {}-{}-{}", csm.session_id(), csm.user_id(), e.what());
                return false;
            }
            return true;
        }
        // 删除所有成员
        bool remove(const std::string &ssid)
        {
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query;
                typedef odb::result<ChatSessionMember> result;
                _db->erase_query<ChatSessionMember>(query::session_id == ssid);
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("删除所有会话成员失败: {}-{}", ssid, e.what());
                return false;
            }
            return true;
        }

        std::vector<std::string> members(const std::string &ssid)
        {
            std::vector<std::string> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query;
                typedef odb::result<ChatSessionMember> result;

         
                result r(_db->query<ChatSessionMember>(query::session_id == ssid));
                for (auto i = r.begin(); i != r.end(); i++)
                {
                    res.push_back(i->user_id());
                }

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("获取会话成员失败！{}-{}", ssid, e.what());
            }
            return res;

        }

    private:
        std::shared_ptr<odb::core::database> _db;
    };
}