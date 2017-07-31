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
#include "config.h"
//------------------------------------------------------------------------------
#if __linux__
#   include <netinet/in.h>
#   include <net/if.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <sys/socket.h>
#   include <sys/ioctl.h>
#   include <linux/netlink.h>
#   include <linux/rtnetlink.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <arpa/inet.h>
//#   include <linux/ip.h>
//#   include <netinet/ip_icmp.h>
#endif
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
#if QT_CORE_LIB
#   include <QString>
#   include <QDebug>
#endif
//------------------------------------------------------------------------------
#include <chrono>
//------------------------------------------------------------------------------
#include "port.hpp"
#include "socket.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
int get_default_gateway(in_addr * addr, in_addr * mask)
{
#if 0 && __ANDROID__
#if _WIN32
#pragma pack(1)
#endif
    struct iphdr {
        uint8_t ihl:4,
            version:4;
        uint8_t tos;
        uint16_t tot_len;
        uint16_t id;
        uint16_t frag_off;
        uint8_t ttl;
        uint8_t protocol;
        uint16_t check;
        uint32_t saddr;
        uint32_t daddr;
    };

    struct icmphdr {
        uint8_t type;          // ICMP packet type
        uint8_t code;          // Type sub code
        uint16_t checksum;
        uint16_t id;
        uint16_t seq;
        uint64_t timestamp;    // not part of ICMP, but we need it
    };

#if _WIN32
#pragma pack()
#endif

    constexpr const auto ICMP_ECHO_REPLY = 0;
    constexpr const auto ICMP_DEST_UNREACH = 3;
    constexpr const auto ICMP_TTL_EXPIRE = 11;
    constexpr const auto ICMP_ECHO_REQUEST = 8;

    constexpr const auto ICMP_MIN = 8;
    constexpr const auto DEFAULT_PACKET_SIZE = 32;
    constexpr const auto DEFAULT_TTL = 30;
    constexpr const auto MAX_PING_DATA_SIZE = 1024;
    constexpr const auto MAX_PING_PACKET_SIZE = MAX_PING_DATA_SIZE + sizeof(iphdr);

    auto ip_checksum = [] (uint16_t * buffer, int size) -> uint16_t {
        uint32_t cksum = 0;

        // Sum all the words together, adding the final byte if size is odd
        while( size > 1 ) {
            cksum += *buffer++;
            size -= sizeof(uint16_t);
        }

        if( size )
            cksum += *(uint16_t *) buffer;

        // Do a little shuffling
        cksum = (cksum >> 16) + (cksum & 0xffff);
        cksum += (cksum >> 16);

        // Return the bitwise complement of the resulting mishmash
        return (uint16_t) (~cksum);
    };

    auto init_ping_packet = [&] (icmphdr * ihdr, int packet_size, int seq_no) {
        // Set up the packet's fields
        ihdr->type = ICMP_ECHO_REQUEST;
        ihdr->code = 0;
        ihdr->checksum = 0;
        ihdr->id = getpid();
        ihdr->seq = seq_no;
        ihdr->timestamp = clock_gettime_ns();

        // "You're dead meat now, packet!"
        const unsigned long int deadmeat = 0xDEADBEEF;
        char * datapart = (char *) ihdr + sizeof(icmphdr);
        int bytes_left = packet_size - sizeof(icmphdr);

        while( bytes_left > 0 ) {
            memcpy(datapart, &deadmeat, std::min(int(sizeof(deadmeat)),
                    bytes_left));
            bytes_left -= sizeof(deadmeat);
            datapart += sizeof(deadmeat);
        }

        // Calculate a checksum on the result
        ihdr->checksum = ip_checksum((uint16_t *) ihdr, packet_size);
    };

    auto send_ping = [] (int sd, const sockaddr_in & dst, icmphdr * send_buf, int packet_size) -> int {
        for(;;) {
            int bwrote = ::sendto(sd, (const char *) send_buf, packet_size, 0, (const sockaddr *) &dst, socklen_t(sizeof(dst)));

            if( bwrote == -1 ) {
                if( errno == EINTR )
                    continue;
                return -1;
            }

            if( bwrote == packet_size )
                break;

            return -1;
        }

        return 0;
    };

    auto recv_ping = [] (int sd, sockaddr_in & src, iphdr * recv_buf, int packet_size) -> int {
        socklen_t from_len = sizeof(src);

        for(;;) {
            int bread = ::recvfrom(sd, (char *) recv_buf, packet_size + sizeof(iphdr), 0, (sockaddr *) &src, &from_len);

            if( bread >= 0 )
                break;

            if( errno != EINTR )
                return -1;
        }

        return 0;
    };

    auto decode_reply = [&] (iphdr * reply, int bytes, sockaddr_in * from) -> int {
        // Skip ahead to the ICMP header within the IP packet
        auto header_len = 4 * reply->ihl;
        icmphdr * ihdr = (icmphdr *) ((char *) reply + header_len);

        // Make sure the reply is sane
        if( bytes < header_len + ICMP_MIN )
            return -1;

        if( ihdr->type != ICMP_ECHO_REPLY ) {
            if( ihdr->type != ICMP_TTL_EXPIRE ) {
                if( ihdr->type == ICMP_DEST_UNREACH ) {
                    std::cerr << "Destination unreachable" << std::endl;
                }
                else {
                    std::cerr << "Unknown ICMP packet type " << int(ihdr->type) <<
                            " received" << std::endl;
                }
                return -1;
            }
            // If "TTL expired", fall through.  Next test will fail if we
            // try it, so we need a way past it.
        }
        else if( ihdr->id != getpid() ) {
            // Must be a reply for another pinger running locally, so just
            // ignore it.
            return -2;
        }

        // Figure out how far the packet travelled
        int nHops = int(256 - reply->ttl);

        if( nHops == 192 ) {
            // TTL came back 64, so ping was probably to a host on the
            // LAN -- call it a single hop.
            nHops = 1;
        }
        else if( nHops == 128 ) {
            // Probably localhost
            nHops = 0;
        }

        // Okay, we ran the gamut, so the packet must be legal -- dump it
        std::cerr << std::endl << bytes << " bytes from " <<
                inet_ntoa(from->sin_addr) << ", icmp_seq " <<
                ihdr->seq << ", ";

        if( ihdr->type == ICMP_TTL_EXPIRE ) {
            std::cerr << "TTL expired." << std::endl;
            *addr = from->sin_addr;
            return 0;
        }
        else {
            std::cerr << nHops << " hop" << (nHops == 1 ? "" : "s");
            std::cerr << ", time: " << (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::nanoseconds(clock_gettime_ns())).count() - ihdr->timestamp) <<
                    "ms." << std::endl;
        }

        return -1;
    };

