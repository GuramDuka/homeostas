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
#ifndef TRACKER_HPP_INCLUDED
#define TRACKER_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <memory>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
//------------------------------------------------------------------------------
#include "indexer.hpp"
#include "server.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker_interface {
public:
    virtual ~directory_tracker_interface() {}
    directory_tracker_interface() {}

    directory_tracker_interface(const directory_tracker_interface & o) {
        *this = o;
    }

    directory_tracker_interface(directory_tracker_interface && o) {
        *this = std::move(o);
    }

    directory_tracker_interface & operator = (const directory_tracker_interface & o) {
        key_ = o.key_;
        dir_user_defined_name_ = o.dir_user_defined_name_;
        dir_path_name_ = o.dir_path_name_;
        return *this;
    }

    directory_tracker_interface & operator = (directory_tracker_interface && o) {
        if( this != &o ) {
            key_ = std::move(o.key_);
            dir_user_defined_name_ = std::move(o.dir_user_defined_name_);
            dir_path_name_ = std::move(o.dir_path_name_);
        }
        return *this;
    }

    directory_tracker_interface(
        const std::key512 & key,
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name) :
        key_(key),
        dir_user_defined_name_(dir_user_defined_name),
        dir_path_name_(dir_path_name) {}

#ifdef QT_CORE_LIB
    directory_tracker_interface(
        const std::key512 & key,
        const QString & dir_user_defined_name,
        const QString & dir_path_name) :
        directory_tracker_interface(
            key,
            dir_user_defined_name.toStdString(),
            dir_path_name.toStdString()) {}
#endif

    const auto & key() const {
        return key_;
    }

    auto & key(const std::key512 & v) {
        key_ = v;
        return *this;
    }

    const auto & dir_user_defined_name() const {
        return dir_user_defined_name_;
    }

    auto & dir_user_defined_name(const std::string & dir_user_defined_name) {
        dir_user_defined_name_ = dir_user_defined_name;
        return *this;
    }

    const auto & dir_path_name() const {
        return dir_path_name_;
    }

    auto & dir_path_name(const std::string & dir_path_name) {
        dir_path_name_ = dir_path_name;
        return *this;
    }

    virtual void startup()   {}
    virtual void shutdown()  {}

    virtual bool is_local()  const { return true; }
    virtual bool is_remote() const { return false; }
    virtual std::key512 remote() const { return std::key512(std::leave_uninitialized); }
protected:
    std::key512 key_;
    std::string dir_user_defined_name_;
    std::string dir_path_name_;
