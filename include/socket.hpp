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
#ifndef SOCKET_HPP_INCLUDED
#define SOCKET_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#if _WIN32
#	include <io.h>
#else
#   include <sys/time.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netinet/tcp.h>
#	include <netinet/ip.h>
#	include <netdb.h>
#	include <sys/time.h>
#	include <sys/uio.h>
#	include <unistd.h>
#	include <fcntl.h>
#   include <poll.h>
#endif
//------------------------------------------------------------------------------
#if QT_CORE_LIB
#   include <QNetworkInterface>
#endif
//------------------------------------------------------------------------------
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cassert>
#include <type_traits>
#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <array>
#include <vector>
#include <new>
#include <ios>
#include <iomanip>
#include <streambuf>
#include <istream>
#include <ostream>
#include <iostream>
#include <algorithm>
#include <locale>
#include <iterator>
#include <functional>
//------------------------------------------------------------------------------
#include "port.hpp"
#include "locale_traits.hpp"
#include "std_ext.hpp"
#include "scope_exit.hpp"
#include "cdc512.hpp"
#include "rand.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
#if _WIN32
    struct iovec {
        void  *iov_base;
        size_t iov_len;
    };
	typedef int socklen_t;
	constexpr int IPTOS_LOWDELAY = 0x10;
    constexpr int SHUT_RD = 0;
    constexpr int SHUT_WR = 1;
    constexpr int SHUT_RDWR = 2;
    constexpr int AF_PACKET = 17;
#else
#   ifndef IPPROTO_IPV4
#       define IPPROTO_IPV4 4
#   endif
    typedef int            SOCKET;
    constexpr int SOCKET_ERROR = -1;
#endif

constexpr int SOCKET_ERROR_INTERRUPT = EINTR;
constexpr int SOCKET_ERROR_TIMEDOUT  = EAGAIN;

// https://en.wikipedia.org/wiki/Maximum_segment_size
constexpr size_t TX_MAX_BYTES = 1220;
constexpr size_t RX_MAX_BYTES = 1220;

// http://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
// portable way to access the L1 data cache line size.
constexpr size_t MIN_BUFFER_SIZE =
#if __cplusplus > 201500L
    std::hardware_destructive_interference_size
#else
    64
#endif
    ;

