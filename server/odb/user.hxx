#pragma once
#include <string>
#include <cstddef>
#include <odb/nullable.hxx>
#include <odb/core.hxx>

namespace crin_lc
{
#pragma db object table("user")
    class User
    {
    public:
        User() {}
        // 用户名新增用户
        User(const std::string &uid, const std::string &nick, const std::string &passwd)
            : _user_id(uid), _nickname(nick), _password(passwd)
        {
        }
        // 手机新增用户
        User(const std::string &uid, const std::string &phone)
            : _user_id(uid), _phone(phone), _nickname(_user_id)
        {
        }

        std::string user_id()
        {
            return _user_id;
        }
        void user_id(const std::string &uid)
        {
            _user_id = uid;
        }

        std::string nickname()
        {
            if (!_nickname)
                return std::string();
            return *_nickname;
        }
        void nickname(const std::string &val)
        {
            _nickname = val;
        }

        std::string description()
        {
            if (!_description)
                return std::string();
            return *_description;
        }
        void description(const std::string &val)
        {
            _description = val;
        }

        std::string password()
        {
            if (!_password)
                return std::string();
            return *_password;
        }
        void password(const std::string &val)
        {
            _password = val;
        }

        std::string phone()
        {
            if (!_phone)
                return std::string();
            return *_phone;
        }
        void phone(const std::string &val)
        {
            _phone = val;
        }

        std::string avatar_id()
        {
            if (!_avatar_id)
                return std::string();
            return *_avatar_id;
        }
        void avatar_id(const std::string &val)
        {
            _avatar_id = val;
        }

    private:
        friend class odb::access;
#pragma db id auto
        unsigned long _id;
#pragma db type("varchar(64)") index unique
        std::string _user_id;
#pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _nickname; // 用户昵称不一定存在-手机号注册
        odb::nullable<std::string> _description;
#pragma db type("varchar(64)")
        odb::nullable<std::string> _password;
#pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _phone;
#pragma db type("varchar(64)")
        odb::nullable<std::string> _avatar_id;
    };
}

// odb -d mysql --generate-query --generate-schema --profile boost/date-time ../../../odb/user.hxx