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

    static auto instance() {
        static discoverer singleton;
        return &singleton;
    }

    bool started() const {
        return thread_ != nullptr;
    }

    void startup();
    void shutdown();

    auto & pubs(const std::vector<socket_addr> & pubs) {
        pubs_ = pubs;
        return *this;
    }

    const auto & pubs() const {
        return pubs_;
    }

    std::vector<socket_addr> discover_host_addresses(const std::key512 & public_key) {
        std::vector<socket_addr> t;

        return t;
    }

    std::key512 discover_p2p_key(const std::key512 & public_key) {

        throw std::xruntime_error("Unknown public key", __FILE__, __LINE__);

        return std::key512();
    }

    auto & detach_db() {
        st_sel_ = nullptr;
        st_sel_by_pid_ = nullptr;
        st_ins_ = nullptr;
        st_upd_ = nullptr;
        db_ = nullptr;
        return *this;
    }
protected:
    void connect_db();
    void listener();
    void worker(std::shared_ptr<active_socket> socket);

    std::vector<socket_addr> pubs_;
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<std::mutex> mtx_;
    std::unique_ptr<std::condition_variable> cv_;

    std::unique_ptr<sqlite3pp::database> db_;
    std::unique_ptr<sqlite3pp::query> st_sel_;
    std::unique_ptr<sqlite3pp::query> st_sel_by_pid_;
    std::unique_ptr<sqlite3pp::command> st_ins_;
    std::unique_ptr<sqlite3pp::command> st_upd_;

    bool shutdown_;
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
