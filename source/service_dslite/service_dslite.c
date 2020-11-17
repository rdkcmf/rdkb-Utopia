/*#######################################################################
# Copyright 2017-2019 ARRIS Enterprises, LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>
#include "time.h"

#define PROG_NAME       "SERVICE-DSLITE"
#define ER_NETDEVNAME   "erouter0"
#define TNL_NETDEVNAME  "ipip6tun0"

/*
 * XXX:
 * no idea why COSA_DML_DEVICE_MODE_DeviceMode is 1, and 2, 3, 4 for IPv4/IPv6/DS
 * and sysevent last_erouter_mode use 0, 1, 2, 3 instead.
 * let's just follow the last_erouter_mode. :-(
 */
enum wan_rt_mod {
    WAN_RTMOD_BRIDGE, //COSA_DML_DEVICE_MODE_Bridge
    WAN_RTMOD_IPV4, // COSA_DML_DEVICE_MODE_Ipv4
    WAN_RTMOD_IPV6, // COSA_DML_DEVICE_MODE_Ipv6
    WAN_RTMOD_DS,   // COSA_DML_DEVICE_MODE_Dualstack
    WAN_RTMOD_UNKNOW,
};

struct serv_dslite {
    int             sefd;
    int             setok;
    char            ifname[IFNAMSIZ];
    enum wan_rt_mod rtmod;
};

struct cmd_op {
    const char      *cmd;
    int             (*exec)(struct serv_dslite *sd);
    const char      *desc;
};

static int serv_dslite_init(struct serv_dslite *sd);
static int serv_dslite_term(struct serv_dslite *sd);
static int dslite_start(struct serv_dslite *sd);
static int dslite_stop(struct serv_dslite *sd);
static int dslite_restart(struct serv_dslite *sd);
static int dslite_clear_status(struct serv_dslite *sd);
static int route_config(struct serv_dslite *sd);
static int route_deconfig(struct serv_dslite *sd);

static struct cmd_op cmd_ops[] = {
    {"start",       dslite_start,      "start service dslite"},
    {"stop",        dslite_stop,       "stop service dslite"},
    {"restart",     dslite_restart,    "restart service dslite"},
    {"clear",       dslite_clear_status,  "clear dslite status"},
};

static sem_t *sem = NULL;
#define SEM_WAIT {if(sem) sem_wait(sem);}
#define SEM_POST {if(sem) sem_post(sem);}

void _get_shell_output(char * cmd, char * out, int len)
{
    FILE * fp;
    char   buf[256];
    char * p;

    memset(buf, 0 , sizeof(buf));
    fp = popen(cmd, "r");
    if (fp)
    {
        fgets(buf, sizeof(buf), fp);
        /*we need to remove the \n char in buf*/
        if ((p = strchr(buf, '\n'))) *p = 0;
        strncpy(out, buf, len-1);
        pclose(fp);
    }
}

static int route_config(struct serv_dslite *sd)
{
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "ip rule add iif %s lookup all_lans", sd->ifname);
    vsystem(cmd);

    snprintf(cmd, sizeof(cmd), "ip rule add oif %s lookup erouter", sd->ifname);
    vsystem(cmd);

    return 0;
}

static int route_deconfig(struct serv_dslite *sd)
{
    char cmd[256] = {0};
    char val[64];

    memset(val, 0, sizeof(val));
    sysevent_get(sd->sefd, sd->setok, "current_wan_ipaddr", val, sizeof(val));

    snprintf(cmd, sizeof(cmd), "ip rule del from %s lookup all_lans", val);
    vsystem(cmd);

    snprintf(cmd, sizeof(cmd), "ip rule del from %s lookup erouter", val);
    vsystem(cmd);

    snprintf(cmd, sizeof(cmd), "ip rule del iif %s lookup all_lans", sd->ifname);
    vsystem(cmd);

    snprintf(cmd, sizeof(cmd), "ip rule del oif %s lookup erouter", sd->ifname);
    vsystem(cmd);

    return 0;
}