private:
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker : virtual public directory_tracker_interface {
public:
    virtual ~directory_tracker() {
        shutdown();
    }

    directory_tracker() {}

    directory_tracker(
        const std::key512 & key,
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name) :
            directory_tracker_interface(key, dir_user_defined_name, dir_path_name) {}

    const auto & oneshot() const {
        return oneshot_;
    }

    auto & oneshot(bool oneshot) {
        oneshot_ = oneshot;
        return *this;
    }

    void startup();
    void shutdown();
protected:
    void connect_db();
    void detach_db();
    void worker();
    void make_path_name();

    std::string db_name_;
    std::string db_path_;
    std::string db_path_name_;

    std::unique_ptr<sqlite3pp::database> db_;

    std::shared_future<void> worker_result_;
    std::mutex mtx_;
    std::condition_variable cv_;

    bool started_  = false;
    bool shutdown_ = false;
    bool oneshot_  = false;
private:
    directory_tracker(const directory_tracker &) = delete;
    void operator = (const directory_tracker &) = delete;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class remote_directory_tracker_interface : virtual public directory_tracker_interface {
public:
    virtual ~remote_directory_tracker_interface() {}
    remote_directory_tracker_interface() {}

    remote_directory_tracker_interface(const remote_directory_tracker_interface & o) : directory_tracker_interface() {
        *this = o;
    }

    remote_directory_tracker_interface(remote_directory_tracker_interface && o) {
        *this = std::move(o);
    }

    remote_directory_tracker_interface & operator = (const remote_directory_tracker_interface & o) {
        directory_tracker_interface::operator = (o);
        host_public_key_ = o.host_public_key_;
        return *this;
    }

    remote_directory_tracker_interface & operator = (remote_directory_tracker_interface && o) {
        if( this != &o ) {
            directory_tracker_interface::operator = (o);
            host_public_key_ = std::move(o.host_public_key_);
        }
        return *this;
    }

    remote_directory_tracker_interface(
        const std::key512 & key,
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name,
        const std::key512 & host_key) :
        directory_tracker_interface(key, dir_user_defined_name, dir_path_name), host_public_key_(host_key) {}

#ifdef QT_CORE_LIB
    remote_directory_tracker_interface(
        const std::key512 & key,
        const QString & dir_user_defined_name,
        const QString & dir_path_name,
        const std::key512 & host_key) :
        remote_directory_tracker_interface(
            key,
            dir_user_defined_name.toStdString(),
            dir_path_name.toStdString(),
            host_key) {}
#endif

    virtual bool is_local()  const { return false; }
    virtual bool is_remote() const { return true; }
    virtual std::key512 remote() const { return host_public_key_; }

    const auto & host_public_key() const {
        return host_public_key_;
    }

    auto & host_public_key(const std::key512 & key) {
        host_public_key_ = key;
        return *this;
    }
protected:
    std::key512 host_public_key_;
private:
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class remote_directory_tracker :
    virtual public remote_directory_tracker_interface,
    virtual public directory_tracker
{
public:
    virtual ~remote_directory_tracker() {
        shutdown();
    }

    remote_directory_tracker() {}

    remote_directory_tracker(
        const std::key512 & key,
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name,
        const std::key512 & host_key) :
            remote_directory_tracker_interface(key, dir_user_defined_name, dir_path_name, host_key) {}

    virtual void startup();
    virtual void shutdown();

    virtual bool is_local()  const { return false; }
    virtual bool is_remote() const { return true; }
    virtual std::key512 remote() const { return remote_directory_tracker_interface::remote(); }

    static void server_module(socket_stream & ss, const std::key512 & key);

    enum OperationCode {
        OperationCodeACK = 0,
        OperationCodeRequestChanges = 1
    };

#if _WIN32
#   pragma pack(1)
#endif
    struct PACKED server_side_entry_response {
        std::string name;
        uint64_t parent_id;
        uint64_t entry_id;
        uint64_t mtime;
        uint64_t file_size;
        uint64_t block_size;
        std::key512::value_type digest[std::key512::ssize()];
        uint8_t is_dir;
    };

    struct PACKED server_side_block_response {
        uint64_t block_no; // if zero then terminate entry
        uint8_t deleted;
        uint8_t commit;
    };
#if _WIN32
#   pragma pack()
#endif

protected:
    void worker();
    void server_worker(socket_stream & ss, const std::key512 & key);
private:
};
//------------------------------------------------------------------------------
inline auto & operator << (
    socket_stream & ss,
    const remote_directory_tracker::server_side_entry_response & e)
{
    ss << e.name
#if BYTE_ORDER == LITTLE_ENDIAN
        << e.parent_id << e.entry_id
        << e.mtime << e.file_size << e.block_size
#elif BYTE_ORDER == BIG_ENDIAN
    << std::htole(e.parent_id) << std::htole(e.entry_id);
    << std::htole(e.mtime) << std::htole(e.file_size) << std::htole(e.block_size)
    << e.digest
    << e.is_dir;
#endif
        << e.digest
        << e.is_dir;
    return ss;
}
//------------------------------------------------------------------------------
inline auto & operator >> (
    socket_stream & ss,
    remote_directory_tracker::server_side_entry_response & e)
{
    ss >> e.name >> e.parent_id >> e.entry_id
        >> e.mtime >> e.file_size >> e.block_size >> e.is_dir;
#if BYTE_ORDER == BIG_ENDIAN
    e.parent_id = std::letoh(e.parent_id);
    e.entry_id = std::letoh(e.entry_id);
    e.mtime = std::letoh(e.mtime);
    e.file_size = std::letoh(e.file_size);
    e.block_size = std::letoh(e.block_size);
#endif
    return ss;
}
//------------------------------------------------------------------------------
inline auto & operator << (
    socket_stream & ss,
    const remote_directory_tracker::server_side_block_response & e)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    ss << e.block_no << e.deleted << e.commit;
#elif BYTE_ORDER == BIG_ENDIAN
    ss << std::htole(e.block_no) << e.deleted << e.commit;
#endif
    return ss;
}
//------------------------------------------------------------------------------
inline auto & operator >> (
    socket_stream & ss,
    remote_directory_tracker::server_side_block_response & e)
{
    ss >> e.block_no >> e.deleted >> e.commit;
#if BYTE_ORDER == BIG_ENDIAN
    e.block_no = std::letoh(e.block_no);
#endif
    return ss;
}
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void tracker_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // TRACKER_HPP_INCLUDED
//------------------------------------------------------------------------------
