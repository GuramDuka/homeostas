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
#include "thread_pool.hpp"
#include "port.hpp"
#include "cdc512.hpp"
#include "tracker.hpp"
#include "client.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_tracker::connect_db()
{
    if( db_ == nullptr ) {
        db_ = std::make_unique<sqlite3pp::database>();

        if( access(db_path_, F_OK) != 0 && errno == ENOENT )
            mkdir(db_path_);

        db_->connect(
            db_path_name_,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
        );

        db_->execute_all(R"EOS(
            PRAGMA busy_timeout = 3000;
            PRAGMA page_size = 4096;
            PRAGMA journal_mode = WAL;
            PRAGMA count_changes = OFF;
            PRAGMA auto_vacuum = FULL;
            PRAGMA cache_size = -2048;
            PRAGMA synchronous = NORMAL;
            /*PRAGMA temp_store = MEMORY;*/
        )EOS");
    }
}
//------------------------------------------------------------------------------
void directory_tracker::detach_db()
{
    db_ = nullptr;
}
//------------------------------------------------------------------------------
void directory_tracker::worker()
{
    at_scope_exit( detach_db() );

    directory_indexer di;
    di.modified_only(true);

    for(;;) {
        try {
            connect_db();
            di.reindex(*db_, dir_path_name_, &shutdown_);
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
            detach_db();
        }

        std::unique_lock<std::mutex> lk(mtx_);

        if( cv_.wait_for(lk, std::chrono::seconds(60), [&] { return shutdown_ || oneshot_; }) )
            break;
    }
}
//------------------------------------------------------------------------------
void directory_tracker::make_path_name()
{
    auto remove_forbidden_characters = [] (const auto & s) {
        std::string t;

        for( const auto & c : s )
            if( (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9')
                || c == '.'
                || c == '_'
                || c == '-' )
                t.push_back(c);

        return t;
    };

    auto make_digest_name = [] (const auto & s) {
        std::string t;

        cdc512 ctx(s.cbegin(), s.cend());

        if( std::slen(t.c_str()) > 13 )
            t = t.substr(0, 13);

        t.push_back('-');
        t += std::to_string(ctx);

        return t;
    };

    auto make_name = [&] (const auto & s) {
        std::string t = remove_forbidden_characters(s);

        if( t != s )
            t = make_digest_name(t);

        return t;
    };

    db_path_ = home_path(false) + ".homeostas";
    db_name_ = make_name(dir_user_defined_name_.empty() ? dir_path_name_ : dir_user_defined_name_)
        + ".sqlite";
    db_path_name_ = db_path_ + path_delimiter + db_name_;
}
//------------------------------------------------------------------------------
void directory_tracker::startup()
{
    if( started_ )
        return;

    make_path_name();

    shutdown_ = false;
    worker_result_ = thread_pool_t::instance()->enqueue(&directory_tracker::worker, this);
    started_ = true;
}
//------------------------------------------------------------------------------
void directory_tracker::shutdown()
{
    if( !started_ )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    lk.unlock();
    cv_.notify_one();

    worker_result_.wait();
    started_ = false;
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void remote_directory_tracker::server_module(socket_stream & ss, const std::key512 & key)
{
    remote_directory_tracker module;
    module.server_worker(ss, key);
}
//------------------------------------------------------------------------------
void remote_directory_tracker::server_worker(socket_stream & ss, const std::key512 & key)
{
    // module->track_remote_directory(host_public_key_, key_, snap_key);
    // assemble server side changes packet and send to client
    // read and send and requested blocks
    connect_db();

    sqlite3pp::command st_rt_ins(*db_, R"EOS(
        INSERT INTO remote_trackers (key, mtime) VALUES (:key, :mtime)
    )EOS");

    sqlite3pp::command st_rt_upd(*db_, R"EOS(
        UPDATE remote_trackers SET mtime = :mtime WHERE key = :key
    )EOS");

    st_rt_ins.bind("key", key, sqlite3pp::nocopy);
    st_rt_ins.bind("mtime", clock_gettime_ns());
    db_->exceptions(false);
    st_rt_ins.execute();
    db_->exceptions(true);

    if( st_rt_ins.rc() == SQLITE_CONSTRAINT_UNIQUE ) {
        st_rt_upd.bind("key", key, sqlite3pp::nocopy);
        st_rt_upd.bind("mtime", clock_gettime_ns());
        st_rt_upd.execute();
    }

    sqlite3pp::query st_rt_sel(*db_, R"EOS(
        SELECT
            b.parent_id, b.name, b.mtime, b.file_size, b.block_size,
            a.entry_id, a.block_no, a.deleted
        FROM
            remote_tracking AS a
                JOIN entries AS b
                ON a.entry_id = b.id
        WHERE
            key = :key
        ORDER BY
            entry_id, block_no
    )EOS");

    sqlite3pp::command st_rt_del(*db_, R"EOS(
        DELETE FROM remote_tracking WHERE
            key = :key
            AND entry_id = :entry_id
            AND block_no = :block_no
    )EOS");

    st_rt_del.bind("key", key, sqlite3pp::nocopy);

    sqlite3pp::query st_sel(*db_, R"EOS(
        SELECT
            parent_id,
            name,
            mtime,
            file_size,
            block_size
        FROM
            entries
        WHERE
            id = :id
    )EOS");

    sqlite3pp::transaction tx(*db_, 0);
    auto delay = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(50)).count();
    auto tx_start = clock_gettime_ns();

    auto tx_deadline = [&] {
        auto now = clock_gettime_ns();
        auto deadline = tx_start + delay;

        if( now >= deadline ) {
            tx.commit();
            tx.start();
            tx_start = now;
        }
    };

    std::unordered_map<uint64_t, uint64_t> id_map;
    auto send_parents = std::function<void(uint64_t)>();

    send_parents = [&] (uint64_t entry_id) {
        if( !shutdown_ && id_map.find(entry_id) == id_map.end() ) {
            st_sel.reset();
            st_sel.bind("id", entry_id);
            auto i = st_sel.begin();

            if( i ) {
                server_side_entry_response sser;
                sser.name       = i->get<const char *>("name");
                sser.parent_id  = i->get<uint64_t>("parent_id");
                sser.entry_id   = i->get<uint64_t>("entry_id");
                sser.mtime      = i->get<uint64_t>("mtime");
                sser.file_size  = i->get<uint64_t>("file_size");
                sser.block_size = i->get<uint64_t>("block_size");
                sser.is_dir     = 1;

                send_parents(sser.parent_id);

                ss >> sser;
            }

            id_map.emplace(std::make_pair(entry_id, entry_id));
        }
    };

    uint8_t operation_code;

    while( !shutdown_ ) {
        ss >> operation_code;

        if( operation_code == OperationCodeRequestChanges ) {
            tx.start();

            server_side_entry_response sser;
            server_side_block_response ssbr;
            uint64_t current_entry_id = 0, entry_id;
            auto e = st_rt_sel.begin();

            ssbr.commit = 0;

            auto send_entry = [&] {
                // send terminated block
                if( current_entry_id != 0 ) {
                    ssbr.block_no = 0;
                    ssbr.deleted  = 0;
                    ss >> ssbr;

                    if( ssbr.commit ) {
                        // wait for ACK
                        ss >> operation_code;

                        if( operation_code != OperationCodeACK )
                            throw std::xruntime_error("Protocol error", __FILE__, __LINE__);

                        st_rt_del.bind("key", key, sqlite3pp::nocopy);
                        st_rt_del.bind("entry_id", sser.entry_id);
                        st_rt_del.bind("block_no", ssbr.block_no);
                        st_rt_del.execute();

                        tx.commit();
                        tx.start();
                    }
                }

                if( e ) {
                    sser.name       = e->get<const char *>("name");
                    sser.parent_id  = e->get<uint64_t>("parent_id");
                    sser.entry_id   = entry_id;
                    sser.mtime      = e->get<uint64_t>("mtime");
                    sser.file_size  = e->get<uint64_t>("file_size");
                    sser.block_size = e->get<uint64_t>("block_size");
                    sser.is_dir     = 0;

                    send_parents(sser.parent_id);

                    ss >> sser;

                    current_entry_id = entry_id;
                }
            };

            while( e ) {
                if( shutdown_ )
                    break;

                entry_id = e->get<uint64_t>("entry_id");

                if( current_entry_id != entry_id )
                    send_entry();

                // send changed blocks
                ssbr.block_no = e->get<uint64_t>("block_no");
                ssbr.deleted  = e->get<uint8_t>("deleted");
                ssbr.commit   = 0;
                ss >> ssbr;

                e++;
            }

            ssbr.commit = 1;
            send_entry();

            tx.commit();
        }
        else
            break;
    }
}
//------------------------------------------------------------------------------
void remote_directory_tracker::worker()
{
    at_scope_exit( detach_db() );

    auto channel = std::make_unique<client>();
    channel->startup();

    std::unordered_map<uint64_t, uint64_t> id_map;
    sqlite3pp::transaction tx(*db_, 0);

    auto tx_start = clock_gettime_ns();

    auto tx_deadline = [&] {
        auto now = clock_gettime_ns();
        auto deadline = tx_start + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(50)).count();

        if( now >= deadline ) {
            tx.commit();
            tx.start();
        }
    };

    for(;;) {
        try {
            connect_db();
            channel->enqueue([&] (socket_stream & ss) {
                uint8_t module_code = ServerModuleRDT;
                ss << module_code;

                // request server side changes
                uint8_t operation_code = OperationCodeRequestChanges;
                ss << operation_code << std::flush;

                server_side_entry_response sser;
                ss >> sser;

                // receive and write blocks changed on server side if it's not equal on client side
                server_side_block_response ssbr;
                ss >> ssbr;
            });
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
            detach_db();
        }

        std::unique_lock<std::mutex> lk(mtx_);

        if( cv_.wait_for(lk, std::chrono::seconds(60), [&] { return shutdown_ || oneshot_; }) )
            break;
    }
}
//------------------------------------------------------------------------------
void remote_directory_tracker::startup()
{
    if( started_ )
        return;

    make_path_name();

    shutdown_ = false;
    worker_result_ = thread_pool_t::instance()->enqueue(&remote_directory_tracker::worker, this);
    started_ = true;
}
//------------------------------------------------------------------------------
void remote_directory_tracker::shutdown()
{
    if( !started_ )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    lk.unlock();
    cv_.notify_one();

    worker_result_.wait();
    started_ = false;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
