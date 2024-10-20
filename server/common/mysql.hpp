#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "logger.hpp"


namespace crin_lc
{
    class ODBFactory
    {
    public:
        static std::shared_ptr<odb::core::database> create(const std::string &user,
                                                           const std::string &pswd,
                                                           const std::string &host,
                                                           const std::string &db,
                                                           const std::string &cset,
                                                           int port,
                                                           int conn_pool_count)
        {
            // 构造连接池工程配置对象
            std::unique_ptr<odb::mysql::connection_pool_factory> cpf(
                new odb::mysql::connection_pool_factory(conn_pool_count, 0));
            // 构造数据库操作对象
            auto ret = std::make_shared<odb::mysql::database>(user, pswd, db, host, port, "", cset, 0, std::move(cpf));
            return ret;
        }
    };

    

    
}