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
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <QPointer>
#include <QTcpSocket>
#include <QHostAddress>
//------------------------------------------------------------------------------
#include "tracker.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class client : protected QTcpSocket {
    private:
        QPointer<QTcpSocket> socket_;
        std::unique_ptr<std::thread> thread_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool shutdown_;

        void worker();
    protected:
    public:
        ~client() {
            shutdown();
        }

        bool started() const {
            return thread_ != nullptr;
        }

        void startup();
        void shutdown();
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
