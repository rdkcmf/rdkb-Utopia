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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**
 * C version of "service_wan" scripts:
 * service_wan.sh/dhcp_link.sh/dhcp_wan.sh/static_link.sh/static_wan.sh
 *
 * The reason to re-implement service_wan with C is for boot time,
 * shell scripts is too slow.
 */

/* 
 * since this utility is event triggered (instead of daemon),
 * we have to use some global var to (sysevents) mark the states. 
 * I prefer daemon, so that we can write state machine clearly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "util.h"

#define PROG_NAME       "SERVICE-WAN"
#if defined(_COSA_BCM_ARM_)
	#define DHCPC_PID_FILE  "/tmp/udhcpc.erouter0.pid"
#else
	#define DHCPC_PID_FILE  "/var/run/eRT_ti_udhcpc.pid"
#endif 

//this value is from erouter0 dhcp client(5*127+10*4)
#define SW_PROT_TIMO   675 
#define RESOLV_CONF_FILE  "/etc/resolv.conf"

#define WAN_STARTED "/var/wan_started"
enum wan_prot {
    WAN_PROT_DHCP,
    WAN_PROT_STATIC,
};

/*
 * XXX:
 * no idea why COSA_DML_DEVICE_MODE_DeviceMode is 1, and 2, 3, 4 for IPv4/IPv6/DS
 * and sysevent last_erouter_mode use 0, 1, 2, 3 instead.
 * let's just follow the last_erouter_mode. :-(
 */
enum wan_rt_mod {
    WAN_RTMOD_UNKNOW,
    WAN_RTMOD_IPV4, // COSA_DML_DEVICE_MODE_Ipv4 - 1
    WAN_RTMOD_IPV6, // COSA_DML_DEVICE_MODE_Ipv6 - 1
    WAN_RTMOD_DS,   // COSA_DML_DEVICE_MODE_Dualstack - 1
};

struct serv_wan {
    int             sefd;
    int             setok;
    char            ifname[IFNAMSIZ];
    enum wan_rt_mod rtmod;
    enum wan_prot   prot;
    int             timo;
};

struct cmd_op {
    const char      *cmd;
    int             (*exec)(struct serv_wan *sw);
    const char      *desc;
};

static int wan_start(struct serv_wan *sw);
static int wan_stop(struct serv_wan *sw);
static int wan_restart(struct serv_wan *sw);
static int wan_iface_up(struct serv_wan *sw);
static int wan_iface_down(struct serv_wan *sw);
static int wan_addr_set(struct serv_wan *sw);
static int wan_addr_unset(struct serv_wan *sw);

static int wan_dhcp_start(struct serv_wan *sw);
static int wan_dhcp_stop(struct serv_wan *sw);
static int wan_dhcp_restart(struct serv_wan *sw);
static int wan_dhcp_release(struct serv_wan *sw);
static int wan_dhcp_renew(struct serv_wan *sw);

static int wan_static_start(struct serv_wan *sw);
static int wan_static_stop(struct serv_wan *sw);

static struct cmd_op cmd_ops[] = {
    {"start",       wan_start,      "start service wan"},
    {"stop",        wan_stop,       "stop service wan"},
    {"restart",     wan_restart,    "restart service wan"},
    {"iface-up",    wan_iface_up,   "bring interface up"},
    {"iface-down",  wan_iface_down, "tear interface down"},
    {"addr-set",    wan_addr_set,   "set IP address with specific protocol"},
    {"addr-unset",  wan_addr_unset, "unset IP address with specific protocol"},

    /* protocol specific */
    {"dhcp-start",  wan_dhcp_start, "trigger DHCP procedure"},
    {"dhcp-stop",   wan_dhcp_stop,  "stop DHCP procedure"},
    {"dhcp-restart",wan_dhcp_restart, "restart DHCP procedure"},
    {"dhcp-release",wan_dhcp_release,"trigger DHCP release"},
    {"dhcp-renew",  wan_dhcp_renew, "trigger DHCP renew"},
};

