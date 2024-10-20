// -*- C++ -*-
//
// This file was generated by ODB, object-relational mapping (ORM)
// compiler for C++.
//

#ifndef USER_ODB_HXX
#define USER_ODB_HXX

// Begin prologue.
//
#include <odb/boost/version.hxx>
#if ODB_BOOST_VERSION != 2047700 // 2.5.0-b.27
#  error ODB and C++ compilers see different libodb-boost interface versions
#endif
#include <odb/boost/date-time/mysql/gregorian-traits.hxx>
#include <odb/boost/date-time/mysql/posix-time-traits.hxx>
//
// End prologue.

#include <odb/version.hxx>

#if ODB_VERSION != 20477UL
#error ODB runtime version mismatch
#endif

#include <odb/pre.hxx>

#include "user.hxx"

#include <memory>
#include <cstddef>

#include <odb/core.hxx>
#include <odb/traits.hxx>
#include <odb/callback.hxx>
#include <odb/wrapper-traits.hxx>
#include <odb/pointer-traits.hxx>
#include <odb/container-traits.hxx>
#include <odb/no-op-cache-traits.hxx>
#include <odb/result.hxx>
#include <odb/simple-object-result.hxx>

#include <odb/details/unused.hxx>
#include <odb/details/shared-ptr.hxx>

namespace odb
{
  // User
  //
  template <>
  struct class_traits< ::crin_lc::User >
  {
    static const class_kind kind = class_object;
  };

  template <>
  class access::object_traits< ::crin_lc::User >
  {
    public:
    typedef ::crin_lc::User object_type;
    typedef ::crin_lc::User* pointer_type;
    typedef odb::pointer_traits<pointer_type> pointer_traits;

    static const bool polymorphic = false;

    typedef long unsigned int id_type;

    static const bool auto_id = true;

    static const bool abstract = false;

    static id_type
    id (const object_type&);

    typedef
    no_op_pointer_cache_traits<pointer_type>
    pointer_cache_traits;

    typedef
    no_op_reference_cache_traits<object_type>
    reference_cache_traits;

    static void
    callback (database&, object_type&, callback_event);

    static void
    callback (database&, const object_type&, callback_event);
  };
}

#include <odb/details/buffer.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/binding.hxx>
#include <odb/mysql/mysql-types.hxx>
#include <odb/mysql/query.hxx>

namespace odb
{
  // User
  //
  template <typename A>
  struct query_columns< ::crin_lc::User, id_mysql, A >
  {
    // id
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        long unsigned int,
        mysql::id_ulonglong >::query_type,
      mysql::id_ulonglong >
    id_type_;

    static const id_type_ id;

    // user_id
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::string,
        mysql::id_string >::query_type,
      mysql::id_string >
    user_id_type_;

    static const user_id_type_ user_id;

    // nickname
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::basic_string< char >,
        mysql::id_string >::query_type,
      mysql::id_string >
    nickname_type_;

    static const nickname_type_ nickname;

    // description
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::basic_string< char >,
        mysql::id_string >::query_type,
      mysql::id_string >
    description_type_;

    static const description_type_ description;

    // password
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::basic_string< char >,
        mysql::id_string >::query_type,
      mysql::id_string >
    password_type_;

    static const password_type_ password;

    // phone
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::basic_string< char >,
        mysql::id_string >::query_type,
      mysql::id_string >
    phone_type_;

    static const phone_type_ phone;

    // avatar_id
    //
    typedef
    mysql::query_column<
      mysql::value_traits<
        ::std::basic_string< char >,
        mysql::id_string >::query_type,
      mysql::id_string >
    avatar_id_type_;

