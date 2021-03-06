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
#ifndef ANNOUNCER_HPP_INCLUDED
#define ANNOUNCER_HPP_INCLUDED
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
#include "thread_pool.hpp"
#include "socket_stream.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class announcer {
public:
    ~announcer() {
        shutdown();
    }

    announcer() {}

    static auto instance() {
        static announcer singleton;
        return &singleton;
    }

    void startup();
    void shutdown();

    auto & host_key(const std::key512 & key) {
        host_key_ = key;
        return *this;
    }

    auto & pubs(const std::vector<socket_addr> & pubs) {
        pubs_ = pubs;
        return *this;
    }
protected:
    void worker();

    std::blob host_key_;
    std::vector<socket_addr> pubs_;
    std::unique_ptr<active_socket> socket_;
    std::shared_future<void> worker_result_;
    std::mutex mtx_;
    std::condition_variable cv_;

    bool shutdown_ = false;
private:
    announcer(const announcer &) = delete;
    void operator = (const announcer &) = delete;
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void announcer_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // ANNOUNCER_HPP_INCLUDED
//------------------------------------------------------------------------------
