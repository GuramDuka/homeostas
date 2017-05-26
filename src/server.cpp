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
#include "scope_exit.hpp"
#include "socket.hpp"
#include "server.hpp"
//#include "dlib/assert.h"
//#include "dlib/sockets.h"
//#include "dlib/iosockstream.h"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void tcp_server::incomingConnection(qintptr socket)
{
    server_->incoming_connection(socket);
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void server::incoming_connection(qintptr socket)
{
    pool_->enqueue([=] {
        worker(socket);
    });
}
//------------------------------------------------------------------------------
void server::listener()
{
    server_->setParent(nullptr);
    server_->moveToThread(QThread::currentThread());

    while( !shutdown_ ) {
        if( !server_->isListening() )
            if( !server_->listen(QHostAddress::Any, 65535) )
                std::qerr << server_->errorString();

        //if( server_->waitForNewConnection(-1) )
        //    pool_->enqueue([&] {
        //        worker(server_->nextPendingConnection());
        //    });
            //pool_->start(new ThreadCallback([&] {
            //    worker(server_->nextPendingConnection());
            //}));
    }
}
//------------------------------------------------------------------------------
void server::worker(qintptr socket_handle)
{
    QTcpSocket * socket = new QTcpSocket;

    if( !socket->setSocketDescriptor(socket_handle, QAbstractSocket::ConnectedState, QIODevice::ReadWrite) ) {
        std::qerr << socket->errorString();
        return;
    }

    //socket->setParent(nullptr);
    //socket->moveToThread(QThread::currentThread());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    out << QString::fromUtf8("0123456789");
    socket->write(block);
    socket->waitForBytesWritten(100);

    auto at_exit = [&] {
        socket->disconnectFromHost();
#ifndef _WIN32
        socket->waitForDisconnected(100);
#endif
        socket->close();
        delete socket;
    };

    at_scope_exit(at_exit());
}
//------------------------------------------------------------------------------
void server::startup()
{
    if( server_ != nullptr )
        return;

    shutdown_ = false;

    {
        active_socket s;
        s.connect("myip.ru", "http");
        s.recv();
        std::qerr << std::string(s.data(), s.size()) << std::endl;
    }

    server_ = new tcp_server(this);

    if( !server_->listen(QHostAddress::Any, 65535) )
        std::qerr << server_->errorString();

    //pool_.reset(new thread_pool<server_thread>);
    pool_.reset(new thread_pool_t);
    //pool_->enqueue([&] { listener(); });
}
//------------------------------------------------------------------------------
void server::shutdown()
{
    if( server_ == nullptr )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    server_->close();
    lk.unlock();
    cv_.notify_one();

    //thread_->quit();
    //thread_->wait();

    pool_ = nullptr;
    server_ = nullptr;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
