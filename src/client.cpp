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
#include "discoverer.hpp"
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
    worker_result_ = thread_pool_t::instance()->enqueue(&client::worker, this);
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

    worker_result_.wait();
    socket_ = nullptr;
}
//------------------------------------------------------------------------------
void client::worker()
{
    socket_stream ss;
    std::key512 p2p_key(std::leave_uninitialized);

    ss.handshake_functor([&] (
        handshake::packet * req,
        handshake::packet * res,
        std::key512 * p_local_transport_key,
        std::key512 * p_remote_transport_key) {
        assert( req != nullptr );

        if( res == nullptr ) {
            // setup request before send to server
            std::copy(std::begin(client_public_key_), std::end(client_public_key_),
                std::begin(req->public_key), std::end(req->public_key));

            // client fingerprint based on p2p key and server public key
            cdc512 fingerprint(p2p_key.begin(), p2p_key.end(),
                server_public_key_.begin(), server_public_key_.end());

            std::copy(std::begin(fingerprint), std::end(fingerprint),
                std::begin(req->fingerprint), std::end(req->fingerprint));

#if QT_CORE_LIB
            qDebug().noquote().nospace() << "client p2p key: " <<
                QString::fromStdString(std::to_string(p2p_key));
            qDebug().noquote().nospace() << "client fingerprint: " <<
                QString::fromStdString(std::to_string(fingerprint));
#endif

        }
        else if( p_local_transport_key == nullptr && p_remote_transport_key == nullptr ) {
            if( server_public_key_ != res->public_key ) {
                req->error = handshake::ErrorInvalidHostPublicKey;
            }
            else {
                // server fingerprint based on p2p key and client public key
                cdc512 fingerprint(std::begin(p2p_key), std::end(p2p_key),
                    std::begin(client_public_key_), std::end(client_public_key_));

                if( fingerprint == res->fingerprint ) {
                    // approved server fingerprint
                    req->error = handshake::ErrorNone;
                }
                else {
                    req->error = handshake::ErrorInvalidFingerprint;
                }
            }
        }
        else {
            // server approved my fingerprint
            *p_local_transport_key = cdc512(std::begin(p2p_key), std::end(p2p_key),
                std::begin(req->session_key), std::end(req->session_key));
            *p_remote_transport_key = cdc512(std::begin(p2p_key), std::end(p2p_key),
                std::begin(res->session_key), std::end(res->session_key));
        }
    });

    auto wait = [&] (const std::chrono::seconds & s) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, s, [&] { return shutdown_; });
    };

    discoverer d;

    // set handshake parameters
    ss.proto(handshake::ProtocolV1);
    ss.encryption(handshake::EncryptionLight);
    ss.encryption_option(handshake::OptionPrefer);
    ss.compression(handshake::CompressionLZ4);
    ss.compression_option(handshake::OptionPrefer);

    while( !shutdown_ ) {
        try {
            auto addrs = d.discover_host(server_public_key_, &p2p_key);

            while( !shutdown_ && !addrs.empty() ) {
                auto e = std::rend(addrs), addr = std::find_if(std::rbegin(addrs), e, [] (const auto & a) {
                    return a.is_loopback();
                });

                if( addr == e )
                    addr = std::rbegin(addrs);

                if( addr != e ) {
                    socket_->exceptions(false);
                    socket_->connect(*addr);
                    socket_->exceptions(true);
                    addrs.erase(addr.base() - 1);

                    if( *socket_ ) {
                        ss.reset(socket_);

                        ss << "Hello" << std::flush;
                        std::string s;
                        ss >> s;
                    }
                }
            }
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
        }
        wait(std::chrono::seconds(60));
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
