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
#include "safec_lib_common.h"

#define PROG_NAME       "SERVICE-DSLITE"
#define ER_NETDEVNAME   "erouter0"
#define TNL_NETDEVNAME  "ipip6tun0"
#define TNL_WANDEVNAME  "wan0"

#define SVC_DSLITE_LOG "/rdklogs/logs/svc_dslite_dbg.txt"
static FILE *fp_dslt_dbg;

/*
 * XXX:
 * no idea why COSA_DML_DEVICE_MODE_DeviceMode is 1, and 2, 3, 4 for IPv4/IPv6/DS
 * and sysevent last_erouter_mode use 0, 1, 2, 3 instead.
 * let's just follow the last_erouter_mode. :-(
 */
enum wan_rt_mod {
    WAN_RTMOD_BRIDGE,   // COSA_DML_DEVICE_MODE_Bridge
    WAN_RTMOD_IPV4,     // COSA_DML_DEVICE_MODE_Ipv4
    WAN_RTMOD_IPV6,     // COSA_DML_DEVICE_MODE_Ipv6
    WAN_RTMOD_DS,       // COSA_DML_DEVICE_MODE_Dualstack
    WAN_RTMOD_UNKNOW,
};

struct serv_dslite {
    int sefd;
    int setok;
    enum wan_rt_mod rtmod;
};

struct cmd_op {
    const char *cmd;
    int (*exec)(struct serv_dslite *sd);
    const char *desc;
};

static int dslite_start (struct serv_dslite *sd);
static int dslite_stop (struct serv_dslite *sd);
static int dslite_restart (struct serv_dslite *sd);
static int dslite_clear_status (struct serv_dslite *sd);

static const struct cmd_op cmd_ops[] = {
    {"start",   dslite_start,        "start service dslite"   },
    {"stop",    dslite_stop,         "stop service dslite"    },
    {"restart", dslite_restart,      "restart service dslite" },
    {"clear",   dslite_clear_status, "clear dslite status"    },
};

static sem_t *sem = NULL;

#define SEM_WAIT do { sem_wait(sem); } while (0)
#define SEM_POST do { sem_post(sem); } while (0)

static void _get_shell_output (char *cmd, char *buf, size_t len)
{
    FILE *fp;

    if (len > 0)
        buf[0] = 0;
    fp = popen (cmd, "r");
    if (fp == NULL)
        return;
    buf = fgets (buf, len, fp);
    pclose (fp);
    if ((len > 0) && (buf != NULL)) {
        len = strlen (buf);
        if ((len > 0) && (buf[len - 1] == '\n'))
            buf[len - 1] = 0;
    }
}

static void route_config (struct serv_dslite *sd)
{
    vsystem ("ip rule add iif " ER_NETDEVNAME " lookup all_lans" "; "
             "ip rule add oif " ER_NETDEVNAME " lookup erouter");
}

static void route_deconfig (struct serv_dslite *sd)
{
    char val[64];

    sysevent_get (sd->sefd, sd->setok, "current_wan_ipaddr", val, sizeof(val));

    vsystem ("ip rule del from %s lookup all_lans" "; "
             "ip rule del from %s lookup erouter" "; "
             "ip rule del iif " ER_NETDEVNAME " lookup all_lans" "; "
             "ip rule del oif " ER_NETDEVNAME " lookup erouter",
             val,
             val);
}

