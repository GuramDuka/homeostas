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
#if QT_CORE_LIB && __ANDROID__
#   include <QString>
#   include <QDebug>
#endif
//------------------------------------------------------------------------------
#ifndef NDEBUG
#   include <fstream>
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
    socket_->exceptions(false).type(SocketTypeUDP);
    public_addr_.clear();

    decltype(public_addr_) new_addr;

    auto get_public_address = [&] {
        public_address_request req;

        socket_->send(&req, sizeof(req));

        public_address_response resp;
        resp.result_code = ResultCodeInvalid;

        auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(250)).count();

        if( socket_->select_rd(timeout_ns) ) {
            socket_->recv(&resp, sizeof(resp));

            new_addr.family(AF_INET).port(0);

            if( resp.result_code == ResultCodeSuccess && gateway_.addr_equ(socket_->peer_addr()) )
                memcpy(
                    new_addr.addr_data(),
                    &resp.addr,
                    std::min(size_t(new_addr.addr_size()), sizeof(resp.addr)));
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

            auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(250)).count();

            auto r = socket_->select_rd(timeout_ns);

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
                    new_addr.port(ntohs(resp.mapped_public_port));

                    if( new_addr != public_addr_ ) {
                        public_addr_ = new_addr;
                        public_port_ = new_addr.port();

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

#ifndef NDEBUG
    std::ifstream f("/proc/net/route", std::ios::binary);
    f.exceptions(std::ios::failbit | std::ios::badbit);
    f.unsetf(std::ios::skipws);
    f.unsetf(std::ios::unitbuf);

    std::string s;

    while( !f.eof() ) {
        std::getline(f, s, '\n');
        std::cerr << s << std::endl;
#   if QT_CORE_LIB && __ANDROID__
        qDebug().noquote().nospace() << QString::fromStdString(s);
#   endif
    }
#endif

    while( !shutdown_ ) {
        bool mapped = false;

        gateway_.family(AF_INET);

        if( gateway_.default_gateway(true) ) {
            gateway_.port(NATPMP_PORT);

#ifndef NDEBUG
            std::cerr << gateway_ << std::endl;
#   if QT_CORE_LIB && __ANDROID__
            qDebug().noquote().nospace() << QString::fromStdString(std::to_string(gateway_));
#   endif
#endif

            socket_->connect(gateway_);

            if( get_public_address() )
                if( port_mapping() )
                    mapped = true;

            socket_->close();
        }

        std::unique_lock<std::mutex> lock(mtx_);
        auto timeout_s = std::chrono::seconds(mapped ? port_mapping_lifetime_ : 1);
        cv_.wait_for(lock, timeout_s, [&] { return shutdown_; });
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
