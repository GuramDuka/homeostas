/*-
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Guram Duka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
//------------------------------------------------------------------------------
#include "port.hpp"
#include "configuration.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
configuration::~configuration()
{
}
//------------------------------------------------------------------------------
configuration::configuration()
{
}
//------------------------------------------------------------------------------
void configuration::connect_db()
{
    if( db_ == nullptr ) {
        db_ = std::make_unique<sqlite3pp::database>();

        auto db_path = home_path(false) + ".homeostas";

        if( access(db_path, F_OK) != 0 && errno == ENOENT )
            mkdir(db_path);

        db_->connect(db_path + path_delimiter + "homeostas.sqlite",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
        );

        db_->execute_all(R"EOS(
            PRAGMA page_size = 4096;
            PRAGMA journal_mode = WAL;
            PRAGMA count_changes = OFF;
            PRAGMA auto_vacuum = FULL;
            PRAGMA cache_size = -2048;
            PRAGMA synchronous = NORMAL;
            /*PRAGMA temp_store = MEMORY;*/
        )EOS");

        db_->execute_all(R"EOS(
            /*CREATE TABLE IF NOT EXISTS rowids (
                id              INTEGER PRIMARY KEY ON CONFLICT REPLACE,
                counter    		INTEGER NOT NULL
            ) WITHOUT ROWID;*/

            CREATE TABLE IF NOT EXISTS config (
                id              INTEGER PRIMARY KEY ON CONFLICT ABORT,
                parent_id		INTEGER NOT NULL,   /* link on entries id */
                name            TEXT NOT NULL,
                value_type		INTEGER,	/* 1 - boolean, 2 - integer, 3 - double, 4 - string, 5 - blob, 6 - digest */
                value_b			INTEGER,	/* boolean */
                value_i			INTEGER,	/* integer */
                value_n			REAL,       /* double */
                value_s			TEXT,		/* string */
                value_l         BLOB,       /* binary */
                UNIQUE(parent_id, name) ON CONFLICT ABORT
            ) WITHOUT ROWID;

            /*CREATE TRIGGER IF NOT EXISTS config_after_insert_trigger
                AFTER INSERT
                ON config
            BEGIN
                REPLACE INTO rowids(id, counter) VALUES (1, new.id);
            END;*/

            CREATE UNIQUE INDEX IF NOT EXISTS i1 ON config (parent_id, name);
        )EOS");

        st_sel_ = std::make_unique<sqlite3pp::query>(*db_, R"EOS(
            SELECT
                id                                                      AS id,
                name                                                    AS name,
                value_type                                              AS value_type,
                COALESCE(value_b, value_i, value_n, value_s, value_l)   AS value
            FROM
                config
            WHERE
                parent_id = :parent_id
                AND name = :name
        )EOS");

        st_sel_by_pid_ = std::make_unique<sqlite3pp::query>(*db_, R"EOS(
            SELECT
                id                                                      AS id,
                name                                                    AS name,
                value_type                                              AS value_type,
                COALESCE(value_b, value_i, value_n, value_s, value_l)   AS value
            FROM
                config
            WHERE
                parent_id = :parent_id
        )EOS");

        st_ins_ = std::make_unique<sqlite3pp::command>(*db_, R"EOS(
            INSERT INTO config
                (id, parent_id, name, value_type, value_b, value_i, value_n, value_s, value_l)
                VALUES
                (:id, :parent_id, :name, :value_type, :value_b, :value_i, :value_n, :value_s, :value_l)
        )EOS");

        st_upd_ = std::make_unique<sqlite3pp::command>(*db_, R"EOS(
            UPDATE config SET
                name        = :name,
                value_type  = :value_type,
                value_b     = :value_b,
                value_i     = :value_i,
                value_n     = :value_n,
                value_s     = :value_s,
                value_l     = :value_l
            WHERE
                parent_id = :parent_id
                AND name = :name
        )EOS");

        st_row_next_id_ = std::make_unique<sqlite3pp::query>(*db_, R"EOS(
            SELECT
                id
            FROM
                config
            WHERE
                id = :id
        )EOS");

        row_id_ = entropy_fast();

        row_next_id_ = [&] {
            at_scope_exit( st_row_next_id_->reset() );

            for(;;) {
                st_row_next_id_->bind("id", row_id_);
                auto i = st_row_next_id_->begin();

                if( i ) {
                    row_id_ = std::ihash<uint64_t>(std::rhash(row_id_));
                }
                else
                    break;
            }

            return row_id_;
        };
    }
}
//------------------------------------------------------------------------------
std::variant configuration::get(uint64_t pid, const char * var_name, const std::variant * p_def_val, uint64_t * p_id)
{
    connect_db();

    st_sel_->bind("parent_id", pid);
    st_sel_->bind("name", var_name, sqlite3pp::nocopy);

    at_scope_exit( st_sel_->reset() );
    auto i = st_sel_->begin();

    if( i ) {
        if( p_id != nullptr )
            *p_id = i->get<uint64_t>("id");

        return get(i);
    }
    else if( p_id != nullptr )
        *p_id = 0;

    if( p_def_val != nullptr )
        return *p_def_val;

    return nullptr;
}
//------------------------------------------------------------------------------
uint64_t configuration::get_pid(const char * var_name, const char ** p_name, bool * p_pid_valid)
{
    uint64_t pid = 0;
    const char * p = var_name;
    bool create_if_not_exists = *p_pid_valid;

    *p_pid_valid = true;

    while( *p != '\0' ) {
        const char * a = p;
        *p_name = a;

        while( *a != '.' && *a != '\0' )
            a++;

        if( *a == '\0' )
            break;

        size_t l = (a - p) * sizeof(char);
        //char * n = (char *) alloca(l + sizeof(char));
        std::unique_ptr<char> b(new char [l + 1]);
        char * n = b.get();
        memcpy(n, p, l);
        n[l] = '\0';

        *p_name = p = a + 1;

        connect_db();

        st_sel_->bind("parent_id", pid);
        st_sel_->bind("name", (const char *) n, sqlite3pp::nocopy);

        at_scope_exit( st_sel_->reset() );
        auto i = st_sel_->begin();

        if( i ) {
            pid = i->get<uint64_t>("id");
        }
        else if( create_if_not_exists ) {
            uint64_t id = row_next_id_();

            st_ins_->bind("id"        , id);
            st_ins_->bind("parent_id" , pid);
            st_ins_->bind("name"      , (const char *) n, sqlite3pp::nocopy);
            st_ins_->bind("value_type", nullptr);
            st_ins_->bind("value_b"   , nullptr);
            st_ins_->bind("value_i"   , nullptr);
            st_ins_->bind("value_n"   , nullptr);
            st_ins_->bind("value_s"   , nullptr);
            st_ins_->bind("value_l"   , nullptr);
            st_ins_->execute();

            pid = id;
        }
        else {
            *p_pid_valid = false;
            break;
        }
    }

    return pid;
}
//------------------------------------------------------------------------------
std::variant configuration::get(const char * var_name, const std::variant * p_def_val)
{
    bool pid_valid = false;
    const char * p = nullptr;
    uint64_t pid = get_pid(var_name, &p, &pid_valid);

    if( pid_valid )
        return get(pid, p, p_def_val);

    if( p_def_val != nullptr )
        return *p_def_val;

    return nullptr;
}
//------------------------------------------------------------------------------
configuration & configuration::set(const char * var_name, const char * val)
{
    bool create_if_not_exists = true;
    const char * p = nullptr;
    uint64_t pid = get_pid(var_name, &p, &create_if_not_exists);

    return set(pid, p, [&] (auto & st) {
        st->bind("value_type", int8_t(std::variant::Text));
        st->bind("value_s"   , val, sqlite3pp::nocopy);
        st->execute();
    });
}
//------------------------------------------------------------------------------
configuration & configuration::set(const char * var_name, const std::variant & val)
{
    bool create_if_not_exists = true;
    const char * p = nullptr;
    uint64_t pid = get_pid(var_name, &p, &create_if_not_exists);

    return set(pid, p, val);
}
//------------------------------------------------------------------------------
variable configuration::get_tree(const char * var_name)
{
    variable var;

    bool pid_valid = false;
    const char * p = nullptr;
    uint64_t pid = get_pid(var_name, &p, &pid_valid);

    if( pid_valid ) {
        var.name_ = p;
        var.value_ = get(pid, p, nullptr, &var.id_);

        std::function<void (variable &)> g = [&] (variable & var) {
            connect_db();
            st_sel_by_pid_->bind("parent_id", var.id_);

            for( auto i = st_sel_by_pid_->begin(); i; ++i ) {
                var.childs()->emplace(std::make_pair(
                    i->get<const char *>("name"),
                    variable(i->get<uint64_t>("id"), i->get<const char *>("name"), get(i))
                ));
            }

            if( var.empty() )
                return;

            for( auto & i : var )
                g(i.second);
        };

        g(var);
    }

    return var;
}
//------------------------------------------------------------------------------
bool configuration::exists(const char * var_name)
{
    bool pid_valid = false;
    const char * p = nullptr;
    uint64_t pid = get_pid(var_name, &p, &pid_valid);

    if( !pid_valid )
        return false;

    connect_db();

    st_sel_->bind("parent_id", pid);
    st_sel_->bind("name"     , p, sqlite3pp::nocopy);

    at_scope_exit( st_sel_->reset() );
    return st_sel_->begin();
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
