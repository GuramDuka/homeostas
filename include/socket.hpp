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
//------------------------------------------------------------------------------
#if __CYGWIN__ || __MINGW32__
#   include <stdarg.h>
#   if __CYGWIN__
#       include <w32api/windef.h>
#   include <w32api/winbase.h>
#   include <w32api/winreg.h>
#   endif
#   include <unknwn.h>
#   include <winreg.h>
#elif _WIN32
#   include <windows.h>
#   include <iphlpapi.h>
#elif __linux__
#   include <sys/uio.h>
#   include <linux/if_packet.h>
#   include <linux/if_ether.h>
#   include <linux/if.h>
#elif __APPLE__
#   include <cstdlib>
#   include <sys/sysctl.h>
#   include <sys/socket.h>
#   include <net/if.h>
#   include <net/route.h>
#elif BSD || __FreeBSD_kernel__ || (sun && __SVR4)
#   include <cstring>
#   include <unistd.h>
#   include <sys/socket.h>
#   include <net/if.h>
#   include <net/route.h>
#elif __HAIKU__
#   include <cstdlib>
#   include <unistd.h>
#   include <net/if.h>
#   include <sys/sockio.h>
#endif
//------------------------------------------------------------------------------
#include <cassert>
#include <type_traits>
#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <array>
#include <new>
#include <ios>
#include <iomanip>
#include <streambuf>
#include <istream>
#include <ostream>
#include <iostream>
#include <algorithm>
#include <locale>
//------------------------------------------------------------------------------
#include "port.hpp"
#include "locale_traits.hpp"
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
#if __cplusplus > 201402L
    std::hardware_destructive_interference_size
#else
    64
#endif
    ;

#ifndef INVALID_SOCKET
constexpr SOCKET INVALID_SOCKET = (SOCKET) -1;
#endif
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

    const auto & family() const {
        return storage.ss_family;
    }

    template <typename T>
    socket_addr & family(const T & family) {
        storage.ss_family = decltype(storage.ss_family) (family);
        return *this;
    }

    const auto port() const {
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
            storage.ss_family == AF_INET6 ? (const void *) &saddr6.sin6_addr : nullptr;
    }

    void * addr_data() {
        return storage.ss_family == AF_INET ? (void *) &saddr4.sin_addr :
            storage.ss_family == AF_INET6 ? (void *) &saddr6.sin6_addr : nullptr;
    }

    socklen_t addr_size() const {
        return storage.ss_family == AF_INET ? sizeof(saddr4.sin_addr) :
            storage.ss_family == AF_INET6 ? sizeof(saddr6.sin6_addr) : 0;
    }

    socklen_t size() const {
        return storage.ss_family == AF_INET ? sizeof(saddr4) :
            storage.ss_family == AF_INET6 ? sizeof(saddr6) : 0;
    }

    auto & from_string(const std::string & addr) {
        const char * p_node = addr.c_str(), * p_service = "0";
        std::string node, service;

        auto p = addr.rfind(':');

        if( p != std::string::npos ) {
            node = addr.substr(0, p);
            p_node = node.c_str();

            service = addr.substr(p + 1);

            if( !service.empty() )
                p_service = service.c_str();
        }

        addrinfo hints, * result = nullptr;

        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_family     = storage.ss_family;
        hints.ai_socktype   = 0;
        hints.ai_flags      = AI_ALL;   // For wildcard IP address
        hints.ai_protocol   = 0;        // Any protocol
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        auto r = getaddrinfo(p_node, p_service, &hints, &result);

        if( r != 0 )
            throw std::runtime_error(
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

        if( result != nullptr )
            memcpy(&storage, result->ai_addr, result->ai_addrlen);

        return *this;
    }

    std::string to_string() const {
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        auto r = getnameinfo((const sockaddr *) &storage, sizeof(storage), hbuf, sizeof(hbuf), sbuf,
                sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);

        if( r != 0 )
            throw std::runtime_error(
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

        if( port() != 0 && slen(sbuf) != 0 )
            return std::string(hbuf) + ":" + std::string(sbuf);

        return std::string(hbuf);
    }

    bool default_gateway(bool no_throw = false) {
        if( storage.ss_family == AF_INET )
            if( get_default_gateway(&saddr4.sin_addr) == 0 )
                return true;

        if( !no_throw )
            throw std::runtime_error("Can't get default gateway");

        return false;
    }

    // There is no portable method to get the default route gateway.
    // So below are four (or five ?) differents functions implementing this.
    // Parsing /proc/net/route is for linux.
    // sysctl is the way to access such informations on BSD systems.
    // Many systems should provide route information through raw PF_ROUTE
    // sockets.
    // In MS Windows, default gateway is found by looking into the registry
    // or by using GetBestRoute().

    static int get_default_gateway(in_addr * addr) {
#if __linux__
        // parse /proc/net/route which is as follow :

        //Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
        //wlan0   0001A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0
        //eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0
        //wlan0   00000000        0101A8C0        0003    0       0       0       00000000        0       0       0
        //eth0    00000000        00000000        0001    0       0       1000    00000000        0       0       0
        //
        // One header line, and then one line by route by route table entry.

        FILE * f = fopen("/proc/net/route", "r");;

        if( f == nullptr )
            return -1;

        char buf[256];
        int line = 0;

        while( fgets(buf, sizeof(buf), f) != nullptr ) {
            if( line > 0 ) { // skip the first line
                char * p = buf;

                // skip the interface name
                while( *p && !isspace(*p) )
                    p++;

                while( *p && isspace(*p) )
                    p++;

                unsigned long d, g;

                if( sscanf(p, "%lx%lx", &d, &g) == 2 ) {
                    if( d == 0 && g != 0 ) { // default
                        addr->s_addr = g;
                        fclose(f);
                        return 0;
                    }
                }
            }
            line++;
        }
        // default route not found
        if( f != nullptr )
            fclose(f);

        return -1;
#elif __APPLE__
        auto ROUNDUP = [] (auto a) {
            return ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long));
        };

#if 0
        /* net.route.0.inet.dump.0.0 ? */
        int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0, 0/*tableid*/};