#if _WIN32
    base_socket dummy;
#endif

    // Figure out how big to make the ping packet
    int packet_size = DEFAULT_PACKET_SIZE;
    int ttl = DEFAULT_TTL;
    ttl = 1;

    sockaddr_in dst, src;
    dst.sin_addr.s_addr = 0x08080808; // google-public-dns-a.google.com
    dst.sin_family = AF_INET;

    int sd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if( sd == -1 )
        return -1;

#if _WIN32
    at_scope_exit( ::closesocket(sd) );
#else
    at_scope_exit( ::close(sd) );
#endif

    if( setsockopt(sd, IPPROTO_IP, IP_TTL, (const char *) &ttl, sizeof(ttl)) == -1 )
        return -1;

    int seq_no = 0;
    icmphdr * send_buf = (icmphdr *) alloca(packet_size);
    iphdr * recv_buf = (iphdr *) alloca(MAX_PING_PACKET_SIZE);

    init_ping_packet(send_buf, packet_size, seq_no);

    // Send the ping and receive the reply
    if( send_ping(sd, dst, send_buf, packet_size) < 0 )
        return -1;

    int result = -1;

    while( 1 ) {
        // Receive replies until we either get a successful read,
        // or a fatal error occurs.
        if( recv_ping(sd, src, recv_buf, MAX_PING_PACKET_SIZE) < 0 ) {
            // Pull the sequence number out of the ICMP header.  If
            // it's bad, we just complain, but otherwise we take
            // off, because the read failed for some reason.
            auto header_len = 4 * recv_buf->ihl;
            icmphdr * ihdr = (icmphdr *) ((char*)recv_buf + header_len);

            if( ihdr->seq != seq_no ) {
                std::cerr << "bad sequence number!" << std::endl;
                continue;
            }
            else {
                break;
            }
        }

        result = decode_reply(recv_buf, packet_size, &src);

        if( result != -2 ) {
            // Success or fatal error (as opposed to a minor error)
            // so take off.
            break;
        }
    }

    return result;
