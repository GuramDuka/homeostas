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
#include "discovery.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void discovery::listener()
{
    auto db_path_name = home_path(false) + ".homeostas" + path_delimiter + "discovery.sqlite";
    sqlite3pp::database db;
    sqlite3_enable_shared_cache(1);

    db.connect(
        db_path_name,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE
    );

    db.execute_all(R"EOS(
        PRAGMA page_size = 4096;
        PRAGMA journal_mode = WAL;
        PRAGMA count_changes = OFF;
        PRAGMA auto_vacuum = FULL;
        PRAGMA cache_size = -2048;
        PRAGMA synchronous = NORMAL;
        /*PRAGMA temp_store = MEMORY;*/
    )EOS");

    db.execute_all(R"EOS(
        CREATE TABLE IF NOT EXISTS known_announcers (
            node            TEXT PRIMARY KEY ON CONFLICT ABORT,
            mtime           INTEGER,
            expire          INTEGER
        ) WITHOUT ROWID;
        CREATE INDEX IF NOT EXISTS i1 ON known_announcers (expired);

        CREATE TABLE IF NOT EXISTS known_peers (
            id              BLOB PRIMARY KEY ON CONFLICT ABORT,
            addr            BLOB NOT NULL,
            mtime			INTEGER,
            expire          INTEGER
        ) WITHOUT ROWID;
        CREATE INDEX IF NOT EXISTS i2 ON known_peers (expired);
    )EOS");

}
//------------------------------------------------------------------------------
void discovery::worker(std::shared_ptr<active_socket> socket)
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
        std::unique_lock<std::mutex> lock(mtx_);
        std::cerr << e << std::endl;
        throw;
    }
}
//------------------------------------------------------------------------------
void discovery::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    thread_.reset(new std::thread(&discovery::listener, this));
}
//------------------------------------------------------------------------------
void discovery::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lock(mtx_);
    shutdown_ = true;
    lock.unlock();
    cv_.notify_one();

    thread_->join();
    thread_ = nullptr;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
