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
#include "natpmp.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void natpmp_client::startup()
{
    if( thread_ != nullptr )
        return;

    shutdown_ = false;
    socket_.reset(new active_socket);
    thread_.reset(new std::thread(&natpmp_client::worker, this));
}
//------------------------------------------------------------------------------
void natpmp_client::shutdown()
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
void natpmp_client::worker()
{
    constexpr const char NATPMP_PORT[] = "5351";

    socket_->exceptions(false);
    public_addr_.clear();

    decltype(public_addr_) new_addr;

    auto get_public_address = [&] {
        public_address_request req;

        socket_->send(&req, sizeof(req));

        public_address_response resp;
        resp.result_code = ResultCodeInvalid;

        auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(250)).count();

        if( socket_->select(timeout_ns) ) {
            socket_->recv(&resp, sizeof(resp));

            if( resp.result_code == ResultCodeSuccess )
                memcpy(new_addr.family(AF_INET).port(0).addr_data(),
                    &resp.addr,
                    std::min(size_t(new_addr.addr_size()), sizeof(resp.addr)));
        }

        return resp.result_code == ResultCodeSuccess;
    };

    auto new_port_mapping = [&] {
        new_port_mapping_request req;

        req.private_port          = htons(private_port_);
        req.public_port           = htons(public_addr_.port());
        req.port_mapping_lifetime = htonl(port_mapping_lifetime_);

        socket_->send(&req, sizeof(req));

        new_port_mapping_response resp;
        resp.result_code = ResultCodeInvalid;

        auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(250)).count();

        if( socket_->select(timeout_ns) ) {
            socket_->recv(&resp, sizeof(resp));

            if( resp.result_code == ResultCodeSuccess ) {//&& resp.op_code == 130 )
                new_addr.port(ntohs(resp.mapped_public_port));

                if( mapped_callback_ && new_addr != public_addr_ ) {
                    public_addr_ = new_addr;
                    mapped_callback_();
                }
            }
        }

        return resp.result_code == ResultCodeSuccess;
    };

    while( !shutdown_ ) {
        gateway_.family(AF_INET).port(0);

        if( gateway_.default_gateway(true) ) {
            socket_->close();
            //socket_->open(SocketDomainINET, SocketTypeUDP, SocketProtoIP);
            socket_->connect(gateway_.to_string(), NATPMP_PORT);

            if( get_public_address() )
                if( new_port_mapping() ) {

                }
        }

        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(3), [&] { return shutdown_; });
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
