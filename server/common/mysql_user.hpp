#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "mysql.hpp"
#include "logger.hpp"
#include "user.hxx"
#include "user-odb.hxx"

namespace crin_lc
{
class UserTable
    {
    public:
        using ptr = std::shared_ptr<UserTable>;
        UserTable(const std::shared_ptr<odb::core::database> &db)
            : _db(db)
        {
        }

        bool insert(const std::shared_ptr<User> &user)
        {
            try
            {
                odb::transaction trans(_db->begin());
                _db->persist(*user);
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("新增用户失败：{} ", user->nickname());
                return false;
            }
            return true;
        }

        bool update(const std::shared_ptr<User> &user)
        {
            try
            {
                odb::transaction trans(_db->begin());
                _db->update(*user);
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("更新用户失败：{} ", user->nickname());
                return false;
            }
            return true;
        }

        std::shared_ptr<User> select_by_nickname(const std::string &nickname)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::nickname == nickname));

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("昵称查询用户失败：{} ", nickname);
            }
            return res;
        }

        std::shared_ptr<User> select_by_phone(const std::string &phone)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::phone == phone));

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("手机号查询用户失败：{} ", phone);
            }
            return res;
        }

        std::shared_ptr<User> select_by_id(const std::string &user_id)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::user_id == user_id));

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("用户ID查询用户失败：{} ", user_id);
            }
            return res;
        }
        std::vector<User> select_multi_users(const std::vector<std::string> &id_list)
        {
            if(id_list.empty())
            {
                return std::vector<User>();
            }
            std::vector<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;

                std::stringstream cond;
                cond << "user_id in (";
                for (const auto &id : id_list)
                {
                    cond << "'" << id << "',";
                }
                std::string sql = cond.str();
                sql.pop_back();
                sql += ")";

                result r(_db->query<User>(sql));
                for (auto i = r.begin(); i != r.end(); i++)
                {
                    res.push_back(*i);
                }

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("通过用户ID批量查询失败！");
            }
            return res;
        }

    private:
        std::shared_ptr<odb::core::database> _db;
    };
}