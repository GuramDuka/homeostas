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
#include "port.hpp"
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
void server::announcer()
{

}
//------------------------------------------------------------------------------
void server::listener(std::shared_ptr<passive_socket> socket)
{
    auto listen_func = [&] (auto socket) {
        auto w = std::bind(&server::worker, this, std::placeholders::_1);

        while( !shutdown_ ) {
            auto s = socket->accept_shared();

            if( shutdown_ || !s || s->invalid() || s->fail() || socket->interrupt() )
                break;

            pool_->enqueue(w, s);
        }
    };

    if( socket ) {
        listen_func(socket);
    }
    else {
        port_ = std::ihash<uint16_t>(clock_gettime_ns());

        if( port_ < 1024 )
            port_ += 1024;

        auto reinitialize = [&] {
            auto port = std::to_string(std::rhash(port_));
            auto wildcards = base_socket::wildcards();

            std::unique_lock<std::mutex> lock(mtx_);

            for( auto s : sockets_ )
                s->interrupt(true).close();

            sockets_.clear();

            for( size_t i = 0; i < wildcards.size(); i++ ) {
                if( sockets_.size() <= i )
                    sockets_.emplace_back(std::make_shared<passive_socket>());

                sockets_[i]->exceptions(false);

                if( !sockets_[i]->listen(wildcards[i], port) ) {
                    sockets_.clear();
                    break;
                }
            }

            for( size_t i = 1; i < sockets_.size(); i++ )
                pool_->enqueue(listen_func, sockets_[i]);
        };

        while( !shutdown_ ) {
            reinitialize();

            if( sockets_.empty() ) {
                if( ++port_ < 1024 )
                    port_ += 1024;
            }
            else {
                natpmp_.reset(new natpmp_client);
                natpmp_->private_port(port_);
                natpmp_->port_mapping_lifetime(3600);
                natpmp_->mapped_callback(&server::announcer, this);
                natpmp_->startup();

                listen_func(sockets_[0]);
            }

            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(30), [&] { return shutdown_; });
        }
    }
}
//------------------------------------------------------------------------------
void server::worker(std::shared_ptr<active_socket> socket)
{
    socket_stream_buffered ss(*socket);

    try {
        std::string s;
        ss >> s;
        //cdc512 ctx(s.begin(), s.end());
        //ss << ctx.to_short_string() << std::flush;

        //std::unique_lock<std::mutex> lock(mtx);
        //std::qerr
        //    << "accepted: "
        //    << std::to_string(client_socket->local_addr())
        //    << " -> "
        //    << std::to_string(client_socket->remote_addr())
        //    << std::endl;
    }
    catch( const std::ios_base::failure & e ) {
        std::unique_lock<std::mutex> lock(mtx_);
        std::qerr << e << std::endl;
        throw;
    }
}
//------------------------------------------------------------------------------
void server::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    thread_.reset(new std::thread(&server::listener, this, nullptr));
    pool_.reset(new thread_pool_t);
}
//------------------------------------------------------------------------------
void server::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lock(mtx_);
    shutdown_ = true;
    for( auto s : sockets_ )
        s->interrupt(true).close();
    lock.unlock();

    thread_->join();

    thread_ = nullptr;
    pool_ = nullptr;
    natpmp_ = nullptr;
    sockets_.clear();
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
