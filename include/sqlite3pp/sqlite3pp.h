// sqlite3pp.h
//
// The MIT License
//
// Copyright (c) 2015 Wongoo Lee (iwongu at gmail dot com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Modified by Guram Duka 2017

#ifndef SQLITE3PP_H
#define SQLITE3PP_H

#define SQLITE3PP_VERSION "1.0.6"
#define SQLITE3PP_VERSION_MAJOR 1
#define SQLITE3PP_VERSION_MINOR 0
#define SQLITE3PP_VERSION_PATCH 6

#include <cstring>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <array>
#include <vector>
#include <limits>
#include <unordered_map>
#include <memory>

#include "sqlite/sqlite3.h"
#include "std_ext.hpp"

namespace sqlite3pp {
    namespace ext {
        class function;
        class aggregate;
    }

    template <typename T>
    struct convert {
        using to_int = int;
    };

    class noncopyable {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

        noncopyable(noncopyable&&) = default;
        noncopyable& operator=(noncopyable&&) = default;

        noncopyable(noncopyable const&) = delete;
        noncopyable& operator=(noncopyable const&) = delete;
    };

    class database;

    class database_error : public std::runtime_error {
    public:
        explicit database_error(char const * msg, int errcode = 0, int extended_errcode = 0) :
            std::runtime_error(msg), errcode_(errcode), extended_errcode_(extended_errcode) {}
        
        const int & errcode() const {
            return errcode_;
        }
        const int & extended_errcode() const {
            return extended_errcode_;
        }
    private:
        int errcode_;
        int extended_errcode_;
    };

    namespace {
        int busy_handler_impl(void* p, int cnt);
        int commit_hook_impl(void* p);
        void rollback_hook_impl(void* p);
        void update_hook_impl(void* p, int opcode, char const* dbname, char const* tablename, long long int rowid);
        int authorizer_impl(void* p, int evcode, char const* p1, char const* p2, char const* dbname, char const* tvname);
    } // namespace
    
    class database : noncopyable {
        friend class statement;
        friend class database_error;
        friend class ext::function;
        friend class ext::aggregate;
    public:
        using busy_handler = std::function<int (int) >;
        using commit_handler = std::function<int ()>;
        using rollback_handler = std::function<void ()>;
        using update_handler = std::function<void (int, char const*, char const*, long long int) >;
        using authorize_handler = std::function<int (int, char const*, char const*, char const*, char const*) >;
        using backup_handler = std::function<void (int, int, int) >;

        explicit database() : db_(nullptr), exceptions_(true) {}

        explicit database(char const* dbname, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, const char* vfs = nullptr)
            : db_(nullptr), exceptions_(true) {
            if (dbname != nullptr)
                connect(dbname, flags, vfs);
        }

        explicit database(const std::string & dbname, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, const char* vfs = nullptr)
            : database(dbname.c_str(), flags, vfs) {
        }

        database(database&& db) :
            db_(std::move(db.db_)),
            bh_(std::move(db.bh_)),
            ch_(std::move(db.ch_)),
            rh_(std::move(db.rh_)),
            uh_(std::move(db.uh_)),
            ah_(std::move(db.ah_))
        {
            db.db_ = nullptr;
        }

        database& operator=(database&& db) {
            db_ = std::move(db.db_);
            db.db_ = nullptr;

            bh_ = std::move(db.bh_);
            ch_ = std::move(db.ch_);
            rh_ = std::move(db.rh_);
            uh_ = std::move(db.uh_);
            ah_ = std::move(db.ah_);

            return *this;
        }

        ~database() {
            disconnect();
        }

        int connect(char const* dbname, int flags, char const* vfs = nullptr) {
            disconnect();

            sqlite3_enable_shared_cache(1);

            int rc = sqlite3_open_v2(dbname, &db_, flags | SQLITE_OPEN_SHAREDCACHE, vfs);
            if (rc != SQLITE_OK)
                throw_database_error();
            else
                enable_extended_result_codes(true);
            return rc;
        }

        int connect(const std::string & dbname, int flags, char const* vfs = nullptr) {
            return connect(dbname.c_str(), flags, vfs);
        }

        int disconnect() {
            auto rc = SQLITE_OK;
            if (db_ != nullptr) {
                rc = sqlite3_close_v2(db_);
                if (rc != SQLITE_OK)
                    throw_database_error();
                if (rc == SQLITE_OK)
                    db_ = nullptr;
            }

            return rc;
        }