static int serv_dslite_init(struct serv_dslite *sd)
{
    char buf[32];

    if ((sd->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT,
                    SE_VERSION, PROG_NAME, &sd->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }

    syscfg_get(NULL, "last_erouter_mode", buf, sizeof(buf));
    switch (atoi(buf)) {
    case 0:
        sd->rtmod = WAN_RTMOD_BRIDGE;
        break;
    case 1:
        sd->rtmod = WAN_RTMOD_IPV4;
        break;
    case 2:
        sd->rtmod = WAN_RTMOD_IPV6;
        break;
    case 3:
        sd->rtmod = WAN_RTMOD_DS;
        break;
    default:
        fprintf(stderr, "%s: unknow RT mode (last_erouter_mode)\n", __FUNCTION__);
        sd->rtmod = WAN_RTMOD_UNKNOW;
        break;
    }

    snprintf(sd->ifname, sizeof(sd->ifname), "%s", ER_NETDEVNAME);

    return 0;
}

static int serv_dslite_term(struct serv_dslite *sd)
{
    sysevent_close(sd->sefd, sd->setok);
    return 0;
}

static struct event_base *exit_base;
static struct in6_addr *in6_addrs=NULL;

static void
dns_cb(int result, char type, int count, int ttl,
    void *addresses, void *arg)
{
    int *dns_ttl = arg;

    if (result != DNS_ERR_NONE) {
        in6_addrs = NULL;
#ifdef DEBUG
        fprintf(stderr, "Unexpected result %d \n", result);
#endif
        goto OUT;
    }

    if(type==DNS_IPv6_AAAA)
    {
        in6_addrs = malloc(sizeof(struct in6_addr));
        memcpy(in6_addrs, addresses, sizeof(struct in6_addr));
        *dns_ttl = ttl;
    }
    else
    {
#ifdef DEBUG
        fprintf(stderr, "Bad type %d \n", type);
#endif
    }
OUT:
    event_base_loopexit(exit_base, NULL);
    return;
}

static struct in6_addr *
dslite_resolve_fqdn_to_ipv6addr(const char *name, unsigned int *dnsttl, const char *nameserver)
{
    struct event_base *base;
    struct evdns_base *dns_base = NULL;
    struct evdns_request* dnsreq;
    char buf[128] = {0};

    in6_addrs = NULL;
    base = event_base_new();
    exit_base = base;
    dns_base = evdns_base_new(base, 0);

    snprintf(buf, sizeof(buf), "%s", nameserver);
    evdns_base_nameserver_ip_add(dns_base, buf);
    dnsreq = evdns_base_resolve_ipv6(dns_base, name, DNS_QUERY_NO_SEARCH, dns_cb, dnsttl);

    /*Wait for the DNS resolution done*/
    if(dnsreq != NULL)
    {
        event_base_dispatch(base);
    }

    event_base_free(base);
    evdns_base_free(dns_base, 0);

    return in6_addrs;
}

static int dslite_start(struct serv_dslite *sd)
{
    char val[64];
    char buf[128];
    char ipv6_str[INET6_ADDRSTRLEN] = {0};
    char DSLITE_AFTR[256];
    char dslite_tnl_ip[64];
    char resolved_ipv6[64];
    char gw_ipv6[64];
    struct in6_addr *addrp;
    struct in6_addr v6_addr;
    unsigned int dnsttl = 0;
    char cmd[256] = {0};
    int dhcp_mode = -1;
    char rule[256] = {0};
    char rule2[256] = {0};
    char return_buffer[256] = {0};
    FILE *fptmp = NULL;
    char dslite_ipv6_frag_enable[64] = {0};

    memset(val, 0, sizeof(val));
    memset(buf, 0, sizeof(buf));

    SEM_WAIT
    syscfg_get(NULL,  "dslite_enable", val, sizeof(val));
    syscfg_get(NULL,  "dslite_active_1", buf, sizeof(buf));
    if(strcmp(val, "1")!=0 || strcmp(buf, "1")!=0)//Either DSLite not enabled or tunnel not enabled
    {
        fprintf(stderr, "%s: DSLite not enabled, can't be started\n", __FUNCTION__);
        SEM_POST
        return 1;
    }

    sysevent_get(sd->sefd, sd->setok, "dslite_service-status", val, sizeof(val));

    if(strcmp(val, "started") ==0)
    {
        fprintf(stderr, "%s: DSLite is already started, exit without doing anything !\n", __FUNCTION__);
        SEM_POST
        return 0;
    }

    if(sd->rtmod != WAN_RTMOD_IPV6 && sd->rtmod != WAN_RTMOD_DS)
    {
        fprintf(stderr, "%s: GW mode is not correct, DSLite can't be started\n", __FUNCTION__);
        SEM_POST
        return 1;
    }

    memset(val, 0, sizeof(val));
    memset(dslite_tnl_ip, 0, sizeof(dslite_tnl_ip));
    memset(resolved_ipv6, 0, sizeof(resolved_ipv6));
    memset(gw_ipv6, 0, sizeof(gw_ipv6));

    /*get the WAN side IPv6 global address */
    sysevent_get(sd->sefd, sd->setok, "tr_"ER_NETDEVNAME"_dhcpv6_client_v6addr", val, sizeof(val));
    strcpy(gw_ipv6, val);
    fprintf(stderr, "%s: The GW IPv6 address is %s\n", __FUNCTION__, gw_ipv6);
    if (strlen(val)==0)
    {
        fprintf(stderr, "%s: GW IPv6 address is not ready, DSLite can't be started\n", __FUNCTION__);
        SEM_POST
        return 1;
    }
    val[0] = '4';
    val[1] = '0';
    strcpy(dslite_tnl_ip, val);//Follow TS9.1, the dslite tunnel interface IPv6 address starts with "40"
    fprintf(stderr, "%s: The dslite tunnel interface IPv6 address is %s\n", __FUNCTION__, dslite_tnl_ip);

    /* do start */
    sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "starting", 0);

    memset(val, 0, sizeof(val));
    memset(buf, 0, sizeof(buf));
    syscfg_get(NULL,  "dslite_mode_1", val, sizeof(val));
    syscfg_get(NULL,  "dslite_addr_type_1", buf, sizeof(buf));

    if(val != NULL)
    {
        memset(DSLITE_AFTR, 0, sizeof(DSLITE_AFTR));
        if(strcmp(val,"1") == 0)//AFTR got from DCHP mode
        {
            dhcp_mode = 1;
            syscfg_get(NULL, "dslite_addr_fqdn_1", DSLITE_AFTR, sizeof(DSLITE_AFTR));
        }
        else if(strcmp(val,"2") == 0) //AFTR got from static mode
        {
            dhcp_mode = 0;
            if(strcmp(buf, "1")==0)
                syscfg_get(NULL, "dslite_addr_fqdn_1", DSLITE_AFTR, sizeof(DSLITE_AFTR));
            else if(strcmp(buf, "2")==0)
                syscfg_get(NULL, "dslite_addr_ipv6_1", DSLITE_AFTR, sizeof(DSLITE_AFTR));
            else
                fprintf(stderr, "%s: Wrong value of dslite prefered address type\n", __FUNCTION__);
        }
        else
            fprintf(stderr, "%s: Wrong value of dslite address provision mode\n", __FUNCTION__);
    }
    if(strlen(DSLITE_AFTR)==0)
    {
        sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "error", 0);
        fprintf(stderr, "%s: AFTR address is NULL, exit!\n", __FUNCTION__);
        SEM_POST
        return 1;
    }
    fprintf(stderr, "%s: AFTR address is %s\n", __FUNCTION__, DSLITE_AFTR);

    /*Filter and store the WAN public IPv6 DNS server to a separate file */
    vsystem("cat /tmp/resolv.conf | grep nameserver | grep : | grep -v \"nameserver ::1\" | awk '/nameserver/{print $2}' > /tmp/ipv6_dns_server.conf");
    fptmp = fopen("/tmp/ipv6_dns_server.conf", "r");
    if(fptmp)
    {
        memset(buf, 0, sizeof(buf));
        if (fgets(buf,sizeof(buf), fptmp)!=NULL)
        {
            int i=strlen(buf);
            if(buf[i-1]=='\n')
            {
                buf[i-1]=0;
            }
        }
        fclose(fptmp);
    }
    else
    {
        sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "error", 0);
        fprintf(stderr, "%s: IPv6 DNS server isn't present !\n", __FUNCTION__);
        SEM_POST
        return 1;
    }

    //Use the above IPv6 nameserver to do DNS resolution for AFTR
    if(inet_pton(AF_INET6, DSLITE_AFTR, &v6_addr)==1)/*IPv6 address format, no need to do DNS resolution*/
    {
        strcpy(resolved_ipv6, DSLITE_AFTR);
        dnsttl = 0;
    }
    else /*domain format, need to do DNS resolution*/
    {
        addrp = dslite_resolve_fqdn_to_ipv6addr(DSLITE_AFTR, &dnsttl, buf);
        if(addrp)
        {
            strcpy(resolved_ipv6, inet_ntop(AF_INET6, addrp, ipv6_str, INET6_ADDRSTRLEN));
            free(addrp);/* free the memory that dslite_resolve_fqdn_to_ipv6addr had allocated*/
        }
        else
            memset(resolved_ipv6, 0, sizeof(resolved_ipv6));
    }

    if(strlen(resolved_ipv6))
    {
        if(strcmp(resolved_ipv6, "::")==0)
        {
            sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "error", 0);
            fprintf(stderr, "%s: AFTR DNSv6 resolution failed as got NULL IPv6 address(::), EXIT !\n", __FUNCTION__);
            SEM_POST
            return 1;
        }

        /*Store the time of DNS resolution and TTL value for dns_refresh process*/
        memset(buf, 0, sizeof(buf));
        memset(val, 0, sizeof(val));
        syscfg_set(NULL, "dslite_aftr_resolved_1", resolved_ipv6);
        sprintf(buf, "%lu", time(NULL));
        syscfg_set(NULL, "dslite_dns_time_1", buf);
        sprintf(val, "%u", dnsttl);
        syscfg_set(NULL, "dslite_dns_ttl_1", val);

        fprintf(stderr, "%s: Resolved AFTR address is %s, time=%s DNS-TTL=%d\n", __FUNCTION__, resolved_ipv6, buf, dnsttl);
    }
    else
    {
        sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "error", 0);
        fprintf(stderr, "%s: DNS resolution failed for unknown reason, EXIT !\n", __FUNCTION__);
        SEM_POST
        return 1;
    }

    //Stop WAN IPv4 service
    route_deconfig(sd);
    vsystem("service_wan dhcp-release");
    vsystem("service_wan dhcp-stop");
    sysevent_set(sd->sefd, sd->setok, "current_wan_ipaddr", "0.0.0.0", 0);

    //Setup the IPv4-in-IPv6 tunnel
    snprintf(cmd, sizeof(cmd), "ip -6 tunnel add %s mode ip4ip6 remote %s local %s dev %s encaplimit none", TNL_NETDEVNAME, resolved_ipv6, gw_ipv6, ER_NETDEVNAME);
    vsystem(cmd);

    //activate tunnel
    snprintf(cmd, sizeof(cmd), "ip link set dev %s txqueuelen 1000 up",TNL_NETDEVNAME);
    vsystem(cmd);

    //set IPv6 address to tunnel interface
    snprintf(cmd, sizeof(cmd), "ip -6 addr add %s dev %s", dslite_tnl_ip, TNL_NETDEVNAME);
    vsystem(cmd);

    //clear the GW IPv4 address(in case of IPv4 address not released successfully)
    snprintf(cmd, sizeof(cmd), "ip -4 addr flush %s",ER_NETDEVNAME);
    vsystem(cmd);

    //set default gateway through the tunnel in GW specific routing table
    snprintf(cmd, sizeof(cmd), "ip route add default dev %s",TNL_NETDEVNAME);
    vsystem(cmd);

    //save tunnel interface here in case IPv4 functions use it, like IGMP proxy
    syscfg_set(NULL,  "dslite_tunnel_interface_1", TNL_NETDEVNAME);
    syscfg_set(NULL,  "dslite_tunneled_interface_1", ER_NETDEVNAME);

    //if GW is the IPv6 only mode, we need to start the LAN to WAN IPv4 function
    if(sd->rtmod == WAN_RTMOD_IPV6)
    {
        //Enable the IPv4 forwarding
        vsystem("echo 1 > /proc/sys/net/ipv4/ip_forward");
        //Start Upnp & IGMP proxy
        vsystem("/etc/utopia/service.d/service_igd.sh lan-status");
        vsystem("/etc/utopia/service.d/service_mcastproxy.sh lan-status");
    }
    else
    {
        //Restart the LAN side DHCPv4 server & DNS proxy
        vsystem("systemctl stop dnsmasq.service");
        vsystem("systemctl start dnsmasq.service");
        // Restart IGMP proxy if in dual stack mode
        vsystem("/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
    }

    //Add the firewall rule for DSLite tunnel interface
    memset(return_buffer, 0, sizeof(return_buffer));
    snprintf(rule, sizeof(rule), "-I FORWARD -o %s -j ACCEPT\n", TNL_NETDEVNAME);
    sysevent_set_unique(sd->sefd, sd->setok, "GeneralPurposeFirewallRule", rule, return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok,"dslite_rule_sysevent_id_1",return_buffer,0);

    memset(return_buffer, 0, sizeof(return_buffer));
    snprintf(rule, sizeof(rule), "-I FORWARD -i %s -j ACCEPT\n", TNL_NETDEVNAME);
    sysevent_set_unique(sd->sefd, sd->setok, "GeneralPurposeFirewallRule", rule, return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok,"dslite_rule_sysevent_id_2",return_buffer,0);

    //TCPMSS Clamping for DSLite
    memset(val, 0, sizeof(val));
    memset(buf, 0, sizeof(buf));
    memset(return_buffer, 0, sizeof(return_buffer));
    syscfg_get(NULL,  "dslite_mss_clamping_enable_1", val, sizeof(val));
    if(strcmp(val, "1")==0)
    {
        syscfg_get(NULL,  "dslite_tcpmss_1", buf, sizeof(buf));
        if(atoi(buf) <= 1460)
        {
            snprintf(rule, sizeof(rule), "-I FORWARD -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %s\n", TNL_NETDEVNAME, buf);
            snprintf(rule2, sizeof(rule2), "-I FORWARD -i %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %s\n", TNL_NETDEVNAME, buf);
        }
        else
        {
            snprintf(rule, sizeof(rule), "-I FORWARD -o %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n", TNL_NETDEVNAME);
            snprintf(rule2, sizeof(rule2), "-I FORWARD -i %s -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n", TNL_NETDEVNAME);
        }
        sysevent_set_unique(sd->sefd, sd->setok, "GeneralPurposeMangleRule", rule, return_buffer, sizeof(return_buffer));
        sysevent_set(sd->sefd, sd->setok,"dslite_rule_sysevent_id_3",return_buffer,0);
        memset(return_buffer, 0, sizeof(return_buffer));
        sysevent_set_unique(sd->sefd, sd->setok, "GeneralPurposeMangleRule", rule2, return_buffer, sizeof(return_buffer));
        sysevent_set(sd->sefd, sd->setok,"dslite_rule_sysevent_id_4",return_buffer,0);
        syscfg_set(NULL, "dslite_tcpmss_prev_1", buf);//Save the TCPMSS value added into the firewall rule
    }

    vsystem("sysevent set firewall-restart");//restart firewall to install the rules
    vsystem("conntrack_flush");
    sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "started", 0);
    syscfg_set(NULL,  "dslite_status_1", "1");
    syscfg_set(NULL,  "dslite_addr_inuse_1", resolved_ipv6);
    if(dhcp_mode==1)
        syscfg_set(NULL,  "dslite_origin_1", "1");//DHCPv6
    else
        syscfg_set(NULL,  "dslite_origin_1", "2");//Static

    //add for clm_48041 BEGIN
    syscfg_get(NULL,  "dslite_ipv6_frag_enable_1", dslite_ipv6_frag_enable, sizeof(dslite_ipv6_frag_enable));

    if(strcmp(dslite_ipv6_frag_enable, "1")!=0)
    {
        vsystem("echo 0 >/proc/arris/dslite_ipv6_frag");
    }
    else
    {
        vsystem("echo 1 >/proc/arris/dslite_ipv6_frag");
    }
    //add for clm_48041 END

    SEM_POST
    return 0;
}

