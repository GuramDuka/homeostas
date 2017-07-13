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

        st_sel_peer_ = std::make_unique<sqlite3pp::query>(*db_, R"EOS(
            SELECT
                p2p_key,
                addrs,
                mtime,
                expire
            FROM
                known_peers
            WHERE
                public_key = :public_key
        )EOS");

        st_ins_peer_ = std::make_unique<sqlite3pp::command>(*db_, R"EOS(
            INSERT INTO known_peers
                (public_key, p2p_key, addrs, mtime, expire)
                VALUES
                (:public_key, :p2p_key, :addrs, :mtime, :expire)
        )EOS");

        st_upd_peer_ = std::make_unique<sqlite3pp::command>(*db_, R"EOS(
            UPDATE known_peers SET
                p2p_key = :p2p_key,
                addrs   = :addrs,
                mtime   = :mtime,
                expire  = :expire
            WHERE
                public_key = :public_key
        )EOS");
    }
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
        std::unique_lock<std::mutex> lock(mtx_);
        std::cerr << e << std::endl;
        throw;
    }
}
//------------------------------------------------------------------------------
void discoverer::startup()
{
    if( socket_ != nullptr )
        return;

    shutdown_ = false;
    socket_   = std::make_unique<active_socket>();
}
//------------------------------------------------------------------------------
void discoverer::shutdown()
{
    if( socket_ == nullptr )
        return;

    std::unique_lock<std::mutex> lock(mtx_);
    shutdown_ = true;
    socket_->close();
    lock.unlock();
    cv_.notify_one();

    worker_result_.wait();
    socket_ = nullptr;
}
//------------------------------------------------------------------------------
discoverer & discoverer::announce_host(
    const std::key512 & public_key,
    const std::vector<socket_addr> * p_addrs,
    const std::key512 * p_p2p_key)
{
    connect_db();

    st_sel_peer_->bind("public_key", public_key, sqlite3pp::nocopy);
#if QT_CORE_LIB
//    qDebug().noquote().nospace() << "announce_host: " <<
//        QString::fromStdString(std::to_string(std::blob(std::begin(public_key), std::end(public_key))));
#endif

    auto bind = [&] (auto & st) {
        st->bind("public_key", public_key, sqlite3pp::nocopy);
        st->bind("addrs", *p_addrs, sqlite3pp::nocopy);
        st->bind("p2p_key", *p_p2p_key, sqlite3pp::nocopy);

        auto ns = clock_gettime_ns();
        st->bind("mtime", ns);
        st->bind("expire", ns + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(60)).count());

        st->execute();
    };

    at_scope_exit( st_sel_peer_->reset() );
    auto i = st_sel_peer_->begin();

    if( i ) {
        std::key512 p2p_key(std::leave_uninitialized);

        // save existing p2p key
        if( p_p2p_key == nullptr ) {
            p2p_key = i->get<std::key512>("p2p_key");
            p_p2p_key = &p2p_key;
        }

        st_sel_peer_->reset();
        bind(st_upd_peer_);
    }
    else {
        bind(st_ins_peer_);
    }

    return *this;
}
//------------------------------------------------------------------------------
std::vector<socket_addr> discoverer::discover_host(const std::key512 & public_key, std::key512 * p_p2p_key)
{
    std::vector<socket_addr> t;

    connect_db();
    st_sel_peer_->bind("public_key", public_key, sqlite3pp::nocopy);
#if QT_CORE_LIB
//    qDebug().noquote().nospace() << "discover_host: " <<
//        QString::fromStdString(std::to_string(std::blob(std::begin(public_key), std::end(public_key))));
#endif

    at_scope_exit( st_sel_peer_->reset() );
    auto i = st_sel_peer_->begin();

    if( i ) {
        t = i->get<decltype(t)>("addrs");

        if( p_p2p_key != nullptr )
            *p_p2p_key = i->get<std::key512>("p2p_key");
    }

    return t;
}
//------------------------------------------------------------------------------
std::key512 discoverer::discover_host_p2p_key(const std::key512 & public_key)
{
    connect_db();
    st_sel_peer_->bind("public_key", public_key, sqlite3pp::nocopy);

    at_scope_exit( st_sel_peer_->reset() );
    auto i = st_sel_peer_->begin();

    if( i )
        return i->get<std::key512>("p2p_key");

    return std::key512(std::zero_initialized);
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