#ifndef INVALID_SOCKET
constexpr SOCKET INVALID_SOCKET = (SOCKET) -1;
#endif
//------------------------------------------------------------------------------
int get_default_gateway(in_addr * addr, in_addr * mask = nullptr);
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
//struct inet_addr {
//    union {
//        in_addr                 addr4;      // IPv4 address
//        in_addr6                addr6;      // IPv6 address
//    };
//};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct socket_addr {
    union {
        sockaddr_storage        storage;
        sockaddr_in             saddr4;      // IPv4 address
        sockaddr_in6            saddr6;      // IPv6 address
    };

    // move constructor
    socket_addr() noexcept {
        storage.ss_family = AF_UNSPEC;
    }

    socket_addr(const addrinfo & ai) noexcept {
        memcpy(&storage.ss_family, ai.ai_addr, std::min(sizeof(storage), size_t(ai.ai_addrlen)));
    }

    socket_addr(const sockaddr * sa, socklen_t sl) noexcept {
        memcpy(&storage, sa, std::min(sizeof(storage), size_t(sl)));
    }

    template <class _Elem, class _Traits, class _Alloc>
    socket_addr(const std::basic_string<_Elem, _Traits, _Alloc> & s);

    // copy constructor
    socket_addr(const socket_addr & o) noexcept {
        *this = o;
    }

    // move constructor
    socket_addr(socket_addr && o) noexcept {
        *this = std::move(o);
    }

    // move assignment operator
    socket_addr & operator = (socket_addr && o) noexcept {
        memmove(this, &o, sizeof(o));
        return *this;
    }

    // copy assignment operator
    socket_addr & operator = (const socket_addr & o) noexcept {
        if( this != &o )
            memcpy(this, &o, sizeof(o));
        return *this;
    }

    bool operator == (const socket_addr & s) const {
        return compare(s) == 0;
    }

    bool operator != (const socket_addr & s) const {
        return compare(s) != 0;
    }

    bool operator > (const socket_addr & s) const {
        return compare(s) > 0;
    }

    bool operator >= (const socket_addr & s) const {
        return compare(s) >= 0;
    }

    bool operator < (const socket_addr & s) const {
        return compare(s) < 0;
    }

    bool operator <= (const socket_addr & s) const {
        return compare(s) <= 0;
    }

    const auto & family() const {
        return storage.ss_family;
    }

    template <typename T>
    socket_addr & family(const T & family) {
        storage.ss_family = decltype(storage.ss_family) (family);
        return *this;
    }

    auto port() const {
        return ntohs(storage.ss_family == AF_INET6 ? saddr6.sin6_port : storage.ss_family == AF_INET ? saddr4.sin_port : 0);
    }

    template <typename T>
    socket_addr & port(const T & port) {
        if( storage.ss_family == AF_INET )
            saddr4.sin_port = htons(decltype(saddr4.sin_port) (port));
        else if( storage.ss_family == AF_INET6 )
            saddr6.sin6_port = htons(decltype(saddr6.sin6_port) (port));
        return *this;
    }

    const void * addr_data() const {
        return storage.ss_family == AF_INET ? (const void *) &saddr4.sin_addr :
            storage.ss_family == AF_INET6 ? (const void *) &saddr6.sin6_addr :
                (const uint8_t *) &storage.ss_family + sizeof(storage.ss_family);
    }

    void * addr_data() {
        return storage.ss_family == AF_INET ? (void *) &saddr4.sin_addr :
            storage.ss_family == AF_INET6 ? (void *) &saddr6.sin6_addr :
                (uint8_t *) &storage.ss_family + sizeof(storage.ss_family);
    }

    socklen_t addr_size() const {
        return storage.ss_family == AF_INET ? sizeof(saddr4.sin_addr) :
            storage.ss_family == AF_INET6 ? sizeof(saddr6.sin6_addr) :
                sizeof(storage) - sizeof(storage.ss_family);
    }

    const void * data() const {
        return storage.ss_family == AF_INET ? (const void *) &saddr4 :
            storage.ss_family == AF_INET6 ? (const void *) &saddr6 :
                (const void *) &storage;
    }

    void * data() {
        return storage.ss_family == AF_INET ? (void *) &saddr4 :
            storage.ss_family == AF_INET6 ? (void *) &saddr6 :
                (void *) &storage;
    }

    socklen_t size() const {
        return storage.ss_family == AF_INET ? sizeof(saddr4) :
            storage.ss_family == AF_INET6 ? sizeof(saddr6) : 0;
    }

    const sockaddr * sock_data() const {
        return storage.ss_family == AF_INET ? (const sockaddr *) &saddr4 :
            storage.ss_family == AF_INET6 ? (const sockaddr *) &saddr6 :
                (const sockaddr *) &storage;
    }

    sockaddr * sock_data() {
        return storage.ss_family == AF_INET ? (sockaddr *) &saddr4 :
            storage.ss_family == AF_INET6 ? (sockaddr *) &saddr6 :
                (sockaddr *) &storage;
    }

    socket_addr & clear() {
        memset(this, 0, sizeof(*this));
#if AF_UNSPEC != 0
        storage.ss_family = AF_UNSPEC;
#endif
        return *this;
    }

    int compare(const socket_addr & s) const {
        return storage.ss_family > s.storage.ss_family ? 1 : storage.ss_family < s.storage.ss_family ? -1
            : port() > s.port() ? 1 : port() < s.port() ? -1
                : memcmp(addr_data(), s.addr_data(), addr_size());
    }

    bool addr_equ(const socket_addr & s) const {
        return addr_size() == s.addr_size() && memcmp(addr_data(), s.addr_data(), addr_size()) == 0;
    }

    bool addr_neq(const socket_addr & s) const {
        return addr_size() != s.addr_size() || memcmp(addr_data(), s.addr_data(), addr_size()) != 0;
    }

    template <typename T>
    static uint32_t cidr_ip4(T b0, T b1, T b2, T b3, T l) {
        uint32_t a = uint32_t(b0);
        a <<= 8;
        a |= b1;
        a <<= 8;
        a |= b2;
        a <<= 8;
        a |= b3;
        a &= (~uint32_t(0)) << (32 - l);
        return a;
    }

    template <typename T>
    auto is_cidr_ip4(T b0, T b1, T b2, T b3, T l) const {
        auto m = cidr_ip4(b0, b1, b2, b3, l);
        return (ntohl(saddr4.sin_addr.s_addr) & m) == m;
    }

    bool is_site_local() const {

        if( storage.ss_family == AF_INET )
            return
                is_cidr_ip4(192, 168, 0, 0, 16) ||
                is_cidr_ip4(172,  16, 0, 0, 20) ||
                is_cidr_ip4( 10,   0, 0, 0, 24) ||
                is_cidr_ip4(100,  64, 0, 0, 10);

        if( storage.ss_family == AF_INET6 )
            return saddr6.sin6_addr.s6_addr[0] == 0xfe && (saddr6.sin6_addr.s6_addr[1] & 0xc0) == 0xc0;

        throw std::xruntime_error("Invalid address family", __FILE__, __LINE__);
    }

    bool is_link_local() const {
        if( storage.ss_family == AF_INET )
            return
                 is_cidr_ip4(169, 254,   0, 0, 16) &&
                !is_cidr_ip4(169, 254,   0, 0, 24) &&
                !is_cidr_ip4(169, 254, 254, 0, 24);

        if( storage.ss_family == AF_INET6 )
            return saddr6.sin6_addr.s6_addr[0] == 0xfe && (saddr6.sin6_addr.s6_addr[1] & 0xc0) == 0x80;

        throw std::xruntime_error("Invalid address family", __FILE__, __LINE__);
    }

    bool is_loopback() const {

        if( storage.ss_family == AF_INET )
            return is_cidr_ip4(127, 0, 0, 0, 24);

        if( storage.ss_family == AF_INET6 )
            return
                   saddr6.sin6_addr.s6_addr[ 0] == 0 && saddr6.sin6_addr.s6_addr[ 1] == 0
                && saddr6.sin6_addr.s6_addr[ 2] == 0 && saddr6.sin6_addr.s6_addr[ 3] == 0
                && saddr6.sin6_addr.s6_addr[ 4] == 0 && saddr6.sin6_addr.s6_addr[ 5] == 0
                && saddr6.sin6_addr.s6_addr[ 6] == 0 && saddr6.sin6_addr.s6_addr[ 7] == 0
                && saddr6.sin6_addr.s6_addr[ 8] == 0 && saddr6.sin6_addr.s6_addr[ 9] == 0
                && saddr6.sin6_addr.s6_addr[10] == 0 && saddr6.sin6_addr.s6_addr[11] == 0
                && saddr6.sin6_addr.s6_addr[12] == 0 && saddr6.sin6_addr.s6_addr[13] == 0
                && saddr6.sin6_addr.s6_addr[14] == 0 && saddr6.sin6_addr.s6_addr[15] == 1;

        throw std::xruntime_error("Invalid address family", __FILE__, __LINE__);
    }

    bool is_wildcard() const {

        if( storage.ss_family == AF_INET )
            return is_cidr_ip4(0, 0, 0, 0, 0);

        if( storage.ss_family == AF_INET6 )
            return
                   saddr6.sin6_addr.s6_addr[ 0] == 0 && saddr6.sin6_addr.s6_addr[ 1] == 0
                && saddr6.sin6_addr.s6_addr[ 2] == 0 && saddr6.sin6_addr.s6_addr[ 3] == 0
                && saddr6.sin6_addr.s6_addr[ 4] == 0 && saddr6.sin6_addr.s6_addr[ 5] == 0
                && saddr6.sin6_addr.s6_addr[ 6] == 0 && saddr6.sin6_addr.s6_addr[ 7] == 0
                && saddr6.sin6_addr.s6_addr[ 8] == 0 && saddr6.sin6_addr.s6_addr[ 9] == 0
                && saddr6.sin6_addr.s6_addr[10] == 0 && saddr6.sin6_addr.s6_addr[11] == 0
                && saddr6.sin6_addr.s6_addr[12] == 0 && saddr6.sin6_addr.s6_addr[13] == 0
                && saddr6.sin6_addr.s6_addr[14] == 0 && saddr6.sin6_addr.s6_addr[15] == 0;

        throw std::xruntime_error("Invalid address family", __FILE__, __LINE__);
    }

    bool default_gateway(bool no_throw = false, socket_addr * p_mask = nullptr) {
        if( storage.ss_family == AF_INET )
            if( get_default_gateway(&saddr4.sin_addr, p_mask != nullptr ? &p_mask->saddr4.sin_addr : nullptr) == 0 )
                return true;

        if( no_throw )
            return false;

        throw std::xruntime_error("Can't get default gateway", __FILE__, __LINE__);
    }
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
typedef enum {
    SocketDomainInvalid = -1,
    SocketDomainUNSPEC  = AF_UNSPEC,
    SocketDomainPACKET  = AF_PACKET,
    SocketDomainINET    = AF_INET,
    SocketDomainINET6   = AF_INET6
} SocketDomain;
//------------------------------------------------------------------------------
typedef enum {
    SocketTypeInvalid   = -1,
    SocketTypeTCP       = SOCK_STREAM,
    SocketTypeUDP       = SOCK_DGRAM,
    SocketTypeRAW       = SOCK_RAW
} SocketType;
//------------------------------------------------------------------------------
typedef enum {
    SocketProtoInvalid = -1,
    SocketProtoIP      = IPPROTO_IP,
    SocketProtoIPV4    = IPPROTO_IPV4,
    SocketProtoIPV6    = IPPROTO_IPV6,
    SocketProtoUDP     = IPPROTO_UDP,
    SocketProtoICMP    = IPPROTO_ICMP,
    SocketProtoICMPV6  = IPPROTO_ICMPV6,
    SocketProtoRAW     = IPPROTO_RAW
} SocketProtocol;
//------------------------------------------------------------------------------
typedef enum {
    SocketError = SOCKET_ERROR,// Generic socket error translates to error below.
    SocketSuccess = 0,         // No socket error.
    SocketInvalid,             // Invalid socket handle.
    SocketInvalidAddress,      // Invalid destination address specified.
    SocketInvalidPort,         // Invalid destination port specified.
    SocketConnectionRefused,   // No server is listening at remote address.
    SocketTimedout,            // Timed out while attempting operation.
    SocketEWouldblock,         // Operation would block if socket were blocking.
    SocketNotConnected,        // Currently not connected.
    SocketEPipe,               // SIGPIPE
    SocketEInprogress,         // Socket is non-blocking and the connection cannot be completed immediately
    SocketInterrupted,         // Call was interrupted by a signal that was caught before a valid connection arrived.
    SocketConnectionAborted,   // The connection has been aborted.
    SocketProtocolError,       // Invalid protocol for operation.
    SocketFirewallError,       // Firewall rules forbid connection.
    SocketInvalidSocketBuffer, // The receive buffer point outside the process's address space.
    SocketConnectionReset,     // Connection was forcibly closed by the remote host.
    SocketAddressInUse,        // Address already in use.
    SocketInvalidPointer,      // Pointer type supplied as argument is invalid.
    SocketAddrNotAvail,
    SocketOpNotSupp,
    SocketEUnknown             // Unknown error
} SocketErrors;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class base_socket {
public:
    ~base_socket() noexcept {
#if _WIN32
        wizard(0);
#endif
    }

    base_socket() noexcept {
#if _WIN32
        wizard(1);
#endif
    }
protected:
private:
#if _WIN32
    static void wizard(int what) noexcept {
        static std::mutex mtx;
        static WSADATA wsa_data;
        static std::atomic_uint wsa_count(0);
        static volatile bool wsa_started = false;

        if( what == 0 && --wsa_count == 0 ) {
            std::unique_lock<std::mutex> lock(mtx);
            WSACleanup();
            wsa_started = false;
        }
        else if( what == 1 ) {
            wsa_count++;

            if( !wsa_started ) {
                std::unique_lock<std::mutex> lock(mtx);

                if( !wsa_started ) {
                    memset(&wsa_data, 0, sizeof(wsa_data));

                    if( WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0 ) {
                        abort();
                        //wsa_count--;
                        //throw_translate_socket_error();
                    }

                    wsa_started = true;
                }
            }
        }
    }
#endif
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::socket_addr & from_string(homeostas::socket_addr & a, const std::basic_string<_Elem, _Traits, _Alloc> & addr, bool no_throw = false) {
#if _WIN32
    homeostas::base_socket dummy;
#endif
    const _Elem * p_node = addr.c_str(), * p_service = nullptr;
    std::string node, service;

    auto p = addr.rfind(_Elem(':'));

    if( p != std::string::npos ) {
        node = addr.substr(0, p);
        p_node = node.c_str();

        service = addr.substr(p + 1);

        if( !service.empty() )
            p_service = service.c_str();
    }

    addrinfo hints, * result = nullptr;

    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_family     = a.storage.ss_family;
    hints.ai_socktype   = 0;
    hints.ai_flags      = (AI_ALL | AI_ADDRCONFIG)
#if AI_MASK
        & AI_MASK
#endif
    ;
    hints.ai_protocol   = 0;        // Any protocol
    hints.ai_canonname  = nullptr;
    hints.ai_addr       = nullptr;
    hints.ai_next       = nullptr;

    auto r = getaddrinfo(p_node, p_service, &hints, &result);

    if( r != 0 ) {
        if( no_throw )
            return a;

        std::string e =
#if _WIN32
#   if QT_CORE_LIB
            QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
            gai_strerrorA(r)
#   endif
#else
            gai_strerror(r)
#endif
        ;

        e = addr + ", " + e;

        throw std::xruntime_error(e, __FILE__, __LINE__);
    }

    if( result != nullptr )
        memcpy(&a.storage, result->ai_addr, result->ai_addrlen);

    return a;
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
auto from_string(const std::basic_string<_Elem, _Traits, _Alloc> & addr, bool no_throw = false) {
    homeostas::socket_addr a;
    from_string(a, addr, no_throw);
    return a;
}
//------------------------------------------------------------------------------
inline std::string to_string(const homeostas::socket_addr & a, bool no_throw = false) {
#if _WIN32
    homeostas::base_socket dummy;
#endif
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    auto r = getnameinfo((const sockaddr *) &a.storage, sizeof(a.storage), hbuf, sizeof(hbuf), sbuf,
            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

    if( r != 0 ) {
        std::string e =
#if _WIN32
#   if QT_CORE_LIB
            QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
            gai_strerrorA(r)
#   endif
#else
            gai_strerror(r)
#endif
        ;

        if( no_throw )
            return e;

        throw std::xruntime_error(e, __FILE__, __LINE__);
    }

    if( a.port() != 0 && homeostas::slen(sbuf) != 0 )
        return std::string(hbuf) + ":" + std::string(sbuf);

    return std::string(hbuf);
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits> inline
basic_ostream<_Elem, _Traits> & operator << (
    basic_ostream<_Elem, _Traits> & ss,
    const homeostas::socket_addr & addr)
{
    return ss << to_string(addr);
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
socket_addr::socket_addr(const std::basic_string<_Elem, _Traits, _Alloc> & s)
{
    storage.ss_family = AF_UNSPEC;
    std::from_string(*this, s, true);
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class basic_socket : public base_socket {
public:
    typedef enum {
        ShutdownRD      = SHUT_RD,
        ShutdownWR      = SHUT_WR,
        ShutdownRDWR    = SHUT_RDWR
    } ShutdownMode;

public:
    ~basic_socket() noexcept {
        exceptions_ = false;
        close();
    }

    // move constructor
    basic_socket(basic_socket && o) noexcept {
        *this = std::move(o);
    }

    // Move assignment operator
    basic_socket & operator = (basic_socket && o) noexcept {
        if( this != &o ) {
            std::memmove(this, &o, sizeof(o));
            o.socket_ = INVALID_SOCKET;
        }
        return *this;
    }

    basic_socket() noexcept :
        socket_(INVALID_SOCKET),
        socket_errno_(SocketInvalid),
        socket_domain_(SocketDomainInvalid),
        socket_type_(SocketTypeInvalid),
        socket_proto_(SocketProtoInvalid),
        socket_flags_(0),
        is_multicast_(false),
        exceptions_(true),
        interrupt_(false)
    {
    }

    basic_socket & open(int socket_domain, int socket_type, int socket_protocol) {
        if( valid() )
            close();

        if( (socket_ = ::socket(socket_domain, socket_type, socket_protocol)) == INVALID_SOCKET ) {
            throw_translate_socket_error();
            return *this;
        }

        socket_domain_  = decltype(socket_domain_) (socket_domain);
        socket_type_    = decltype(socket_type_) (socket_type);
        socket_proto_   = decltype(socket_proto_) (socket_protocol);
        socket_errno_   = SocketSuccess;

        return *this;
    }

    basic_socket & open(SocketDomain socket_domain = SocketDomainINET, SocketType socket_type = SocketTypeTCP, SocketProtocol socket_protocol = SocketProtoIP) {
        return open(int(socket_domain), int(socket_type), int(socket_protocol));
    }

    basic_socket & close() {
        //std::unique_lock<std::mutex> lock(mtx_);

        if( valid() ) {
            ::shutdown(socket_, SHUT_RDWR);
            auto r =
#if _WIN32
                ::closesocket
#else
                ::close
#endif
            (socket_);

            if( r == 0 ) {
                socket_ = INVALID_SOCKET;
                socket_errno_ = SocketInvalid;
            }
            else {
                translate_socket_error();
            }
        }

        return *this;
    }

    basic_socket & shutdown_socket() {
        ::shutdown(socket_, SHUT_RDWR);
        return *this;
    }

    basic_socket & shutdown(ShutdownMode how = ShutdownWR) {
        if( ::shutdown(socket_, how) == SocketError )
            throw_translate_socket_error();
        return *this;
    }

    bool select(fd_set * p_read_fds, fd_set * p_write_fds, fd_set * p_except_fds, uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), uint64_t * p_ellapsed = nullptr) {
        uint64_t started = p_ellapsed != nullptr ? clock_gettime_ns() : 0;
#if _WIN32 || _POSIX_C_SOURCE < 200112L
        timeval * p_timeout = nullptr, timeout;
#else
        timespec * p_timeout = nullptr, timeout;
#endif
        //---------------------------------------------------------------------
        // If timeout has been specified then set value, otherwise set timeout
        // to NULL which will block until a descriptor is ready for read/write
        // or an error has occurred.
        //---------------------------------------------------------------------
        if( timeout_ns != ~uint64_t(0) ) {
            timeout.tv_sec = decltype(timeout.tv_sec) (timeout_ns / 1000000000u);
#if _WIN32 || _POSIX_C_SOURCE < 200112L
            timeout.tv_usec = decltype(timeout.tv_usec) ((timeout_ns % 1000000000u) / 1000u);

            if( timeout.tv_sec == 0 && timeout.tv_usec == 0 && timeout_ns != 0 )
                timeout.tv_usec = 1;
#else
            timeout.tv_nsec = decltype(timeout.tv_nsec) (timeout_ns % 1000000000u);
#endif
            p_timeout = &timeout;
        }

        bool r = false;
#if _WIN32 || _POSIX_C_SOURCE < 200112L
        int n = ::select(int(socket_ + 1), p_read_fds, p_write_fds, p_except_fds, p_timeout);
#else
        int n = ::pselect(int(socket_ + 1), p_read_fds, p_write_fds, p_except_fds, p_timeout, nullptr);
#endif

        if( n == -1 ) {
            throw_translate_socket_error();
        }
        else if( n == 0 ) {
            socket_errno_ = SocketTimedout;
        }
        //----------------------------------------------------------------------
        // If a file descriptor (read/write) is set then check the
        // socket error (SO_ERROR) to see if there is a pending error.
        //----------------------------------------------------------------------
        else if( (p_read_fds   != nullptr && FD_ISSET(socket_, p_read_fds)) ||
                 (p_write_fds  != nullptr && FD_ISSET(socket_, p_write_fds)) ||
                 (p_except_fds != nullptr && FD_ISSET(socket_, p_except_fds)) ) {
            int err = 0;
            socklen_t l = sizeof(err);

            if( getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char *) &err, &l) != 0 ) {
                translate_socket_error(err);
                throw_socket_error();
            }
            else if( err == 0 ) {
                socket_errno_ = SocketSuccess;
                r = true;
            }
        }

        if ( p_ellapsed != nullptr )
            *p_ellapsed = clock_gettime_ns() - started;

        return r;
    }

#if __linux__
    bool poll(uint64_t timeout_ns, uint64_t * p_ellapsed, decltype(pollfd::events) pool_event) {
        uint64_t started = p_ellapsed != nullptr ? clock_gettime_ns() : 0;
        pollfd pfd = { socket_, pool_event, 0 };
#   if __ANDROID__
        int ms = timeout_ns == std::numeric_limits<uint64_t>::max() ?
            std::numeric_limits<int>::min() : int(timeout_ns / (1000u * 1000u));

        if( ms == 0 && timeout_ns != 0 )
            ms = 1;

        int r = ::poll(&pfd, 1, ms);
#   else
        timespec * p_timeout = nullptr, timeout;

        if( timeout_ns != ~uint64_t(0) ) {
            timeout.tv_sec = decltype(timeout.tv_sec) (timeout_ns / 1000000000u);
            timeout.tv_nsec = decltype(timeout.tv_nsec) (timeout_ns % 1000000000u);
            p_timeout = &timeout;
        }

        int r = ppoll(&pfd, 1, p_timeout, nullptr);
#   endif

        switch( r ) {
            case  0 : // timeout
                break;
            default :
                if( (pfd.revents & POLLNVAL) == 0 )
                    return true;
                errno = EBADF;
                // FALLTHROUGH
            case -1 : // error
                throw_translate_socket_error();
                break;
        }

        if ( p_ellapsed != nullptr )
            *p_ellapsed = clock_gettime_ns() - started;

        return false;
    }
#endif

    bool select_rd(uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), uint64_t * p_ellapsed = nullptr) {
#if __linux__
        return poll(timeout_ns, p_ellapsed, POLLIN);
#else
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(socket_, &fds);

        return select(&fds, nullptr, nullptr, timeout_ns, p_ellapsed);
#endif
    }

    bool select_wr(uint64_t timeout_ns = std::numeric_limits<uint64_t>::max(), uint64_t * p_ellapsed = nullptr) {
#if __linux__
        return poll(timeout_ns, p_ellapsed, POLLOUT);
#else
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(socket_, &fds);

        return select(nullptr, &fds, nullptr, timeout_ns, p_ellapsed);
#endif
    }

    bool select(uint64_t timeout_ns = ~uint64_t(0), uint64_t * p_ellapsed = nullptr) {
        fd_set error_fds, read_fds, write_fds;

        FD_ZERO(&error_fds);
        FD_SET(socket_, &error_fds);
        FD_ZERO(&read_fds);
        FD_SET(socket_, &read_fds);
        FD_ZERO(&write_fds);
        FD_SET(socket_, &write_fds);

        return select(&error_fds, &read_fds, &write_fds, timeout_ns, p_ellapsed);
    }

    bool valid() const {
        return socket_ != INVALID_SOCKET;
    }

    bool invalid() const {
        return socket_ == INVALID_SOCKET;
    }

    size_t recv(void * buffer, size_t size = ~size_t(0), size_t max_recv = RX_MAX_BYTES) {
        size_t bytes_received = 0;
        socklen_t as = 0;
        sockaddr * sa = nullptr;

        if( socket_type_ == SocketTypeUDP ) {
            as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : sizeof(peer_addr_);
            sa = (sockaddr *) &peer_addr_;
        }
        // if size == 0 recv one chunk of any size

        for(;;) {
            int nb = int(std::min(size, std::min(size_t(std::numeric_limits<int>::max()), max_recv)));

            if( nb == 0 )
                nb = int(std::min(size_t(std::numeric_limits<int>::max()), max_recv));

            intptr_t r = 0;

            for(;;) {
                r = ::recvfrom(socket_, (char *) buffer, nb, socket_flags_, sa, &as);

                if( r != SocketError )
                    break;

                if( translate_socket_error() != SocketInterrupted ) {
                    throw_socket_error();
                    break;
                }
            }

            if( r == SocketError || r == 0 )
                break;

            bytes_received += r;

            if( size == 0 )
                break;

            size -= r;

            if( size == 0 )
                break;

            buffer = reinterpret_cast<uint8_t *>(buffer) + r;
        }

        socket_errno_ = SocketSuccess;

        return bytes_received;
    }

    template <typename T,
        typename std::enable_if<std::is_array<T>::value>::type * = nullptr
    >
    T recv(size_t size = 0) {
        T buffer;
        recv(buffer.data(), size, buffer.size() * sizeof(*buffer.data()));
        return buffer; // RVO ?
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value ||
            std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value
        >::type * = nullptr
    >
    T recv(size_t size = ~size_t(0), size_t max_recv = RX_MAX_BYTES) {
        T buffer, buf;
        buf.resize(max_recv / sizeof(T) + (max_recv % sizeof(T) ? 1 : 0));
        size_t buf_size = buf.size() * sizeof(T);

        for(;;) {
            auto r = recv((void *) buf.data(), 0, buf_size);

            if( socket_errno_ != SocketSuccess || r == 0 )
                break;

            auto remainder = r % sizeof(T);

            if( remainder != 0 ) {
                auto rr = recv((uint8_t *) buf.data() + r, remainder, remainder);

                if( socket_errno_ != SocketSuccess || rr == 0 )
                    break;

                r += rr;
            }

            buffer.insert(std::end(buffer), std::begin(buf), std::end(buf));

            if( size == 0 )
                break;

            size -= r;
        }

        return buffer; // RVO ?
    }

    size_t send(const void * buf, size_t size, size_t max_send = TX_MAX_BYTES) {
        size_t bytes_sent = 0;
        socklen_t as = 0;
        const sockaddr * sa = nullptr;
        int flags = socket_flags_;

        if( socket_type_ == SocketTypeUDP ) {
            as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : sizeof(remote_addr_);
            sa = (const sockaddr *) (is_multicast_ ? &multicast_group_ : &remote_addr_.saddr4);
        }
        else {
#ifdef MSG_NOSIGNAL
            flags |= MSG_NOSIGNAL;
#endif
        }

        while( size > 0 ) {
            int nb = int(std::min(size, max_send));
            intptr_t w = 0;

            for(;;) {
                w = ::sendto(socket_, (const char *) buf, nb, flags, sa, as);

                if( w != SocketError )
                    break;

                if( translate_socket_error() != SocketInterrupted ) {
                    throw_socket_error();
                    break;
                }
            }

            if( w == SocketError || w == 0 )
                break;

            size -= w;
            bytes_sent += w;
        }

        socket_errno_ = SocketSuccess;

        return bytes_sent;
	}

    size_t send(const char * s, size_t max_send = TX_MAX_BYTES) {
        return send(s, slen(s), max_send);
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value
        >::type * = nullptr
    >
    size_t send(const T & s, size_t max_send = TX_MAX_BYTES) {
        return send((const void *) s.c_str(), s.size() * sizeof(T::value_type), max_send);
    }

    size_t send(const struct iovec * vec, size_t n) {
        socket_errno_ = SocketSuccess;
        size_t bytes_sent = 0;
#if _WIN32
        for( size_t i = 0; i < n; i++ ) {
            auto n = send((uint8_t *) vec[i].iov_base, vec[i].iov_len);

            if( socket_errno_ != SocketSuccess )
                break;

            bytes_sent += n;
        }

#else
        auto w = ::writev(socket_, vec, n);

        if( w == SocketError)
            translate_socket_error();

        bytes_sent = w;
#endif
        return bytes_sent;
    }

    basic_socket & option_linger(bool enable, uint16_t timeout) {
        linger l;
        l.l_onoff   = enable ? 1 : 0;
        l.l_linger  = timeout;

        if( setsockopt(socket_, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof(l)) != 0 )
            throw_translate_socket_error();

        return *this;
    }

    basic_socket option_reuse_addr() {
        int32_t nReuse = IPTOS_LOWDELAY;

        if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char *) &nReuse, sizeof(int32_t)) != 0 )
            throw_translate_socket_error();

        return *this;
    }

    basic_socket & multicast(uint8_t multicast_ttl) {
        if( socket_type_ == SocketTypeUDP ) {
            if( setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) &multicast_ttl, sizeof(multicast_ttl)) != 0 ) {
                throw_translate_socket_error();
			}
            else {
                is_multicast_ = true;
            }
		}

        return *this;
	}

    bool multicast() const {
        return is_multicast_;
    }

    bool bind_interface(const char * pInterface) {
		bool           bRetVal = false;
		struct in_addr stInterfaceAddr;

        if( is_multicast_ ) {
            if( pInterface != nullptr )	{
                stInterfaceAddr.s_addr = ::inet_addr(pInterface);

                if( setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_IF, (const char *) &stInterfaceAddr, sizeof(stInterfaceAddr)) == SocketSuccess )
					bRetVal = true;
			}
		}
        else {
            socket_errno_ = SocketProtocolError;
		}

		return bRetVal;
	}

    bool receive_timeout(int32_t nRecvTimeoutSec, int32_t nRecvTimeoutUsec) {
        bool bRetVal = true;

        struct timeval m_stRecvTimeout;
        memset(&m_stRecvTimeout, 0, sizeof(struct timeval));

        m_stRecvTimeout.tv_sec = nRecvTimeoutSec;
        m_stRecvTimeout.tv_usec = nRecvTimeoutUsec;

        //--------------------------------------------------------------------------
        // Sanity check to make sure the options are supported!
        //--------------------------------------------------------------------------
        if( setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &m_stRecvTimeout,
                       sizeof(timeval)) == SocketError )
        {
            bRetVal = false;
            translate_socket_error();
        }

        return bRetVal;
    }

    bool send_timeout(int32_t nSendTimeoutSec, int32_t nSendTimeoutUsec) {
        bool bRetVal = true;

        struct timeval m_stSendTimeout;
        memset(&m_stSendTimeout, 0, sizeof(struct timeval));
        m_stSendTimeout.tv_sec = nSendTimeoutSec;
        m_stSendTimeout.tv_usec = nSendTimeoutUsec;

        //--------------------------------------------------------------------------
        // Sanity check to make sure the options are supported!
        //--------------------------------------------------------------------------
        if( setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, (const char *) &m_stSendTimeout,
                       sizeof(timeval)) == SocketError )
        {
            bRetVal = false;
            translate_socket_error();
        }

        return bRetVal;
    }

    SocketErrors socket_error() {
        return socket_errno_;
    }

    int32_t socket_dscp() {
        int32_t    nTempVal = 0;
		socklen_t  nLen = 0;

        if( valid() ) {
            if( getsockopt(socket_, IPPROTO_IP, IP_TOS, (char *) &nTempVal, &nLen) == SocketError )
                translate_socket_error();

			nTempVal *= 4;
			nTempVal >>= 4;
		}

		return nTempVal;
	}

    bool socket_dscp(int32_t nDscp)	{
		bool  bRetVal = true;
		int32_t nTempVal = nDscp;

		nTempVal <<= 4;
		nTempVal /= 4;

        if( valid() )
            if( setsockopt(socket_, IPPROTO_IP, IP_TOS, (const char *) &nTempVal, sizeof(nTempVal)) == SocketError ) {
                translate_socket_error();
				bRetVal = false;
			}

		return bRetVal;
	}

    const auto & socket() const {
        return socket_;
    }

    const auto & socket_type() const {
        return socket_type_;
    }

    const auto & local_addr() const {
        return local_addr_;
    }

    auto & local_addr(const socket_addr & addr) {
        local_addr_ = addr;
        return *this;
    }

    const auto & remote_addr() const {
        return remote_addr_;
    }

    auto & remote_addr(const socket_addr & addr) {
        remote_addr_ = addr;
        return *this;
    }

    const auto & peer_addr() const {
        return peer_addr_;
    }

    uint32_t receive_window_size() {
        return window_size(SO_RCVBUF);
    }

    uint32_t send_window_size() {
        return window_size(SO_SNDBUF);
    }

    uint32_t receive_window_size(uint32_t nWindowSize) {
        return window_size(SO_RCVBUF, nWindowSize);
    }

    uint32_t send_window_size(uint32_t nWindowSize) {
        return window_size(SO_SNDBUF, nWindowSize);
    }

    bool disable_nagle_algoritm()	{
		bool  bRetVal = false;
		int32_t nTcpNoDelay = 1;

		//----------------------------------------------------------------------
		// Set TCP NoDelay flag to true
		//----------------------------------------------------------------------
        if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char *) &nTcpNoDelay, sizeof(int32_t)) == 0)
		{
			bRetVal = true;
		}

        translate_socket_error();

		return bRetVal;
	}


    bool enable_nagle_algoritm() {
		bool  bRetVal = false;
		int32_t nTcpNoDelay = 0;

		//----------------------------------------------------------------------
		// Set TCP NoDelay flag to false
		//----------------------------------------------------------------------
        if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char *) &nTcpNoDelay, sizeof(int32_t)) == 0)
		{
			bRetVal = true;
		}

        translate_socket_error();

		return bRetVal;
	}

    const auto & exceptions() const {
        return exceptions_;
    }

    basic_socket & exceptions(bool exceptions) {
        exceptions_ = exceptions;
        return *this;
    }

    const auto & domain() const {
        return socket_domain_;
    }

    basic_socket & type(SocketDomain domain) {
        socket_domain_ = domain;
        return *this;
    }

    const auto & type() const {
        return socket_type_;
    }

    basic_socket & type(SocketType type) {
        socket_type_ = type;
        return *this;
    }

    const auto & proto() const {
        return socket_proto_;
    }

    basic_socket & proto(SocketProtocol proto) {
        socket_proto_ = proto;
        return *this;
    }

    const auto & flags() const {
        return socket_flags_;
    }

    basic_socket & flags(int flags) {
        socket_flags_ = flags;
        return *this;
    }

    const auto & socket_errno() const {
        return socket_errno_;
    }

    basic_socket & socket_errno(SocketErrors err) {
        socket_errno_ = err;
        return *this;
    }

    bool success() const {
        return socket_errno_ == SocketSuccess;
    }

    bool fail() const {
        return socket_errno_ != SocketSuccess;
    }

    operator bool () const {
        return socket_errno_ == SocketSuccess;
    }

    const char * str_error() const {
        return str_error(socket_errno_);
    }

    void throw_socket_error() {
        if( exceptions_ )
            throw std::xruntime_error(str_error(socket_errno_), __FILE__, __LINE__);
    }

    void throw_socket_error(const std::string & msg) {
        if( exceptions_ )
            throw std::xruntime_error(msg, __FILE__, __LINE__);
    }

    void throw_translate_socket_error() {
        translate_socket_error();
        throw_socket_error();
    }

    basic_socket & interrupt(bool interrupt) {
        interrupt_ = interrupt;
        return *this;
    }

    auto interrupt() const {
        return interrupt_;
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, socket_addr>::value
        >::type * = nullptr
    >
    static T wildcards() {
        T container;
#if _WIN32
        base_socket dummy;
#endif
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
        hints.ai_socktype   = 0;
        hints.ai_flags      = AI_PASSIVE;   // For wildcard IP address
        hints.ai_protocol   = 0;            // Any protocol
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        auto r = getaddrinfo(nullptr, "0", &hints, &result);

        if( r != 0 ) {
            auto gai_err =
#if _WIN32
#   if QT_CORE_LIB
            QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
            gai_strerrorA(r)
#   endif
#else
            gai_strerror(r)
#endif
            ;
            throw std::xruntime_error(gai_err, __FILE__, __LINE__);
        }
        else {
            at_scope_exit(
                freeaddrinfo(result);
            );

            for( rp = result; rp != nullptr; rp = rp->ai_next )
                container.emplace_back(socket_addr(*rp));
        }

        std::sort(std::begin(container), std::end(container));
        auto last = std::unique(std::begin(container), std::end(container));
        container.erase(last, std::end(container));

        return container;
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, std::basic_string<typename T::value_type::value_type, typename T::value_type::traits_type, typename T::value_type::allocator_type>>::value
        >::type * = nullptr
    >
    static T wildcards() {
        T container;

        for( const auto & addr : wildcards<std::vector<socket_addr>>() )
            container.push_back(std::to_string(addr));

        return container;
    }

    static auto wildcards() {
        return wildcards<std::vector<socket_addr>>();
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, socket_addr>::value
        >::type * = nullptr
    >
    static T interfaces() {
        T container;
#if _WIN32
        base_socket dummy;
#endif
        char node[8192], domain[4096];

        if( gethostname(node, sizeof(domain)) == 0
            && getdomainname(domain, sizeof(domain)) == 0 ) {

            if( slen(domain) != 0 && strcmp(domain, "(none)") != 0 )
                scat(scat(node, "."), domain);

            addrinfo hints, * result = nullptr, * rp = nullptr;

            memset(&hints, 0, sizeof(addrinfo));
            hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
            hints.ai_socktype   = 0;
            hints.ai_flags      = (AI_ALL | AI_ADDRCONFIG)
#if AI_MASK
                & AI_MASK
#endif
            ;
            hints.ai_protocol   = 0;            // Any protocol
            hints.ai_canonname  = nullptr;
            hints.ai_addr       = nullptr;
            hints.ai_next       = nullptr;

            auto r = getaddrinfo(node, "0", &hints, &result);

            if( r != 0 && r != EAI_NODATA && r != EAI_NONAME ) {
                auto gai_err =
#if _WIN32
#   if QT_CORE_LIB
                QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
                gai_strerrorA(r)
#   endif
#else
                gai_strerror(r)
#endif
                ;
                throw std::xruntime_error(gai_err, __FILE__, __LINE__);
            }

            at_scope_exit( freeaddrinfo(result) );

            for( rp = result; rp != nullptr; rp = rp->ai_next )
                container.push_back(socket_addr(*rp));
#if QT_CORE_LIB
            if( container.empty() || (container.size() == 1 && container[0].is_loopback()) ) {
                socket_addr a;

                for( const auto & iface : QNetworkInterface::allInterfaces() )
                    for( const auto & addr : iface.addressEntries() ) {
                        const auto & ip = addr.ip();
                        if( ip.protocol() == QAbstractSocket::IPv4Protocol ) {
                            a.family(AF_INET);
                            a.saddr4.sin_addr.s_addr = htonl(ip.toIPv4Address());
                        }
                        else if( ip.protocol() == QAbstractSocket::IPv6Protocol ) {
                            a.family(AF_INET6);
                            Q_IPV6ADDR a6 = ip.toIPv6Address();
                            memcpy(&a.saddr6.sin6_addr, &a6, std::min(sizeof(a.saddr6.sin6_addr), sizeof(Q_IPV6ADDR)));
                        }

                        if( a.family() != AF_UNSPEC && !a.is_loopback() )
                            container.push_back(a);
                    }
            }
#endif
        }

        std::sort(std::begin(container), std::end(container));
        auto last = std::unique(std::begin(container), std::end(container));
        container.erase(last, std::end(container));

        return container;
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, std::basic_string<typename T::value_type::value_type, typename T::value_type::traits_type, typename T::value_type::allocator_type>>::value
        >::type * = nullptr
    >
    static T interfaces() {
        T container;

        for( const auto & addr : interfaces<std::vector<socket_addr>>() )
            container.push_back(std::to_string(addr));

        return container;
    }

    static auto interfaces() {
        return interfaces<std::vector<socket_addr>>();
    }
protected:
    uint32_t window_size(uint32_t nOptionName) {
		uint32_t nTcpWinSize = 0;

		//-------------------------------------------------------------------------
		// no socket given, return system default allocate our own new socket
		//-------------------------------------------------------------------------
        if( socket_ != INVALID_SOCKET ) {
			socklen_t nLen = sizeof(nTcpWinSize);

			//---------------------------------------------------------------------
			// query for buffer size
			//---------------------------------------------------------------------
            getsockopt(socket_, SOL_SOCKET, nOptionName, (char *) &nTcpWinSize, &nLen);
            translate_socket_error();
		}
        else {
            socket_errno_ = SocketInvalid;
		}

		return nTcpWinSize;
	}

    uint32_t window_size(uint32_t nOptionName, uint32_t nWindowSize) {
		//-------------------------------------------------------------------------
		// no socket given, return system default allocate our own new socket
		//-------------------------------------------------------------------------
        if( socket_ != INVALID_SOCKET ) {
            if( setsockopt(socket_, SOL_SOCKET, nOptionName, (const char *) &nWindowSize, sizeof(nWindowSize)) == SocketError )
                translate_socket_error();
		}
        else {
            socket_errno_ = SocketInvalid;
		}

		return nWindowSize;
	}

    bool flush() {
        int32_t nTcpNoDelay = 1;
        int32_t nCurFlags = 0;
        uint8_t tmpbuf = 0;
        bool  bRetVal = false;

        //--------------------------------------------------------------------------
        // Get the current setting of the TCP_NODELAY flag.
        //--------------------------------------------------------------------------
        socklen_t sz = sizeof(nCurFlags);

        if( getsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char *) &nCurFlags, &sz) == 0 ) {
            //----------------------------------------------------------------------
            // Set TCP NoDelay flag
            //----------------------------------------------------------------------
            if( setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char *) &nTcpNoDelay, sz) == 0 ) {
                //------------------------------------------------------------------
                // Send empty byte stream to flush the TCP send buffer
                //------------------------------------------------------------------
                send(&tmpbuf, 0);

                if( socket_errno_ != SocketError )
                    bRetVal = true;
            }

            //----------------------------------------------------------------------
            // Reset the TCP_NODELAY flag to original state.
            //----------------------------------------------------------------------
            setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char *) &nCurFlags, sz);
        }

        return bRetVal;
    }

    static const char * str_error(SocketErrors err) {
        switch( err ) {
            case SocketError:
                return "Generic socket error translates to error below.";
            case SocketSuccess:
                return "No socket error.";
            case SocketInvalid:
                return "Invalid socket handle.";
            case SocketInvalidAddress:
                return "Invalid destination address specified.";
            case SocketInvalidPort:
                return "Invalid destination port specified.";
            case SocketConnectionRefused:
                return "No server is listening at remote address.";
            case SocketTimedout:
                return "Timed out while attempting operation.";
            case SocketEWouldblock:
                return "Operation would block if socket were blocking.";
            case SocketNotConnected:
                return "Currently not connected.";
            case SocketEInprogress:
                return "Socket is non-blocking and the connection cannot be completed immediately";
            case SocketInterrupted:
                return "Call was interrupted by a signal that was caught before a valid connection arrived.";
            case SocketConnectionAborted:
                return "The connection has been aborted.";
            case SocketProtocolError:
                return "Invalid protocol for operation.";
            case SocketFirewallError:
                return "Firewall rules forbid connection.";
            case SocketInvalidSocketBuffer:
                return "The receive buffer point outside the process's address space.";
            case SocketConnectionReset:
                return "Connection was forcibly closed by the remote host.";
            case SocketAddressInUse:
                return "Address already in use.";
            case SocketInvalidPointer:
                return "Pointer type supplied as argument is invalid.";
            case SocketAddrNotAvail:
                return "Cannot assign requested address. The requested address is not valid in its context.";
            case SocketEUnknown:
                return "Unknown error";
            case SocketOpNotSupp:
                return "The attempted operation is not supported for the type of object referenced.";
            default:
                return "No such CSimpleSocket error";
        }
    }

    SocketErrors translate_socket_error(int err) {
        switch( err ) {
#if _WIN32
            case EXIT_SUCCESS:
                socket_errno_ = SocketSuccess;
                break;
            case WSAEBADF:
            case WSAENOTCONN:
                socket_errno_ = SocketNotConnected;
                break;
            case WSAEINTR:
                socket_errno_ = SocketInterrupted;
                break;
            case WSAEACCES:
            case WSAEAFNOSUPPORT:
            case WSAEINVAL:
            case WSAEMFILE:
            case WSAENOBUFS:
            case WSAEPROTONOSUPPORT:
                socket_errno_ = SocketInvalid;
                break;
            case WSAECONNREFUSED :
                socket_errno_ = SocketConnectionRefused;
                break;
            case WSAETIMEDOUT:
                socket_errno_ = SocketTimedout;
                break;
            case WSAEINPROGRESS:
                socket_errno_ = SocketEInprogress;
                break;
            case WSAECONNABORTED:
                socket_errno_ = SocketConnectionAborted;
                break;
            case WSAEWOULDBLOCK:
                socket_errno_ = SocketEWouldblock;
                break;
            case WSAENOTSOCK:
                socket_errno_ = SocketInvalid;
                break;
            case WSAECONNRESET:
                socket_errno_ = SocketConnectionReset;
                break;
            case WSANO_DATA:
                socket_errno_ = SocketInvalidAddress;
                break;
            case WSAEADDRINUSE:
                socket_errno_ = SocketAddressInUse;
                break;
            case WSAEFAULT:
                socket_errno_ = SocketInvalidPointer;
                break;
            case WSAEADDRNOTAVAIL:
                socket_errno_ = SocketAddrNotAvail;
                break;
            case WSAEOPNOTSUPP:
                socket_errno_ = SocketOpNotSupp;
                break;
            default:
                socket_errno_ = SocketEUnknown;
                break;
#else
            case EXIT_SUCCESS:
                socket_errno_ = SocketSuccess;
                break;
            case ENOTCONN:
                socket_errno_ = SocketNotConnected;
                break;
            case ENOTSOCK:
            case EBADF:
            case EACCES:
            case EAFNOSUPPORT:
            case EMFILE:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case EPROTONOSUPPORT:
                socket_errno_ = SocketInvalid;
                break;
            case EPIPE:
                socket_errno_ = SocketEPipe;
                break;
            case ECONNREFUSED :
                socket_errno_ = SocketConnectionRefused;
                break;
            case ETIMEDOUT:
                socket_errno_ = SocketTimedout;
                break;
            case EINPROGRESS:
                socket_errno_ = SocketEInprogress;
                break;
            case EWOULDBLOCK:
                //        case EAGAIN:
                socket_errno_ = SocketEWouldblock;
                break;
            case EINTR:
                socket_errno_ = SocketInterrupted;
                break;
            case ECONNABORTED:
                socket_errno_ = SocketConnectionAborted;
                break;
            case EINVAL:
            case EPROTO:
                socket_errno_ = SocketProtocolError;
                break;
            case EPERM:
                socket_errno_ = SocketFirewallError;
                break;
            case EFAULT:
                socket_errno_ = SocketInvalidSocketBuffer;
                break;
            case ECONNRESET:
            case ENOPROTOOPT:
                socket_errno_ = SocketConnectionReset;
                break;
            default:
                socket_errno_ = SocketEUnknown;
                break;
#endif
        }
        return socket_errno_;
    }

    SocketErrors translate_socket_error() {
        auto err =
#if _WIN32
        WSAGetLastError()
#else
        errno
#endif
        ;
        return translate_socket_error(err);
    }

    template <typename T, typename F>
    static void move(T & t, T & o, F) {
        std::memcpy(&t, &o, sizeof(o));
        o.socket_ = INVALID_SOCKET;
    }

    template <typename T, typename F,
        typename std::enable_if<std::is_function<F>::value>::type * = nullptr
    >
    static T & move(T & t, T & o, F f) {
        if( &t != &o ) {
            move(t, o, nullptr);
            f();
        }
        return t;
    }

    template <typename T>
    static T & move(T & t, T & o) {
        if( &t != &o )
            move(t, o, nullptr);
        return t;
    }

    SOCKET                      socket_;            // socket handle
    SocketErrors                socket_errno_;      // number of last error
    SocketDomain                socket_domain_;     // socket type AF_INET, AF_INET6
    SocketType                  socket_type_;       // socket type - UDP, TCP or RAW
    SocketProtocol              socket_proto_;
    int                         socket_flags_;      // socket flags
    bool                        is_multicast_;      // is the UDP socket multicast;
    bool                        exceptions_;
    bool                        interrupt_;
    socket_addr                 remote_addr_;
    socket_addr                 local_addr_;
    socket_addr                 peer_addr_;
    sockaddr_in                 multicast_group_;
