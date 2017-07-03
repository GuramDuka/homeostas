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
#include "natpmp.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void natpmp::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    socket_.reset(new active_socket);
    thread_.reset(new std::thread(&natpmp::worker, this));
}
//------------------------------------------------------------------------------
void natpmp::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    socket_->close();
    lk.unlock();
    cv_.notify_one();

    thread_->join();
    thread_ = nullptr;
    socket_ = nullptr;
}
//------------------------------------------------------------------------------
void natpmp::worker()
{
    constexpr const auto timeout_250ms =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::milliseconds(250)).count();

    decltype(public_addr_) new_public_addr;

    auto get_public_address = [&] {
        public_address_request req;

        socket_->send(&req, sizeof(req));

        public_address_response resp;

        if( socket_->select_rd(timeout_250ms) ) {
            socket_->recv(&resp, sizeof(resp));
            new_public_addr.family(AF_INET).port(0);

            if( resp.result_code == ResultCodeSuccess && gateway_.addr_equ(socket_->peer_addr()) )
                memcpy(
                    new_public_addr.addr_data(),
                    &resp.addr,
                    std::min(size_t(new_public_addr.addr_size()), sizeof(resp.addr)));
        }

        return resp.result_code == ResultCodeSuccess;
    };

    auto port_mapping = [&] {
        new_port_mapping_request req;
        new_port_mapping_response resp;

        auto try_map = [&] {
            req.private_port          = htons(private_port_);
            req.public_port           = htons(public_port_);
            req.port_mapping_lifetime = htonl(port_mapping_lifetime_ + 3);

            socket_->send(&req, sizeof(req));

            resp.result_code = ResultCodeInvalid;

            auto r = socket_->select_rd(timeout_250ms);

            if( r )
                socket_->recv(&resp, sizeof(resp));

            return r;
        };

        req.op_code = 2; // OP Code = 1 for UDP or 2 for TCP

        if( try_map() ) {

            if( resp.result_code == ResultCodeSuccess ) {//&& resp.op_code == 130 )
                req.op_code = 1; // OP Code = 1 for UDP or 2 for TCP
                req.public_port = resp.mapped_public_port;

                if( try_map() ) {
                    new_public_addr.port(ntohs(resp.mapped_public_port));

                    if( new_public_addr != public_addr_ ) {
                        public_addr_ = new_public_addr;
                        public_port_ = new_public_addr.port();

#ifndef NDEBUG
                        //std::cerr << "public address & mapped port: " << public_addr_ << std::endl;
#endif

                        if( mapped_callback_ )
                            mapped_callback_();
                    }
                }
            }
        }

        if( resp.result_code != ResultCodeSuccess )
            public_port_ = 0;

        return resp.result_code == ResultCodeSuccess;
    };

    auto try_map = [&] (const socket_addr & g) {
        socket_->remote_addr(gateway_ = g);

        try {
            if( get_public_address() && port_mapping() )
                return true;
        }
        catch( ... ) {
            if( socket_->socket_errno() != SocketConnectionReset )
                throw;
            socket_->open(g.family(), SocketTypeUDP, SocketProtoIP);
        }

        return false;
    };

    auto wait = [&] (const std::chrono::seconds & s) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, s, [&] { return shutdown_; });
    };

    socket_addr gateway_mask, last_gateway, gate;
    gateway_mask.clear();
    gate.family(AF_INET);

    while( !shutdown_ ) {
        try {
            bool mapped = false;

            socket_->open(gate.family(), SocketTypeUDP, SocketProtoIP);

            if( last_gateway.family() != AF_UNSPEC )
                mapped = try_map(last_gateway);

            if( !mapped && gate.default_gateway(true, &gateway_mask) ) {
                gate.port(NATPMP_PORT);

                auto nmask = ~gateway_mask.saddr4.sin_addr.s_addr;
                bool invalid_gateway = (gate.saddr4.sin_addr.s_addr & nmask) == 0;

                while( !shutdown_ && !mapped ) {
                    if( invalid_gateway ) {
                        // scan local network, try next address
                        gate.saddr4.sin_addr.s_addr = htonl(ntohl(gate.saddr4.sin_addr.s_addr) + 1);
                        // check end of local network ip range, (192.168.1.255 for example)
                        if( (gate.saddr4.sin_addr.s_addr & nmask) == nmask )
                            break;
#if QT_CORE_LIB && !defined(NDEBUG)
                        qDebug().noquote().nospace() << "pong gate: " << QString::fromStdString(std::to_string(socket_addr(gate).port(0)));
#endif
                    }

                    mapped = try_map(gate);

                    if( mapped ) {
                        last_gateway = gate;
                        break;
                    }

                    if( !invalid_gateway )
                        break;
                }
            }

            socket_->close();

            std::unique_lock<std::mutex> lock(mtx_);
            auto timeout_s = std::chrono::seconds(mapped ? port_mapping_lifetime_ : 60);
            cv_.wait_for(lock, timeout_s, [&] { return shutdown_; });
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << QString::fromStdString(e.what());
#endif
            public_addr_.clear();
            wait(std::chrono::seconds(3));
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
#if QT_CORE_LIB
            qDebug().noquote().nospace() << "undefined c++ exception catched";
#endif
            public_addr_.clear();
            wait(std::chrono::seconds(3));
        }
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
