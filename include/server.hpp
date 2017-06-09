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
#ifndef SERVER_HPP_INCLUDED
#define SERVER_HPP_INCLUDED
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
#include "socket.hpp"
#include "natpmp.hpp"
#include "tracker.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class server {
public:
    ~server() {
        shutdown();
    }

    bool started() const {
        return thread_ != nullptr;
    }

    void startup();
    void shutdown();
protected:
    void announcer();
    void listener(std::shared_ptr<passive_socket> socket);
    void worker(std::shared_ptr<active_socket> socket);

    std::unique_ptr<natpmp_client> natpmp_;
    std::vector<std::shared_ptr<passive_socket>> sockets_;
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<thread_pool_t> pool_;
    std::mutex mtx_;
    std::condition_variable cv_;

    uint16_t port_;

    bool shutdown_;
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void server_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // SERVER_HPP_INCLUDED
//------------------------------------------------------------------------------