static int dhcp_stop(const char *ifname)
{
    FILE *fp;
    char pid_str[10];
    int pid = -1;

    if ((fp = fopen(DHCPC_PID_FILE, "rb")) != NULL) {
        if (fgets(pid_str, sizeof(pid_str), fp) != NULL && atoi(pid_str) > 0)
            pid = atoi(pid_str);

        fclose(fp);
    }

    if (pid <= 0)
        pid = pid_of("ti_udhcpc", ifname);

    if (pid > 0) {
        kill(pid, SIGUSR2); // triger DHCP release
        sleep(1);
        kill(pid, SIGTERM); // terminate DHCP client

        /*
        sleep(1);
        if (pid_of("ti_udhcpc", ifname) == pid) {
            fprintf(stderr, "%s: ti_udhcpc is still exist ! kill -9 it\n", __FUNCTION__);
            kill(pid, SIGKILL);
        }
        */
    }
    unlink(DHCPC_PID_FILE);

    unlink("/tmp/udhcp.log");
    return 0;
}

static int dhcp_start(const char *ifname)
{
   int err;
  /*TCHXB6 is configured to use udhcpc */
#if defined (_COSA_BCM_ARM_)
	err = vsystem("/sbin/udhcpc -i %s -p %s ",ifname, DHCPC_PID_FILE);
#else
    err = vsystem("ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i %s "
                "-H DocsisGateway -p %s -B -b 1",
                ifname, DHCPC_PID_FILE);

#endif 
/*
	err = vsystem("strace -o /tmp/stracelog -f ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i %s "
              "-H DocsisGateway -p %s -B -b 1",
              ifname, DHCPC_PID_FILE);
*/
	if (err != 0)
                   fprintf(stderr, "%s: fail to launch erouter plugin\n", __FUNCTION__);

	err = 0; //temporary hack for ARRISXB3-3748

    return err == 0 ? 0 : -1;
}

static int route_config(const char *ifname)
{
    if (vsystem("ip rule add iif %s lookup all_lans && "
                "ip rule add oif %s lookup erouter && "
                "ip -6 rule add oif %s lookup erouter ",
                ifname, ifname, ifname) != 0)
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;

    return 0;
}

static int route_deconfig(const char *ifname)
{
    if (vsystem("ip rule del iif %s lookup all_lans && "
                "ip rule del oif %s lookup erouter && "
                " ip -6 rule del oif %s lookup erouter ",
                ifname, ifname, ifname) != 0)
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;

    return 0;
}