#endif
        /* net.route.0.inet.flags.gateway */
        int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_GATEWAY};
        size_t l;
        char * buf, * p;
        struct rt_msghdr * rt;
        struct sockaddr * sa;
        struct sockaddr * sa_tab[RTAX_MAX];
        int i;
        int r = FAILED;
        if(sysctl(mib, sizeof(mib)/sizeof(int), 0, &l, 0, 0) < 0) {
            return FAILED;
        }
        if(l>0) {
            buf = malloc(l);
            if(sysctl(mib, sizeof(mib)/sizeof(int), buf, &l, 0, 0) < 0) {
                free(buf);
                return FAILED;
            }
            for(p=buf; p<buf+l; p+=rt->rtm_msglen) {
                rt = (struct rt_msghdr *)p;
                sa = (struct sockaddr *)(rt + 1);
                for(i=0; i<RTAX_MAX; i++) {
                    if(rt->rtm_addrs & (1 << i)) {
                        sa_tab[i] = sa;
                        sa = (struct sockaddr *)((char *)sa + ROUNDUP(sa->sa_len));
                    } else {
                        sa_tab[i] = NULL;
                    }
                }
                if( ((rt->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY))
                  && sa_tab[RTAX_DST]->sa_family == AF_INET
                  && sa_tab[RTAX_GATEWAY]->sa_family == AF_INET) {
                    if(((struct sockaddr_in *)sa_tab[RTAX_DST])->sin_addr.s_addr == 0) {
                        *addr = ((struct sockaddr_in *)(sa_tab[RTAX_GATEWAY]))->sin_addr.s_addr;
                        r = SUCCESS;
                    }
                }
            }
            free(buf);
        }

        return r;
#elif BSD || __FreeBSD_kernel__ || (sun && __SVR4)
          // Thanks to Darren Kenny for this code
        auto NEXTADDR = (auto w, auto u) {
            if (rtm_addrs & w)
            l = sizeof(struct sockaddr); memmove(cp, &(u), l); cp += l;
        };

        struct {
            struct rt_msghdr m_rtm;
            char       m_space[512];
        } m_rtmsg;

        int s, seq, l, rtm_addrs, i;
        pid_t pid;
        struct sockaddr so_dst, so_mask;
        char *cp = m_rtmsg.m_space;
        struct sockaddr *gate = NULL, *sa;
        struct rt_msghdr *msg_hdr;

        pid = getpid();
        seq = 0;
        rtm_addrs = RTA_DST | RTA_NETMASK;

        memset(&so_dst, 0, sizeof(so_dst));
        memset(&so_mask, 0, sizeof(so_mask));
        memset(&m_rtmsg.m_rtm, 0, sizeof(struct rt_msghdr));

        m_rtmsg.m_rtm.rtm_type = RTM_GET;
        m_rtmsg.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
        m_rtmsg.m_rtm.rtm_version = RTM_VERSION;
        m_rtmsg.m_rtm.rtm_seq = ++seq;
        m_rtmsg.m_rtm.rtm_addrs = rtm_addrs;

        so_dst.sa_family = AF_INET;
        so_mask.sa_family = AF_INET;

        NEXTADDR(RTA_DST, so_dst);
        NEXTADDR(RTA_NETMASK, so_mask);

        m_rtmsg.m_rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;

        s = socket(PF_ROUTE, SOCK_RAW, 0);

        if (write(s, (char *)&m_rtmsg, l) < 0) {
            close(s);
            return FAILED;
        }

        do {
            l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
        } while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));

        close(s);

        msg_hdr = &rtm;

        cp = ((char *)(msg_hdr + 1));

        if (msg_hdr->rtm_addrs) {
            for (i = 1; i; i <<= 1)
                if (i & msg_hdr->rtm_addrs) {
                    sa = (struct sockaddr *)cp;
                    if (i == RTA_GATEWAY )
                        gate = sa;

                    cp += sizeof(struct sockaddr);
                }
                else
                    return FAILED;
        }

        if( gate != NULL ) {
            *addr = ((struct sockaddr_in *)gate)->sin_addr.s_addr;
            return SUCCESS;
        }
        return FAILED;