private:
    basic_socket(const basic_socket & basic_socket);
    void operator = (const basic_socket &);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class passive_socket;
//------------------------------------------------------------------------------
class active_socket : public basic_socket {
public:
    friend class passive_socket;

    // move constructor
    active_socket(active_socket && o) noexcept {
        *this = std::move(o);
    }

    // Move assignment operator
    active_socket & operator = (active_socket && o) noexcept {
        return move(*this, o);
    }

    active_socket(SocketType socket_type = SocketTypeTCP) {
        socket_domain_ = SocketDomainUNSPEC;
        socket_type_   = socket_type;
        socket_proto_  = SocketProtoIP;
    }

    active_socket & connect(const std::string & node, const std::string & service) {
        return connect(node.empty() ? nullptr : node.c_str(), service.empty() ? nullptr : service.c_str());
    }

    active_socket & connect(const std::string & node, const char * service) {
        return connect(node.empty() ? nullptr : node.c_str(), service);
    }

    active_socket & connect(const char * node, const std::string & service) {
        return connect(node, service.empty() ? nullptr : service.c_str());
    }

    active_socket & bind(const socket_addr & addr) {
#ifndef _WIN32
        //--------------------------------------------------------------------------
        // Set the following socket option SO_REUSEADDR.  This will allow the file
        // descriptor to be reused immediately after the socket is closed instead
        // of setting in a TIMED_WAIT state.
        //--------------------------------------------------------------------------
        int32_t reuse = 1;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
        setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(reuse));
#endif
        //int32_t low = IPTOS_LOWDELAY;
        //setsockopt(socket_, IPPROTO_TCP, IP_TOS, (const char *) &low, sizeof(low));
#endif