static int dslite_stop(struct serv_dslite *sd)
{
    char cmd[256] = {0};
    char remote_addr[64];
    char local_addr[64];
    char val[64];
    char buf[64];
    char return_buffer[256] = {0};

    SEM_WAIT

    /*We need to clear the DSLite temporary status buffers at first, either the following
    DSLite stop operations are executed or not*/
    dslite_clear_status(sd);

    memset(val, 0, sizeof(val));
    sysevent_get(sd->sefd, sd->setok, "dslite_service-status", val, sizeof(val));

    if(strcmp(val, "stopped") == 0)
    {
        fprintf(stderr, "%s: DSLite is already stopped, exit without doing anything!\n", __FUNCTION__);
        SEM_POST
        return 0;
    }

    /* do stop */
    sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "stopping", 0);

    //Stop the IPv4-in-IPv6 tunnel
    memset(remote_addr, 0, sizeof(remote_addr));
    memset(local_addr, 0, sizeof(local_addr));
    snprintf(cmd, sizeof(cmd), "ip -6 tunnel show | grep %s | awk '/remote/{print $4}'", TNL_NETDEVNAME);
    _get_shell_output(cmd, remote_addr, sizeof(remote_addr));
    fprintf(stderr, "%s: Remote address is %s\n", __FUNCTION__, remote_addr);
    snprintf(cmd, sizeof(cmd), "ip -6 tunnel show | grep %s | awk '/remote/{print $6}'", TNL_NETDEVNAME);
    _get_shell_output(cmd, local_addr, sizeof(local_addr));
    fprintf(stderr, "%s: Local address is %s\n", __FUNCTION__, local_addr);

    if(strlen(remote_addr) != 0 && strlen(local_addr) != 0)
    {
        /* Change for CLM-51561. The ip -6 tunnel del cmd cause that zebra exit abnormally and generate coredump file.
           So stop the zebra before it. */
        memset(buf, 0, sizeof(buf));
        snprintf(cmd, sizeof(cmd), "ps > /var/tmp/process; cat /var/tmp/process | grep zebra");
        _get_shell_output(cmd, buf, sizeof(buf));
        if(strlen(buf) > 0)
        {
            vsystem("systemctl stop arris-zebra.service");
            vsystem("rm -rf /var/tmp/process");
        }

        snprintf(cmd, sizeof(cmd), "ip -6 tunnel del %s mode ip4ip6 remote %s local %s dev %s encaplimit none", TNL_NETDEVNAME, remote_addr, local_addr, ER_NETDEVNAME);
        vsystem(cmd);

        /*The Zebra process will exit for unknown reason when execute the above tunnel delete operation.
        This may be a bug of Zebra SW package. But here we make a workaround to restart the zebra process if it's exited
        */
        memset(buf, 0, sizeof(buf));
        snprintf(cmd, sizeof(cmd), "ps > /var/tmp/process; cat /var/tmp/process | grep zebra");
        _get_shell_output(cmd, buf, sizeof(buf));
        if(strlen(buf) == 0)
        {
            vsystem("systemctl start arris-zebra.service");
            vsystem("rm -rf /var/tmp/process");
        }
    }
    else
    {
        sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "stopped", 0);
        fprintf(stderr, "%s: The tunnel is already deleted\n", __FUNCTION__);
        SEM_POST
        return 0;
    }

    if(sd->rtmod != WAN_RTMOD_IPV6)
    {
        //Start WAN IPv4 service
        route_config(sd);
        vsystem("service_wan dhcp-start");
    }

    //Restore default gateway route rule
    snprintf(cmd, sizeof(cmd), "ip route del default dev %s",TNL_NETDEVNAME);
    vsystem(cmd);

    //if GW is the IPv6 only mode, we need to shutdown the LAN to WAN IPv4 function
    if(sd->rtmod == WAN_RTMOD_IPV6)
    {
        //Disable the IPv4 forwarding
        vsystem("echo 0 > /proc/sys/net/ipv4/ip_forward");
        //Stop Upnp & IGMP proxy
        vsystem("/etc/utopia/service.d/service_igd.sh igd-stop");
        vsystem("/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-stop");
    }
    else
    {
        /*Restart the LAN side DHCPv4 server & DNS proxy */
        vsystem("systemctl stop dnsmasq.service");

        vsystem("systemctl start dnsmasq.service");
        // Restart IGMP proxy if in dual stack mode
        vsystem("/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
    }

    //Delete the firewall rule for DSLite tunnel interface
    memset(return_buffer, 0, sizeof(return_buffer));
    sysevent_get(sd->sefd, sd->setok, "dslite_rule_sysevent_id_1", return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok, return_buffer,"",0);

    memset(return_buffer, 0, sizeof(return_buffer));
    sysevent_get(sd->sefd, sd->setok, "dslite_rule_sysevent_id_2", return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok, return_buffer,"",0);

    /*Either the TCPMSS Clamping enabled or disabled, the firewall rule need to be deleted*/
    memset(return_buffer, 0, sizeof(return_buffer));
    sysevent_get(sd->sefd, sd->setok, "dslite_rule_sysevent_id_3", return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok, return_buffer,"",0);
    memset(return_buffer, 0, sizeof(return_buffer));
    sysevent_get(sd->sefd, sd->setok, "dslite_rule_sysevent_id_4", return_buffer, sizeof(return_buffer));
    sysevent_set(sd->sefd, sd->setok, return_buffer,"",0);

    vsystem("sysevent set firewall-restart");//restart firewall to install the rules
    vsystem("conntrack_flush");

    vsystem("echo 0 >/proc/arris/dslite_ipv6_frag");

    sysevent_set(sd->sefd, sd->setok, "dslite_service-status", "stopped", 0);

    SEM_POST
    return 0;
}