        int attach(char const* dbname, char const* name) {
            return executef("ATTACH '%q' AS '%q'", dbname, name);
        }

        int detach(char const* name) {
            return executef("DETACH '%q'", name);
        }

        int backup(char const* dbname, database& destdb, char const* destdbname, backup_handler h, int step_page = 5) {
            sqlite3_backup* bkup = sqlite3_backup_init(destdb.db_, destdbname, db_, dbname);
            if (!bkup) {
                return error_code();
            }
            auto rc = SQLITE_OK;
            do {
                rc = sqlite3_backup_step(bkup, step_page);
                if (h) {
                    h(sqlite3_backup_remaining(bkup), sqlite3_backup_pagecount(bkup), rc);
                }
            } while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);
            sqlite3_backup_finish(bkup);
            return rc;
        }

        int backup(database& destdb, backup_handler h) {
            return backup("main", destdb, "main", h);
        }
        
        void set_busy_handler(busy_handler h) {
            bh_ = h;
            sqlite3_busy_handler(db_, bh_ ? busy_handler_impl : 0, &bh_);
        }

        void set_commit_handler(commit_handler h) {
            ch_ = h;
            sqlite3_commit_hook(db_, ch_ ? commit_hook_impl : 0, &ch_);
        }

        void set_rollback_handler(rollback_handler h) {
            rh_ = h;
            sqlite3_rollback_hook(db_, rh_ ? rollback_hook_impl : 0, &rh_);
        }

        void set_update_handler(update_handler h) {
            uh_ = h;
            sqlite3_update_hook(db_, uh_ ? update_hook_impl : 0, &uh_);
        }

        void set_authorize_handler(authorize_handler h) {
            ah_ = h;
            sqlite3_set_authorizer(db_, ah_ ? authorizer_impl : 0, &ah_);
        }

        long long int last_insert_rowid() const {
            return sqlite3_last_insert_rowid(db_);
        }