        if( ::bind(socket_, (const sockaddr *) addr.sock_data(), socklen_t(addr.size())) == SocketError ) {
            throw_translate_socket_error();
            return *this;
        }

        socket_errno_ = SocketSuccess;

        return *this;
    }

    active_socket & connect(const socket_addr & addr, const socket_addr * p_bind_addr = nullptr) {
        open(addr.family(), socket_type_, socket_proto_);

        if( invalid() )
            return *this;

        if( p_bind_addr != nullptr ) {
            bind(*p_bind_addr);

            if( socket_errno_ != SocketSuccess )
                return *this;
        }

        if( ::connect(socket_, (const sockaddr *) addr.data(), addr.size()) == SocketError ) {
            throw_translate_socket_error();
            return *this;
        }

        socklen_t as = sizeof(local_addr_);
        getpeername(socket_, (sockaddr *) &remote_addr_, &as);
        getsockname(socket_, (sockaddr *) &local_addr_, &as);

        interrupt_ = false;
        socket_errno_ = SocketSuccess;

        return *this;
    }

    active_socket & connect(const char * node, const char * service) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = socket_domain_ == SocketDomainInvalid ? 0 : socket_domain_; // Allow IPv4 or IPv6
        hints.ai_socktype   = socket_type_ == SocketTypeInvalid ? 0 : socket_type_;
        hints.ai_flags      = (AI_ALL | AI_ADDRCONFIG)
