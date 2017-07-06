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
#include "server.hpp"
#include "client.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void client::startup() {
    if( socket_ != nullptr )
        return;

    shutdown_ = false;
    socket_ = std::make_unique<active_socket>();
    thread_pool_t::instance()->enqueue(&client::worker, this);
}
//------------------------------------------------------------------------------
void client::shutdown()
{
    if( socket_ == nullptr )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    socket_->close();
    lk.unlock();
    cv_.notify_one();

    socket_ = nullptr;
}
//------------------------------------------------------------------------------
void client::worker()
{
    auto wait = [&] (const std::chrono::seconds & s) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, s, [&] { return shutdown_; });
    };

    homeostas::configuration config;
    //addr.family(AF_INET).saddr4.sin_addr.s_addr = inet_addr("127.0.0.1");
    //std::blob private_id;

    socket_stream ss;
    std::key512 peer_pubic_key, peer_p2p_key;
    std::vector<socket_addr> peer_addrs;

    ss.handshake_functor([&] (
        handshake::packet * req,
        handshake::packet * res,
        std::key512 * p_local_transport_key,
        std::key512 * p_remote_transport_key) {
        assert( req != nullptr );

        if( res == nullptr ) {
            // setup request before send to server
            std::copy(std::begin(host_public_key_), std::end(host_public_key_),
                std::begin(req->public_key), std::end(req->public_key));
            // client fingerprint based on p2p key and server public key
            std::key512 fingerprint = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(peer_pubic_key), std::end(peer_pubic_key));
            std::copy(std::begin(fingerprint), std::end(fingerprint),
                std::begin(req->fingerprint), std::end(req->fingerprint));
        }
        else if( p_local_transport_key == nullptr && p_remote_transport_key == nullptr ) {
            // server fingerprint based on p2p key and client fingerprint
            std::key512 fingerprint = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(req->fingerprint), std::end(req->fingerprint));

            if( fingerprint == res->fingerprint ) {
                // approved server fingerprint
                req->error = handshake::ErrorNone;
            }
            else {
                req->error = handshake::ErrorInvalidFingerprint;
            }
        }
        else {
            // server approved my fingerprint
            *p_local_transport_key = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(req->session_key), std::end(req->session_key));
            *p_remote_transport_key = cdc512(std::begin(peer_p2p_key), std::end(peer_p2p_key),
                std::begin(res->session_key), std::end(res->session_key));
        }
    });

    while( !shutdown_ ) {
        try {
            uint16_t port = 0;

            while( !shutdown_ ) {
                auto p = config.get("host.port");

                if( p != nullptr ) {
                    port = std::rhash(p.get<uint16_t>());
                    //private_id = config.get("host.private_id").get<std::blob>();
                    //config.detach();
                    break;
                }

                wait(std::chrono::seconds(1));
            }

            if( shutdown_ )
                break;

            addr.port(port);

            try {
                socket_->connect(addr);
                ss.reset(socket_);
                // set handshake parameters

            }
            catch( const std::exception & e ) {
                std::cerr << e << std::endl;
#if QT_CORE_LIB
                qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
                wait(std::chrono::seconds(3));
            }
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
            wait(std::chrono::seconds(60));
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << "undefined c++ exception catched";
#endif
            wait(std::chrono::seconds(60));
        }
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
