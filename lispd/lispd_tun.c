/*
 * lispd_tun.c
 *
 * This file is part of LISP Mobile Node Implementation.
 *
 * Copyright (C) 2012 Cisco Systems, Inc, 2012. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Please send any bug reports or fixes you make to the email address(es):
 *    LISP-MN developers <devel@lispmob.org>
 *
 * Based on code from Chris White <chris@logicalelegance.com>
 *
 * Written or modified by:
 *    Alberto Rodriguez Natal <arnatal@ac.upc.edu>
 */

#include "lispd_external.h"
#include "lispd_log.h"
#include "lispd_routing_tables_lib.h"
#include "lispd_tun.h"


int create_tun(
    char                *tun_dev_name,
    unsigned int        tun_receive_size,
    int                 tun_mtu,
    int                 *tun_receive_fd,
    int                 *tun_ifindex,
    uint8_t             **tun_receive_buf)
{

    struct ifreq ifr;
    int err = 0;
    int tmpsocket = 0;
    int flags = IFF_TUN | IFF_NO_PI; // Create a tunnel without persistence
    char *clonedev = CLONEDEV;


    /* Arguments taken by the function:
     *
     * char *dev: the name of an interface (or '\0'). MUST have enough
     *   space to hold the interface name if '\0' is passed
     * int flags: interface flags (eg, IFF_TUN etc.)
     */

    /* open the clone device */
    if( (*tun_receive_fd = open(clonedev, O_RDWR)) < 0 ) {
        LISPD_LOG(LISP_LOG_CRIT, "TUN/TAP: Failed to open clone device");
        exit_cleanup();
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, tun_dev_name, IFNAMSIZ - 1);

    // try to create the device
    if ((err = ioctl(*tun_receive_fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(*tun_receive_fd);
        LISPD_LOG(LISP_LOG_CRIT, "TUN/TAP: Failed to create tunnel interface, errno: ", LISPD_INTEGER(errno) );
        if (errno == 16){
            LISPD_LOG(LISP_LOG_CRIT, "Check no other instance of lispd is running. Exiting ...");
        }
        exit_cleanup();
    }

    // get the ifindex for the tun/tap
    tmpsocket = socket(AF_INET, SOCK_DGRAM, 0); // Dummy socket for the ioctl, type/details unimportant
    if ((err = ioctl(tmpsocket, SIOCGIFINDEX, (void *)&ifr)) < 0) {
        close(*tun_receive_fd);
        close(tmpsocket);
        LISPD_LOG(LISP_LOG_CRIT, "TUN/TAP: unable to determine ifindex for tunnel interface, errno: ", LISPD_INTEGER(errno ) );
        exit_cleanup();
    } else {
        LISPD_LOG(LISP_LOG_DEBUG_3, "TUN/TAP ifindex is: ", LISPD_INTEGER(ifr.ifr_ifindex) );
        *tun_ifindex = ifr.ifr_ifindex;

        // Set the MTU to the configured MTU
        ifr.ifr_ifru.ifru_mtu = tun_mtu;
        if ((err = ioctl(tmpsocket, SIOCSIFMTU, &ifr)) < 0) {
            close(tmpsocket);
            lispd_log_msg(LISP_LOG_CRIT, "TUN/TAP: unable to set interface MTU to %d, errno: %d.", tun_mtu, errno);
            exit_cleanup();
        } else {
            LISPD_LOG(LISP_LOG_DEBUG_1, "TUN/TAP mtu set to ", LISPD_INTEGER(tun_mtu) );
        }
    }


    close(tmpsocket);

    *tun_receive_buf = (uint8_t *)malloc(tun_receive_size);

    if (tun_receive_buf == NULL){
        LISPD_LOG(LISP_LOG_WARNING, "create_tun: Unable to allocate memory for tun_receive_buf: ", strerror(errno));
        return(BAD);
    }

    /* this is the special file descriptor that the caller will use to talk
     * with the virtual interface */
    LISPD_LOG(LISP_LOG_DEBUG_2, "Tunnel fd at creation is ", LISPD_INTEGER(*tun_receive_fd) );

    /*
    if (!tuntap_install_default_routes()) {
        return(FALSE);
    }*/

    return(GOOD);
}

/*
 * tun_bring_up_iface()
 *
 * Bring up interface
 */
int tun_bring_up_iface(char *tun_dev_name)
{
    struct ifinfomsg    *ifi = NULL;
    struct nlmsghdr     *nlh = NULL;
    char                sndbuf[4096];
    int                 retval = 0;
    int                 sockfd = 0;
    int                 tun_ifindex = 0;

    tun_ifindex = if_nametoindex (tun_dev_name);

    sockfd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

    if (sockfd < 0) {
        LISPD_LOG(LISP_LOG_ERR, "Failed to connect to netlink socket");
        return(BAD);
    }

    /*
     * Build the command
     */
    memset(sndbuf, 0, 4096);
    nlh = (struct nlmsghdr *)sndbuf;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    nlh->nlmsg_flags = NLM_F_REQUEST | (NLM_F_CREATE | NLM_F_REPLACE);
    nlh->nlmsg_type = RTM_SETLINK;

    ifi = (struct ifinfomsg *)(sndbuf + sizeof(struct nlmsghdr));
    ifi->ifi_family = AF_UNSPEC;
    ifi->ifi_type = IFLA_UNSPEC;
    ifi->ifi_index = tun_ifindex;
    ifi->ifi_flags = IFF_UP | IFF_RUNNING; // Bring it up
    ifi->ifi_change = 0xFFFFFFFF;

    retval = send(sockfd, sndbuf, nlh->nlmsg_len, 0);

    if (retval < 0) {
        LISPD_LOG(LISP_LOG_ERR, "send() failed ", strerror(errno));
        close(sockfd);
        return(BAD);
    }

    LISPD_LOG(LISP_LOG_DEBUG_1, "TUN interface UP.");
    close(sockfd);
    return(GOOD);
}

/*
 * tun_add_eid_to_iface()
 *
 * Add an EID to the TUN/TAP interface
 */
int tun_add_eid_to_iface(
    lisp_addr_t         eid_address,
    char                *tun_dev_name)
{
    struct rtattr       *rta = NULL;
    struct ifaddrmsg    *ifa = NULL;
    struct nlmsghdr     *nlh = NULL;
    char                sndbuf[4096];
    int                 retval = 0;
    int                 sockfd = 0;
    int                 tun_ifindex = 0;

    int                 addr_size = 0;
    int                 prefix_length = 0;

    tun_ifindex = if_nametoindex (tun_dev_name);

    sockfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

    if (sockfd < 0) {
        LISPD_LOG(LISP_LOG_ERR, "Failed to connect to netlink socket");
        return(BAD);
    }

    if (eid_address.afi == AF_INET){
        addr_size = sizeof(struct in_addr);
        prefix_length = 32;
    }else {
        addr_size = sizeof(struct in6_addr);
        prefix_length = 128;
    }

    /*
     * Build the command
     */
    memset(sndbuf, 0, 4096);
    nlh = (struct nlmsghdr *)sndbuf;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg) + sizeof(struct rtattr) + addr_size);
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
    nlh->nlmsg_type = RTM_NEWADDR;
    ifa = (struct ifaddrmsg *)(sndbuf + sizeof(struct nlmsghdr));

    ifa->ifa_prefixlen = prefix_length;
    ifa->ifa_family = eid_address.afi;
    ifa->ifa_index  = tun_ifindex;
    ifa->ifa_scope = RT_SCOPE_UNIVERSE;
    ifa->ifa_flags = 0; // Bring it up

    rta = (struct rtattr *)(sndbuf + sizeof(struct nlmsghdr) + sizeof(struct ifaddrmsg));
    rta->rta_type = IFA_LOCAL;
    rta->rta_len = sizeof(struct rtattr) + addr_size;
    memcopy_lisp_addr((void *)((char *)rta + sizeof(struct rtattr)),&eid_address);

    retval = send(sockfd, sndbuf, nlh->nlmsg_len, 0);

    if (retval < 0) {
        LISPD_LOG(LISP_LOG_ERR, "send() failed ", strerror(errno));
        close(sockfd);
        return(BAD);
    }

    LISPD_LOG(LISP_LOG_DEBUG_1, "added EID ", LISPD_EID(get_char_from_lisp_addr_t(eid_address)) ," to TUN interface.");
    close(sockfd);
    return(GOOD);
}