#if AI_MASK
            & AI_MASK
#endif
        ;
        hints.ai_protocol   = socket_proto_ == SocketProtoInvalid ? 0 : socket_proto_;
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        auto r = getaddrinfo(node, service, &hints, &result);

        if( r != 0 ) {
            socket_errno_ = SocketEUnknown;
            auto e =
                std::string(str_error()) + ", " +
#if _WIN32
#   if QT_CORE_LIB
                QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
                gai_strerrorA(r)
#   endif
#else
                gai_strerror(r)
#endif
            ;
            throw_socket_error(e);
            return *this;
        }

        if( result == nullptr ) {
            socket_errno_ = SocketEUnknown;
            throw_socket_error();
            return *this;
        }

        auto exceptions_safe = exceptions_;

        at_scope_exit(
            freeaddrinfo(result);
            exceptions_ = exceptions_safe;
        );

        exceptions_ = false;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect(2).
        // If socket(2) (or connect(2)) fails, we (close the socket
        // and) try the next address.

        for( rp = result; rp != nullptr; rp = rp->ai_next ) {
            if( rp->ai_family != AF_INET
                && rp->ai_family != AF_INET6
                && rp->ai_socktype != socket_type_ )
                continue;

            open(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if( valid() )
                if( ::connect(socket_, rp->ai_addr, socklen_t(rp->ai_addrlen)) != SocketError )
                    break;

            if( rp->ai_next == nullptr ) {
                exceptions_ = exceptions_safe;
                throw_translate_socket_error();
                return *this;
            }
        }

        if( rp == nullptr ) {
            socket_errno_ = SocketNotConnected;
            throw_socket_error();
            return *this;
        }

        socklen_t as = sizeof(remote_addr_);
        getpeername(socket_, (sockaddr *) &remote_addr_, &as);
        as = sizeof(local_addr_);
        getsockname(socket_, (sockaddr *) &local_addr_, &as);

        interrupt_ = false;
        socket_errno_ = SocketSuccess;

        return *this;
    }

    static std::shared_ptr<active_socket> shared() {
        return std::make_unique<active_socket>();
    }
