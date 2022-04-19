/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifdef WAN_FAILOVER_SUPPORTED

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

#include <sysevent/sysevent.h>
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"

#include "secure_wrapper.h"
#include "safec_lib_common.h"

static int sysevent_fd;
static token_t sysevent_token;

static char mesh_wan_ifname[32]={0};

#define CCSP_SUBSYS     "eRT."
void* bus_handle = NULL ;
const char* component_id = "ccsp.netmonitor";

#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str)) 

#define REMOTEWAN_ROUTER_IPV6 "remotewan_router_ipv6"

#define PSM_MESH_WAN_IFNAME "dmsb.Mesh.WAN.Interface.Name"

int rtnl_receive(int fd, struct msghdr *msg, int flags)
{
    int len;

    do { 
        len = recvmsg(fd, msg, flags);
    } while (len < 0 && (errno == EINTR || errno == EAGAIN));

    if (len < 0) {
        perror("Netlink receive failed");
        return -errno;
    }

    if (len == 0) { 
        perror("EOF on netlink");
        return -ENODATA;
    }

    return len;
}

static int rtnl_recvmsg(int fd, struct msghdr *msg, char **answer)
{
    struct iovec *iov = msg->msg_iov;
    char *buf;
    int len;

    iov->iov_base = NULL;
    iov->iov_len = 0;

    len = rtnl_receive(fd, msg, MSG_PEEK | MSG_TRUNC);

    if (len < 0) {
        return len;
    }

    buf = malloc(len);

    if (!buf) {
        perror("malloc failed");
        return -ENOMEM;
    }

    iov->iov_base = buf;
    iov->iov_len = len;

    len = rtnl_receive(fd, msg, 0);

    if (len < 0) {
        free(buf);
        return len;
    }

    *answer = buf;

    return len;
}

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

    while (RTA_OK(rta, len)) {
        if (rta->rta_type <= max) {
            tb[rta->rta_type] = rta;
        }

        rta = RTA_NEXT(rta,len);
    }
}

static inline int rtm_get_table(struct rtmsg *r, struct rtattr **tb)
{
    __u32 table = r->rtm_table;

    if (tb[RTA_TABLE]) {
        table = *(__u32 *)RTA_DATA(tb[RTA_TABLE]);
    }

    return table;
}

void print_route(struct nlmsghdr* nl_header_answer)
{
    struct rtmsg* r = NLMSG_DATA(nl_header_answer);
    int len = nl_header_answer->nlmsg_len;
    struct rtattr* tb[RTA_MAX+1];
    char router_ip[512];

    len -= NLMSG_LENGTH(sizeof(*r));

    if (len < 0) {
        perror("Wrong message length");
        return;
    }
    
    parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);
   int table ;

    table = rtm_get_table(r, tb);
    
    if (r->rtm_family != AF_INET6 || table != RT_TABLE_MAIN) {
        return;
    }

    if (tb[RTA_DST]) {
        if ((r->rtm_dst_len != 24) && (r->rtm_dst_len != 16)) {
            return;
        }

        printf("%s/%u ", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_DST]), router_ip, sizeof(router_ip)), r->rtm_dst_len);

    } 
    if (tb[RTA_OIF]) {
        char if_nam_buf[IF_NAMESIZE];
        int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);

        if_indextoname(ifidx, if_nam_buf);

        if (tb[RTA_GATEWAY] && (strncmp(mesh_wan_ifname,if_nam_buf,sizeof(mesh_wan_ifname)-1) == 0 ) ) {
            inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), router_ip, sizeof(router_ip));

            printf("router_ip is %s\n",router_ip);
            if (router_ip[0] != '\0' && strlen(router_ip) !=0 )
            {
                sysevent_set(sysevent_fd, sysevent_token,REMOTEWAN_ROUTER_IPV6,router_ip,0);
            }
        }
    }
}

int open_netlink()
{
    struct sockaddr_nl saddr;

    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    if (sock < 0) {
        perror("Failed to open netlink socket");
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));

    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = getpid();
    saddr.nl_groups = RTMGRP_IPV6_ROUTE;

    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("Failed to bind to netlink socket");
        close(sock);
        return -1;
    }

    return sock;
}

int do_route_dump_requst(int sock)
{
    struct {
        struct nlmsghdr nlh;
        struct rtmsg rtm;
    } nl_request;

    nl_request.nlh.nlmsg_type = RTM_GETROUTE;
    nl_request.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nl_request.nlh.nlmsg_len = sizeof(nl_request);
    nl_request.nlh.nlmsg_seq = time(NULL);
    nl_request.rtm.rtm_family = AF_INET6;

    return send(sock, &nl_request, sizeof(nl_request), 0);
}

int get_route_dump_response(int sock)
{
    struct sockaddr_nl nladdr;
    struct iovec iov;
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    char *buf = NULL;

    int status = rtnl_recvmsg(sock, &msg, &buf);

    struct nlmsghdr *h = (struct nlmsghdr *)buf;
    int msglen = status;

    printf("Main routing table IPv6\n");

    while (NLMSG_OK(h, msglen)) {
        if (h->nlmsg_flags & NLM_F_DUMP_INTR) {
            fprintf(stderr, "Dump was interrupted\n");
            free(buf);
            return -1;
        }

        if (nladdr.nl_pid != 0) {
            continue;
        }

        if (h->nlmsg_type == NLMSG_ERROR) {
            perror("netlink reported error");
            free(buf);
        }

        print_route(h);

        h = NLMSG_NEXT(h, msglen);
    }
    free(buf);
    return status;
}