#elif _WIN32
            constexpr size_t MAX_KEY_LENGTH = 255;
            constexpr size_t MAX_VALUE_LENGTH = 16383;

            HKEY networkCardsKey;
            HKEY networkCardKey;
            HKEY interfacesKey;
            HKEY interfaceKey;
            DWORD i = 0;
            DWORD numSubKeys = 0;
            CHAR keyName[MAX_KEY_LENGTH];
            DWORD keyNameLength = MAX_KEY_LENGTH;
            CHAR keyValue[MAX_VALUE_LENGTH];
            DWORD keyValueLength = MAX_VALUE_LENGTH;
            DWORD keyValueType = REG_SZ;
            CHAR gatewayValue[MAX_VALUE_LENGTH];
            DWORD gatewayValueLength = MAX_VALUE_LENGTH;
            DWORD gatewayValueType = REG_MULTI_SZ;
            bool done = 0;

            constexpr CONST CHAR networkCardsPath[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
            constexpr CONST CHAR interfacesPath[] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
            constexpr CONST CHAR STR_SERVICENAME[] = "ServiceName";
            constexpr CONST CHAR STR_DHCPDEFAULTGATEWAY[] = "DhcpDefaultGateway";
            constexpr CONST CHAR STR_DEFAULTGATEWAY[] = "DefaultGateway";

            // The windows registry lists its primary network devices in the following location:
            // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards
            //
            // Each network device has its own subfolder, named with an index, with various properties:
            // -NetworkCards
            //   -5
            //     -Description = Broadcom 802.11n Network Adapter
            //     -ServiceName = {E35A72F8-5065-4097-8DFE-C7790774EE4D}
            //   -8
            //     -Description = Marvell Yukon 88E8058 PCI-E Gigabit Ethernet Controller
            //     -ServiceName = {86226414-5545-4335-A9D1-5BD7120119AD}
            //
            // The above service name is the name of a subfolder within:
            // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces
            //
            // There may be more subfolders in this interfaces path than listed in the network cards path above:
            // -Interfaces
            //   -{3a539854-6a70-11db-887c-806e6f6e6963}
            //     -DhcpIPAddress = 0.0.0.0
            //     -[more]
            //   -{E35A72F8-5065-4097-8DFE-C7790774EE4D}
            //     -DhcpIPAddress = 10.0.1.4
            //     -DhcpDefaultGateway = 10.0.1.1
            //     -[more]
            //   -{86226414-5545-4335-A9D1-5BD7120119AD}
            //     -DhcpIpAddress = 10.0.1.5
            //     -DhcpDefaultGateay = 10.0.1.1
            //     -[more]
            //
            // In order to extract this information, we enumerate each network card, and extract the ServiceName value.
            // This is then used to open the interface subfolder, and attempt to extract a DhcpDefaultGateway value.
            // Once one is found, we're done.
            //
            // It may be possible to simply enumerate the interface folders until we find one with a DhcpDefaultGateway value.
            // However, the technique used is the technique most cited on the web, and we assume it to be more correct.

            if( RegOpenKeyExA(HKEY_LOCAL_MACHINE,                // Open registry key or predifined key
                                             networkCardsPath,   // Name of registry subkey to open
                                             0,                  // Reserved - must be zero
                                             KEY_READ,           // Mask - desired access rights
                                             &networkCardsKey)   // Pointer to output key
                    != ERROR_SUCCESS )
            {
                // Unable to open network cards keys
                return -1;
            }

            if( RegOpenKeyExA(HKEY_LOCAL_MACHINE,                // Open registry key or predefined key
                                             interfacesPath,     // Name of registry subkey to open
                                             0,                  // Reserved - must be zero
                                             KEY_READ,           // Mask - desired access rights
                                             &interfacesKey)     // Pointer to output key
                    != ERROR_SUCCESS )
            {
                // Unable to open interfaces key
                RegCloseKey(networkCardsKey);
                return -1;
            }

            // Figure out how many subfolders are within the NetworkCards folder
            RegQueryInfoKeyA(networkCardsKey, NULL, NULL, NULL, &numSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

            //printf( "Number of subkeys: %u\n", (unsigned int)numSubKeys);

            // Enumrate through each subfolder within the NetworkCards folder
            for( i = 0; i < numSubKeys && !done; i++ ) {
                keyNameLength = MAX_KEY_LENGTH;

                if( RegEnumKeyExA(networkCardsKey,                // Open registry key
                                                 i,               // Index of subkey to retrieve
                                                 keyName,         // Buffer that receives the name of the subkey
                                                 &keyNameLength,  // Variable that receives the size of the above buffer
                                                 NULL,            // Reserved - must be NULL
                                                 NULL,            // Buffer that receives the class string
                                                 NULL,            // Variable that receives the size of the above buffer
                                                 NULL)            // Variable that receives the last write time of subkey
                        == ERROR_SUCCESS )
                {
                    if( RegOpenKeyExA(networkCardsKey,  keyName, 0, KEY_READ, &networkCardKey) == ERROR_SUCCESS ) {
                        keyValueLength = MAX_VALUE_LENGTH;

                        if( RegQueryValueExA(networkCardKey,                  // Open registry key
                                                            STR_SERVICENAME,  // Name of key to query
                                                            NULL,             // Reserved - must be NULL
                                                            &keyValueType,    // Receives value type
                                                            (LPBYTE)keyValue, // Receives value
                                                            &keyValueLength)  // Receives value length in bytes
                                == ERROR_SUCCESS )
                        {
        //					printf("keyValue: %s\n", keyValue);
                            if( RegOpenKeyExA(interfacesKey, keyValue, 0, KEY_READ, &interfaceKey) == ERROR_SUCCESS ) {
                                gatewayValueLength = MAX_VALUE_LENGTH;

                                if( RegQueryValueExA(interfaceKey,                          // Open registry key
                                                                    STR_DHCPDEFAULTGATEWAY, // Name of key to query
                                                                    NULL,                   // Reserved - must be NULL
                                                                    &gatewayValueType,      // Receives value type
                                                                    (LPBYTE)gatewayValue,   // Receives value
                                                                    &gatewayValueLength)    // Receives value length in bytes
                                        == ERROR_SUCCESS )
                                {
                                    // Check to make sure it's a string
                                    if( (gatewayValueType == REG_MULTI_SZ || gatewayValueType == REG_SZ) && (gatewayValueLength > 1) ) {
                                        //printf("gatewayValue: %s\n", gatewayValue);
                                        done = true;
                                    }
                                }
                                else if( RegQueryValueExA(interfaceKey,                     // Open registry key
                                                                    STR_DEFAULTGATEWAY,     // Name of key to query
                                                                    NULL,                   // Reserved - must be NULL
                                                                    &gatewayValueType,      // Receives value type
                                                                    (LPBYTE)gatewayValue,   // Receives value
                                                                    &gatewayValueLength)    // Receives value length in bytes
                                         == ERROR_SUCCESS )
                                {
                                    // Check to make sure it's a string
                                    if( (gatewayValueType == REG_MULTI_SZ || gatewayValueType == REG_SZ) && (gatewayValueLength > 1) ) {
                                        //printf("gatewayValue: %s\n", gatewayValue);
                                        done = true;
                                    }
                                }
                                RegCloseKey(interfaceKey);
                            }
                        }
                        RegCloseKey(networkCardKey);
                    }
                }
            }

            RegCloseKey(interfacesKey);
            RegCloseKey(networkCardsKey);

            if( done ) {
                addr->S_un.S_addr = inet_addr(gatewayValue);
                return 0;
            }

#   if __CYGWIN__ || __MINGW32__
            return -1;
#   else
            MIB_IPFORWARDROW ip_forward;
            memset(&ip_forward, 0, sizeof(ip_forward));

            if( GetBestRoute(inet_addr("0.0.0.0"), 0, &ip_forward) != NO_ERROR )
                return -1;

            addr->s_addr = ip_forward.dwForwardNextHop;

            return 0;
#   endif
#elif __HAIKU__
            int fd, ret = -1;
            struct ifconf config;
            void *buffer = NULL;
            struct ifreq *interface;

            if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                return -1;
            }
            if (ioctl(fd, SIOCGRTSIZE, &config, sizeof(config)) != 0) {
                goto fail;
            }
            if (config.ifc_value < 1) {
                goto fail; /* No routes */
            }
            if ((buffer = malloc(config.ifc_value)) == NULL) {
                goto fail;
            }
            config.ifc_len = config.ifc_value;
            config.ifc_buf = buffer;
            if (ioctl(fd, SIOCGRTTABLE, &config, sizeof(config)) != 0) {
                goto fail;
            }
            for (interface = buffer;
              (uint8_t *)interface < (uint8_t *)buffer + config.ifc_len; ) {
                struct route_entry route = interface->ifr_route;
                int intfSize;
                if (route.flags & (RTF_GATEWAY | RTF_DEFAULT)) {
                    *addr = ((struct sockaddr_in *)route.gateway)->sin_addr.s_addr;
                    ret = 0;
                    break;
                }
                intfSize = sizeof(route) + IF_NAMESIZE;
                if (route.destination != NULL) {
                    intfSize += route.destination->sa_len;
                }
                if (route.mask != NULL) {
                    intfSize += route.mask->sa_len;
                }
                if (route.gateway != NULL) {
                    intfSize += route.gateway->sa_len;
                }
                interface = (struct ifreq *)((uint8_t *)interface + intfSize);
            }
        fail:
            free(buffer);
            close(fd);
            return ret;
        }
