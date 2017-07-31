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
#ifndef DISCOVERER_HPP_INCLUDED
#define DISCOVERER_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <type_traits>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
#include "sqlite3pp/sqlite3pp.h"
#include "thread_pool.hpp"
#include "socket_stream.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class discoverer {
public:
    ~discoverer() {
        shutdown();
    }

    discoverer() {}

    static auto instance() {
        static discoverer singleton;
        return &singleton;
    }

    void startup();
    void shutdown();

    discoverer & announce_host(const std::key512 & public_key,
        const std::vector<socket_addr> * p_addrs = nullptr,
        const std::key512 * p_p2p_key = nullptr);

    std::vector<socket_addr> discover_host(const std::key512 & public_key, std::key512 * p_p2p_key = nullptr);
    std::key512 discover_host_p2p_key(const std::key512 & public_key);

    auto & detach_db() {
        st_sel_peer_ = nullptr;
        st_ins_peer_ = nullptr;
        st_upd_peer_ = nullptr;
        db_ = nullptr;
        return *this;
    }
protected:
    void connect_db();
    void worker(std::shared_ptr<active_socket> socket);

    std::unique_ptr<active_socket> socket_;
    std::shared_future<void> worker_result_;
    std::mutex mtx_;
    std::condition_variable cv_;

    std::unique_ptr<sqlite3pp::database> db_;
    std::unique_ptr<sqlite3pp::query>    st_sel_peer_;
    std::unique_ptr<sqlite3pp::command>  st_ins_peer_;
    std::unique_ptr<sqlite3pp::command>  st_upd_peer_;

    bool shutdown_;
private:
    discoverer(const discoverer &) = delete;
    void operator = (const discoverer &) = delete;
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void discoverer_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // DISCOVERER_HPP_INCLUDED
//------------------------------------------------------------------------------