int checkFileExists(const char *fname)
{
    FILE *file;
    if (file = fopen(fname, "r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

static int wan_start(struct serv_wan *sw)
{
    char status[16];
    int ret;
    /* state check */
    sysevent_get(sw->sefd, sw->setok, "wan_service-status", status, sizeof(status));
    if (strcmp(status, "starting") == 0 || strcmp(status, "started") == 0) {
        fprintf(stderr, "%s: service wan has already %s !\n", __FUNCTION__, status);
        return 0;
    } else if (strcmp(status, "stopping") == 0) {
        fprintf(stderr, "%s: cannot start in status %s !\n", __FUNCTION__, status);
        return -1;
    }

    /* do start */
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "starting", 0);

    if (wan_iface_up(sw) != 0) {
        fprintf(stderr, "%s: wan_iface_up error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

    if (sw->rtmod != WAN_RTMOD_IPV4 && sw->rtmod != WAN_RTMOD_DS)
        goto done; /* no need to config addr/route if IPv4 not enabled */

    if (wan_addr_set(sw) != 0) {
        fprintf(stderr, "%s: wan_addr_set error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

    if (route_config(sw->ifname) != 0) {
        fprintf(stderr, "%s: route_config error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

done:
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "started", 0);

    printf("Network Response script called to capture network response\n ");
    /*Network Response captured ans stored in /var/tmp/network_response.txt*/
	
    system("sh /etc/network_response.sh &");

    ret = checkFileExists(WAN_STARTED);
    printf("Check wan started ret is %d\n",ret);
    if ( 0 == ret )
    {
	system("touch /var/wan_started");
    }
    else
    {
	char  str[100] = {0};
        printf("%s wan_service-status is started again, upload logs\n",__FUNCTION__);
	sprintf(str,"/rdklogger/uploadRDKBLogs.sh \"\" HTTP \"\" false ");
	system(str);
    }

    return 0;
}

static int wan_stop(struct serv_wan *sw)
{
    char val[64];
    char status[16];

    /* state check */
    sysevent_get(sw->sefd, sw->setok, "wan_service-status", status, sizeof(status));
    if (strcmp(status, "stopping") == 0 || strcmp(status, "stopped") == 0) {
        fprintf(stderr, "%s: service wan has already %s !\n", __FUNCTION__, status);
        return 0;
    } else if (strcmp(status, "starting") == 0) {
        fprintf(stderr, "%s: cannot start in status %s !\n", __FUNCTION__, status);
        return -1;
    }
 
    /* do stop */
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "stopping", 0);

    if (sw->rtmod == WAN_RTMOD_IPV4 || sw->rtmod == WAN_RTMOD_DS) {
        if (route_deconfig(sw->ifname) != 0) {
            fprintf(stderr, "%s: route_deconfig error\n", __FUNCTION__);
            sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
            return -1;
        }

        if (wan_addr_unset(sw) != 0) {
            fprintf(stderr, "%s: wan_addr_unset error\n", __FUNCTION__);
            sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
            return -1;
        }
    }

    if (wan_iface_down(sw) != 0) {
        fprintf(stderr, "%s: wan_iface_down error\n", __FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "wan_service-status", "error", 0);
        return -1;
    }

    printf("%s wan_service-status is stopped, take log back up\n",__FUNCTION__);
    sysevent_set(sw->sefd, sw->setok, "wan_service-status", "stopped", 0);
	char  str[100] = {0};
	sprintf(str,"/rdklogger/backupLogs.sh false \"\" wan-stopped");
    system(str);
    return 0;
}

static int wan_restart(struct serv_wan *sw)
{
    int err;

    sysevent_set(sw->sefd, sw->setok, "wan-restarting", "1", 0);

    if (wan_stop(sw) != 0)
        fprintf(stderr, "%s: wan_stop error\n", __FUNCTION__);

    if ((err = wan_start(sw)) != 0)
        fprintf(stderr, "%s: wan_start error\n", __FUNCTION__);

    sysevent_set(sw->sefd, sw->setok, "wan-restarting", "0", 0);
    return err;
}

static int wan_iface_up(struct serv_wan *sw)
{
#if 1 // XXX: MOVE these code to IPv6 scripts, why put them in IPv4 service wan ??
    char proven[64];
    char mtu[16];

    switch (sw->rtmod) {
    case WAN_RTMOD_IPV6:
    case WAN_RTMOD_DS:
        syscfg_get(NULL, "router_adv_provisioning_enable", proven, sizeof(proven));
        if (atoi(proven) == 1) {
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra", sw->ifname, "2");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra_defrtr", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra_pinfo", sw->ifname, "0");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "1");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", sw->ifname, "0");
        } else {
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/accept_ra", sw->ifname, "0");
            sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "0");
        }

        sysctl_iface_set("/proc/sys/net/ipv6/conf/all/forwarding", NULL, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", sw->ifname, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", "wan0", "0");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", "mta0", "0");
        break;
    default:
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", sw->ifname, "0");
        break;
    }
#endif

    syscfg_get(NULL, "wan_mtu", mtu, sizeof(mtu));
    if (atoi(mtu) < 1500 && atoi(mtu) > 0)
        vsystem("ip -4 link set %s mtu %s", sw->ifname, mtu);

    sysctl_iface_set("/proc/sys/net/ipv4/conf/%s/arp_announce", sw->ifname, "1");
    vsystem("ip -4 link set %s up", sw->ifname);
    return 0;
}

static int wan_iface_down(struct serv_wan *sw)
{
    int err;

    err = vsystem("ip -4 link set %s down", sw->ifname);

	err = 0; //temporary hack for ARRISXB3-3748
    return err == 0 ? 0 : -1;
}

static int wan_addr_set(struct serv_wan *sw)
{
    char val[64];
    char state[16];
    int timo;
    FILE *fp;
    char ipaddr[16];

    sysevent_set(sw->sefd, sw->setok, "wan-status", "starting", 0);
    sysevent_set(sw->sefd, sw->setok, "wan-errinfo", NULL, 0);

    switch (sw->prot) {
    case WAN_PROT_DHCP:
        if (wan_dhcp_start(sw) != 0) {
            fprintf(stderr, "%s: wan_dhcp_start error\n", __FUNCTION__);
            return -1;
        }

        break;
    case WAN_PROT_STATIC:
        if (wan_static_start(sw) != 0) {
            fprintf(stderr, "%s: wan_static_start error\n", __FUNCTION__);
            return -1;
        }

        break;
    default:
        fprintf(stderr, "%s: unknow wan protocol\n", __FUNCTION__);
        return -1;
    }

    fprintf(stderr, "[%s] start waiting for protocol ...\n", PROG_NAME);
    for (timo = sw->timo; timo > 0; timo--) {
        sysevent_get(sw->sefd, sw->setok, "current_ipv4_link_state", state, sizeof(state));
        if (strcmp(state, "up") == 0)
            break;
        sleep(1);
    }
    if (timo == 0)
        fprintf(stderr, "[%s] wait for protocol TIMEOUT !\n", PROG_NAME);
    else
        fprintf(stderr, "[%s] wait for protocol SUCCESS !\n", PROG_NAME);

    /* set sysevents and trigger for other modules */
    sysevent_set(sw->sefd, sw->setok, "current_wan_ifname", sw->ifname, 0);

    sysevent_get(sw->sefd, sw->setok, "ipv4_wan_subnet", val, sizeof(val));
    if (strlen(val))
        sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", val, 0);
    else
        sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", "255.255.255.0", 0);

    sysevent_get(sw->sefd, sw->setok, "ipv4_wan_ipaddr", val, sizeof(val));
    if (strlen(val))
        sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", val, 0);
    else
        sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", "0.0.0.0", 0);

    syscfg_get(NULL, "dhcp_server_propagate_wan_domain", val, sizeof(val));
    if (atoi(val) != 1)
        syscfg_get(NULL, "dhcp_server_propagate_wan_nameserver", val, sizeof(val));

    if (atoi(val) == 1) {
        //if ((fp = fopen("/var/tmp/lan_not_restart", "wb")) != NULL)
        //    fclose(fp);
        sysevent_set(sw->sefd, sw->setok, "dhcp_server-restart", "lan_not_restart", 0);
    }

#if 1 // wan-status triggers service_routed, which will restart firewall
    /* this logic are really strange, it means whan lan is ok but "start-misc" is not, 
     * do not start firewall fully. but "start-misc" means ? 
     * why not use "lan-status" ? 
     *
     * It not good idea to trigger other module here, firewall itself should register 
     * "lan-status" and "wan-status" and determine which part should be launched. */
    sysevent_get(sw->sefd, sw->setok, "start-misc", val, sizeof(val));
    sysevent_get(sw->sefd, sw->setok, "current_lan_ipaddr", ipaddr, sizeof(ipaddr));
    if (strcmp(val, "ready") != 0 && strlen(ipaddr) && strcmp(ipaddr, "0.0.0.0") != 0) {
        fprintf(stderr, "%s: start-misc: %s current_lan_ipaddr %s\n", __FUNCTION__, val, ipaddr);
        fprintf(stderr, "[%s] start firewall partially\n", PROG_NAME);

        sysevent_get(sw->sefd, sw->setok, "parcon_nfq_status", val, sizeof(val));
        if (strcmp(val, "started") != 0) {
            iface_get_hwaddr(sw->ifname, val, sizeof(val));
            vsystem("((nfq_handler 4 %s &)&)", val);
            sysevent_set(sw->sefd, sw->setok, "parcon_nfq_status", "started", 0);
        }
        vsystem("firewall && gw_lan_refresh && execute_dir /etc/utopia/post.d/ restart");
    } else {
        fprintf(stderr, "[%s] start firewall fully\n", PROG_NAME);
        printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);
    }
#endif

    sysctl_iface_set("/proc/sys/net/ipv4/ip_forward", NULL, "1");
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "up", 0);
    sysevent_set(sw->sefd, sw->setok, "firewall_flush_conntrack", "1", 0);

    sysevent_set(sw->sefd, sw->setok, "wan-status", "started", 0);
/*XB6 brlan0 comes up earlier so ned to find the way to restart the firewall
 IPv6 not yet supported so we can't restart in service routed  because of missing zebra.conf*/
#ifdef INTEL_PUMA7
        printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
        sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);
#endif
    return 0;
}

static int wan_addr_unset(struct serv_wan *sw)
{

    sysevent_set(sw->sefd, sw->setok, "wan-status", "stopping", 0);
    sysevent_set(sw->sefd, sw->setok, "wan-errinfo", NULL, 0);

    sysevent_set(sw->sefd, sw->setok, "current_wan_ipaddr", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "current_wan_subnet", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "down", 0);

    switch (sw->prot) {
    case WAN_PROT_DHCP:
        if (wan_dhcp_stop(sw) != 0) {
            fprintf(stderr, "%s: wan_dhcp_stop error\n", __FUNCTION__);
            return -1;
        }

        break;
    case WAN_PROT_STATIC:
        if (wan_static_stop(sw) != 0) {
            fprintf(stderr, "%s: wan_static_stop error\n", __FUNCTION__);
            return -1;
        }

        break;
    default:
        fprintf(stderr, "%s: unknow wan protocol\n", __FUNCTION__);
        return -1;
    }

    vsystem("ip -4 addr flush dev %s", sw->ifname);

    printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
    sysevent_set(sw->sefd, sw->setok, "firewall-restart", NULL, 0);

    sysevent_set(sw->sefd, sw->setok, "wan-status", "stopped", 0);
    return 0;
}

static int wan_dhcp_start(struct serv_wan *sw)
{
    int pid; 
    int has_pid_file = 0;

    pid = pid_of("ti_udhcpc", sw->ifname);
    if (access(DHCPC_PID_FILE, F_OK) == 0)
        has_pid_file = 1;

    if (pid > 0 && has_pid_file) {
        fprintf(stderr, "%s: DHCP client has already running as PID %d\n", __FUNCTION__, pid);
        return 0;
    }
    
    if (pid > 0 && !has_pid_file)
        kill(pid, SIGKILL);
    else if (pid <= 0 && has_pid_file)
        dhcp_stop(sw->ifname);

    return dhcp_start(sw->ifname);
}

static int wan_dhcp_stop(struct serv_wan *sw)
{
    return dhcp_stop(sw->ifname);
}

static int wan_dhcp_restart(struct serv_wan *sw)
{
    if (dhcp_stop(sw->ifname) != 0)
        fprintf(stderr, "%s: dhcp_stop error\n", __FUNCTION__);

    return dhcp_start(sw->ifname);
}

static int wan_dhcp_release(struct serv_wan *sw)
{
    FILE *fp;
    char pid[10];

    if ((fp = fopen(DHCPC_PID_FILE, "rb")) == NULL)
        return -1;

    if (fgets(pid, sizeof(pid), fp) != NULL && atoi(pid) > 0)
        kill(atoi(pid), SIGUSR2); // triger DHCP release

    fclose(fp);

    vsystem("ip -4 addr flush dev %s", sw->ifname);
    return 0;
}

static int wan_dhcp_renew(struct serv_wan *sw)
{
    FILE *fp;
    char pid[10];
    char line[64], *cp;

    if ((fp = fopen(DHCPC_PID_FILE, "rb")) == NULL)
        return dhcp_start(sw->ifname);

    if (fgets(pid, sizeof(pid), fp) != NULL && atoi(pid) > 0)
        kill(atoi(pid), SIGUSR1); // triger DHCP release

    fclose(fp);
    sysevent_set(sw->sefd, sw->setok, "current_wan_state", "up", 0);

    if ((fp = fopen("/proc/uptime", "rb")) == NULL)
        return -1;
    if (fgets(line, sizeof(line), fp) != NULL) {
        if ((cp = strchr(line, ',')) != NULL)
            *cp = '\0';
        sysevent_set(sw->sefd, sw->setok, "wan_start_time", line, 0);
    }
    fclose(fp);

    return 0;
}

static int resolv_static_config(struct serv_wan *sw)
{
    FILE *fp = NULL;
    char wan_domain[64] = {0};
    char name_server[3][32] = {0};
    char v6_name_server[32] = {0};
    int i = 0;
    char name_str[16] = {0};

    if((fp = fopen(RESOLV_CONF_FILE, "w+")) == NULL)
    {
        fprintf(stderr, "%s: Open %s error!\n", __FUNCTION__, RESOLV_CONF_FILE);
        return -1;
    }

    syscfg_get(NULL, "wan_domain", wan_domain, sizeof(wan_domain));
    if(wan_domain[0] != '\0') {
        fprintf(fp, "search %s\n", wan_domain);
        sysevent_set(sw->sefd, sw->setok, "dhcp_domain", wan_domain, 0);
    }

    memset(name_server, 0, sizeof(name_server));
    for(; i < 3; i++) {
        snprintf(name_str, sizeof(name_str), "nameserver%d", i+1);
        syscfg_get(NULL, name_str, name_server[i], sizeof(name_server[i]));
        if(name_server[i][0] != '\0' && strcmp(name_server[i], "0.0.0.0")) {
            printf("nameserver%d:%s\n", i+1, name_server[i]);
            fprintf(fp, "nameserver %s\n", name_server[i]);
        }
    }

    fclose(fp);
    return 0;
}

static int resolv_static_deconfig(struct serv_wan *sw)
{
    FILE *fp = NULL;

    if((fp = fopen(RESOLV_CONF_FILE, "w+")) == NULL) {
        fprintf(stderr, "%s: Open %s error!\n", __FUNCTION__, RESOLV_CONF_FILE);
        return -1;
    }

    fclose(fp);
    return 0;
}

static int wan_static_start(struct serv_wan *sw)
{
    char wan_ipaddr[16] = {0};
    char wan_netmask[16] = {0};
    char wan_default_gw[16] = {0};

    if(resolv_static_config(sw) != 0) {
        fprintf(stderr, "%s: Config resolv file failed!\n");
    }

    /*get static config*/
    syscfg_get(NULL, "wan_ipaddr", wan_ipaddr, sizeof(wan_ipaddr));
    syscfg_get(NULL, "wan_netmask", wan_netmask, sizeof(wan_netmask));
    syscfg_get(NULL, "wan_default_gateway", wan_default_gw, sizeof(wan_default_gw));

    if(vsystem("ip -4 addr add %s/%s broadcast + dev %s", wan_ipaddr, wan_netmask, sw->ifname) != 0) {
        fprintf(stderr, "%s: Add address to interface %s failed!\n", __FUNCTION__, sw->ifname);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    if(vsystem("ip -4 link set %s up", sw->ifname) != 0) {
        fprintf(stderr, "%s: Set interface %s up failed!\n", __FUNCTION__, sw->ifname);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    if(vsystem("ip -4 route add table erouter default dev %s via %s && "
                "ip rule add from %s lookup erouter", sw->ifname, wan_default_gw, wan_ipaddr) != 0)
    {
        fprintf(stderr, "%s: router related config failed!\n", __FUNCTION__);
	return 0; //temporary hack for ARRISXB3-3748
        //hack hack hack return -1;
    }

    /*set related sysevent*/
    sysevent_set(sw->sefd, sw->setok, "default_router", wan_default_gw, 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_ipaddr", wan_ipaddr, 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_subnet", wan_netmask, 0);
    sysevent_set(sw->sefd, sw->setok, "current_ipv4_link_state", "up", 0);
    sysevent_set(sw->sefd, sw->setok, "dhcp_server-restart", NULL, 0);

    return 0;
}

static int wan_static_stop(struct serv_wan *sw)
{
    char wan_ipaddr[16] = {0};

    if(resolv_static_deconfig(sw) != 0) {
        fprintf(stderr, "%s: deconfig resolv file failed!\n", __FUNCTION__);
    }

    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_ipaddr", "0.0.0.0", 0);
    sysevent_set(sw->sefd, sw->setok, "ipv4_wan_subnet", "0.0.0.0", 0);

    sysevent_set(sw->sefd, sw->setok, "default_router", NULL, 0);
    syscfg_get(NULL, "wan_ipaddr", wan_ipaddr, sizeof(wan_ipaddr));
    vsystem("ip rule del from %s lookup erouter", wan_ipaddr);
    vsystem("ip -4 route del table erouter default dev %s", sw->ifname);

    sysevent_set(sw->sefd, sw->setok, "current_ipv4_link_state", "down", 0);

    return 0;
}

static int serv_wan_init(struct serv_wan *sw, const char *ifname, const char *prot)
{
    char buf[32];

    if ((sw->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT, 
                    SE_VERSION, PROG_NAME, &sw->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }

    if (ifname)
        snprintf(sw->ifname, sizeof(sw->ifname), "%s", ifname);
    else
        syscfg_get(NULL, "wan_physical_ifname", sw->ifname, sizeof(sw->ifname));

    if (!strlen(sw->ifname)) {
        fprintf(stderr, "%s: fail to get ifname\n", __FUNCTION__);
        return -1;
    }

    if (prot)
        snprintf(buf, sizeof(buf), "%s", prot);
    else
        syscfg_get(NULL, "wan_proto", buf, sizeof(buf));

    if (strcasecmp(buf, "dhcp") == 0)
        sw->prot = WAN_PROT_DHCP;
    else if (strcasecmp(buf, "static") == 0)
        sw->prot = WAN_PROT_STATIC;
    else {
        fprintf(stderr, "%s: fail to get wan protocol\n", __FUNCTION__);
        return -1;
    }

    syscfg_get(NULL, "last_erouter_mode", buf, sizeof(buf));
    switch (atoi(buf)) {
    case 1:
        sw->rtmod = WAN_RTMOD_IPV4;
        break;
    case 2:
        sw->rtmod = WAN_RTMOD_IPV6;
        break;
    case 3:
        sw->rtmod = WAN_RTMOD_DS;
        break;
    default:
        fprintf(stderr, "%s: unknow RT mode (last_erouter_mode)\n", __FUNCTION__);
        sw->rtmod = WAN_RTMOD_UNKNOW;
        break;
    }

    sw->timo = SW_PROT_TIMO;

    return 0;
}

static int serv_wan_term(struct serv_wan *sw)
{
    sysevent_close(sw->sefd, sw->setok);
    return 0;
}

static void usage(void)
{
    int i;

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "    %s COMMAND [ INTERFACE [ PROTOCOL ] ]\n", PROG_NAME);
    fprintf(stderr, "COMMANDS\n");
    for (i = 0; i < NELEMS(cmd_ops); i++)
        fprintf(stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
    fprintf(stderr, "PROTOCOLS\n");
        fprintf(stderr, "    dhcp, static\n");
}

int main(int argc, char *argv[])
{
    int i;
    struct serv_wan sw;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_wan_init(&sw, (argc > 2 ? argv[2] : NULL), (argc > 3 ? argv[3] : NULL)) != 0)
        exit(1);

    /* execute commands */
    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] exec: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&sw) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_wan_term(&sw) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}