int set_tun_default_route_v4()
{

    /*
     * Assign route to 0.0.0.0/1 and 128.0.0.0/1 via tun interface
     */
    lisp_addr_t dest;
    lisp_addr_t *src = NULL;
    lisp_addr_t gw;
    uint32_t prefix_len = 0;
    uint32_t metric = 0;

    prefix_len = 1;
    metric = 0;

    get_lisp_addr_from_char("0.0.0.0",&gw);

#ifdef ROUTER
    if (default_out_iface_v4 != NULL){
       src = default_out_iface_v4->ipv4_address;
    }
#endif

    get_lisp_addr_from_char("0.0.0.0",&dest);

    add_route(AF_INET,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);


    get_lisp_addr_from_char("128.0.0.0",&dest);

    add_route(AF_INET,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);
    return(GOOD);
}


int set_tun_default_route_v6()
{

    /*
     * Assign route to ::/1 and 8000::/1 via tun interface
     */

    lisp_addr_t dest;
    lisp_addr_t *src = NULL;
    lisp_addr_t gw;
    uint32_t prefix_len = 0;
    uint32_t metric = 0;

    prefix_len = 1;
    metric = 512;

    get_lisp_addr_from_char("::",&gw);

#ifdef ROUTER
    if (default_out_iface_v6 != NULL){
        src = default_out_iface_v6->ipv6_address;
    }
#endif

    get_lisp_addr_from_char("::",&dest);

    add_route(AF_INET6,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);

    get_lisp_addr_from_char("8000::",&dest);

    add_route(AF_INET6,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);

    return(GOOD);
}


int del_tun_default_route_v6()
{

    /*
     * Assign route to ::/1 and 8000::/1 via tun interface
     */

    lisp_addr_t dest;
    lisp_addr_t *src = NULL;
    lisp_addr_t gw;
    uint32_t prefix_len = 0;
    uint32_t metric = 0;

    prefix_len = 1;
    metric = 512;

    get_lisp_addr_from_char("::",&gw);

    get_lisp_addr_from_char("::",&dest);

    del_route(AF_INET6,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);

    get_lisp_addr_from_char("8000::",&dest);

    del_route(AF_INET6,
            tun_ifindex,
            &dest,
            src,
            NULL,
            prefix_len,
            metric,
            RT_TABLE_MAIN);

    return(GOOD);
}


/*
 * Editor modelines
 *
 * vi: set shiftwidth=4 tabstop=4 expandtab:
 * :indentSize=4:tabSize=4:noTabs=true:
 */