#elif 0 && __ANDROID__
    int sd = -1;

    if( (sd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
        return -1;

    at_scope_exit( ::close(sd) );

    ifreq ifs[40]; // Maximum number of interfaces to scan
    ifconf ifc;

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if( ioctl(sd, SIOCGIFCONF, &ifc) < 0 )
        return -1;

    // scan through interface list
    ifreq * ifr, * ifend = ifs + (ifc.ifc_len / sizeof(ifreq));

    for( ifr = ifc.ifc_req; ifr < ifend; ifr++ ) {
        if( ifr->ifr_addr.sa_family != AF_INET )
            continue;

#ifndef NDEBUG
        std::cerr << ifr->ifr_name << ": " << inet_ntoa(((sockaddr_in *) &ifr->ifr_addr)->sin_addr) << std::endl;
#   if QT_CORE_LIB && __ANDROID__
        qDebug().noquote().nospace() << ifr->ifr_name << ": " << inet_ntoa(((sockaddr_in *) &ifr->ifr_addr)->sin_addr);
#   endif
#endif

        // get interface name
        ifreq ifq;
        strncpy(ifq.ifr_name, ifr->ifr_name, sizeof(ifq.ifr_name));

        // check that the interface is up
        if( ioctl(sd, SIOCGIFFLAGS, &ifq) < 0 )
            continue;

        if( (ifq.ifr_flags & IFF_UP) != 0 ) {
            // get interface addr
            addr->s_addr = ((sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
            //return 0;//break;
        }
    }

    addr->s_addr = htonl(127 << 24 | 'd' << 16 | 'g' << 8 | 'w');

    return 0;
#elif __linux__ && !__ANDROID__
    struct route_info {
        struct in_addr dst;
        struct in_addr src;
        struct in_addr gateway;
        char if_name[IF_NAMESIZE];
    } rt_info;

    const size_t BUFSIZE = 8192;
    //char gateway[255];

    auto read_nl_sock = [] (int sock, char * buf, unsigned int seq_num, unsigned int pid) -> int {
        nlmsghdr * nlHdr;
        int readLen = 0, msgLen = 0;

        do {
            // Recieve response from the kernel
            if( (readLen = recv(sock, buf, BUFSIZE - msgLen, 0)) < 0 ) {
                perror("SOCK READ: ");
                return -1;
            }

            nlHdr = (nlmsghdr *) buf;

            // Check if the header is valid
#if __clang__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#endif
            if( (NLMSG_OK(nlHdr, readLen) == 0 )
                || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
                perror("Error in recieved packet");
                return -1;
            }
#if __clang__
#   pragma GCC diagnostic pop
#endif

            // Check if the its the last message
            if( nlHdr->nlmsg_type == NLMSG_DONE )
                break;

            // Else move the pointer to buffer appropriately
            buf += readLen;
            msgLen += readLen;

            // Check if its a multi part message
            if( (nlHdr->nlmsg_flags & NLM_F_MULTI) == 0 )
                break;

        } while( (nlHdr->nlmsg_seq != seq_num) || (nlHdr->nlmsg_pid != pid) );

        return msgLen;
    };

    // For parsing the route info returned
    auto parse_routes = [&] (struct nlmsghdr * nl_hdr) {
        rtmsg * rt_msg = (struct rtmsg *) NLMSG_DATA(nl_hdr);

        // If the route is not for AF_INET or does not belong to main routing table
        // then return.
        if( rt_msg->rtm_family != AF_INET || rt_msg->rtm_table != RT_TABLE_MAIN )
            return false;

        // get the rtattr field
        rtattr * rt_attr = (rtattr *) RTM_RTA(rt_msg);
        int rt_len = RTM_PAYLOAD(nl_hdr);

        while( RTA_OK(rt_attr, rt_len) ) {
            switch( rt_attr->rta_type ) {
                case RTA_OIF:
                    if_indextoname(*(int *) RTA_DATA(rt_attr), rt_info.if_name);
                    break;
                case RTA_GATEWAY:
                    rt_info.gateway.s_addr = *(u_int *) RTA_DATA(rt_attr);
                    break;
                case RTA_PREFSRC:
                    rt_info.src.s_addr = *(u_int *) RTA_DATA(rt_attr);
                    break;
                case RTA_DST:
                    rt_info.dst.s_addr = *(u_int *) RTA_DATA(rt_attr);
                    break;
            }

            rt_attr = RTA_NEXT(rt_attr, rt_len);
        }

        if( rt_info.dst.s_addr == 0 ) {
            //sprintf(gateway, "%s", (char *) inet_ntoa(rt_info.gateway));
            addr->s_addr = rt_info.gateway.s_addr;
            if( mask != nullptr )
                mask = 0;
            return true;
        }

        return false;
    };

    nlmsghdr * nl_msg;
    char msg_buf[BUFSIZE];

    int sock, len;
    unsigned int msg_seq = 0;

    if( (sock = ::socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0 )
        return -1;

    at_scope_exit( ::close(sock) );
    //memset(msg_buf, 0, BUFSIZE);

    // point the header and the msg structure pointers into the buffer
    nl_msg = (nlmsghdr *) msg_buf;

    // Fill in the nlmsg header
    nl_msg->nlmsg_len  = NLMSG_LENGTH(sizeof(rtmsg));  // Length of message.
    nl_msg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .

    nl_msg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
    nl_msg->nlmsg_seq   = msg_seq++;   // Sequence of the message packet.
    nl_msg->nlmsg_pid   = getpid();    // PID of process sending the request.

    // Send the request
    if( ::send(sock, nl_msg, nl_msg->nlmsg_len, 0) < 0 ) {
        //printf("Write To Socket Failed...\n");
        return -1;
    }

    // Read the response
    if( (len = read_nl_sock(sock, msg_buf, msg_seq, nl_msg->nlmsg_pid)) < 0 ) {
        //printf("Read From Socket Failed...\n");
        return -1;
    }

    // Parse and print the response
    //fprintf(stdout, "Destination\tGateway\tInterface\tSource\n");
#if __clang__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#endif
    for( ; NLMSG_OK(nl_msg, len); nl_msg = NLMSG_NEXT(nl_msg, len) ) {
        memset(&rt_info, 0, sizeof(rt_info));
        if( parse_routes(nl_msg) )
            return 0;
    }
#if __clang__
#   pragma GCC diagnostic pop
#endif

    return -1;
#elif __linux__ && __ANDROID__
    // parse /proc/net/route which is as follow:

    //Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
    //wlan0   0001A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0
    //eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0
    //wlan0   00000000        0101A8C0        0003    0       0       0       00000000        0       0       0
    //eth0    00000000        00000000        0001    0       0       1000    00000000        0       0       0
    //
    // One header line, and then one line by route by route table entry.

    // on android bionic parse /proc/net/route which is as follow:
    // Iface	Destination   Gateway 	      Flags	  RefCnt  Use	  Metric	Mask          MTU	  Window  IRTT
    // wlan0	0005A8C0      00000000	      0001	  0       0       0         00FFFFFF	  0       0       0

    /*FILE * f = fopen("/proc/net/route", "r");

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
#if __ANDROID__
                if( d != 0 && g == 0 ) { // default on android bionic
                    addr->s_addr = d;
                    fclose(f);
                    return 0;
                }
#endif
            }
        }
        line++;
    }
    // default route not found
    if( f != nullptr )
        fclose(f);

    return -1;*/

    FILE * fp = ::fopen("/proc/net/route", "r");

    if( fp == nullptr )
        return -1;

    at_scope_exit( ::fclose(fp) );

    char line[256];
    int count = 0;
    unsigned int lowest_metric = UINT_MAX;
    in_addr_t best_gw = 0, gw_mask = 0;
    bool found = false;

    while( fgets(line, sizeof(line), fp) != nullptr ) {
        if( count++ == 0 )
            continue;

        unsigned int net_x = 0;
        unsigned int mask_x = 0;
        unsigned int gw_x = 0;
        unsigned int metric = 0;
        unsigned int flags = 0;

        char name[16];
        name[0] = 0;

        int np = ::sscanf(line, "%15s\t%x\t%x\t%x\t%*s\t%*s\t%d\t%x",
                              name,
                              &net_x,
                              &gw_x,
                              &flags,
                              &metric,
                              &mask_x);

        if( np == 6 && (flags & IFF_UP) != 0 ) {
#if __ANDROID__
            if( !gw_x && net_x && mask_x && metric < lowest_metric ) {
                found = true;
                best_gw = net_x;
                gw_mask = mask_x;
                lowest_metric = metric;
            }
#endif

            if( !net_x && !mask_x && metric < lowest_metric ) {
                found = true;
                best_gw = gw_x;
                gw_mask = mask_x;
                lowest_metric = metric;
            }
        }
    }

    if( found ) {
        addr->s_addr = best_gw;

        if( mask != nullptr )
            mask->s_addr = gw_mask;

        return 0;
    }

    return -1;
#elif __APPLE__
    auto ROUNDUP = [] (auto a) {
        return ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long));
    };

#if 0
    // net.route.0.inet.dump.0.0 ?
    int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0, 0/*tableid*/};
#endif
    // net.route.0.inet.flags.gateway
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
    int s, seq, l, rtm_addrs, i;

    struct {
        struct rt_msghdr m_rtm;
        char       m_space[512];
    } m_rtmsg;

    char * cp = m_rtmsg.m_space;

    auto NEXTADDR = [&] (auto w, auto u) {
        if( rtm_addrs & w )
            l = sizeof(struct sockaddr);

        memmove(cp, &u, l);
        cp += l;
    };

    pid_t pid;
    struct sockaddr so_dst, so_mask;
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
        return -1;
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
                return -1;
    }

    if( gate != NULL ) {
        *addr = ((struct sockaddr_in *)gate)->sin_addr.s_addr;
        return 0;
    }
    return -1;
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
    CHAR subnetmaskValue[MAX_VALUE_LENGTH];
    DWORD subnetmaskValueLength = MAX_VALUE_LENGTH;
    DWORD subnetmaskValueType = REG_MULTI_SZ;
    bool done = false, done_mask = false;

    constexpr CONST CHAR networkCardsPath[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    constexpr CONST CHAR interfacesPath[] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
    constexpr CONST CHAR STR_SERVICENAME[] = "ServiceName";
    constexpr CONST CHAR STR_DHCPDEFAULTGATEWAY[] = "DhcpDefaultGateway";
    constexpr CONST CHAR STR_DEFAULTGATEWAY[] = "DefaultGateway";
    constexpr CONST CHAR STR_SUBNETMASK[] = "SubnetMask";

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

                        if( done ) {
                            if( RegQueryValueExA(interfaceKey,                            // Open registry key
                                                                STR_SUBNETMASK,           // Name of key to query
                                                                NULL,                     // Reserved - must be NULL
                                                                &subnetmaskValueType,     // Receives value type
                                                                (LPBYTE) subnetmaskValue, // Receives value
                                                                &gatewayValueLength)      // Receives value length in bytes
                                     == ERROR_SUCCESS )
                            {
                                // Check to make sure it's a string
                                if( (subnetmaskValueType == REG_MULTI_SZ || subnetmaskValueType == REG_SZ) && (subnetmaskValueLength > 1) ) {
                                    //printf("gatewayValue: %s\n", gatewayValue);
                                    done_mask = true;
                                }
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

        if( done_mask && mask != nullptr )
            mask->S_un.S_addr = inet_addr(subnetmaskValue);

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

    if( mask != nullptr )
        mask = 0;

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
#else
    errno = ENOSYS;

    return -1;
#endif
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