    static const avatar_id_type_ avatar_id;
  };

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::id_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  id (A::table_name, "`id`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::user_id_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  user_id (A::table_name, "`user_id`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::nickname_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  nickname (A::table_name, "`nickname`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::description_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  description (A::table_name, "`description`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::password_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  password (A::table_name, "`password`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::phone_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  phone (A::table_name, "`phone`", 0);

  template <typename A>
  const typename query_columns< ::crin_lc::User, id_mysql, A >::avatar_id_type_
  query_columns< ::crin_lc::User, id_mysql, A >::
  avatar_id (A::table_name, "`avatar_id`", 0);

  template <typename A>
  struct pointer_query_columns< ::crin_lc::User, id_mysql, A >:
    query_columns< ::crin_lc::User, id_mysql, A >
  {
  };

  template <>
  class access::object_traits_impl< ::crin_lc::User, id_mysql >:
    public access::object_traits< ::crin_lc::User >
  {
    public:
    struct id_image_type
    {
      unsigned long long id_value;
      my_bool id_null;

      std::size_t version;
    };

    struct image_type
    {
      // _id
      //
      unsigned long long _id_value;
      my_bool _id_null;

      // _user_id
      //
      details::buffer _user_id_value;
      unsigned long _user_id_size;
      my_bool _user_id_null;

      // _nickname
      //
      details::buffer _nickname_value;
      unsigned long _nickname_size;
      my_bool _nickname_null;

      // _description
      //
      details::buffer _description_value;
      unsigned long _description_size;
      my_bool _description_null;

      // _password
      //
      details::buffer _password_value;
      unsigned long _password_size;
      my_bool _password_null;

      // _phone
      //
      details::buffer _phone_value;
      unsigned long _phone_size;
      my_bool _phone_null;

      // _avatar_id
      //
      details::buffer _avatar_id_value;
      unsigned long _avatar_id_size;
      my_bool _avatar_id_null;

      std::size_t version;
    };

    struct extra_statement_cache_type;

    using object_traits<object_type>::id;

    static id_type
    id (const id_image_type&);

    static id_type
    id (const image_type&);

    static bool
    grow (image_type&,
          my_bool*);

    static void
    bind (MYSQL_BIND*,
          image_type&,
          mysql::statement_kind);

    static void
    bind (MYSQL_BIND*, id_image_type&);

    static bool
    init (image_type&,
          const object_type&,
          mysql::statement_kind);

    static void
    init (object_type&,
          const image_type&,
          database*);

    static void
    init (id_image_type&, const id_type&);

    typedef mysql::object_statements<object_type> statements_type;

    typedef mysql::query_base query_base_type;

    static const std::size_t column_count = 7UL;
    static const std::size_t id_column_count = 1UL;
    static const std::size_t inverse_column_count = 0UL;
    static const std::size_t readonly_column_count = 0UL;
    static const std::size_t managed_optimistic_column_count = 0UL;

    static const std::size_t separate_load_column_count = 0UL;
    static const std::size_t separate_update_column_count = 0UL;

    static const bool versioned = false;

    static const char persist_statement[];
    static const char find_statement[];
    static const char update_statement[];
    static const char erase_statement[];
    static const char query_statement[];
    static const char erase_query_statement[];

    static const char table_name[];

    static void
    persist (database&, object_type&);

    static pointer_type
    find (database&, const id_type&);

    static bool
    find (database&, const id_type&, object_type&);

    static bool
    reload (database&, object_type&);

    static void
    update (database&, const object_type&);

    static void
    erase (database&, const id_type&);

    static void
    erase (database&, const object_type&);

    static result<object_type>
    query (database&, const query_base_type&);

    static unsigned long long
    erase_query (database&, const query_base_type&);

    public:
    static bool
    find_ (statements_type&,
           const id_type*);

    static void
    load_ (statements_type&,
           object_type&,
           bool reload);
  };

  template <>
  class access::object_traits_impl< ::crin_lc::User, id_common >:
    public access::object_traits_impl< ::crin_lc::User, id_mysql >
  {
  };

  // User
  //
}

#include "user-odb.ixx"

#include <odb/post.hxx>

#endif // USER_ODB_HXX