static int dslite_restart(struct serv_dslite *sd)
{
    dslite_stop(sd);
    dslite_start(sd);

    return 0;
}

static int dslite_clear_status(struct serv_dslite *sd)
{
    /*Clear the dslite status syscfg*/
    syscfg_set(NULL,  "dslite_status_1", "2");
    syscfg_unset(NULL,  "dslite_addr_inuse_1");
    syscfg_unset(NULL,  "dslite_origin_1");
    syscfg_unset(NULL,  "dslite_tunnel_interface_1");
    syscfg_unset(NULL,  "dslite_tunneled_interface_1");

    /*Clear the tmp buffer for DNS resolution*/
    syscfg_unset(NULL,  "dslite_aftr_resolved_1");
    syscfg_unset(NULL,  "dslite_dns_time_1");
    syscfg_unset(NULL,  "dslite_dns_ttl_1");
    return 0;
}

static void usage(void)
{
    int i;

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "    %s COMMAND \n", PROG_NAME);
    fprintf(stderr, "COMMANDS\n");
    for (i = 0; i < NELEMS(cmd_ops); i++)
        fprintf(stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
}

int main(int argc, char *argv[])
{
    int i;
    struct serv_dslite sd;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_dslite_init(&sd) != 0)
        exit(1);

    sem = sem_open(PROG_NAME,O_CREAT, 0666,1);

    /* execute commands */
    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] exec: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&sd) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_dslite_term(&sd) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}