#else
        errno = ENOSYS;

        return -1;
#endif
    }
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
inline string to_string(const homeostas::socket_addr & addr) {
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    auto r = getnameinfo((const sockaddr *) &addr, socklen_t(sizeof(addr)),
        hbuf, socklen_t(sizeof(hbuf)),
        sbuf, socklen_t(sizeof(sbuf)),
        NI_NUMERICHOST | NI_NUMERICSERV);

    if( r != 0 )
        return string();

    return string(hbuf) + ":" + sbuf;
}
//------------------------------------------------------------------------------
inline string to_string(const homeostas::socket_addr & addr, bool no_throw) {
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    auto r = getnameinfo((const sockaddr *) &addr, socklen_t(sizeof(addr)),
        hbuf, socklen_t(sizeof(hbuf)),
        sbuf, socklen_t(sizeof(sbuf)),
        NI_NUMERICHOST | NI_NUMERICSERV);

    if( r != 0 && !no_throw )
        throw std::runtime_error(
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

    return string(hbuf) + ":" + sbuf;
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
namespace homeostas {
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
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class base_socket {
public:
    typedef enum {
        ShutdownRD      = SHUT_RD,
        ShutdownWR      = SHUT_WR,
        ShutdownRDWR    = SHUT_RDWR
    } ShutdownMode;

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
        SocketEUnknown             // Unknown error
    } SocketErrors;

public:

    ~base_socket() noexcept {
        exceptions_ = false;
        close();

#if _WIN32
        if( --wsa_count_ == 0 ) {
            std::unique_lock<std::mutex> lock(mtx_);
            WSACleanup();
            wsa_started_ = false;
        }
#endif
    }

    // move constructor
    base_socket(base_socket && o) noexcept {
        *this = std::move(o);
    }

    // Move assignment operator
    base_socket & operator = (base_socket && o) noexcept {
        if( this != &o ) {
            std::memmove(this, &o, sizeof(o));
            o.socket_ = INVALID_SOCKET;
        }
        return *this;
    }

    base_socket() noexcept :
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
#if _WIN32
        wsa_count_++;

        if( !wsa_started_ ) {
            std::unique_lock<std::mutex> lock(mtx_);

            if( !wsa_started_ ) {
                memset(&wsa_data_, 0, sizeof(wsa_data_));

                if( WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0 ) {
                    //wsa_count_--;
                    //throw_translate_socket_error();
                }

                wsa_started_ = true;
            }
        }
#endif
    }

    base_socket & open(int socket_domain, int socket_type, int socket_protocol) {
        if( valid() )
            close();

        errno = SocketSuccess;

        if( (socket_ = ::socket(socket_domain, socket_type, socket_protocol)) == INVALID_SOCKET ) {
            throw_translate_socket_error();
            return *this;
        }

        socket_domain_  = decltype(socket_domain_) (socket_domain);
        socket_type_    = decltype(socket_type_) (socket_type);
        socket_proto_   = decltype(socket_proto_) (socket_protocol);

        return *this;
    }

    base_socket & open(SocketDomain socket_domain = SocketDomainINET, SocketType socket_type = SocketTypeTCP, SocketProtocol socket_protocol = SocketProtoIP) {
        return open(int(socket_domain), int(socket_type), int(socket_protocol));
    }

    base_socket & close() {
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

    base_socket & shutdown_socket() {
        ::shutdown(socket_, SHUT_RDWR);
        return *this;
    }

    base_socket & shutdown(ShutdownMode how = ShutdownWR) {
        if( ::shutdown(socket_, how) == SocketError )
            throw_translate_socket_error();
        return *this;
    }

    bool select(uint64_t timeout_ns = ~uint64_t(0), uint64_t * p_ellapsed = nullptr) {
        uint64_t started = p_ellapsed != nullptr ? clock_gettime_ns() : 0;
        struct timeval * p_timeout = nullptr;
        struct timeval   timeout;

        //---------------------------------------------------------------------
        // If timeout has been specified then set value, otherwise set timeout
        // to NULL which will block until a descriptor is ready for read/write
        // or an error has occurred.
        //---------------------------------------------------------------------
        if( timeout_ns != ~uint64_t(0) ) {
            timeout.tv_sec = decltype(timeout.tv_sec) (timeout_ns / 1000000000ull);
            timeout.tv_usec = decltype(timeout.tv_usec) (timeout_ns % 1000000000ull);
            p_timeout = &timeout;
        }

        bool r = false;
        fd_set error_fds, read_fds, write_fds;

        FD_ZERO(&error_fds);
        FD_SET(socket_, &error_fds);
        FD_ZERO(&read_fds);
        FD_SET(socket_, &read_fds);
        FD_ZERO(&write_fds);
        FD_SET(socket_, &write_fds);

        int n = ::select(1, &read_fds, &write_fds, &error_fds, p_timeout);

        if ( p_ellapsed != nullptr )
            *p_ellapsed = clock_gettime_ns() - started;

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
        else if( FD_ISSET(socket_, &read_fds) || FD_ISSET(socket_, &write_fds) ) {
            int err = 0;
            socklen_t l = sizeof(err);

            if( getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char *) &err, &l) != 0 ) {
                throw_translate_socket_error();
            }
            else if( err == 0 ) {
                socket_errno_ = SocketSuccess;
                r = true;
            }
        }

        return r;
    }

    bool valid() const {
        return socket_ != INVALID_SOCKET;
    }

    bool invalid() const {
        return socket_ == INVALID_SOCKET;
    }

    size_t recv(void * buffer, size_t size = ~size_t(0), size_t max_recv = RX_MAX_BYTES) {
        size_t bytes_received = 0;
        socklen_t as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : sizeof(local_addr_);

        // if size == 0 recv one chunk of any size

        for(;;) {
            int nb = int(std::min(size, std::min(size_t(std::numeric_limits<int>::max()), max_recv)));

            if( nb == 0 )
                nb = int(std::min(size_t(std::numeric_limits<int>::max()), max_recv));

            intptr_t r = 0;

            for(;;) {
                switch( socket_type_ ) {
                    case SocketTypeTCP  :
                        r = ::recv(socket_, (char *) buffer, nb, socket_flags_);// MSG_WAITALL
                        break;
                    case SocketTypeUDP  :
                        r = ::recvfrom(socket_, (char *) buffer, nb, socket_flags_,
                            (sockaddr *) (is_multicast_ ? &multicast_group_ : local_addr_.addr_data()), &as);
                        break;
                    default:
                        break;
                }

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

            buffer = reinterpret_cast<uint8_t *>(buffer) + r;
            size -= r;
        }

        socket_errno_ = SocketSuccess;

        return bytes_received;
    }

    template <typename T,
        typename = std::enable_if_t<std::is_base_of<T, std::array<typename T::value_type,
#if __GNUG__
            T::_Nm
#elif _MSC_VER
            typename T::_Size
#endif
            >>::value>
    >
    T recv(size_t size = 0) {
        T buffer;
        recv(buffer.data(), size, buffer.size() * sizeof(*buffer.data()));
        return buffer; // RVO ?
    }

    template <typename T,
        typename = std::enable_if_t<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value ||
            std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value
        >
    >
    T recv(size_t size = ~size_t(0), size_t max_recv = RX_MAX_BYTES) {
        typedef typename T::value_type VT;
        T buffer;
        VT * buf = (VT *) alloca(max_recv);
        size_t buf_size = max_recv - (max_recv % sizeof(VT));
        buf_size *= sizeof(VT);

        for(;;) {
            auto r = recv(buf, 0, buf_size);

            if( socket_errno_ != SocketSuccess || r == 0 )
                break;

            r -= r % sizeof(VT);

            buffer.append(buf, r);

            if( size == 0 )
                break;

            size -= r;
        }

        return buffer; // RVO ?
    }

    size_t send(const void * buf, size_t size, size_t max_send = TX_MAX_BYTES) {
        size_t bytes_sent = 0;
        socklen_t as = is_multicast_ ? sizeof(multicast_group_.sin_addr) : sizeof(remote_addr_);

        while( size > 0 ) {
            int nb = int(std::min(size, max_send));
            intptr_t w = 0;

            for(;;) {
                switch( socket_type_ ) {
                    case SocketTypeTCP  :
                        w = ::send(socket_, (const char *) buf, nb, socket_flags_);
                        break;
                    case SocketTypeUDP  :
                        w = ::sendto(socket_, (const char *) buf, nb, socket_flags_,
                            (const sockaddr *) (is_multicast_ ? &multicast_group_ : &remote_addr_.saddr4),
                            as);
                        break;
                    default :
                        break;
                }

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
        typename = std::enable_if_t<
            std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value
        >
    >
    size_t send(const T & s, size_t max_send = TX_MAX_BYTES) {
        return send(s.data(), s.size() * T::value_type, max_send);
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

    base_socket & option_linger(bool enable, uint16_t timeout) {
        linger l;
        l.l_onoff   = enable ? 1 : 0;
        l.l_linger  = timeout;

        if( setsockopt(socket_, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof(l)) != 0 )
            throw_translate_socket_error();

        return *this;
    }

    base_socket option_reuse_addr() {
        int32_t nReuse = IPTOS_LOWDELAY;

        if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char *) &nReuse, sizeof(int32_t)) != 0 )
            throw_translate_socket_error();

        return *this;
    }

    base_socket & multicast(uint8_t multicast_ttl) {
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

    SocketErrors ocket_error() {
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

    const auto & exceptions() const {
        return exceptions_;
    }

    base_socket & exceptions(bool exceptions) {
        exceptions_ = exceptions;
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
            throw std::runtime_error(str_error(socket_errno_));
    }

    void throw_socket_error(const std::string & msg) {
        if( exceptions_ )
            throw std::runtime_error(msg);
    }

    void throw_translate_socket_error() {
        translate_socket_error();
        throw_socket_error();
    }

    base_socket & interrupt(bool interrupt) {
        interrupt_ = interrupt;
        return *this;
    }

    auto interrupt() const {
        return interrupt_;
    }

    template <typename T,
        typename = std::enable_if_t<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, std::basic_string<typename T::value_type::value_type, typename T::value_type::traits_type, typename T::value_type::allocator_type>>::value
        >
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
            gai_err = gai_err;
        }
        else {
            at_scope_exit(
                freeaddrinfo(result);
            );

            for( rp = result; rp != nullptr; rp = rp->ai_next ) {
                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                auto r = getnameinfo(rp->ai_addr, socklen_t(rp->ai_addrlen),
                    hbuf, socklen_t(sizeof(hbuf)),
                    sbuf, socklen_t(sizeof(sbuf)),
                    NI_NUMERICHOST | NI_NUMERICSERV);

                if( r == 0 )
                    container.push_back(hbuf);
            }
        }

        return container;
    }

    static auto wildcards() {
        return wildcards<std::vector<std::string>>();
    }

    template <typename T,
        typename = std::enable_if_t<
            std::is_base_of<T, std::vector<typename T::value_type, typename T::allocator_type>>::value &&
            std::is_base_of<typename T::value_type, std::basic_string<typename T::value_type::value_type, typename T::value_type::traits_type, typename T::value_type::allocator_type>>::value
        >
    >
    static T interfaces() {
        T container;
#if _WIN32
        base_socket dummy;
#endif
        char node[8192], domain[4096];

        if( gethostname(node, sizeof(domain)) == 0
            && getdomainname(domain, sizeof(domain)) == 0 ) {

            if( slen(domain) != 0 )
                scat(scat(node, "."), domain);

            addrinfo hints, * result, * rp;

            memset(&hints, 0, sizeof(addrinfo));
            hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
            hints.ai_socktype   = 0;
            hints.ai_flags      = AI_ALL;
            hints.ai_protocol   = 0;            // Any protocol
            hints.ai_canonname  = nullptr;
            hints.ai_addr       = nullptr;
            hints.ai_next       = nullptr;

            auto r = getaddrinfo(node, "0", &hints, &result);

            if( r == 0 ) {
                at_scope_exit( freeaddrinfo(result) );

                for( rp = result; rp != nullptr; rp = rp->ai_next ) {
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    if( getnameinfo(rp->ai_addr, socklen_t(rp->ai_addrlen),
                            hbuf, socklen_t(sizeof(hbuf)),
                            sbuf, socklen_t(sizeof(sbuf)),
                            NI_NUMERICHOST | NI_NUMERICSERV) == 0 )
                        container.push_back(hbuf);
                }
            }
        }

        return container;
    }

    static auto interfaces() {
        return interfaces<std::vector<std::string>>();
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
            default:
                return "No such CSimpleSocket error";
        }
    }

    SocketErrors translate_socket_error() {
#if _WIN32
        switch( WSAGetLastError() ) {
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
            default:
                socket_errno_ = SocketEUnknown;
                break;
        }
#else
        switch( errno ) {
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
            case EPIPE:
                socket_errno_ = SocketInvalid;
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
            }
#endif
        return socket_errno_;
    }

    static std::mutex mtx_;

#if _WIN32
    static WSADATA wsa_data_;
    static std::atomic_uint wsa_count_;
    static volatile bool wsa_started_;
#endif

    template <typename T, typename F>
    static void move(T & t, T & o, F) {
        std::memcpy(&t, &o, sizeof(o));
        o.socket_ = INVALID_SOCKET;
    }

    template <typename T, typename F,
        typename = std::enable_if_t<std::is_function<F>::value>>
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
    SocketProtocol                 socket_proto_;
    int                         socket_flags_;      // socket flags
    bool                        is_multicast_;      // is the UDP socket multicast;
    bool                        exceptions_;
    bool                        interrupt_;
    socket_addr                 remote_addr_;
    socket_addr                 local_addr_;
    sockaddr_in                 multicast_group_;
private:
    base_socket(const base_socket & base_socket);
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

    // move constructor
    active_socket(active_socket && o) noexcept {
        *this = std::move(o);
    }

    // Move assignment operator
    active_socket & operator = (active_socket && o) noexcept {
        return move(*this, o);
    }

    active_socket(SocketType socket_type = SocketTypeTCP) {
        socket_type_ = socket_type;
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

    active_socket & connect(const char * node, const char * service) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
        //hints.ai_socktype   = SOCK_DGRAM;   // Datagram socket
        hints.ai_socktype   = socket_type_;
        hints.ai_flags      = AI_ALL;
        hints.ai_protocol   = 0;            // Any protocol
        hints.ai_canonname  = nullptr;
        hints.ai_addr       = nullptr;
        hints.ai_next       = nullptr;

        auto r = getaddrinfo(node, service, &hints, &result);

        if( r != 0 ) {
            socket_errno_ = SocketEUnknown;
            throw_socket_error(
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
            );
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

            if( valid() ) {
                memcpy(&remote_addr_, rp->ai_addr, rp->ai_addrlen);

                if( ::connect(socket_, (sockaddr *) &remote_addr_, sizeof(remote_addr_)) != SocketError )
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

        socklen_t as = sizeof(local_addr_);
        getpeername(socket_, (sockaddr *) &remote_addr_, &as);
        getsockname(socket_, (sockaddr *) &local_addr_, &as);

        interrupt_ = false;
        socket_errno_ = SocketSuccess;

        return *this;
    }

    static std::shared_ptr<active_socket> shared() {
        std::unique_ptr<active_socket> uniq(new active_socket);
        return std::move(uniq);
    }
protected:
private:
    active_socket(const active_socket & base_socket);
    void operator = (const active_socket &);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class passive_socket : public base_socket {
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
        socket_type_ = socket_type;
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

    passive_socket & listen(const std::string & node, const std::string & service, int backlog = 0) {
        return listen(node.empty() ? nullptr : node.c_str(), service.empty() ? nullptr : service.c_str(), backlog);
    }

    passive_socket & listen(const char * node, const char * service, int backlog = 0) {
        addrinfo hints, * result, * rp;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family     = AF_UNSPEC;    // Allow IPv4 or IPv6
        //hints.ai_socktype   = SOCK_DGRAM;   // Datagram socket
        hints.ai_socktype   = socket_type_;
        hints.ai_flags      = AI_PASSIVE;   // For wildcard IP address
        hints.ai_protocol   = 0;            // Any protocol
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

                memcpy(&local_addr_, rp->ai_addr, rp->ai_addrlen);

                if( ::bind(socket_, (sockaddr *) &local_addr_, sizeof(local_addr_)) != SocketError )
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
        socklen_t as = sizeof(remote_addr_);

        while( !interrupt_ ) {
            select();

            SOCKET socket = ::accept(socket_, (sockaddr *) &remote_addr_, &as);

            if( socket != INVALID_SOCKET ) {
                client_socket->socket_          = socket;
                client_socket->socket_errno_    = SocketSuccess;
                client_socket->socket_domain_   = socket_domain_;
                client_socket->socket_type_     = socket_type_;

                getpeername(socket, (sockaddr *) &client_socket->remote_addr_, &as);
                getsockname(socket, (sockaddr *) &client_socket->local_addr_, &as);
                break;
            }

            if( translate_socket_error() != SocketInterrupted ) {
                if( !interrupt_ && exceptions_safe )
                    throw std::runtime_error(str_error());
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
    passive_socket(const passive_socket & base_socket);
    void operator = (const passive_socket &);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_streambuf : public std::basic_streambuf<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_streambuf<_Elem, _Traits>::__streambuf_type;
    using typename __streambuf_type::traits_type;
    using typename __streambuf_type::char_type;
    using typename __streambuf_type::int_type;

    using __streambuf_type::gptr;
    using __streambuf_type::egptr;
    using __streambuf_type::gbump;
    using __streambuf_type::setg;
    using __streambuf_type::eback;
    using __streambuf_type::pbase;
    using __streambuf_type::pptr;
    using __streambuf_type::epptr;
    using __streambuf_type::pbump;
    using __streambuf_type::setp;
#endif

    virtual ~basic_socket_streambuf() {
        overflow(traits_type::eof()); // flush write buffer
    }

    basic_socket_streambuf(base_socket & socket) : socket_(&socket) {
        setg(gbuf_.data(), gbuf_.data() + gbuf_.size(), gbuf_.data() + gbuf_.size());
        setp(pbuf_.data(), pbuf_.data() + pbuf_.size());
    }
protected:
    virtual std::streamsize showmanyc() const {
        // return the number of chars in the input sequence
        if( gptr() != nullptr && gptr() < egptr() )
            return egptr() - gptr();

        return 0;
    }

    virtual std::streamsize xsgetn(char_type * s, std::streamsize n) {
        int rval = showmanyc();

        if( rval >= n ) {
            memcpy(s, gptr(), n * sizeof(char_type));
            gbump(n);
            return n;
        }

        memcpy(s, gptr(), rval * sizeof(char_type));
        gbump(rval);

        if( underflow() != traits_type::eof() )
            return rval + xsgetn(s + rval, n - rval);

        return rval;
    }

    virtual int_type underflow() {
        assert( gptr() != nullptr );
        // input stream has been disabled
        // return traits_type::eof();

        if( gptr() >= egptr() ) {
            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            auto r = socket_->recv(eback(), 0, gbuf_.size() * sizeof(char_type));

            if( r <= 0 )
                return traits_type::eof();

            // reset input buffer pointers
            setg(gbuf_.data(), gbuf_.data(), gbuf_.data() + r);
        }

        return *gptr();
    }

    virtual int_type uflow() {
        auto ret = underflow();

        if( ret != traits_type::eof() )
            gbump(1);

        return ret;
    }

    virtual int_type pbackfail(int_type c = traits_type::eof()) {
        c = c;
        return traits_type::eof();
    }

    virtual std::streamsize xsputn(const char_type * s, std::streamsize n) {
        std::streamsize x = 0;

        while( n != 0 ) {
            auto wval = epptr() - pptr();
            auto mval = std::min(wval, decltype(wval) (n));

            memcpy(pptr(), s, mval * sizeof(char_type));
            pbump(mval);

            n -= mval;
            s += mval;
            x += mval;

            if( n == 0 || overflow() == traits_type::eof() )
                break;
        }

        return x;
    }

    virtual int_type overflow(int_type c = traits_type::eof()) {
        // if pbase () == 0, no write is allowed and thus return eof.
        // if c == eof, we sync the output and return 0.
        // if pptr () == epptr (), buffer is full and thus sync the output,
        //                         insert c into buffer, and return c.
        // In all cases, if error happens, throw exception.

        if( pbase() == nullptr )
            return traits_type::eof();

        if( c == traits_type::eof() )
            return sync();

        if( pptr() == epptr() )
            sync();

        *pptr() = char_type(c);
        pbump(1);

        return c;
    }

    virtual int sync() {
        // returns ​0​ on success, -1 otherwise.
        auto wval = pptr() - pbase();

        // we have some data to flush
        if( wval != 0 ) {
            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            auto w = socket_->send((const void *) pbase(), wval * sizeof(char_type));

            if( w != wval * sizeof(char_type) )
                return -1;

            // reset output buffer pointers
            setp(pbuf_.data(), pbuf_.data() + pbuf_.size());
        }

        return 0;
    }

    base_socket * socket_;

    std::array<char_type, RX_MAX_BYTES / sizeof(char_type)> gbuf_;
    std::array<char_type, TX_MAX_BYTES / sizeof(char_type)> pbuf_;
};
//------------------------------------------------------------------------------
typedef basic_socket_streambuf<char, std::char_traits<char>> socket_streambuf;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_istream : public std::basic_istream<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_istream<_Elem, _Traits>::__istream_type;
    using typename __istream_type::__streambuf_type;

    using __istream_type::exceptions;
    using __istream_type::unsetf;
    using __istream_type::rdbuf;
    using __istream_type::setstate;
#endif

    basic_socket_istream(basic_socket_streambuf<_Elem, _Traits> * sbuf) :
        std::basic_istream<_Elem, _Traits>(sbuf
#if __GNUG__ > 5 || _MSC_VER
            , std::ios::binary
#endif
        )
    {
        // http://en.cppreference.com/w/cpp/io/basic_ios/exceptions
        exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        unsetf(std::ios::skipws);
        unsetf(std::ios::unitbuf);
    }

    /*template <typename T,
        typename = std::enable_if_t<
            std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value &&
            std::is_same<typename T::value_type, _Elem>::value &&
            std::is_same<typename T::traits_type, _Traits>::value
        >
    >
    auto & operator >> (T & s) {
        // extract a null-terminated string
        for(;;) {
            auto c = rdbuf()->sbumpc();

            if( c == _Traits::eof() ) {
                setstate(std::ios::eofbit);
                break;
            }

            if( c == _Elem('\0') )
                break;

            s.push_back(_Elem(c));
        }

        return *this;
    }*/

    //typedef std::basic_istream<_Elem, _Traits> & (* __func_manipul_type)(std::basic_istream<_Elem, _Traits> &);

    //basic_socket_istream<_Elem, _Traits> & operator >> (__func_manipul_type f) {

    //template <typename T,
    //    typename = std::enable_if_t<
    //        std::is_base_of<T, std::basic_istream<_Elem, _Traits>>::value
    //    >
    //>
    //basic_socket_istream<_Elem, _Traits> & operator >> (T & (* f)(T &)) {
    //    (*f) (*this);
    //    return *this;
    //}
};
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
std::basic_istream<_Elem, _Traits> & operator >> (
    basic_socket_istream<_Elem, _Traits> & ss,
    std::basic_string<_Elem, _Traits, _Alloc> & s)
{
    return std::getline(*static_cast<std::basic_istream<_Elem, _Traits> *>(&ss), s, _Elem());
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_ostream : public std::basic_ostream<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_ostream<_Elem, _Traits>::__ostream_type;
    using typename __ostream_type::__streambuf_type;

    using __ostream_type::exceptions;
    using __ostream_type::unsetf;
    using __ostream_type::rdbuf;
    using __ostream_type::setstate;
#endif

    basic_socket_ostream(basic_socket_streambuf<_Elem, _Traits> * sbuf) :
        std::basic_ostream<_Elem, _Traits>(sbuf
#if __GNUG__ > 5 || _MSC_VER
            , std::ios::binary
#endif
        )
    {
        // http://en.cppreference.com/w/cpp/io/basic_ios/exceptions
        exceptions(std::ios::failbit | std::ios::badbit);
        unsetf(std::ios::skipws);
        unsetf(std::ios::unitbuf);
    }

    //template <typename T,
    //    typename = std::enable_if_t<
    //        std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value &&
    //        std::is_same<typename T::value_type, _Elem>::value &&
    //        std::is_same<typename T::traits_type, _Traits>::value
    //    >
    //>
    //auto & operator << (const T & s) {
    //    // send a null-terminated string
    //    *static_cast<std::basic_ostream<_Elem, _Traits> *>(this) << (s) << std::ends;
    //    return *this;
    //}

    //typedef std::basic_ostream<_Elem, _Traits> & (* __func_manipul_type)(std::basic_ostream<_Elem, _Traits> &);

    //basic_socket_ostream<_Elem, _Traits> & operator << (__func_manipul_type f) {

    //template <typename T,
    //    typename = std::enable_if_t<
    //        std::is_base_of<T, std::basic_ostream<_Elem, _Traits>>::value
    //    >
    //>
    //basic_socket_ostream<_Elem, _Traits> & operator << (T & (* f)(T &)) {
    //    (*f) (*this);
    //    return *this;
    //}
};
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
std::basic_ostream<_Elem, _Traits> & operator << (
    basic_socket_ostream<_Elem, _Traits> & ss,
    const std::basic_string<_Elem, _Traits, _Alloc> & s)
{
    return *static_cast<std::basic_ostream<_Elem, _Traits> *>(&ss) << s << std::ends;
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_stream :
    public basic_socket_istream<_Elem, _Traits>,
    public basic_socket_ostream<_Elem, _Traits>
{
public:
    basic_socket_stream(basic_socket_streambuf<_Elem, _Traits> * sbuf) :
        basic_socket_istream<_Elem, _Traits>(sbuf),
        basic_socket_ostream<_Elem, _Traits>(sbuf) {
    }
};
//------------------------------------------------------------------------------
typedef basic_socket_stream<char, std::char_traits<char>> socket_stream;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_stream_buffered :
    public basic_socket_streambuf<_Elem, _Traits>,
    public basic_socket_istream<_Elem, _Traits>,
    public basic_socket_ostream<_Elem, _Traits>
{
public:
    basic_socket_stream_buffered(base_socket & socket) :
        basic_socket_streambuf<_Elem, _Traits>(socket),
        basic_socket_istream<_Elem, _Traits>(this),
        basic_socket_ostream<_Elem, _Traits>(this) {
    }
};
//------------------------------------------------------------------------------
typedef basic_socket_stream_buffered<char, std::char_traits<char>> socket_stream_buffered;
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
