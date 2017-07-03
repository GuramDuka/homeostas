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
#include <string>
//------------------------------------------------------------------------------
#include "scope_exit.hpp"
#include "port.hpp"
#include "socket.hpp"
#include "server.hpp"
#if QT_CORE_LIB
#   include <QString>
#   include <QDebug>
#endif
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void server::listener()
{
    auto accepter = [this] (auto socket) {
        auto w = std::bind(&server::worker, this, std::placeholders::_1);

        for(;;) {
            auto s = socket->accept_shared();

            if( shutdown_ || !s || s->invalid() || s->fail() || socket->interrupt() )
                break;

            thread_pool_t::instance()->enqueue(w, s);
        }
    };

    auto addr_equ = [] (const auto & a, const auto & b) {
        return a.addr_equ(b);
    };

    auto port = std::ihash<uint16_t>(clock_gettime_ns());
    std::vector<socket_addr> interfaces;

    auto recheck_interfaces = [&] {
        std::vector<socket_addr> t;

        for( const auto & a : basic_socket::interfaces<decltype(t)>() ) {
            if( a.is_link_local() || a.is_loopback() )
                continue;
            t.emplace_back(a);
        }

        std::sort(t.begin(), t.end());

        auto r = !std::equal(t.begin(), t.end(), interfaces.begin(), interfaces.end(), addr_equ)
            && t.size() != interfaces.size();

        if( r )
            interfaces = std::move(t);

        return r;
    };

    auto recheck_public_addrs = [&] {
        std::vector<socket_addr> t;

        for( const auto & a : interfaces ) {
            if( a.is_site_local() || a.is_link_local() || a.is_loopback() || a.is_wildcard() )
                continue;
            t.emplace_back(a);
            t.back().port(port_);
        }

        std::sort(t.begin(), t.end());

        std::unique_lock<std::mutex> lock(mtx_);

        auto r = !std::equal(t.begin(), t.end(), public_addrs_.begin(), public_addrs_.end(), addr_equ)
            && t.size() != public_addrs_.size();

        if( r )
            public_addrs_ = std::move(t);

        return r;
    };

    auto reinitialize = [&] {
        port_ = std::rhash(port);

        std::unique_lock<std::mutex> lock(mtx_);

        for( auto s : sockets_ )
            s->interrupt(true).close();

        sockets_.clear();

        for( size_t i = 0; i < interfaces.size(); i++ ) {
            if( sockets_.size() <= i )
                sockets_.emplace_back(std::make_shared<passive_socket>());

            if( !sockets_[i]->listen(interfaces[i].port(port_)) )
                break;
        }

        if( sockets_.size() != interfaces.size() )
            sockets_.clear();

        for( size_t i = 0; i < sockets_.size(); i++ )
            thread_pool_t::instance()->enqueue(accepter, sockets_[i]);
    };

    auto wait = [&] (const std::chrono::seconds & s) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, s, [&] { return shutdown_; });
    };

    at_scope_exit(
        announcer_ = nullptr;
        natpmp_ = nullptr;
    );

    while( !shutdown_ ) {
        try {

            bool port_changed = false;

            while( !shutdown_ ) {
                if( port < 1024 )
                    port += 1024;

                if( recheck_interfaces() || sockets_.empty() )
                    reinitialize();

                if( !sockets_.empty() )
                    break;

                port_++;
                port_changed = true;

                wait(std::chrono::seconds(interfaces.empty() ? 60 : 1));
            }

            if( shutdown_ )
                break;

            bool public_addrs_changed = recheck_public_addrs();

            if( public_addrs_.empty() ) {
                if( natpmp_ == nullptr || natpmp_->private_port() != port_ )
                    public_addrs_changed = true;

                if( public_addrs_changed ) {
                    if( natpmp_ == nullptr )
                        natpmp_.reset(new natpmp);

                    announcer_ = nullptr;
                    natpmp_->shutdown();

                    natpmp_->private_port(port_);
                    natpmp_->port_mapping_lifetime(60);
                    natpmp_->mapped_callback([&] {
                        std::vector<socket_addr> t;
                        t.emplace_back(natpmp_->public_addr());
                        std::cerr << t.back() << std::endl;
#if QT_CORE_LIB
                        qDebug().noquote().nospace() << QString::fromStdString(std::to_string(t.back()));
#endif

                        announcer_.reset(new announcer);
                        announcer_->pubs(t);
                        announcer_->startup();
                    });
                    natpmp_->startup();
                }
            }
            else {
                natpmp_ = nullptr;

                if( public_addrs_changed || port_changed ) {
                    announcer_.reset(new announcer);
                    announcer_->pubs(public_addrs_);
                    announcer_->startup();
                }
            }

            wait(std::chrono::seconds(60));
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
            wait(std::chrono::seconds(3));
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << "undefined c++ exception catched";
#endif
            wait(std::chrono::seconds(3));
        }
    }
}
//------------------------------------------------------------------------------
void server::worker(std::shared_ptr<active_socket> socket)
{
    socket_stream ss(socket);

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
        std::cerr << e << std::endl;
        throw;
    }
}
//------------------------------------------------------------------------------
void server::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    thread_.reset(new std::thread(&server::listener, this));
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
    cv_.notify_one();

    thread_->join();

    thread_ = nullptr;
    natpmp_ = nullptr;
    announcer_ = nullptr;
    sockets_.clear();
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
