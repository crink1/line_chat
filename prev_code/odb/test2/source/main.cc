#include <string>
#include <memory> // std::auto_ptr
#include <cstdlib> // std::exit
#include <iostream>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "student.hxx"
#include "student-odb.hxx"
#include <gflags/gflags.h>


DEFINE_string(ip, "127.0.0.1", "mysql服务器地址");
DEFINE_int32(port, 3306, "mysql服务器端口");
DEFINE_string(db, "TestDB", "数据库默认库名称");
DEFINE_string(user, "root", "用户名");
DEFINE_string(pswd, "123456", "密码");
DEFINE_string(cset, "utf8", "mysql字符集");
DEFINE_int32(max_pool, 3, "mysql连接池的最大连接数量");

void insert_class(odb::mysql::database &db)
{
    try
    {
        odb::transaction trans(db.begin());
        Classes c1("芜湖年级芜湖班");
        Classes c2("芜湖2年级芜湖2班");

        db.persist(c1);
        db.persist(c2);
        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "插入数据出错：e.what()" << std::endl;
    }
    

}

void insert_student(odb::mysql::database &db)
{
    try
    {
        odb::transaction trans(db.begin());
        Student s1(1, "芜湖", 15, 1);
        Student s2(2, "芜湖22", 745, 2);
        Student s3(3, "芜湖32", 543, 2);



        db.persist(s1);
        db.persist(s2);
        db.persist(s3);

        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "插入数据出错：e.what()" << std::endl;
    }
}

void update_student(odb::mysql::database &db, Student &stu)
{
    try
    {
        odb::transaction trans(db.begin());

        db.update(stu);
       

        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "更新数据出错：e.what()" << std::endl;
    }
}

Student select_student(odb::mysql::database &db)
{
    Student res;
    try
    {
        odb::transaction trans(db.begin());
       typedef odb::query<Student> query;
       typedef odb::result<Student> result;
        result r(db.query<Student>(query::name == "芜湖22"));
        if(r.size()!= 1)
        {
            std::cout << "数据量不匹配" << std::endl;
            return Student();
        }
        res = *r.begin();

        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "查询数据出错：e.what()" << std::endl;
    }
    return res;
}

void remove_student(odb::mysql::database &db)
{
    try
    {
        odb::transaction trans(db.begin());
       typedef odb::query<Student> query;
        db.erase_query<Student>(query::name == "芜湖32");
       

        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "更新数据出错：e.what()" << std::endl;
    }
}

void classes_student(odb::mysql::database &db)
{
    try
    {
        odb::transaction trans(db.begin());
       typedef odb::query<struct classes_student> query;
       typedef odb::result<struct classes_student> result;
        result r(db.query<struct classes_student>(query::classes::name == "芜湖年级芜湖班"));
        if(r.size() <= 0)
        {
            std::cout << "数据量不匹配" << std::endl;
            return;
        }
        for(auto i = r.begin(); i != r.end(); i++ )
        {
            std::cout << i->_id << std::endl;
            std::cout << i->_sn << std::endl;
            std::cout << i->_name << std::endl;
            std::cout << *i->_age << std::endl;
            std::cout << i->_classes_name << std::endl;
            std::cout <<std::endl;
        }

        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "查询数据出错：e.what()" << std::endl;
    }
}

void all_student(odb::mysql::database &db)
{
    try
    {
        odb::transaction trans(db.begin());
       typedef odb::query<struct all_name> query;
       typedef odb::result<struct all_name> result;
        result r(db.query<struct all_name>());
        for(auto i = r.begin(); i != r.end(); i++ )
        {
           
            std::cout << i->_name << std::endl;
           
           
        }
        //提交事务
        trans.commit();
    }
    catch(std::exception &e)
    {
        std::cout << "查询数据出错：" << e.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    //构造连接池工程配置对象
    std::unique_ptr<odb::mysql::connection_pool_factory> cpf(
        new odb::mysql::connection_pool_factory(FLAGS_max_pool, 0));
    //构造数据库操作对象
    odb::mysql::database db(FLAGS_user, FLAGS_pswd, FLAGS_db, FLAGS_ip, FLAGS_port, 
                            "", FLAGS_cset, 0, std::move(cpf));
   

    //数据操作
    //insert_class(db);
    //insert_student(db);
    // auto stu = select_student(db);
    // std::cout << stu.sn() << std::endl;
    // std::cout << stu.name() << std::endl;
    // if(stu.age()) std::cout << *stu.age() << std::endl;
    // std::cout << stu.classes_id() << std::endl;
    // stu.age(14);
    // update_student(db, stu);
    //remove_student(db);
    //classes_student(db);
    all_student(db);
    
    return 0;
}
