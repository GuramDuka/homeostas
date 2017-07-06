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
#include "discoverer.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void discoverer::connect_db()
{
    if( db_ == nullptr ) {
        db_ = std::make_unique<sqlite3pp::database>();

        auto db_path = home_path(false) + ".homeostas";

        if( access(db_path, F_OK) != 0 && errno == ENOENT )
            mkdir(db_path);

        db_->connect(db_path + path_delimiter + "discovery.sqlite",
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
            CREATE TABLE IF NOT EXISTS known_announcers (
                node            TEXT PRIMARY KEY ON CONFLICT ABORT,
                mtime           INTEGER,
                expire          INTEGER NOT NULL
            ) WITHOUT ROWID;
            CREATE INDEX IF NOT EXISTS i1 ON known_announcers (expire);

            CREATE TABLE IF NOT EXISTS known_peers (
                public_key      BLOB PRIMARY KEY ON CONFLICT ABORT,
                p2p_key         BLOB,
                addrs           BLOB,
                mtime			INTEGER,
                expire          INTEGER NOT NULL
            ) WITHOUT ROWID;
            CREATE INDEX IF NOT EXISTS i2 ON known_peers (expire);
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
    }
}
//------------------------------------------------------------------------------
void discoverer::listener()
{
}
//------------------------------------------------------------------------------
void discoverer::worker(std::shared_ptr<active_socket> socket)
{
    socket_stream ss(socket);

    try {
        std::string s;
        ss >> s;
        //cdc512 ctx(s.begin(), s.end());
        //ss << ctx.to_short_string() << std::flush;

        //std::unique_lock<std::mutex> lock(mtx);
        //std::qerr
        //    << "accepted: "
        //    << std::to_string(client_socket->local_addr())
        //    << " -> "
        //    << std::to_string(client_socket->remote_addr())
        //    << std::endl;
    }
    catch( const std::ios_base::failure & e ) {
        std::unique_lock<std::mutex> lock(*mtx_);
        std::cerr << e << std::endl;
        throw;
    }
}
//------------------------------------------------------------------------------
void discoverer::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    mtx_ = std::make_unique<std::mutex>();
    cv_ = std::make_unique<std::condition_variable>();
    thread_ = std::make_unique<std::thread>(&discoverer::listener, this);
}
//------------------------------------------------------------------------------
void discoverer::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lock(*mtx_);
    shutdown_ = true;
    lock.unlock();
    cv_->notify_one();

    thread_->join();
    thread_ = nullptr;
    cv_ = nullptr;
    mtx_ = nullptr;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
