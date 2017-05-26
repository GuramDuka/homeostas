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
#include <QPointer>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
//------------------------------------------------------------------------------
#include "thread_pool.hpp"
#include "socket.hpp"
#include "tracker.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class server_thread : public QThread {
        Q_OBJECT
    private:
        std::function<void()> task_ = nullptr;
    protected:
        void run() Q_DECL_OVERRIDE {
            task_();
        }
    public:
        template <class F, class... Args>
        explicit server_thread(F&& f, Args&&... args) {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto task = std::make_shared<std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            task_ = [task] { (*task)(); };
            start();
        }

        template <typename = std::is_base_of<QThread, server_thread>>
        void join() {
            wait();
        }

        struct this_thread {
            static uintptr_t get_id() {
                return reinterpret_cast<uintptr_t>(QThread::currentThreadId());
            }
        };
    //public slots:
    //signals:
    //    void resultReady(const QString &s);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class server;
//------------------------------------------------------------------------------
class tcp_server : public QTcpServer {
        Q_OBJECT
    public:
        tcp_server(server * server) : server_(server) {}
    protected:
        void incomingConnection(qintptr socket) Q_DECL_OVERRIDE;
    private:
        server * server_;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class server {
    friend class tcp_server;
    private:
        QPointer<tcp_server> server_;
        //QPointer<server_thread> thread_;
        //QPointer<QThreadPool> pool_;
        //std::unique_ptr<thread_pool<server_thread>> pool_;
        std::unique_ptr<thread_pool_t> pool_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool shutdown_;

        void listener();
        void incoming_connection(qintptr socket);
        void worker(qintptr socket_handle);
    protected:
    public:
        ~server() {
            shutdown();
        }

        bool started() const {
            return server_ != nullptr;
        }

        void startup();
        void shutdown();
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
