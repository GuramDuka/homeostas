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
#include <mutex>
#include "socket.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#if _WIN32
static WSADATA wsa_data;
static std::mutex wsa_mutex;
static std::atomic_uint wsa_count(0);
static volatile bool wsa_started = false;
#endif
//------------------------------------------------------------------------------
base_socket & base_socket::open(int socket_domain, int socket_type, int socket_protocol)
{
    if( is_socket_valid() )
        close();

    errno = SocketSuccess;

#if _WIN32
    wsa_count++;

    if( !wsa_started ) {
        std::unique_lock<std::mutex> lock(wsa_mutex);

        if( !wsa_started ) {
            memset(&wsa_data, 0, sizeof(wsa_data));

            if( WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0 ) {
                wsa_count--;
                throw std::runtime_error(str_error(SocketInvalidSocket));
            }

            wsa_started = true;
        }
    }
#endif

    if( (socket_ = ::socket(socket_domain, socket_type, socket_protocol)) == INVALID_SOCKET ) {
        translate_socket_error();
#if _WIN32
        if( --wsa_count == 0 ) {
            std::unique_lock<std::mutex> lock(wsa_mutex);
            WSACleanup();
            wsa_started = false;
        }
#endif
        throw std::runtime_error(str_error());
    }

    socket_domain_  = (decltype(socket_domain_)) socket_domain;
    socket_type_    = (decltype(socket_type_)) socket_type;

    return *this;
}
//------------------------------------------------------------------------------
base_socket & base_socket::close(bool no_trow)
{
    buffer_ = nullptr;

    if( is_socket_valid() ) {
        if( ::close(socket_) != SocketError ) {
            socket_ = INVALID_SOCKET;
#if _WIN32
            if( --wsa_count == 0 ) {
                std::unique_lock<std::mutex> lock(wsa_mutex);
                WSACleanup();
                wsa_started = false;
            }
#endif
        }
        else if( no_trow ){
            translate_socket_error();
        }
        else {
            throw_socket_error();
        }
    }

    return *this;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
