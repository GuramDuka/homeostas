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
#endif

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cerrno>

#if __linux
#include <sys/uio.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#endif

#if __APPLE__
#include <net/if.h>
#endif
//------------------------------------------------------------------------------
#include <atomic>
#include <memory>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
#include "scope_exit.hpp"
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
    typedef int            SOCKET;
    constexpr int SOCKET_ERROR = -1;
#endif

constexpr int SOCKET_ERROR_INTERUPT = EINTR;
constexpr int SOCKET_ERROR_TIMEDOUT = EAGAIN;
constexpr int TX_MAX_BYTES = 32 * 1024;
constexpr int RX_MAX_BYTES = 16 * 1024;
constexpr int MIN_BUFFER_SIZE = 32;

#ifndef INVALID_SOCKET
constexpr SOCKET INVALID_SOCKET = (SOCKET) -1;
#endif
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct inet_addr {
    union {
        in_addr                 addr4;      // IPv4 address
        in_addr6                addr6;      // IPv6 address
    };
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct socket_addr {
    union {
        sockaddr_in             saddr4;      // IPv4 address
        sockaddr_in6            saddr6;      // IPv6 address
    };

    const auto & family() const {
        return saddr4.sin_family;
    }

    template <typename T>
    socket_addr & family(const T & family) {
        saddr4.sin_family = decltype(saddr4.sin_family) (family);
        return *this;
    }

    const auto & port() const {
        return saddr4.sin_port;
    }

    template <typename T>
    socket_addr & port(const T & port) {
        saddr4.sin_port = decltype(saddr4.sin_port) (port);
        return *this;
    }

    const void * addr_data() const {
        return saddr4.sin_family == AF_INET ? (const void *) &saddr4.sin_addr :
            saddr4.sin_family == AF_INET6 ? (const void *) &saddr6.sin6_addr : nullptr;
    }

    void * addr_data() {
        return saddr4.sin_family == AF_INET ? (void *) &saddr4.sin_addr :
            saddr4.sin_family == AF_INET6 ? (void *) &saddr6.sin6_addr : nullptr;
    }

    socklen_t addr_size() const {
        return saddr4.sin_family == AF_INET ? sizeof(saddr4.sin_addr) :
            saddr4.sin_family == AF_INET6 ? sizeof(saddr6.sin6_addr) : 0;
    }
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
typedef enum {
    SocketDomainInvalid = -1,
    SocketDomainUNSPEC = AF_UNSPEC,
    SocketDomainPACKET = AF_PACKET,
    SocketDomainINET = AF_INET,
    SocketDomainINET6 = AF_INET6
} SocketDomain;
//------------------------------------------------------------------------------
typedef enum {
    SocketTypeInvalid = -1,
    SocketTypeTcp = SOCK_STREAM,
    SocketTypeUdp = SOCK_DGRAM,
    SocketTypeRaw = SOCK_RAW
} SocketType;
//------------------------------------------------------------------------------
struct socket_addr_t : public socket_addr {
    SocketType type;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class base_socket {
public:
    typedef enum {
        Receives    = SHUT_RD,
        Sends       = SHUT_WR,
        Both        = SHUT_RDWR
    } ShutdownMode;

    typedef enum {
        SocketError = SOCKET_ERROR,//< Generic socket error translates to error below.
        SocketSuccess = 0,         //< No socket error.
        SocketInvalidSocket,       //< Invalid socket handle.
        SocketInvalidAddress,      //< Invalid destination address specified.
        SocketInvalidPort,         //< Invalid destination port specified.
        SocketConnectionRefused,   //< No server is listening at remote address.
        SocketTimedout,            //< Timed out while attempting operation.
        SocketEwouldblock,         //< Operation would block if socket were blocking.
        SocketNotconnected,        //< Currently not connected.
        SocketEinprogress,         //< Socket is non-blocking and the connection cannot be completed immediately
        SocketInterrupted,         //< Call was interrupted by a signal that was caught before a valid connection arrived.
        SocketConnectionAborted,   //< The connection has been aborted.
        SocketProtocolError,       //< Invalid protocol for operation.
        SocketFirewallError,       //< Firewall rules forbid connection.
        SocketInvalidSocketBuffer, //< The receive buffer point outside the process's address space.
        SocketConnectionReset,     //< Connection was forcibly closed by the remote host.
        SocketAddressInUse,        //< Address already in use.
        SocketInvalidPointer,      //< Pointer type supplied as argument is invalid.
        SocketEunknown             //< Unknown error
    } SocketErrors;

public:
    ~base_socket() {
        close(true);
    }

    base_socket() :
        socket_(INVALID_SOCKET),
        socket_errno_(SocketInvalidSocket),
        buffer_size_(0),
        //socket_domain_(SocketDomainInvalid),
        socket_type_(SocketTypeInvalid),
        bytes_received_(0),
        bytes_sent_(0),
        socket_flags_(0)
	{
    }

    base_socket & open(int socket_domain, int socket_type, int socket_protocol = 0);

    base_socket & open(SocketDomain socket_domain = SocketDomainINET, SocketType socket_type = SocketTypeTcp, int socket_protocol = 0) {
        return open(socket_domain, socket_type, socket_protocol);
    }

    base_socket & close(bool no_trow = false);

    base_socket & shutdown(ShutdownMode nShutdown = Sends) {
        if( ::shutdown(socket_, nShutdown) == SocketError )
            throw_socket_error();
        return *this;
    }

    bool select(int32_t nTimeoutSec = 0, int32_t nTimeoutUSec = 0) {
        bool            bRetVal = false;
        struct timeval *pTimeout = NULL;
        struct timeval  timeout;
        int32_t         nNumDescriptors = -1;
        int32_t         nError = 0;

        fd_set error_fds, read_fds, write_fds;

        FD_ZERO(&error_fds);
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(socket_, &error_fds);
        FD_SET(socket_, &read_fds);
        FD_SET(socket_, &write_fds);

        //---------------------------------------------------------------------
        // If timeout has been specified then set value, otherwise set timeout
        // to NULL which will block until a descriptor is ready for read/write
        // or an error has occurred.
        //---------------------------------------------------------------------
        if( nTimeoutSec > 0 || nTimeoutUSec > 0 ) {
            timeout.tv_sec = nTimeoutSec;
            timeout.tv_usec = nTimeoutUSec;
            pTimeout = &timeout;
        }

        nNumDescriptors = ::select(socket_ + 1, &read_fds, &write_fds, &error_fds, pTimeout);

        //----------------------------------------------------------------------
        // Handle timeout
        //----------------------------------------------------------------------
        if( nNumDescriptors == 0 ) {
            socket_errno_ = SocketTimedout;
        }
        //----------------------------------------------------------------------
        // If a file descriptor (read/write) is set then check the
        // socket error (SO_ERROR) to see if there is a pending error.
        //----------------------------------------------------------------------
        else if( FD_ISSET(socket_, &read_fds) || FD_ISSET(socket_, &write_fds) ) {
            int32_t nLen = sizeof(nError);

            if( getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char *) &nError, &nLen) == 0 ) {
                errno = nError;

                if( nError == 0 )
                    bRetVal = true;
            }

            translate_socket_error();
        }

        return bRetVal;
    }

    bool is_socket_valid() const {
        return socket_ != INVALID_SOCKET;
    }

    intptr_t recv(size_t bytesToRecv = ~size_t(0), size_t maxRecv = RX_MAX_BYTES) {
        bytes_received_ = 0;

        //--------------------------------------------------------------------------
        // If the socket is invalid then return false.
        //--------------------------------------------------------------------------
        if( !is_socket_valid() )
            return bytes_received_;

        auto next_buffer_size = [this] {
            return buffer_size_ >= MIN_BUFFER_SIZE ? buffer_size_ << 1 : MIN_BUFFER_SIZE;
        };

        auto expand_buffer = [&] {
            if( buffer_size_ < bytes_received_ + maxRecv ) {
                auto nbs = next_buffer_size();
                std::unique_ptr<uint8_t> new_buf(new uint8_t [nbs]);
                memcpy(new_buf.get(), buffer_.get(), bytes_received_);
                buffer_.swap(new_buf);
                buffer_size_ = nbs;
            }
        };

        socket_errno_ = SocketSuccess;
        socklen_t as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : local_addr_.addr_size();

        while( bytesToRecv > 0 ) {
            int nb = int(bytesToRecv > maxRecv ? maxRecv : bytesToRecv);
            intptr_t r = 0;

            for(;;) {
                expand_buffer();

                auto pWorkBuffer = (char *) (buffer_.get() + bytes_received_);

                switch( socket_type_ ) {
                    //----------------------------------------------------------------------
                    // If zero bytes are received, then return.  If SocketERROR is
                    // received, free buffer and return CSocket::SocketError (-1) to caller.
                    //----------------------------------------------------------------------
                    case SocketTypeTcp  :
                        r = ::recv(socket_, pWorkBuffer, nb, socket_flags_);// MSG_WAITALL
                        break;
                    case SocketTypeUdp  :
                        r = ::recvfrom(socket_, pWorkBuffer, nb, 0,
                            (sockaddr *) (is_multicast_ ? &multicast_group_ : local_addr_.addr_data()), &as);
                        break;
                    default:
                        break;
                }

                if( r != SocketError || translate_socket_error() != SocketInterrupted )
                    break;
            }

            if( r == SocketError || r == 0 )
                break;

            bytesToRecv -= r;
            bytes_received_ += r;
        }

        return bytes_received_;
    }

    size_t send(const uint8_t * pBuf, size_t bytesToSend, size_t maxSend = TX_MAX_BYTES) {
        socket_errno_ = SocketSuccess;
        bytes_sent_ = 0;

        if( !is_socket_valid() )
            return bytes_sent_;

        socklen_t as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : remote_addr_.addr_size();

        while( bytesToSend > 0 ) {
            int nb = int(bytesToSend > maxSend ? maxSend : bytesToSend);
            intptr_t w = 0;
            //---------------------------------------------------------
            // Check error condition and attempt to resend if call
            // was interrupted by a signal.
            //---------------------------------------------------------
            for(;;) {
                switch( socket_type_ ) {
                    case SocketTypeTcp  :
                        w = ::send(socket_, (const char *) pBuf, nb, 0);
                        break;
                    case SocketTypeUdp  :
                        w = ::sendto(socket_, (const char *) pBuf, nb, 0,
                            (const sockaddr *) (is_multicast_ ? &multicast_group_ : &remote_addr_.saddr4),
                            as);
                        break;
                    default :
                        break;
                }

                if( w != SocketError || translate_socket_error() != SocketInterrupted )
                    break;
            }

            if( w == SocketError || w == 0 )
                break;

            bytesToSend -= w;
            bytes_sent_ += w;
        }

        return bytes_sent_;
	}

    size_t send(const struct iovec * sendVector, size_t nNumItems) {
        socket_errno_ = SocketSuccess;
#if _WIN32
        size_t nBytesSent = 0;
        //--------------------------------------------------------------------------
        // Send each buffer as a separate send, windows does not support this
        // function call.
        //--------------------------------------------------------------------------
        for( size_t i = 0; i < nNumItems; i++ ) {
            auto nBytes = send((uint8_t *) sendVector[i].iov_base, sendVector[i].iov_len);

            if( socket_errno_ != SocketSuccess )
                break;

            nBytesSent += nBytes;
        }

        bytes_sent_ = nBytesSent;
#else
        bytes_sent_ = 0;

        auto w = ::writev(socket_, sendVector, nNumItems);

        if( w == SocketError)
            TranslateSocketError();

        bytes_sent_ = w;
#endif
        return bytes_sent_;
    }

    const char * data() const {
        return reinterpret_cast<const char *>(buffer_.get());
    }

    char * data() {
        return reinterpret_cast<char *>(buffer_.get());
    }

    auto size() const {
        return bytes_received_;
    }

    auto bytes_received() const {
        return bytes_received_;
    }

    auto bytes_sent() const {
        return bytes_sent_;
    }

    bool option_linger(bool bEnable, uint16_t nTime) {
        bool bRetVal = false;

        linger l;
        l.l_onoff = (bEnable == true) ? 1: 0;
        l.l_linger = nTime;

        if( setsockopt(socket_, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof(l)) == 0)
            bRetVal = true;

        translate_socket_error();

        return bRetVal;
    }

    bool option_reuse_addr() {
        bool  bRetVal = false;
        int32_t nReuse  = IPTOS_LOWDELAY;

        if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char *) &nReuse, sizeof(int32_t)) == 0 )
            bRetVal = true;

        translate_socket_error();

        return bRetVal;
    }

    bool multicast(bool bEnable, uint8_t multicastTTL) {
		bool bRetVal = false;

        if( socket_type_ == SocketTypeUdp ) {
            if( setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) &multicastTTL, sizeof(multicastTTL)) == SocketError ) {
                translate_socket_error();
				bRetVal = false;
			}
            else {
				bRetVal = true;
                is_multicast_ = bEnable;
            }
		}
        else {
            socket_errno_ = SocketProtocolError;
		}

		return bRetVal;
	}

    bool multicast() {
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

    SocketErrors ocket_error() {
        return socket_errno_;
    }

    int32_t socket_dscp() {
        int32_t    nTempVal = 0;
		socklen_t  nLen = 0;

        if( is_socket_valid() ) {
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

        if( is_socket_valid() )
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

    const auto & remote_addr() const {
        return remote_addr_;
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
            socket_errno_ = SocketInvalidSocket;
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
            socket_errno_ = SocketInvalidSocket;
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
            case SocketInvalidSocket:
                return "Invalid socket handle.";
            case SocketInvalidAddress:
                return "Invalid destination address specified.";
            case SocketInvalidPort:
                return "Invalid destination port specified.";
            case SocketConnectionRefused:
                return "No server is listening at remote address.";
            case SocketTimedout:
                return "Timed out while attempting operation.";
            case SocketEwouldblock:
                return "Operation would block if socket were blocking.";
            case SocketNotconnected:
                return "Currently not connected.";
            case SocketEinprogress:
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
            case SocketEunknown:
                return "Unknown error";
            default:
                return "No such CSimpleSocket error";
        }
    }

    const char * str_error() const {
        return str_error(socket_errno_);
    }

    void throw_socket_error() {
        if( translate_socket_error() != SocketSuccess )
            throw std::runtime_error(str_error());
    }

    SocketErrors translate_socket_error() {
#if _WIN32
        switch( WSAGetLastError() ) {
            case EXIT_SUCCESS:
                socket_errno_ = SocketSuccess;
                break;
            case WSAEBADF:
            case WSAENOTCONN:
                socket_errno_ = SocketNotconnected;
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
                socket_errno_ = SocketInvalidSocket;
                break;
            case WSAECONNREFUSED :
                socket_errno_ = SocketConnectionRefused;
                break;
            case WSAETIMEDOUT:
                socket_errno_ = SocketTimedout;
                break;
            case WSAEINPROGRESS:
                socket_errno_ = SocketEinprogress;
                break;
            case WSAECONNABORTED:
                socket_errno_ = SocketConnectionAborted;
                break;
            case WSAEWOULDBLOCK:
                socket_errno_ = SocketEwouldblock;
                break;
            case WSAENOTSOCK:
                socket_errno_ = SocketInvalidSocket;
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
            default:
                socket_errno_ = SocketEunknown;
                break;
        }
#else
        switch( errno ) {
            case EXIT_SUCCESS:
                socket_errno_ = SocketSuccess;
                break;
            case ENOTCONN:
                socket_errno_ = SocketNotconnected;
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
            case EPIPE:
                socket_errno_ = SocketInvalidSocket;
                break;
            case ECONNREFUSED :
                socket_errno_ = SocketConnectionRefused;
                break;
            case ETIMEDOUT:
                socket_errno_ = SocketTimedout;
                break;
            case EINPROGRESS:
                socket_errno_ = SocketEinprogress;
                break;
            case EWOULDBLOCK:
                //        case EAGAIN:
                socket_errno_ = SocketEwouldblock;
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
                socket_errno_ = SocketEunknown;
                break;
            }
#endif
        return socket_errno_;
    }

    SOCKET                      socket_;            // socket handle
    SocketErrors                socket_errno_;      // number of last error
    std::unique_ptr<uint8_t>    buffer_;            // internal send/receive buffer
    size_t                      buffer_size_;       // size of internal send/receive buffer
    SocketDomain                socket_domain_;     // socket type AF_INET, AF_INET6
    SocketType                  socket_type_;       // socket type - UDP, TCP or RAW
    size_t                      bytes_received_;    // number of bytes received
    size_t                      bytes_sent_;        // number of bytes sent
    uint32_t                    socket_flags_;      // socket flags
    bool                        is_multicast_;      // is the UDP socket multicast;
    socket_addr                 remote_addr_;
    socket_addr                 local_addr_;
    sockaddr_in                 multicast_group_;
private:
    base_socket(const base_socket & socket);
    void operator = (const base_socket &);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class passive_socket;
//------------------------------------------------------------------------------
class active_socket : public base_socket {
public:
    friend class passive_socket;

    ~active_socket() {
        close();
    }

    active_socket(SocketType socket_type = SocketTypeTcp) {
        socket_type_ = socket_type;
    }

    active_socket & connect(const char * node, const char * service) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
        //hints.ai_socktype   = SOCK_DGRAM;   // Datagram socket
        hints.ai_socktype   = 0;            // Any socket
        hints.ai_flags      = AI_ALL;
        hints.ai_protocol   = 0;            // Any protocol
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        if( getaddrinfo(node, service, &hints, &result) != 0 ) {
            socket_errno_ = SocketEunknown;
            throw std::runtime_error(str_error());
            //throw std::runtime_error(gai_strerror(s));
        }

        if( result == nullptr ) {
            socket_errno_ = SocketEunknown;
            throw std::runtime_error(str_error());
            //throw std::runtime_error("No address succeeded");
        }

        at_scope_exit(
            freeaddrinfo(result);
            close();
        );

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect(2).
        // If socket(2) (or connect(2)) fails, we (close the socket
        // and) try the next address.

        for( rp = result; rp != nullptr; rp = rp->ai_next ) {
            if( rp->ai_family != AF_INET
                && rp->ai_family != AF_INET6
                && rp->ai_socktype != SOCK_DGRAM
                && rp->ai_socktype != SOCK_STREAM
                && rp->ai_socktype != SOCK_RAW )
                continue;

            open(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            memcpy(&remote_addr_, rp->ai_addr, rp->ai_addrlen);

            if( ::connect(socket_, (sockaddr *) &remote_addr_, remote_addr_.addr_size()) != SocketError )
                break;

            throw_socket_error();
        }

        if( rp == nullptr ) {
            socket_errno_ = SocketNotconnected;
            throw std::runtime_error(str_error());
        }

        socklen_t as = remote_addr_.addr_size();
        getsockname(socket_, (sockaddr *) &remote_addr_, &as);
        getpeername(socket_, (sockaddr *) &local_addr_, &as);

        socket_errno_ = SocketSuccess;

        return *this;
    }

};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class passive_socket : public base_socket {
public:
    ~passive_socket() {
        close();
    }

    passive_socket(SocketType socket_type = SocketTypeTcp) {
        socket_type_ = socket_type;
    }

    bool bind_multicast(const char * pInterface, const char * pGroup, uint16_t nPort) {
        bool           bRetVal = false;
#if _WIN32
        ULONG          inAddr;
#else
        int32_t        nReuse;
        in_addr_t      inAddr;

        nReuse = IPTOS_LOWDELAY;
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
        if( pInterface == nullptr || strlen(pInterface) == 0 ) {
            multicast_group_.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else if( (inAddr = ::inet_addr(pInterface)) != INADDR_NONE ) {
            multicast_group_.sin_addr.s_addr = inAddr;
        }

        //--------------------------------------------------------------------------
        // Bind to the specified port
        //--------------------------------------------------------------------------
        if( ::bind(socket_, (struct sockaddr *) &multicast_group_, sizeof(multicast_group_)) == 0 ) {
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

    passive_socket & listen(const char * node, const char * service, uintptr_t backlog) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
        //hints.ai_socktype   = SOCK_DGRAM;   // Datagram socket
        hints.ai_socktype   = 0;            // Any socket
        hints.ai_flags      = AI_PASSIVE;   // For wildcard IP address
        hints.ai_protocol   = 0;            // Any protocol
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        if( getaddrinfo(node, service, &hints, &result) != 0 ) {
            socket_errno_ = SocketEunknown;
            throw std::runtime_error(str_error());
            //throw std::runtime_error(gai_strerror(s));
        }

        if( result == nullptr ) {
            socket_errno_ = SocketEunknown;
            throw std::runtime_error(str_error());
            //throw std::runtime_error("No address succeeded");
        }

        at_scope_exit(
            freeaddrinfo(result);
            close();
        );

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect(2).
        // If socket(2) (or connect(2)) fails, we (close the socket
        // and) try the next address.

        for( rp = result; rp != nullptr; rp = rp->ai_next ) {
            if( rp->ai_family != AF_INET
                && rp->ai_family != AF_INET6
                && rp->ai_socktype != SOCK_DGRAM
                && rp->ai_socktype != SOCK_STREAM
                && rp->ai_socktype != SOCK_RAW )
                continue;

#ifndef _WIN32
            //--------------------------------------------------------------------------
            // Set the following socket option SO_REUSEADDR.  This will allow the file
            // descriptor to be reused immediately after the socket is closed instead
            // of setting in a TIMED_WAIT state.
            //--------------------------------------------------------------------------
            int32_t reuse = 1;
            setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse));
            setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(reuse));
            int32_t low = IPTOS_LOWDELAY;
            setsockopt(socket_, IPPROTO_TCP, IP_TOS, (const char *) &low, sizeof(low));
#endif
            open(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            memcpy(&local_addr_, rp->ai_addr, rp->ai_addrlen);

            if( ::bind(socket_, (sockaddr *) &local_addr_, local_addr_.addr_size()) != SocketError )
                break;

        }

        if( rp == nullptr ) {
            socket_errno_ = SocketNotconnected;
            throw std::runtime_error(str_error());
        }

        if( socket_type_ == SocketTypeTcp )
            if( ::listen(socket_, int(backlog)) == SocketError )
                throw_socket_error();

        socket_errno_ = SocketSuccess;

        return *this;
    }

    active_socket * accept() {
        if( socket_type_ != SocketTypeTcp ) {
            socket_errno_ = SocketProtocolError;
            throw std::runtime_error(str_error());
        }

        std::unique_ptr<active_socket> client_socket(new active_socket);
        socklen_t    as = local_addr_.addr_size();

        for(;;) {
            SOCKET socket = ::accept(socket_, (sockaddr *) &local_addr_, &as);

            if( socket != INVALID_SOCKET ) {
                client_socket->socket_ = socket;
                client_socket->socket_errno_ = SocketSuccess;
                client_socket->socket_domain_ = socket_domain_;
                client_socket->socket_type_ = socket_type_;
                //-------------------------------------------------------------
                // Store client and server IP and port information for this
                // connection.
                //-------------------------------------------------------------
                getsockname(socket_, (sockaddr *) &client_socket->remote_addr_, &as);
                getpeername(socket_, (sockaddr *) &client_socket->local_addr_, &as);
                break;
            }

            if( translate_socket_error() != SocketInterrupted )
                throw std::runtime_error(str_error());
        }

        return client_socket.release();
    }
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
