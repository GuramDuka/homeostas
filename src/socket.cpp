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
#include "socket.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
// There is no portable method to get the default route gateway.
// So below are four (or five ?) differents functions implementing this.
// Parsing /proc/net/route is for linux.
// sysctl is the way to access such informations on BSD systems.
// Many systems should provide route information through raw PF_ROUTE
// sockets.
// In MS Windows, default gateway is found by looking into the registry
// or by using GetBestRoute().
int get_default_gateway(in_addr * addr)
{
#if __linux__
    // parse /proc/net/route which is as follow:

    //Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
    //wlan0   0001A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0
    //eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0
    //wlan0   00000000        0101A8C0        0003    0       0       0       00000000        0       0       0
    //eth0    00000000        00000000        0001    0       0       1000    00000000        0       0       0
    //
    // One header line, and then one line by route by route table entry.

    // on android bionic parse /proc/net/route which is as follow:
    // Iface	Destination	Gateway 	Flags	RefCnt	Use	Metric	Mask        MTU	Window	IRTT
    // wlan0	0005A8C0	00000000	0001	0       0	0       00FFFFFF	0	0       0

    FILE * f = fopen("/proc/net/route", "r");

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

                if( d != 0 && g == 0 ) { // default on android bionic
                    addr->s_addr = d;
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
#else
    errno = ENOSYS;

    return -1;
#endif
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUFSIZE 8192
char gateway[255];

struct route_info {
    struct in_addr dstAddr;
    struct in_addr srcAddr;
    struct in_addr gateWay;
    char ifName[IF_NAMESIZE];
};

int readNlSock(int sockFd, char *bufPtr, unsigned int seqNum, unsigned int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

 do {
    /* Recieve response from the kernel */
        if ((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
            perror("SOCK READ: ");
            return -1;
        }

        nlHdr = (struct nlmsghdr *) bufPtr;

    /* Check if the header is valid */
        if ((NLMSG_OK(nlHdr, readLen) == 0)
            || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
            perror("Error in recieved packet");
            return -1;
        }

    /* Check if the its the last message */
        if (nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        } else {
    /* Else move the pointer to buffer appropriately */
            bufPtr += readLen;
            msgLen += readLen;
        }

    /* Check if its a multi part message */
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
           /* return if its not */
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
    return msgLen;
}
/* For printing the routes. */
void printRoute(struct route_info *rtInfo)
{
    char tempBuf[512];

/* Print Destination address */
    if (rtInfo->dstAddr.s_addr != 0)
        strcpy(tempBuf,  inet_ntoa(rtInfo->dstAddr));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\t", tempBuf);

/* Print Gateway address */
    if (rtInfo->gateWay.s_addr != 0)
        strcpy(tempBuf, (char *) inet_ntoa(rtInfo->gateWay));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\t", tempBuf);

    /* Print Interface Name*/
    fprintf(stdout, "%s\t", rtInfo->ifName);

    /* Print Source address */
    if (rtInfo->srcAddr.s_addr != 0)
        strcpy(tempBuf, inet_ntoa(rtInfo->srcAddr));
    else
        sprintf(tempBuf, "*.*.*.*\t");
    fprintf(stdout, "%s\n", tempBuf);
}

void printGateway()
{
    printf("%s\n", gateway);
}
/* For parsing the route info returned */
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

    rtMsg = (struct rtmsg *) NLMSG_DATA(nlHdr);

/* If the route is not for AF_INET or does not belong to main routing table
then return. */
    if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

/* get the rtattr field */
    rtAttr = (struct rtattr *) RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
        switch (rtAttr->rta_type) {
        case RTA_OIF:
            if_indextoname(*(int *) RTA_DATA(rtAttr), rtInfo->ifName);
            break;
        case RTA_GATEWAY:
            rtInfo->gateWay.s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        case RTA_PREFSRC:
            rtInfo->srcAddr.s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        case RTA_DST:
            rtInfo->dstAddr .s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        }
    }
    //printf("%s\n", inet_ntoa(rtInfo->dstAddr));

    if (rtInfo->dstAddr.s_addr == 0)
        sprintf(gateway, (char *) inet_ntoa(rtInfo->gateWay));
    //printRoute(rtInfo);

    return;
}

int main_()
{
    struct nlmsghdr *nlMsg;
    //struct rtmsg *rtMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];

    int sock, len;
    unsigned int msgSeq = 0;

/* Create Socket */
    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
        perror("Socket Creation: ");

    memset(msgBuf, 0, BUFSIZE);

/* point the header and the msg structure pointers into the buffer */
    nlMsg = (struct nlmsghdr *) msgBuf;
    //rtMsg = (struct rtmsg *) NLMSG_DATA(nlMsg);

/* Fill in the nlmsg header*/
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));  // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid();    // PID of process sending the request.

/* Send the request */
    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        printf("Write To Socket Failed...\n");
        return -1;
    }

/* Read the response */
    if ((len = readNlSock(sock, msgBuf, msgSeq, (unsigned int) getpid())) < 0) {
        printf("Read From Socket Failed...\n");
    return -1;
    }
/* Parse and print the response */
    rtInfo = (struct route_info *) malloc(sizeof(struct route_info));
//fprintf(stdout, "Destination\tGateway\tInterface\tSource\n");
    for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(nlMsg, rtInfo);
    }
    free(rtInfo);
    close(sock);

    printGateway();
    return 0;
}
