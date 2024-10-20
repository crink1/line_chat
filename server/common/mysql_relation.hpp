#pragma once
#include "mysql.hpp"
#include "relation.hxx"
#include "relation-odb.hxx"

namespace crin_lc
{
    class RelationTable
    {
    public:
        using ptr = std::shared_ptr<RelationTable>;
        RelationTable(const std::shared_ptr<odb::core::database> &db)
            : _db(db)
        {
        }

        // 新增关系
        bool insert(const std::string &uid, const std::string &fid)
        {
            Relation u1(uid, fid);
            Relation u2(fid, uid);
            try
            {
                odb::transaction trans(_db->begin());
                _db->persist(u1);
                _db->persist(u2);
                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("新增好友关系失败：{} - {}", uid, fid);
                return false;
            }
            return true;
        }
        // 解除关系
        bool remove(const std::string &uid, const std::string &fid)
        {
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<Relation> query;
                typedef odb::result<Relation> result;
                _db->erase_query<Relation>(query::user_id == uid && query::peer_id == fid);
                _db->erase_query<Relation>(query::user_id == fid && query::peer_id == uid);
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("删除好友关系失败 {}:{} -- {}！", uid, fid, e.what());
                return false;
            }
            return true;
        }
        // 判断关系
        bool exists(const std::string &uid, const std::string &fid)
        {
            typedef odb::query<Relation> query;
            typedef odb::result<Relation> result;
            bool ret = false;
            try
            {
                
                odb::transaction trans(_db->begin());
                result r = _db->query<Relation>(query::user_id == uid && query::peer_id == fid);
                ret = !r.empty();
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("获取好友关系失败:{}-{} -- {}！", uid, fid, e.what());
            }
           
            return ret; 
        }
        // 获取好友ID
        std::vector<std::string> friends(const std::string &uid)
        {
            std::vector<std::string> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<Relation> query;
                typedef odb::result<Relation> result;

                result r(_db->query<Relation>(query::user_id == uid));
                for (auto i = r.begin(); i != r.end(); i++)
                {
                    res.push_back(i->peer_id());
                }

                // 提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                LOG_ERROR("通过用户ID批量查询好友失败！");
            }
            return res;
        }

    private:
        std::shared_ptr<odb::core::database> _db;
    };
}