static int msg_handler(struct sockaddr_nl *nl, struct nlmsghdr *msg)
    {

        (void) nl ;
        if  ( msg->nlmsg_type == RTM_NEWROUTE )
        {
            printf("msg_handler: RTM_IPV6_ROUTE\n");
            struct rtmsg *rtm;
            struct rtattr *tb[RTA_MAX + 1];
           // struct ifaddrmsg *ifa;

           // ifa = NLMSG_DATA (msg);
            rtm = NLMSG_DATA (msg);
            int len =0;
            len = msg->nlmsg_len - NLMSG_LENGTH (sizeof (struct rtmsg));
            if (len < 0)
                return -1;
          
            memset (tb, 0, sizeof tb);
            parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);

               int table ;

            table = rtm_get_table(rtm, tb);
            if (rtm->rtm_family != AF_INET6 || table != RT_TABLE_MAIN) {
              //  if (r->rtm_family != AF_INET6 ) {
                return -1;
            }
            
            char router_ip[512];
            memset(router_ip,0,sizeof(router_ip));

            if (tb[RTA_OIF]) {
                char if_name[IF_NAMESIZE];
                int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);

                if_indextoname(ifidx, if_name);
               if (strncmp(mesh_wan_ifname,if_name,sizeof(mesh_wan_ifname)-1) == 0 ) {

                  /*  if (tb[IFA_ADDRESS])
                    {
                        inet_ntop (ifa->ifa_family, RTA_DATA (tb[IFA_ADDRESS]),router_ip, sizeof(router_ip));
                        printf ("IFA_ADDRESS is : %s\n",router_ip);
                    }*/
                    if (tb[RTA_GATEWAY]) {
                            inet_ntop(rtm->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), router_ip, sizeof(router_ip));
                            printf("RTA_GATEWAY address is : %s\n", router_ip);
                    }
                    if (router_ip[0] != '\0' && strlen(router_ip) != 0 )
                    {
                        printf("router ip is %s, interface is %s\n",router_ip,if_name);
                        sysevent_set(sysevent_fd, sysevent_token,REMOTEWAN_ROUTER_IPV6,router_ip,0);
                        sysevent_set(sysevent_fd, sysevent_token,"firewall-restart",NULL,0);
                    }
                }
            }
        }
        return 0;
    }

    int read_event(int sockint, int (*msg_handler)(struct sockaddr_nl *,
                                                   struct nlmsghdr *))
    {
        int status;
        int ret = 0;
        char buf[4096];
        struct iovec iov = { buf, sizeof buf };
        struct sockaddr_nl snl;
        struct msghdr msg = { (void*)&snl, sizeof snl, &iov, 1, NULL, 0, 0};
        struct nlmsghdr *h;

        status = recvmsg(sockint, &msg, 0);

        if(status < 0)
        {
            /* Socket non-blocking so bail out once we have read everything */
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                return ret;

            /* Anything else is an error */
            printf("read_netlink: Error recvmsg: %d\n", status);
            perror("read_netlink: Error: ");
            return status;
        }

        if(status == 0)
        {
            printf("read_netlink: EOF\n");
        }

        /* We need to handle more than one message per 'recvmsg' */
        for(h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int)status); 
        h = NLMSG_NEXT (h, status))
        {
            /* Finish reading */
            if (h->nlmsg_type == NLMSG_DONE)
                return ret;

            /* Message is some kind of error */
            if (h->nlmsg_type == NLMSG_ERROR)
            {
                printf("read_netlink: Message is an error - decode TBD\n");
                return -1; // Error
            }

            /* Call message handler */
            if(msg_handler)
            {
                ret = (*msg_handler)(&snl, h);
                if(ret < 0)
                {
                    printf("read_netlink: Message hander error %d\n", ret);
                    return ret;
                }
            }
            else
            {
                printf("read_netlink: Error NULL message handler\n");
                return -1;
            }
        }

        return ret;
    }
    
int main()
{
    sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "iproute_monitor", &sysevent_token);
    if (sysevent_fd < 0)
    {
        return -1;
    }
    int ret;
    char* pCfg = CCSP_MSG_BUS_CFG;

    ret = CCSP_Message_Bus_Init((char *)component_id, pCfg, &bus_handle, (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);

    if ( ret == -1 )
    {
        // Dbus connection error
        printf("DBUS connection error %d\n", CCSP_MESSAGE_BUS_CANNOT_CONNECT);
        bus_handle = NULL;
        return -1;
    }

   char *pStr = NULL;
   int rc = -1;
   errno_t  safec_rc  = -1;

   rc = PSM_VALUE_GET_STRING(PSM_MESH_WAN_IFNAME,pStr);
   if(rc == CCSP_SUCCESS && pStr != NULL){

         safec_rc = strcpy_s(mesh_wan_ifname, sizeof(mesh_wan_ifname),pStr);
         ERR_CHK(safec_rc);
         Ansc_FreeMemory_Callback(pStr);
         pStr = NULL;
   }

   printf("PSM_MESH_WAN_IFNAME is %s\n",mesh_wan_ifname);

    int nl_sock = open_netlink();

    if (do_route_dump_requst(nl_sock) < 0) {
        perror("Failed to perfom request");
        close(nl_sock);
        return -1;
    }

    get_route_dump_response(nl_sock);

    while (1)
            read_event(nl_sock, msg_handler);

    close (nl_sock);
    return 0;
}
#endif