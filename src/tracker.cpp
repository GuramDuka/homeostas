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
#include "cdc512.hpp"
#include "tracker.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_tracker::worker()
{
    sqlite3pp::database db;
    sqlite3_enable_shared_cache(1);

    directory_indexer di;
    di.modified_only(true);

    auto connect_db = [&] {
        if( db.connected() )
            return;

        db.exceptions(false).connect(
            db_path_name_,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE
        );

        auto rc = sqlite3pp::command(db, R"EOS(
            PRAGMA page_size = 4096;
            PRAGMA journal_mode = WAL;
            PRAGMA count_changes = OFF;
            PRAGMA auto_vacuum = FULL;
            PRAGMA cache_size = -2048;
            PRAGMA synchronous = NORMAL;
            /*PRAGMA temp_store = MEMORY;*/
        )EOS").execute_all();

        if( rc != SQLITE_OK )
            db.disconnect();

        db.exceptions(true);
    };

    for(;;) {
        try {
            connect_db();
            di.reindex(db, dir_path_name_, &shutdown_);
        }
        catch( const std::exception & e ) {
            error_ = e.what();
            db.disconnect();
        }

        std::unique_lock<std::mutex> lk(mtx_);

        if( cv_.wait_for(lk, std::chrono::seconds(60), [&] { return shutdown_ || oneshot_; }) )
            break;
    }

    db.disconnect();
}
//------------------------------------------------------------------------------
void directory_tracker::startup()
{
    if( thread_ != nullptr )
        return;

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

        if( slen(t.c_str()) > 13 )
            t = t.substr(0, 13);

        t.push_back('-');
        t += ctx.to_string();

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

    if( access(db_path_, F_OK) != 0 && errno == ENOENT )
        mkdir(db_path_);

    shutdown_ = false;
    thread_.reset(new std::thread(&directory_tracker::worker, this));
}
//------------------------------------------------------------------------------
void directory_tracker::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    lk.unlock();
    cv_.notify_one();

    thread_->join();
    thread_ = nullptr;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