static int serv_dslite_init (struct serv_dslite *sd)
{
    char buf[12];

    if ((sd->sefd = sysevent_open (SE_SERV, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, PROG_NAME, &sd->setok)) < 0) {
        fprintf (fp_dslt_dbg, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    syscfg_get (NULL, "last_erouter_mode", buf, sizeof(buf));

    switch (atoi (buf)) {
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
            fprintf (fp_dslt_dbg, "%s: unknow RT mode (last_erouter_mode)\n", __FUNCTION__);
            sd->rtmod = WAN_RTMOD_UNKNOW;
            break;
    }

    if ((sem = sem_open (PROG_NAME, O_CREAT, 0666, 1)) == SEM_FAILED)
        return -1;

    return 0;
}

static int serv_dslite_term (struct serv_dslite *sd)
{
    sysevent_close (sd->sefd, sd->setok);

    return 0;
}

static struct event_base *exit_base;
static struct in6_addr *in6_addrs = NULL;

static void dns_cb (int result, char type, int count, int ttl, void *addresses, void *arg)
{
    int *dns_ttl = arg;

    if (result != DNS_ERR_NONE)
    {
        in6_addrs = NULL;
#ifdef DEBUG
        fprintf(fp_dslt_dbg, "Unexpected result %d \n", result);
#endif
        goto OUT;
    }

    if (type == DNS_IPv6_AAAA)
    {
        in6_addrs = malloc (sizeof(struct in6_addr));
        memcpy (in6_addrs, addresses, sizeof(struct in6_addr));
        *dns_ttl = ttl;
    }
    else
    {
#ifdef DEBUG
        fprintf (fp_dslt_dbg, "Bad type %d \n", type);
#endif
    }

OUT:
    event_base_loopexit (exit_base, NULL);
}

static struct in6_addr *dslite_resolve_fqdn_to_ipv6addr (const char *name, unsigned int *dnsttl, const char *nameserver)
{
    struct event_base *base;
    struct evdns_base *dns_base = NULL;
    struct evdns_request *dnsreq;

    in6_addrs = NULL;
    base = event_base_new();
    exit_base = base;
    dns_base = evdns_base_new (base, 0);

    evdns_base_nameserver_ip_add (dns_base, nameserver);
    dnsreq = evdns_base_resolve_ipv6 (dns_base, name, DNS_QUERY_NO_SEARCH, dns_cb, dnsttl);

    /* Wait for the DNS resolution done */
    if (dnsreq != NULL)
    {
        event_base_dispatch (base);
    }

    event_base_free (base);
    evdns_base_free (dns_base, 0);

    return in6_addrs;
}

static int get_aftr (char *DSLITE_AFTR, char *dslite_mode, char *dslite_addr_type, int size_aftr)
{
    int retvalue = -1;

    DSLITE_AFTR[0] = 0;

    if (strcmp (dslite_mode, "1") == 0) //AFTR got from DCHP mode
    {
        syscfg_get (NULL, "dslite_addr_fqdn_1", DSLITE_AFTR, size_aftr);
        retvalue = 1;
    }
    else if (strcmp (dslite_mode, "2") == 0) //AFTR got from static mode
    {
        if (strcmp (dslite_addr_type, "1") == 0)
            syscfg_get (NULL, "dslite_addr_fqdn_1", DSLITE_AFTR, size_aftr);
        else if (strcmp (dslite_addr_type, "2") == 0)
            syscfg_get (NULL, "dslite_addr_ipv6_1", DSLITE_AFTR, size_aftr);
        else
            fprintf (fp_dslt_dbg, "%s: Wrong value of dslite prefered address type\n", __FUNCTION__);

        retvalue = 0;
    }
    else
    {
        fprintf (fp_dslt_dbg, "%s: Wrong value of dslite address provision mode\n", __FUNCTION__);
    }

    return retvalue;
}

static void restart_zebra (struct serv_dslite *sd)
{
    FILE *zebra_pid_fd;
    FILE *zebra_cmdline_fd;
    char pid_str[10];
    char cmdline_buf[255];
    int pid = -1;
    int restart_needed = 1;

    if ((zebra_pid_fd = fopen("/var/zebra.pid", "rb")) != NULL)
    {
        if (fgets(pid_str, sizeof(pid_str), zebra_pid_fd) != NULL && atoi(pid_str) > 0)
        {
            pid = atoi(pid_str);
        }
        fclose(zebra_pid_fd);
    }

    if (pid > 0)
    {
        sprintf(cmdline_buf, "/proc/%d/cmdline", pid);
        if ((zebra_cmdline_fd = fopen(cmdline_buf, "rb")) != NULL)
        {
            if (fgets(cmdline_buf, sizeof(cmdline_buf), zebra_cmdline_fd) != NULL)
            {
                if (strstr(cmdline_buf, "zebra"))
                {
                    restart_needed = 0;
                }
            }
            fclose(zebra_cmdline_fd);
        }
    }

    if (restart_needed)
    {
        sysevent_set (sd->sefd, sd->setok, "zebra-restart", "", 0);
    }
}

static int dslite_start (struct serv_dslite *sd)
{
    char val[64];
    char buf[128];
    char DSLITE_AFTR[256];
    char dslite_tnl_ipv4[32];
    char dslite_tnl_ipv6[64];
    char resolved_ipv6[64];
    char gw_ipv6[64];
    struct in6_addr v6_addr;
    unsigned int dnsttl = 0;
    int dhcp_mode = -1;
    char rule[256];
    char rule2[256];
    char return_buffer[256];
    FILE *fptmp = NULL;
    char dslite_mode[16], dslite_addr_type[16];
    size_t len;
    
    if ((sd->rtmod != WAN_RTMOD_IPV6) && (sd->rtmod != WAN_RTMOD_DS))
    {
        fprintf (fp_dslt_dbg, "%s: GW mode is not correct, DSLite can't be started\n", __FUNCTION__);
        return 1;
    }

    SEM_WAIT;

    syscfg_get (NULL, "dslite_enable", val, sizeof(val));
    syscfg_get (NULL, "dslite_active_1", buf, sizeof(buf));
    if ((strcmp (val, "1") != 0) || (strcmp (buf, "1") != 0)) // Either DSLite not enabled or tunnel not enabled
    {
        fprintf(fp_dslt_dbg, "%s: DSLite not enabled, can't be started\n", __FUNCTION__);
        SEM_POST;
        return 1;
    }

    fprintf(fp_dslt_dbg, "%s: DSLite is enabled *********\n", __FUNCTION__);

    sysevent_get (sd->sefd, sd->setok, "dslite_service-status", val, sizeof(val));
    if (strcmp (val, "started") == 0)
    {
        fprintf(fp_dslt_dbg, "%s: DSLite is already started, exit without doing anything !\n", __FUNCTION__);
        SEM_POST;
        return 0;
    }

    /* get the WAN side IPv6 global address */
    sysevent_get (sd->sefd, sd->setok, "tr_" ER_NETDEVNAME "_dhcpv6_client_v6addr", gw_ipv6, sizeof(gw_ipv6));
    fprintf (fp_dslt_dbg, "%s: The GW IPv6 address is %s\n", __FUNCTION__, gw_ipv6);

    /*
       Confirm that the string is not empty, but also that it's long enough to
       still be valid if the first two chars are over-written (see below).
    */
    len = strlen (gw_ipv6);
    if (len < 2)
    {
        fprintf (fp_dslt_dbg, "%s: GW IPv6 address is not ready, DSLite can't be started\n", __FUNCTION__);
        SEM_POST;
        return 1;
    }

    // Follow TS9.1, the dslite tunnel interface IPv6 address starts with "40"
    dslite_tnl_ipv6[0] = '4';
    dslite_tnl_ipv6[1] = '0';
    memcpy (&dslite_tnl_ipv6[2], &gw_ipv6[2], (len + 1) - 2);

    fprintf (fp_dslt_dbg, "%s: The dslite tunnel interface IPv6 address is %s\n", __FUNCTION__, dslite_tnl_ipv6);

    /* do start */
    sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "starting", 0);

    syscfg_get (NULL, "dslite_mode_1", dslite_mode, sizeof(dslite_mode));
    syscfg_get (NULL, "dslite_addr_type_1", dslite_addr_type, sizeof(dslite_addr_type));

    dhcp_mode = get_aftr (DSLITE_AFTR, dslite_mode, dslite_addr_type, sizeof(DSLITE_AFTR));

    if ((strlen (DSLITE_AFTR) == 0) || (strcmp (DSLITE_AFTR, "none") == 0))
    {
        sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "error", 0);
        fprintf (fp_dslt_dbg, "%s: AFTR address is NULL/None, exit! AFTR = %s\n", __FUNCTION__, DSLITE_AFTR);
        SEM_POST;
        return 1;
    }
    fprintf (fp_dslt_dbg, "%s: AFTR address is %s\n", __FUNCTION__, DSLITE_AFTR);

    /* Filter and store the WAN public IPv6 DNS server to a separate file */
    vsystem ("cat /etc/resolv.conf | grep nameserver | grep : | grep -v \"nameserver ::1\" | awk '/nameserver/{print $2}' > /tmp/ipv6_dns_server.conf");
    fptmp = fopen ("/tmp/ipv6_dns_server.conf", "r");
    if (fptmp == NULL)
    {
        sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "error", 0);
        fprintf (fp_dslt_dbg, "%s: IPv6 DNS server isn't present !\n", __FUNCTION__);
        SEM_POST;
        return 1;
    }

    if (fgets (buf, sizeof(buf), fptmp) != NULL)
    {
        len = strlen (buf);
        if ((len > 0) && (buf[len - 1] == '\n'))
            buf[len - 1] = 0;
    }

    fclose (fptmp);

    resolved_ipv6[0] = 0;

    // Use the above IPv6 nameserver to do DNS resolution for AFTR

    if (inet_pton (AF_INET6, DSLITE_AFTR, &v6_addr) == 1)   /* IPv6 address format, no need to do DNS resolution */
    {
        strcpy (resolved_ipv6, DSLITE_AFTR);
        dnsttl = 0;
    }
    else /* domain format, need to do DNS resolution */
    {
        struct in6_addr *addrp = dslite_resolve_fqdn_to_ipv6addr (DSLITE_AFTR, &dnsttl, buf);

        if (addrp)
        {
            char ipv6_str[INET6_ADDRSTRLEN];

            if (inet_ntop (AF_INET6, addrp, ipv6_str, sizeof(ipv6_str)) != NULL)
            {
                strcpy (resolved_ipv6, ipv6_str);
            }

            free (addrp); /* free the memory that dslite_resolve_fqdn_to_ipv6addr had allocated */
        }
    }

    if (resolved_ipv6[0] != 0)
    {
        if (strcmp (resolved_ipv6, "::") == 0)
        {
            sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "error", 0);
            fprintf (fp_dslt_dbg, "%s: AFTR DNSv6 resolution failed as got NULL IPv6 address(::), EXIT !\n", __FUNCTION__);
            SEM_POST;
            return 1;
        }

        /*Store the time of DNS resolution and TTL value for dns_refresh process*/
        syscfg_set (NULL, "dslite_aftr_resolved_1", resolved_ipv6);
        /* Don't use syscfg_set_u() in this case as the string in buf is reused by debug code below */
        sprintf (buf, "%lu", time(NULL));
        syscfg_set (NULL, "dslite_dns_time_1", buf);
        syscfg_set_u (NULL, "dslite_dns_ttl_1", dnsttl);

        fprintf (fp_dslt_dbg, "%s: Resolved AFTR address is %s, time=%s DNS-TTL=%d\n", __FUNCTION__, resolved_ipv6, buf, dnsttl);
    }
    else
    {
        sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "dns_error", 0);
        sysevent_set (sd->sefd, sd->setok, "tr_" ER_NETDEVNAME "_dhcpv6_client_v6addr", gw_ipv6, 0);
        fprintf (fp_dslt_dbg, "%s: DNS resolution failed for unknown reason, RETRY\n", __FUNCTION__);
        SEM_POST;
        return 1;
    }

    //Stop WAN IPv4 service
    route_deconfig (sd);

    vsystem ("service_wan dhcp-release" "; "
             "service_wan dhcp-stop");

    sysevent_set (sd->sefd, sd->setok, "current_wan_ipaddr", "0.0.0.0", 0);

    //Setup the IPv4-in-IPv6 tunnel
    vsystem ("ip -6 tunnel add " TNL_NETDEVNAME " mode ip4ip6 remote %s local %s dev " ER_NETDEVNAME " encaplimit none", resolved_ipv6, gw_ipv6);

    //Enabling AutoConf for ip4ip6 interface
    sysctl_iface_set ("/proc/sys/net/ipv6/conf/%s/autoconf", TNL_NETDEVNAME, "1");

    syscfg_get (NULL, "dslite_tunnel_v4addr_1", dslite_tnl_ipv4, sizeof(dslite_tnl_ipv4));
    if (dslite_tnl_ipv4[0] != 0)
    {
        vsystem ("ip link set dev " TNL_NETDEVNAME " txqueuelen 1000 up" "; "   // Activate tunnel
                 "ip -6 addr add %s dev " TNL_NETDEVNAME "; "                   // Set IPv6 address to tunnel interface
                 "ip addr add %s dev " TNL_NETDEVNAME "; "                      // Set IPv4 address to tunnel interface
                 "ip -4 addr flush " ER_NETDEVNAME,                             // Clear the GW IPv4 address (in case of IPv4 address not released successfully)
                 dslite_tnl_ipv6, dslite_tnl_ipv4);
    }
    else
    {
        vsystem ("ip link set dev " TNL_NETDEVNAME " txqueuelen 1000 up" "; "   // Activate tunnel
                 "ip -6 addr add %s dev " TNL_NETDEVNAME "; "                   // Set IPv6 address to tunnel interface
                 "ip -4 addr flush " ER_NETDEVNAME,                             // Clear the GW IPv4 address (in case of IPv4 address not released successfully)
                 dslite_tnl_ipv6);
    }

    /* Keeping the default route as wan0 interface for SNMP and any other Docsis traffic.
     * Updating the default route (table erouter) through the tunnel for the traffic from LAN client(s).
     */

    // set default gateway through the tunnel in GW specific routing table
    vsystem ("ip route add default dev " TNL_NETDEVNAME " table erouter");

    //set default gateway through the tunnel in routing table 14
    vsystem ("ip route add default dev " TNL_NETDEVNAME " table 14");

    // If GW is the IPv6 only mode, we need to start the LAN to WAN IPv4 function
    if (sd->rtmod == WAN_RTMOD_IPV6)
    {
        vsystem ("echo 1 > /proc/sys/net/ipv4/ip_forward" "; "                  // Enable the IPv4 forwarding
                 "/etc/utopia/service.d/service_igd.sh lan-status" "; "         // Start Upnp & IGMP proxy
                 "/etc/utopia/service.d/service_mcastproxy.sh lan-status");
    }
    else
    {
        /* Restart the LAN side DHCPv4 server, DNS proxy and IGMP proxy if in dual stack mode */
#if defined(_LG_OFW_)
        vsystem ("/etc/utopia/service.d/service_dhcp_server.sh dhcp_server-stop" "; "
                 "/etc/utopia/service.d/service_dhcp_server.sh dhcp_server-start" "; "
                 "/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
#else
        vsystem ("systemctl stop dnsmasq.service" "; "
                 "systemctl start dnsmasq.service" "; "
                 "/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
#endif
    }

    //Add the firewall rule for DSLite tunnel interface
    memset (return_buffer, 0, sizeof(return_buffer));
    snprintf (rule, sizeof(rule), "-I FORWARD -o " TNL_NETDEVNAME " -j ACCEPT\n");
    sysevent_set_unique (sd->sefd, sd->setok, "GeneralPurposeFirewallRule", rule, return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, "dslite_rule_sysevent_id_1", return_buffer, 0);

    memset (return_buffer, 0, sizeof(return_buffer));
    snprintf (rule, sizeof(rule), "-I FORWARD -i " TNL_NETDEVNAME " -j ACCEPT\n");
    sysevent_set_unique (sd->sefd, sd->setok, "GeneralPurposeFirewallRule", rule, return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, "dslite_rule_sysevent_id_2", return_buffer, 0);

    //TCPMSS Clamping for DSLite
    syscfg_get (NULL, "dslite_mss_clamping_enable_1", val, sizeof(val));
    if (strcmp (val, "1") == 0)
    {
        syscfg_get (NULL, "dslite_tcpmss_1", buf, sizeof(buf));
        if (atoi(buf) <= 1460)
        {
            snprintf(rule, sizeof(rule), "-I FORWARD -o " TNL_NETDEVNAME " -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %s\n", buf);
            snprintf(rule2, sizeof(rule2), "-I FORWARD -i " TNL_NETDEVNAME " -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss %s\n", buf);
        }
        else
        {
            snprintf(rule, sizeof(rule), "-I FORWARD -o " TNL_NETDEVNAME " -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
            snprintf(rule2, sizeof(rule2), "-I FORWARD -i " TNL_NETDEVNAME " -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
        }

        memset (return_buffer, 0, sizeof(return_buffer));
        sysevent_set_unique (sd->sefd, sd->setok, "GeneralPurposeMangleRule", rule, return_buffer, sizeof(return_buffer));
        sysevent_set (sd->sefd, sd->setok, "dslite_rule_sysevent_id_3", return_buffer, 0);

        memset (return_buffer, 0, sizeof(return_buffer));
        sysevent_set_unique (sd->sefd, sd->setok, "GeneralPurposeMangleRule", rule2, return_buffer, sizeof(return_buffer));
        sysevent_set (sd->sefd, sd->setok, "dslite_rule_sysevent_id_4", return_buffer, 0);

        syscfg_set (NULL, "dslite_tcpmss_prev_1", buf); //Save the TCPMSS value added into the firewall rule
    }

    vsystem ("sysevent set firewall-restart" "; "       //restart firewall to install the rules
             "conntrack_flush");

    sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "started", 0);

    // save tunnel interface details to syscfg here in case IPv4 functions use it, like IGMP proxy
    // (Currently dslite_tunnel_interface_1 is used but dslite_tunneled_interface_1 is not).
    syscfg_set (NULL, "dslite_tunnel_interface_1", TNL_NETDEVNAME);
    syscfg_set (NULL, "dslite_tunneled_interface_1", ER_NETDEVNAME);

    syscfg_set (NULL, "dslite_status_1", "1");
    syscfg_set (NULL, "dslite_addr_inuse_1", resolved_ipv6);

    if (dhcp_mode == 1)
        syscfg_set (NULL, "dslite_origin_1", "1");  //DHCPv6
    else
        syscfg_set (NULL, "dslite_origin_1", "2");  //Static

    SEM_POST;

    return 0;
}

static int dslite_stop (struct serv_dslite *sd)
{
    char val[64];
    char remote_addr[64];
    char local_addr[64];
    char return_buffer[256];

    SEM_WAIT;

    /*We need to clear the DSLite temporary status buffers at first, either the following
    DSLite stop operations are executed or not*/
    dslite_clear_status (sd);

    sysevent_get (sd->sefd, sd->setok, "dslite_service-status", val, sizeof(val));
    if (strcmp (val, "stopped") == 0)
    {
        fprintf(fp_dslt_dbg, "%s: DSLite is already stopped, exit without doing anything!\n", __FUNCTION__);
        SEM_POST;
        return 0;
    }

    /* do stop */
    sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "stopping", 0);

    //Stop the IPv4-in-IPv6 tunnel
    _get_shell_output ("ip -6 tunnel show | grep " TNL_NETDEVNAME " | awk '/remote/{print $4}'", remote_addr, sizeof(remote_addr));
    _get_shell_output ("ip -6 tunnel show | grep " TNL_NETDEVNAME " | awk '/remote/{print $6}'", local_addr, sizeof(local_addr));

    fprintf (fp_dslt_dbg, "%s: Remote address is %s\n", __FUNCTION__, remote_addr);
    fprintf (fp_dslt_dbg, "%s: Local address is %s\n", __FUNCTION__, local_addr);

    if ((strlen (remote_addr) != 0) && (strlen (local_addr) != 0))
    {
        vsystem ("ip -6 tunnel del " TNL_NETDEVNAME " mode ip4ip6 remote %s local %s dev " ER_NETDEVNAME " encaplimit none", remote_addr, local_addr);

        /*The Zebra process will exit for unknown reason when execute the above tunnel delete operation.
        This may be a bug of Zebra SW package. But here we make a workaround to restart the zebra process if it's exited
        */
        restart_zebra(sd);
    }
    else
    {
        sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "stopped", 0);
        fprintf (fp_dslt_dbg, "%s: The tunnel is already deleted\n", __FUNCTION__);
        SEM_POST;
        return 0;
    }

    if (sd->rtmod != WAN_RTMOD_IPV6)
    {
        //Start WAN IPv4 service
        route_config (sd);
        vsystem ("service_wan dhcp-start");
    }

    //Restore default gateway route rule
    vsystem ("ip route del default dev " TNL_NETDEVNAME " table erouter");

    vsystem ("ip route del default dev " TNL_NETDEVNAME " table 14");

    //if GW is the IPv6 only mode, we need to shutdown the LAN to WAN IPv4 function
    if (sd->rtmod == WAN_RTMOD_IPV6)
    {
        vsystem ("echo 0 > /proc/sys/net/ipv4/ip_forward" "; "                  //Disable the IPv4 forwarding
                 "/etc/utopia/service.d/service_igd.sh igd-stop" "; "           //Stop Upnp & IGMP proxy
                 "/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-stop");
    }
    else
    {
        /* Restart the LAN side DHCPv4 server, DNS proxy and IGMP proxy if in dual stack mode */
#if defined(_LG_OFW_)
        vsystem ("/etc/utopia/service.d/service_dhcp_server.sh dhcp_server-stop" "; "
                 "/etc/utopia/service.d/service_dhcp_server.sh dhcp_server-start" "; "
                 "/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
#else
        vsystem ("systemctl stop dnsmasq.service" "; "
                 "systemctl start dnsmasq.service" "; "
                 "/etc/utopia/service.d/service_mcastproxy.sh mcastproxy-restart");
#endif
    }

    // Delete the firewall rules for DSLite tunnel interface
    sysevent_get (sd->sefd, sd->setok, "dslite_rule_sysevent_id_1", return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, return_buffer, "", 0);

    sysevent_get (sd->sefd, sd->setok, "dslite_rule_sysevent_id_2", return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, return_buffer, "", 0);

    /* Whether or not the TCPMSS Clamping is enabled, the firewall rules need to be deleted */
    sysevent_get (sd->sefd, sd->setok, "dslite_rule_sysevent_id_3", return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, return_buffer, "", 0);

    sysevent_get (sd->sefd, sd->setok, "dslite_rule_sysevent_id_4", return_buffer, sizeof(return_buffer));
    sysevent_set (sd->sefd, sd->setok, return_buffer, "", 0);

    vsystem ("sysevent set firewall-restart" "; "        //restart firewall to install the rules
             "conntrack_flush");

    sysevent_set (sd->sefd, sd->setok, "dslite_service-status", "stopped", 0);

    SEM_POST;

    return 0;
}

static int dslite_restart (struct serv_dslite *sd)
{
    dslite_stop (sd);
    dslite_start (sd);

    return 0;
}

static int dslite_clear_status (struct serv_dslite *sd)
{
    /*Clear the dslite status syscfg*/
    syscfg_set (NULL, "dslite_status_1", "2");
    syscfg_unset (NULL, "dslite_addr_inuse_1");
    syscfg_unset (NULL, "dslite_origin_1");
    syscfg_unset (NULL, "dslite_tunnel_interface_1");
    syscfg_unset (NULL, "dslite_tunneled_interface_1");

    /*Clear the tmp buffer for DNS resolution*/
    syscfg_unset (NULL, "dslite_aftr_resolved_1");
    syscfg_unset (NULL, "dslite_dns_time_1");
    syscfg_unset (NULL, "dslite_dns_ttl_1");

    return 0;
}

static void usage (void)
{
    int i;

    fprintf (stderr, "USAGE\n");
    fprintf (stderr, "    %s COMMAND \n", PROG_NAME);
    fprintf (stderr, "COMMANDS\n");

    for (i = 0; i < NELEMS(cmd_ops); i++)
    {
        fprintf (stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
    }
}

int main (int argc, char *argv[])
{
    int i;
    struct serv_dslite sd;

    if (argc < 2)
    {
        usage();
        exit (1);
    }

    if ((fp_dslt_dbg = fopen(SVC_DSLITE_LOG, "a+")) == NULL) {
        fprintf(stderr, "service_dslite, File(%s) Open Error\n", SVC_DSLITE_LOG);
        exit(1);
    }

    /*
       Treat the clear command as a special case: since it only clears
       syscfg values, as an optimisation it can be run without
       serv_dslite_init() and serv_dslite_term().
    */
    if (strcmp(argv[1], "clear") == 0)
    {
        dslite_clear_status (NULL);
        fclose(fp_dslt_dbg);
        return 0;
    }

    /*
        For other commands, search the commands table.
    */
    for (i = 0; i < NELEMS(cmd_ops); i++)
    {
        if (strcmp(argv[1], cmd_ops[i].cmd) == 0)
        {
            break;
        }
    }

    if (i == NELEMS(cmd_ops))
    {
        fprintf (fp_dslt_dbg, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);
        fclose(fp_dslt_dbg);
        exit (1);
    }

    if (serv_dslite_init (&sd) != 0)
    {
        fclose(fp_dslt_dbg);
        exit (1);
    }

    if (cmd_ops[i].exec (&sd) != 0)
    {
        fprintf (fp_dslt_dbg, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);
    }

    if (serv_dslite_term (&sd) != 0)
    {
        fclose(fp_dslt_dbg);
        exit (1);
    }

    fclose(fp_dslt_dbg);
    return 0;
}