        int enable_foreign_keys(bool enable = true) {
            return sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_FKEY, enable ? 1 : 0, nullptr);
        }

        int enable_triggers(bool enable = true) {
            return sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_TRIGGER, enable ? 1 : 0, nullptr);
        }

        int enable_extended_result_codes(bool enable = true) {
            return sqlite3_extended_result_codes(db_, enable ? 1 : 0);
        }

        int changes() const {
            return sqlite3_changes(db_);
        }

        int error_code() const {
            return sqlite3_errcode(db_);
        }

        int extended_error_code() const {
            return sqlite3_extended_errcode(db_);
        }

        char const* error_msg() const {
            return sqlite3_errmsg(db_);
        }

        int execute(char const* sql) {
            int rc = sqlite3_exec(db_, sql, 0, 0, 0);
            if (rc != SQLITE_OK)
                throw_database_error();
            return rc;
        }

        int execute(const std::string & sql) {
            return execute(sql.c_str());
        }

        int execute_all(char const* sql);

        int execute_all(const std::string & sql) {
            return execute_all(sql.c_str());
        }

        int executef(char const* sql, ...) {
            va_list ap;
            va_start(ap, sql);
            std::shared_ptr<char> msql(sqlite3_vmprintf(sql, ap), sqlite3_free);
            va_end(ap);

            return execute(msql.get());
        }

        int set_busy_timeout(int ms) {
            auto rc = sqlite3_busy_timeout(db_, ms);
            if (rc != SQLITE_OK)
                throw_database_error();
            return rc;
        }

        void throw_database_error() const {
            if (exceptions_)
                throw database_error(
                    sqlite3_errmsg(db_),
                    sqlite3_errcode(db_),
                    sqlite3_extended_errcode(db_));
        }

        database & exceptions(bool exceptions) {
            exceptions_ = exceptions;
            return *this;
        }

        const bool & exceptions() const {
            return exceptions_;
        }

        sqlite3 * const & handle() const {
            return db_;
        }

        bool connected() const {
            return db_ != nullptr;
        }

    private:
        sqlite3* db_;

        busy_handler bh_;
        commit_handler ch_;
        rollback_handler rh_;
        update_handler uh_;
        authorize_handler ah_;
        bool exceptions_;
    };

    namespace {
        int busy_handler_impl(void* p, int cnt) {
            auto h = static_cast<database::busy_handler*> (p);
            return (*h)(cnt);
        }

        int commit_hook_impl(void* p) {
            auto h = static_cast<database::commit_handler*> (p);
            return (*h)();
        }

        void rollback_hook_impl(void* p) {
            auto h = static_cast<database::rollback_handler*> (p);
            (*h)();
        }

        void update_hook_impl(void* p, int opcode, char const* dbname, char const* tablename, long long int rowid) {
            auto h = static_cast<database::update_handler*> (p);
            (*h)(opcode, dbname, tablename, rowid);
        }

        int authorizer_impl(void* p, int evcode, char const* p1, char const* p2, char const* dbname, char const* tvname) {
            auto h = static_cast<database::authorize_handler*> (p);
            return (*h)(evcode, p1, p2, dbname, tvname);
        }

    } // namespace

    enum copy_semantic {
        copy, nocopy
    };

    class statement : noncopyable {
    public:
        int prepare(char const* stmt) {
            finish();
            prepare_impl(stmt);
            param_cache_.clear();
            build_param_cache();
            return rc_;
        }

        int prepare(const std::string & stmt) {
            return prepare(stmt.c_str());
        }

        int finish() {
            rc_ = SQLITE_OK;
            if (stmt_ != nullptr) {
                finish_impl(stmt_);
                if (rc_ != SQLITE_OK)
                    db_.throw_database_error();
                stmt_ = nullptr;
            }
            tail_ = nullptr;

            return rc_;
        }

        operator bool () const {
            return rc_ != SQLITE_OK;
        }
        
        int step() {
            rc_ = sqlite3_step(stmt_);
            if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE)
                db_.throw_database_error();
            return rc_;
        }

        int reset() {
            rc_ = sqlite3_reset(stmt_);
            //if (rc_ != SQLITE_OK)
            //    db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, bool value) {
            rc_ = sqlite3_bind_int(stmt_, idx, value ? 1 : 0);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int(stmt_, idx, value ? 1 : 0);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }
        
        int bind(int idx, int value) {
            rc_ = sqlite3_bind_int(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, unsigned value) {
            rc_ = sqlite3_bind_int(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }
        
        int bind(int idx, double value) {
            rc_ = sqlite3_bind_double(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_double(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, long long int value) {
            rc_ = sqlite3_bind_int64(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int64(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, long long unsigned value) {
            rc_ = sqlite3_bind_int64(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int64(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

#if __clang__ || __GNUG__ >= 7
        int bind(int idx, int64_t value) {
            rc_ = sqlite3_bind_int64(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int64(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, uint64_t value) {
            rc_ = sqlite3_bind_int64(stmt_, idx, value);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_int64(stmt_, idx, value);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }
#endif

        int bind(int idx, char const* value, copy_semantic fcopy) {
            auto l = std::slen(value);
            rc_ = sqlite3_bind_text64(stmt_, idx, value, l, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC, SQLITE_UTF8);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_text64(stmt_, idx, value, l, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC, SQLITE_UTF8);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, const std::string & value, copy_semantic fcopy) {
            auto l = value.size() * sizeof(value[0]);
            rc_ = sqlite3_bind_text64(stmt_, idx, value.c_str(), l, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC, SQLITE_UTF8);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_text64(stmt_, idx, value.c_str(), l, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC, SQLITE_UTF8);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, void const* value, size_t n, copy_semantic fcopy) {
            rc_ = sqlite3_bind_blob64(stmt_, idx, value, n, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_blob64(stmt_, idx, value, n, fcopy == copy ? SQLITE_TRANSIENT : SQLITE_STATIC);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int bind(int idx, std::nullptr_t) {
            rc_ = sqlite3_bind_null(stmt_, idx);
            if (rc_ == SQLITE_MISUSE) {
                sqlite3_reset(stmt_);
                rc_ = sqlite3_bind_null(stmt_, idx);
            }
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        template <std::size_t _Sz>
        int param_name2idx(const char (&name) [_Sz]) const {
            auto i = param_cache_.find(name);

            if( i == param_cache_.cend() )
                throw database_error("Invalid parameter name");

            return i->second;
        }

        int param_name2idx(const std::string & name) const {
            return param_name2idx(name.c_str());
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], bool value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }
        
        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], int value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], unsigned value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }
        
        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], double value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], long long int value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], long long unsigned value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }

#if __clang__ || __GNUG__ >= 7
        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], int64_t value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], uint64_t value) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value);
        }
#endif

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], char const* value, copy_semantic fcopy) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value, fcopy);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], void const* value, int n, copy_semantic fcopy) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value, n, fcopy);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], std::string const& value, copy_semantic fcopy) {
            //auto idx = sqlite3_bind_parameter_index(stmt_, name);
            return bind(param_name2idx(name), value, fcopy);
        }

        template <std::size_t _Sz>
        int bind(const char (&name) [_Sz], std::nullptr_t) {
            return bind(param_name2idx(name), nullptr);
        }

        int bind(const std::string & name, std::nullptr_t) {
            return bind(param_name2idx(name.c_str()), nullptr);
        }

        template <typename T, std::size_t _Sz, typename std::enable_if<
            std::container_traits<T>::is_array_or_vector>::type * = nullptr
        >
        int bind(const char (&name) [_Sz], const T & value, copy_semantic fcopy) {
            if( &value == nullptr )
                return bind(name, nullptr);
            return bind(param_name2idx(name), value.data(), value.size() * sizeof(value[0]), fcopy);
        }

        template <typename T, std::size_t _Sz, typename std::enable_if<
            std::container_traits<T>::is_array_or_vector>::type * = nullptr
        >
        int bind(const std::string & name, const T & value, copy_semantic fcopy) {
            if( &value == nullptr )
                return bind(name, nullptr);
            return bind(param_name2idx(name.c_str()), value.data(), value.size() * sizeof(value[0]), fcopy);
        }
    protected:
        ~statement() {
            // finish() can return error. If you want to check the error, call
            // finish() explicitly before this object is destructed.
            auto safe = db_.exceptions();
            db_.exceptions(false);
            finish();
            db_.exceptions(safe);
        }

        explicit statement(database& db, char const* stmt = nullptr)
            : db_(db), stmt_(nullptr), tail_(nullptr), rc_(SQLITE_OK)
        {
            if (stmt != nullptr)
                prepare(stmt);
        }

        int prepare_impl(const char * stmt) {
            auto l = std::slen(stmt);
            int n = int(std::min(l, decltype(l) (std::numeric_limits<int>::max())));
            rc_ = sqlite3_prepare_v2(db_.db_, stmt, n, &stmt_, &tail_);
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        int finish_impl(sqlite3_stmt* stmt) {
            rc_ = sqlite3_finalize(stmt);
            if (rc_ != SQLITE_OK)
                db_.throw_database_error();
            return rc_;
        }

        const auto & rc() const {
            return rc_;
        }

        const auto & db() const {
            return db_;
        }

    protected:
        database & db_;
        sqlite3_stmt * stmt_;
        char const * tail_;
        int rc_;

        struct str_hash {
            size_t operator() (const char * val) const {
                size_t h = 0;

                while( *val != '\0' ) {
                    h += *val++;
                    h += (h << 9);
                    h ^= (h >> 5);
                }

                h += (h << 3);
                h ^= (h >> 10);
                h += (h << 14);

                return h;
           }
        };

        struct str_equal {
           bool operator ()(const char * val1, const char * val2) const {
              return strcmp(val1, val2) == 0;
           }
        };

        typedef std::unordered_map<const char *, int, str_hash, str_equal> cache_type;
        mutable cache_type param_cache_;

        void build_param_cache() const {
            int k = sqlite3_bind_parameter_count(stmt_);

            for( int i = 1; i <= k; i++ ) {
                auto p = sqlite3_bind_parameter_name(stmt_, i);
                if( *p == ':' )
                    p++;
                param_cache_.emplace(std::make_pair(p, i));
            }
        }
    };

    class command : public statement {
    public:
        explicit command(database& db, char const* stmt = nullptr) : statement(db, stmt) {}
        explicit command(database& db, const std::string & stmt) : command(db, stmt.c_str()) {}

        int execute(int ignore_rc = -1) {
            rc_ = reset();
            if (rc_ == SQLITE_OK)
                rc_ = step();
            if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE && rc_ != ignore_rc)
                db_.throw_database_error();
            return rc_;
        }

        int execute_all(int ignore_rc = -1) {
            execute(ignore_rc);

            const char * sql = tail_;

            while( std::slen(sql) > 0 ) { // sqlite3_complete() is broken.
                sqlite3_stmt * old_stmt = stmt_;

                prepare_impl(sql);

                // If the input text contains no SQL (if the input is an empty string or a comment) then stmt_ is set to nullptr
                if (stmt_ == nullptr) {
                    stmt_ = old_stmt;
                    break;
                }

                if ((rc_ = sqlite3_transfer_bindings(old_stmt, stmt_)) != SQLITE_OK) {
                    database_error e(
                        db_.error_msg(),
                        db_.error_code(),
                        db_.extended_error_code());
                    finish_impl(old_stmt);
                    if( db_.exceptions( ))
                        throw e;
                }

                finish_impl(old_stmt);
                execute(ignore_rc);

                if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE && rc_ != ignore_rc)
                    db_.throw_database_error();

                sql = tail_;
            }

            return rc_;
        }
    };

    inline int database::execute_all(const char *sql) {
        return command(*this, sql).execute_all();
    }

    enum query_column_type {
        SQL_INT = SQLITE_INTEGER,
        SQL_FLT = SQLITE_FLOAT,
        SQL_TXT = SQLITE_TEXT,
        SQL_BLB = SQLITE_BLOB,
        SQL_NIL = SQLITE_NULL
    };
    
    class query : public statement {
    public:
        class row {
        public:
            explicit row(query * cmd) : cmd_(cmd) {}

            int data_count() const {
                return sqlite3_data_count(cmd_->stmt_);
            }
            
            query_column_type column_type(int idx) const {
                return static_cast<query_column_type>(sqlite3_column_type(cmd_->stmt_, idx));
            }

            template <typename T, std::size_t _Sz>
            query_column_type column_type(const char (&name) [_Sz]) const {
                return static_cast<query_column_type>(sqlite3_column_type(cmd_->stmt_, cmd_->column_name2idx(name)));
            }

            query_column_type column_type(const std::string & name) const {
                return static_cast<query_column_type>(sqlite3_column_type(cmd_->stmt_, cmd_->column_name2idx(name)));
            }
            
            bool column_isnull(int idx) const {
                return column_type(idx) == SQL_NIL;
            }
            
            template <typename T, std::size_t _Sz>
            bool column_isnull(const char (&name) [_Sz]) const {
                return column_type(cmd_->column_name2idx(name)) == SQL_NIL;
            }

            bool column_isnull(const std::string & name) const {
                return column_type(cmd_->column_name2idx(name.c_str())) == SQL_NIL;
            }
            
            int column_bytes(int idx) const {
                return sqlite3_column_bytes(cmd_->stmt_, idx);
            }

            template <typename T,
                typename std::enable_if<std::is_integral<T>::value>::type * = nullptr
            >
            T get(int idx) const {
                return T(sqlite3_column_int64(cmd_->stmt_, idx));
            }

            template <typename T,
                typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr
            >
            T get(int idx) const {
                return T(sqlite3_column_double(cmd_->stmt_, idx));
            }

            template <typename T,
                typename std::enable_if<std::is_same<T, const char *>::value>::type * = nullptr
            >
            T get(int idx) const {
                return reinterpret_cast<const char *>(sqlite3_column_text(cmd_->stmt_, idx));
            }

            template <typename T,
                typename std::enable_if<std::is_base_of<std::string, T>::value>::type * = nullptr
            >
            T get(int idx) const {
                return reinterpret_cast<const char *>(sqlite3_column_text(cmd_->stmt_, idx));
            }

            template <typename T,
                typename std::enable_if<std::is_same<T, const void *>::value>::type * = nullptr
            >
            T get(int idx) const {
                return sqlite3_column_blob(cmd_->stmt_, idx);
            }

            template <typename T, typename std::enable_if<std::container_traits<T>::is_vector>::type * = nullptr>
            T get(int idx) const {
                typedef typename T::value_type value_type;
                auto range = std::make_range(static_cast<const value_type *>(sqlite3_column_blob(cmd_->stmt_, idx)),
                    sqlite3_column_bytes(cmd_->stmt_, idx) / sizeof(value_type));
                return T(range.begin(), range.end());
            }

            template <typename T, typename std::enable_if<std::container_traits<T>::is_array>::type * = nullptr>
            T get(int idx) const {
                typedef typename T::value_type value_type;
                auto range = std::make_range(static_cast<const value_type *>(sqlite3_column_blob(cmd_->stmt_, idx)),
                    sqlite3_column_bytes(cmd_->stmt_, idx) / sizeof(value_type));
                T result({}); // by default array zero initialized
                auto i = std::copy(range, std::begin(result), std::end(result));
                std::fill(i, std::end(result), value_type(0));
                return result;
            }

            template <typename T, std::size_t _Sz>
            T get(const char (&name) [_Sz]) const {
                return get<T>(cmd_->column_name2idx(name));
            }

            template <typename T>
            T get(const std::string & name) const {
                return get<T>(cmd_->column_name2idx(name.c_str()));
            }
        protected:
            query * cmd_;
        private:
        };

        class query_iterator : public std::iterator<std::input_iterator_tag, row>, private row {
        public:
            query_iterator() : row(nullptr), rc_(SQLITE_DONE) {}

            explicit query_iterator(query * cmd) : row(cmd) {
                rc_ = cmd_->step();
            }

            operator bool () const {
                return rc_ != SQLITE_DONE;
            }

            bool operator == (query_iterator const& other) const {
                return rc_ == other.rc_;
            }

            bool operator != (query_iterator const& other) const {
                return rc_ != other.rc_;
            }

            // prefix form
            query_iterator & operator ++ () {
                rc_ = cmd_->step();
                if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE)
                    cmd_->db_.throw_database_error();
                return *this;
            }

            // postfix form
            query_iterator & operator ++ (int) {
                rc_ = cmd_->step();
                if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE)
                    cmd_->db_.throw_database_error();
                return *this;
            }

            value_type & operator * () const {
                return *const_cast<value_type *>(static_cast<const value_type *>(this));
            }

            value_type * operator -> () const {
                return const_cast<value_type *>(static_cast<const value_type *>(this));
            }
        private:
            int rc_;
        };

        explicit query(database& db, char const* stmt = nullptr) : statement(db, nullptr) {
            prepare(stmt);
        }

        explicit query(database& db, const std::string & stmt) : query(db, stmt.c_str()) {}

        // overload
        int prepare(const char * stmt) {
            statement::prepare(stmt);
            column_cache_.clear();
            build_column_cache();
            return rc_;
        }

        // overload
        int prepare(const std::string & stmt) {
            return prepare(stmt.c_str());
        }

        int column_count() const {
            return sqlite3_column_count(stmt_);
        }

        char const* column_name(int idx) const {
            return sqlite3_column_name(stmt_, idx);
        }

        char const* column_decltype(int idx) const {
            return sqlite3_column_decltype(stmt_, idx);
        }

        template <std::size_t _Sz>
        int column_name2idx(const char (&name) [_Sz]) const {
            auto i = column_cache_.find(name);

            if( i == column_cache_.cend() )
                throw database_error("Invalid column name");

            return i->second;
        }

        int column_name2idx(const char * name) const {
            auto i = column_cache_.find(name);
            
            if( i == column_cache_.cend() )
                throw database_error("Invalid column name");

            return i->second;
        }

        int column_name2idx(const std::string & name) const {
            return column_name2idx(name.c_str());
        }

        using iterator = query_iterator;

        iterator begin() {
            return query_iterator(this);
        }

        iterator end() {
            return query_iterator();
        }

    private:
        mutable cache_type column_cache_;
        
        void build_column_cache() const {
            int k = sqlite3_column_count(stmt_);
                
            for( int i = 0; i < k; i++ )
                column_cache_.emplace(std::make_pair(sqlite3_column_name(stmt_, i), i));
        }
    };

    class transaction : noncopyable {
    public:
        explicit transaction(database * db, bool immediate = false, bool rollback = false)
            : db_(db), rollback_(rollback)
        {
            start(immediate);
        }

        ~transaction() {
            if (db_ != nullptr) {
                // execute() can return error. If you want to check the error,
                // call commit() or rollback() explicitly before this object is
                // destructed.
                db_->execute(rollback_ ? "ROLLBACK" : "COMMIT");
            }
        }

        auto & release() {
            db_ = nullptr;
            return *this;
        }

        int start(bool immediate = false) {
            int rc = db_->execute(immediate ? "BEGIN IMMEDIATE" : "BEGIN DEFERRED");
            if (rc != SQLITE_OK)
                db_->throw_database_error();
            return rc;
        }

        int commit() {
            int rc = db_->execute("COMMIT");
            if (rc != SQLITE_OK)
                db_->throw_database_error();
            return rc;
        }

        int rollback() {
            int rc = db_->execute("ROLLBACK");
            if (rc != SQLITE_OK)
                db_->throw_database_error();
            return rc;
        }

    private:
        database * db_;
        bool rollback_;
    };
} // namespace sqlite3pp

#endif
