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
#ifndef CLIENT_HPP_INCLUDED
#define CLIENT_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <future>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
//------------------------------------------------------------------------------
#include "socket_stream.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class client {
public:
    ~client() {
        shutdown();
    }

    client() {}

    static auto instance() {
        static client singleton;
        return &singleton;
    }

    void startup();
    void shutdown();

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();

        task_ = [task] { (*task)(); };

        return res;
    }

    auto & server_public_key(const std::key512 & key) {
        server_public_key_ = key;
        return *this;
    }

    const auto & server_public_key() const {
        return server_public_key_;
    }

    auto & client_public_key(const std::key512 & key) {
        client_public_key_ = key;
        return *this;
    }

    const auto & client_public_key() const {
        return client_public_key_;
    }
protected:
    void worker();

    std::key512 server_public_key_;
    std::key512 client_public_key_;
    std::function<void()> task_;
    std::unique_ptr<active_socket> socket_;
    std::shared_future<void> worker_result_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool shutdown_;
private:
    client(const client &) = delete;
    void operator = (const client &) = delete;
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void client_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // CLIENT_HPP_INCLUDED
//------------------------------------------------------------------------------
