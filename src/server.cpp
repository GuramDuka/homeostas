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
#if QT_CORE_LIB
#   include <QString>
#   include <QDebug>
#endif
//------------------------------------------------------------------------------
#include "std_ext.hpp"
#include "port.hpp"
#include "socket.hpp"
#include "server.hpp"
#include "discoverer.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void server::accepter(std::shared_ptr<passive_socket> socket)
{
    auto w = std::bind(&server::worker, this, std::placeholders::_1);

    for(;;) {
        auto s = socket->accept_shared();

        if( shutdown_ || !s || s->invalid() || s->fail() || socket->interrupt() )
            break;

        auto result_future = thread_pool_t::instance()->enqueue(w, s);

        std::unique_lock<std::mutex> lock(mtx_);
        workers_results_.push_back(result_future);
    }
}
//------------------------------------------------------------------------------
void server::control()
{
    discoverer d;

    auto port = [&] {
        auto addrs = d.discover_host(host_public_key_);

        for( const auto & a : addrs )
            if( a.is_link_local() || a.is_loopback() )
                return a.port();

        return std::ihash<uint16_t>(entropy_fast());
    }();

    std::vector<socket_addr> interfaces;

    auto recheck_interfaces = [&] {
        auto t = basic_socket::interfaces();
        std::sort(t.begin(), t.end());

        auto r = !std::equal(t.begin(), t.end(), interfaces.begin(), interfaces.end(), socket_addr::addr_equ)
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

        auto r = !std::equal(t.begin(), t.end(), public_addrs_.begin(), public_addrs_.end(), socket_addr::addr_equ)
            && t.size() != public_addrs_.size();

        if( r )
            public_addrs_ = std::move(t);

        return r;
    };

    std::vector<std::shared_ptr<passive_socket>> sockets;
    std::vector<std::shared_future<void>> accepters_results;

    auto reinitialize = [&] {
        auto nport = std::rhash(port);

        for( auto s : sockets )
            s->interrupt(true).close();
        for( auto a : accepters_results )
            a.wait();
        for( auto w : workers_results_ )
            w.wait();

        std::unique_lock<std::mutex> lock(mtx_);
        sockets.clear();
        accepters_results.clear();
        workers_results_.clear();

        for( size_t i = 0; i < interfaces.size(); i++ ) {
            auto socket = std::make_shared<passive_socket>();
            auto safe = socket->exceptions();
            at_scope_exit( socket->exceptions(safe) );
            socket->exceptions(false);
            socket->listen(interfaces[i].port(port));

            if( socket )
                sockets.emplace_back(socket);
        }

        for( size_t i = 0; i < sockets.size(); i++ )
            accepters_results.push_back(
                thread_pool_t::instance()->enqueue(&server::accepter, this, sockets[i]));

        if( !sockets.empty() )
            port_ = nport;
    };

    auto wait = [&] (const std::chrono::seconds & s) {
        std::unique_lock<std::mutex> lock(mtx_);

        workers_results_.remove_if([&] (auto w) {
            return w.wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready;
        });

        cv_.wait_for(lock, s, [&] { return shutdown_; });
    };

    at_scope_exit(
        for( auto s : sockets )
            s->interrupt(true).close();
        for( auto a : accepters_results )
            a.wait();
        for( auto w : workers_results_ )
            w.wait();
        workers_results_.clear();
        announcer_ = nullptr;
        natpmp_ = nullptr;
    );

    while( !shutdown_ ) {
        try {
            bool port_changed = false;

            while( !shutdown_ ) {
                if( port < 1024 )
                    port += 1024;

                if( recheck_interfaces() || sockets.empty() )
                    reinitialize();

                if( !sockets.empty() )
                    break;

                port++;
                port_changed = true;

                wait(std::chrono::seconds(sockets.empty() ? 60 : 1));
            }

            if( shutdown_ )
                break;

            d.announce_host(host_public_key_, &interfaces);

            bool public_addrs_changed = recheck_public_addrs();

            if( public_addrs_.empty() ) {
                if( natpmp_ == nullptr || natpmp_->private_port() != port_ )
                    public_addrs_changed = true;

                if( public_addrs_changed ) {
                    if( natpmp_ == nullptr )
                        natpmp_ = std::make_unique<natpmp>();

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

                        announcer_ = std::make_unique<announcer>();
                        announcer_->pubs(t);
                        announcer_->startup();
                    });
                    natpmp_->startup();
                }
            }
            else {
                natpmp_ = nullptr;

                if( public_addrs_changed || port_changed ) {
                    announcer_ = std::make_unique<announcer>();
                    announcer_->pubs(public_addrs_);
                    announcer_->startup();
                }
            }
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << "undefined c++ exception catched";
#endif
        }

        wait(std::chrono::seconds(60));
    }
}
//------------------------------------------------------------------------------
void server::worker(std::shared_ptr<active_socket> socket)
{
    socket_stream ss(socket);
    std::key512 peer_pubic_key, peer_p2p_key;

    ss.handshake_functor([&] (
        handshake::packet * req,
        handshake::packet * res,
        std::key512 * p_local_transport_key,
        std::key512 * p_remote_transport_key) {
        assert( req != nullptr );

        if( res != nullptr ) {
            // client request received
            discoverer d;
            peer_pubic_key = req->public_key;
            peer_p2p_key = d.discover_host_p2p_key(peer_pubic_key);

            // client fingerprint based on p2p key and server public key
            std::key512 fingerprint = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(host_public_key_), std::end(host_public_key_));

            if( fingerprint == req->fingerprint ) {
                // approved client fingerprint
                res->error = handshake::ErrorNone;
                // setup response before send to client
                std::copy(std::begin(host_public_key_), std::end(host_public_key_),
                    std::begin(res->public_key), std::end(res->public_key));

                cdc512 fingerprint(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                    std::begin(req->fingerprint), std::end(req->fingerprint));
                std::copy(std::begin(fingerprint), std::end(fingerprint),
                    std::begin(res->fingerprint), std::end(res->fingerprint));
            }
            else {
                res->error = handshake::ErrorInvalidFingerprint;
            }
        }

        if( p_local_transport_key != nullptr && p_remote_transport_key != nullptr ) {
            // client approved my fingerprint
            *p_local_transport_key = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(res->session_key), std::end(res->session_key));
            *p_remote_transport_key = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(req->session_key), std::end(req->session_key));
        }
    });

    // set handshake parameters
    ss.proto(handshake::ProtocolV1);
    ss.encryption(handshake::EncryptionLight);
    ss.encryption_option(handshake::OptionPrefer);
    ss.compression(handshake::CompressionLZ4);
    ss.compression_option(handshake::OptionPrefer);

    while( !shutdown_ ) {
        try {
            std::string s;
            ss >> s;
            ss << "Ehlo" << std::flush;

            //std::cerr
            //    << "accepted: "
            //    << std::to_string(client_socket->local_addr())
            //    << " -> "
            //    << std::to_string(client_socket->remote_addr())
            //    << std::endl;
        }
        catch( const std::ios_base::failure & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << "undefined c++ exception catched";
#endif
        }
    }
}
//------------------------------------------------------------------------------
void server::startup()
{
    if( started_ )
        return;

    shutdown_ = false;
    control_result_ = thread_pool_t::instance()->enqueue(&server::control, this);
    started_ = true;
}
//------------------------------------------------------------------------------
void server::shutdown()
{
    if( !started_ )
        return;

    std::unique_lock<std::mutex> lock(mtx_);
    shutdown_ = true;
    lock.unlock();
    cv_.notify_one();

    control_result_.wait();
    natpmp_ = nullptr;
    announcer_ = nullptr;
    started_ = false;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