protected:
private:
    active_socket(const active_socket & basic_socket);
    void operator = (const active_socket &);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class passive_socket : public basic_socket {
public:
    // move constructor
    passive_socket(passive_socket && o) noexcept {
        *this = std::move(o);
    }

    // Move assignment operator
    passive_socket & operator = (passive_socket && o) noexcept {
        return move(*this, o);
    }

    passive_socket(SocketType socket_type = SocketTypeTCP) {
        socket_domain_ = SocketDomainUNSPEC;
        socket_type_   = socket_type;
        socket_proto_  = SocketProtoIP;
    }

    bool bind_multicast(const char * pInterface, const char * pGroup, uint16_t nPort) {
        bool           bRetVal = false;
#if _WIN32
        ULONG          inAddr;
#else
        in_addr_t      inAddr;
        int32_t        tos = IPTOS_LOWDELAY;

        setsockopt(socket_, IPPROTO_IP, IP_TOS, (const char *) &tos, sizeof(tos));
#endif

        //--------------------------------------------------------------------------
        // Set the following socket option SO_REUSEADDR.  This will allow the file
        // descriptor to be reused immediately after the socket is closed instead
        // of setting in a TIMED_WAIT state.
        //--------------------------------------------------------------------------
        memset(&multicast_group_, 0, sizeof(multicast_group_));
        multicast_group_.sin_family = AF_INET;
        multicast_group_.sin_port = htons(nPort);

        //--------------------------------------------------------------------------
        // If no IP Address (interface ethn) is supplied, or the loop back is
        // specified then bind to any interface, else bind to specified interface.
        //--------------------------------------------------------------------------
        if( pInterface == nullptr || slen(pInterface) == 0 ) {
            multicast_group_.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else if( (inAddr = ::inet_addr(pInterface)) != INADDR_NONE ) {
            multicast_group_.sin_addr.s_addr = inAddr;
        }

        //--------------------------------------------------------------------------
        // Bind to the specified port
        //--------------------------------------------------------------------------
        if( ::bind(socket_, (const sockaddr *) &multicast_group_, sizeof(multicast_group_)) == 0 ) {
            //----------------------------------------------------------------------
            // Join the multicast group
            //----------------------------------------------------------------------
            ip_mreq multicast_request_;   // group address for multicast
            multicast_request_.imr_multiaddr.s_addr = ::inet_addr(pGroup);
            multicast_request_.imr_interface.s_addr = multicast_group_.sin_addr.s_addr;

            if( setsockopt(socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                           (const char *) &multicast_request_,
                           sizeof(multicast_request_)) == SocketSuccess )
                bRetVal = true;
        }

        //--------------------------------------------------------------------------
        // If there was a socket error then close the socket to clean out the
        // connection in the backlog.
        //--------------------------------------------------------------------------
        translate_socket_error();

        if( bRetVal == false )
            close();

        return bRetVal;
    }

    passive_socket & listen(const std::string & node, const std::string & service, int backlog = 0) {
        return listen(node.empty() ? nullptr : node.c_str(), service.empty() ? nullptr : service.c_str(), backlog);
    }

    passive_socket & listen(const char * node, const char * service, int backlog = 0) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = socket_domain_; // Allow IPv4 or IPv6
        hints.ai_socktype   = socket_type_;
        hints.ai_flags      = AI_PASSIVE;     // For wildcard IP address
        hints.ai_protocol   = socket_proto_;
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        auto r = ::getaddrinfo(node, service, &hints, &result);

        if( r != 0 ) {
            socket_errno_ = SocketEUnknown;
            throw_socket_error(std::string(str_error()) + ", " +
#if _WIN32
#   if QT_CORE_LIB
                QString::fromWCharArray(gai_strerrorW(r)).toStdString()
#   else
                gai_strerrorA(r)
#   endif
#else
                gai_strerror(r)
#endif
            );
        }

        auto exceptions_safe = exceptions_;

        at_scope_exit(
            freeaddrinfo(result);
            exceptions_ = exceptions_safe;
        );

        exceptions_ = false;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect(2).
        // If socket(2) (or connect(2)) fails, we (close the socket
        // and) try the next address.

        for( rp = result; rp != nullptr; rp = rp->ai_next ) {
            if( rp->ai_family != AF_INET
                && rp->ai_family != AF_INET6
                && rp->ai_socktype != socket_type_ )
                continue;

            open(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if( valid() ) {
#ifndef _WIN32
                //--------------------------------------------------------------------------
                // Set the following socket option SO_REUSEADDR.  This will allow the file
                // descriptor to be reused immediately after the socket is closed instead
                // of setting in a TIMED_WAIT state.
                //--------------------------------------------------------------------------
                int32_t reuse = 1;
                setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
                setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(reuse));
#endif
                int32_t low = IPTOS_LOWDELAY;
                setsockopt(socket_, IPPROTO_TCP, IP_TOS, (const char *) &low, sizeof(low));
#endif

                if( ::bind(socket_, (const sockaddr *) rp->ai_addr, socklen_t(rp->ai_addrlen)) != SocketError )
                    break;
            }

            if( rp->ai_next == nullptr ) {
                throw_translate_socket_error();
                return *this;
            }
        }

        if( rp == nullptr ) {
            socket_errno_ = SocketNotConnected;
            throw_socket_error();
            return *this;
        }

        if( socket_type_ == SocketTypeTCP )
            if( ::listen(socket_, int(backlog)) == SocketError ) {
                throw_translate_socket_error();
                return *this;
            }

        socklen_t as = sizeof(local_addr_);
        getsockname(socket_, (sockaddr *) &local_addr_, &as);

        interrupt_ = false;
        socket_errno_ = SocketSuccess;

        return *this;
    }

    passive_socket & listen(const socket_addr & addr, int backlog = 0) {
        open(addr.family(), socket_type_, socket_proto_);

        if( invalid() )
            return *this;
#ifndef _WIN32
        //--------------------------------------------------------------------------
        // Set the following socket option SO_REUSEADDR.  This will allow the file
        // descriptor to be reused immediately after the socket is closed instead
        // of setting in a TIMED_WAIT state.
        //--------------------------------------------------------------------------
        int32_t reuse = 1;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
        setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(reuse));
#endif
        int32_t low = IPTOS_LOWDELAY;
        setsockopt(socket_, IPPROTO_TCP, IP_TOS, (const char *) &low, sizeof(low));
#endif

        if( ::bind(socket_, (const sockaddr *) addr.data(), addr.size()) == SocketError ) {
            throw_translate_socket_error();
            return *this;
        }

        if( socket_type_ == SocketTypeTCP )
            if( ::listen(socket_, int(backlog)) == SocketError ) {
                throw_translate_socket_error();
                return *this;
            }

        socklen_t as = sizeof(local_addr_);
        getsockname(socket_, (sockaddr *) &local_addr_, &as);

        interrupt_ = false;
        socket_errno_ = SocketSuccess;

        return *this;
    }

    active_socket * accept() {
        if( socket_type_ != SocketTypeTCP ) {
            socket_errno_ = SocketProtocolError;
            throw_socket_error();
            return nullptr;
        }

        auto exceptions_safe = exceptions_;
        at_scope_exit( exceptions_ = exceptions_safe );
        exceptions_ = false;

        std::unique_ptr<active_socket> client_socket(new active_socket);

        while( !interrupt_ ) {
            select_rd();
            socklen_t as = remote_addr_.size();
            SOCKET socket = ::accept(socket_, (sockaddr *) remote_addr_.data(), &as);

            if( socket != INVALID_SOCKET ) {
                client_socket->socket_          = socket;
                client_socket->socket_errno_    = SocketSuccess;
                client_socket->socket_domain_   = socket_domain_;
                client_socket->socket_type_     = socket_type_;

                as = sizeof(remote_addr_);
                getpeername(socket, (sockaddr *) &client_socket->remote_addr_, &as);
                as = sizeof(local_addr_);
                getsockname(socket, (sockaddr *) &client_socket->local_addr_, &as);
                break;
            }

            if( translate_socket_error() != SocketInterrupted ) {
                if( !interrupt_ && exceptions_safe )
                    throw std::xruntime_error(str_error(), __FILE__, __LINE__);
                return nullptr;
            }
        }

        return client_socket.release();
    }

    std::shared_ptr<active_socket> accept_shared() {
        std::unique_ptr<active_socket> uniq(accept());
        return std::move(uniq);
    }
protected:
private:
    passive_socket(const passive_socket & basic_socket);
    void operator = (const passive_socket &);
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void socket_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // SOCKET_HPP_INCLUDED
//------------------------------------------------------------------------------
