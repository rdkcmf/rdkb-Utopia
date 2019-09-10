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
 * C version of "service_routed.sh" script.
 *
 * The reason to re-implement service_routed with C is for boot time,
 * shell scripts is too slow.
 */

/* 
 * since this utility is event triggered (instead of daemon),
 * we have to use some global var to (sysevents) mark the states. 
 * I prefer daemon, so that we can write state machine clearly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <net/if.h>
#include <signal.h>
#include "util.h"

#define PROG_NAME       "SERVICE-ROUTED"

#define ZEBRA_PID_FILE  "/var/zebra.pid"
#define RIPD_PID_FILE   "/var/ripd.pid"
#define ZEBRA_CONF_FILE "/var/zebra.conf"
#define RIPD_CONF_FILE  "/etc/ripd.conf"


#ifdef MULTILAN_FEATURE
#define COSA_DML_DHCPV6_CLIENT_IFNAME                 "erouter0"
#define COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_pretm"
#define COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_vldtm"
#endif
struct serv_routed {
    int         sefd;
    int         setok;

    bool        lan_ready;
    bool        wan_ready;
};

static int fw_restart(struct serv_routed *sr)
{
    char val[16];
    char wan_if[IFNAMSIZ];

    sysevent_get(sr->sefd, sr->setok, "parcon_nfq_status", val, sizeof(val));
    if (strcmp(val, "started") != 0) {
        syscfg_get(NULL, "wan_physical_ifname", wan_if, sizeof(wan_if));

        iface_get_hwaddr(wan_if, val, sizeof(val));
        vsystem("((nfq_handler 4 %s &)&)", val);
        sysevent_set(sr->sefd, sr->setok, "parcon_nfq_status", "started", 0);
    }
    printf("%s Triggering RDKB_FIREWALL_RESTART\n",__FUNCTION__);
    sysevent_set(sr->sefd, sr->setok, "firewall-restart", NULL, 0);
    return 0;
}

static int daemon_stop(const char *pid_file, const char *prog)
{
    FILE *fp;
    char pid_str[10];
    int pid = -1;

    if (!pid_file && !prog)
        return -1;

    if (pid_file) {
        if ((fp = fopen(pid_file, "rb")) != NULL) {
            if (fgets(pid_str, sizeof(pid_str), fp) != NULL && atoi(pid_str) > 0)
                pid = atoi(pid_str);

            fclose(fp);
        }
    }

    if (pid <= 0 && prog)
        pid = pid_of(prog, NULL);

    if (pid > 0) {
        kill(pid, SIGTERM);
    }
    
    if (pid_file)
        unlink(pid_file);
    return 0;
}

#ifdef MULTILAN_FEATURE
static int get_active_lanif(int sefd, token_t setok, unsigned int *insts, unsigned int *num)
{
    char active_insts[32] = {0};
    char lan_pd_if[128] = {0};
    char *p = NULL;
    int i = 0;
    char if_name[16] = {0};
    char buf[64] = {0};

#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION

    syscfg_get(NULL, "lan_pd_interfaces", lan_pd_if, sizeof(lan_pd_if));
    if (lan_pd_if[0] == '\0') {
        *num = 0;
        return *num;
    }

    sysevent_get(sefd, setok, "multinet-instances", active_insts, sizeof(active_insts));
    p = strtok(active_insts, " ");

    while (p != NULL) {
        snprintf(buf, sizeof(buf), "multinet_%s-name", p);
        sysevent_get(sefd, setok, buf, if_name, sizeof(if_name));
        if (if_name[0] != '\0' && strstr(lan_pd_if, if_name)) { /*active interface and need prefix delegation*/
            insts[i] = atoi(p);
            i++;
        }

        p = strtok(NULL, " ");
    }

#else

    /* service_ipv6 sets active IPv6 interfaces instances. */
    sysevent_get(sefd, setok, "ipv6_active_inst", active_insts, sizeof(active_insts));
    p = strtok(active_insts, " ");
    while (p != NULL) {
        insts[i++] = atoi(p);
        p = strtok(NULL, " ");
    }

#endif
    *num = i;

    return *num;
}
#else
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
static int get_active_lanif(int sefd, token_t setok, unsigned int *insts, unsigned int *num)
{
    char active_insts[32] = {0};
    char lan_pd_if[128] = {0};
    char *p = NULL;
    int i = 0;
    char if_name[16] = {0};
    char buf[64] = {0};

    syscfg_get(NULL, "lan_pd_interfaces", lan_pd_if, sizeof(lan_pd_if));
    if (lan_pd_if[0] == '\0') {
        *num = 0;
        return *num;
    }

    sysevent_get(sefd, setok, "multinet-instances", active_insts, sizeof(active_insts));
    p = strtok(active_insts, " ");

    while (p != NULL) {
        snprintf(buf, sizeof(buf), "multinet_%s-name", p);
        sysevent_get(sefd, setok, buf, if_name, sizeof(if_name));
        if (if_name[0] != '\0' && strstr(lan_pd_if, if_name)) { /*active interface and need prefix delegation*/
            insts[i] = atoi(p);
            i++;
        }

        p = strtok(NULL, " ");
    }

    *num = i;

    return *num;
}
#endif
#endif


static  int route_enable()
{
    char rt_state[16];

    syscfg_get(NULL, "tr_routing_enabled", rt_state, sizeof(rt_state));

    /* During Modem reboot the forwarding value is reset to default so below lines
       will retain the value of routing after the reboot */
    if(strcmp(rt_state, "0") == 0) {
        if ((vsystem("echo 0 > /proc/sys/net/ipv4/conf/erouter0/forwarding") == 0) ||
                (vsystem("echo 0 > /proc/sys/net/ipv6/conf/all/forwarding") == 0)) {
            return -1;
        }
    }
    else {
        if ((vsystem("echo 1 > /proc/sys/net/ipv4/conf/erouter0/forwarding") == 0) ||
                (vsystem("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding") == 0)) {
            return -1;
        }
    }

    return 0;
}
static int route_set(struct serv_routed *sr)
{
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
    char lan_if[32] = {0};
    int i;

    get_active_lanif(sr->sefd, sr->setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sr->sefd, sr->setok, evt_name, lan_if, sizeof(lan_if));

        vsystem("ip -6 rule add iif %s table all_lans", lan_if);
        vsystem("ip -6 rule add iif %s table erouter", lan_if);
    }
#endif
    if (vsystem("ip -6 rule add iif brlan0 table erouter;"
            "gw=$(ip -6 route show default dev erouter0 | awk '/via/ {print $3}');"
            "if [ \"$gw\" != \"\" ]; then"
            "  ip -6 route add default via $gw dev erouter0 table erouter;"
            "fi") != 0)
        return -1;
    return 0;
}

static int route_unset(struct serv_routed *sr)
{
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
    char lan_if[32] = {0};
    int i;

    get_active_lanif(sr->sefd, sr->setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sr->sefd, sr->setok, evt_name, lan_if, sizeof(lan_if));

        vsystem("ip -6 rule del iif %s table all_lans", lan_if);
        vsystem("ip -6 rule del iif %s table erouter", lan_if);
    }
#else
    if (vsystem("ip -6 route del default dev erouter0 table erouter"
            " && ip -6 rule del iif brlan0 table erouter") != 0)
        return -1;
#endif
    return 0;
}

static int gen_zebra_conf(int sefd, token_t setok)
{
    FILE *fp = NULL;
#if !defined (INTEL_PUMA7)
    char rtmod[16], static_rt_cnt[16], ra_en[16], dh6s_en[16];
#else
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    char rtmod[16], static_rt_cnt[16], ra_en[16], dh6s_en[16], ra_interval[8] = {0};
#endif
    char name_servs[1024] = {0};
    char dnssl[2560] = {0};
    char prefix[64], orig_prefix[64], lan_addr[64];
    char preferred_lft[16], valid_lft[16];
#ifdef MULTILAN_FEATURE
    char dnssl_lft[16];
    unsigned int dnssllft = 0;
#endif
    char m_flag[16], o_flag[16];
    char rec[256], val[512];
    char buf[6];
    FILE *responsefd = NULL;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    int inCaptivePortal = 0;
    int nopt, i = 0, j = 0; /*RDKB-12965 & CID:-34147*/
    char lan_if[IFNAMSIZ];
    char *start, *tok, *sp;
    static const char *zebra_conf_base = \
        "hostname zebra\n"
        "!password zebra\n"
        "!enable password admin\n"
        "!log stdout\n"
        "log file /var/log/zebra.log errors\n"
        "table 255\n";
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
    int  StaticDNSServersEnabled = 0;
#ifdef _HUB4_PRODUCT_REQ_
    char lan_addr_prefix[64] = {0};
#endif
#if defined (INTEL_PUMA7)
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    char wan_st[16] = {0};
#endif

#ifdef _HUB4_PRODUCT_REQ_
    char server_type[16] = {0};
    char prev_valid_lft[16] = {0};
    int result = 0;
    int ipv6_enable = 0;
    int ula_enable = 0;
#endif
    if ((fp = fopen(ZEBRA_CONF_FILE, "wb")) == NULL) {
        fprintf(stderr, "%s: fail to open file %s\n", __FUNCTION__, ZEBRA_CONF_FILE);
        return -1;
    }

    if (fwrite(zebra_conf_base, strlen(zebra_conf_base), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

#if defined(_COSA_FOR_BCI_)
    char dhcpv6Enable[8]={0};
    // Set bool to determine if dhcpv6 enabled
    syscfg_get(NULL, "dhcpv6s00::serverenable", dhcpv6Enable , sizeof(dhcpv6Enable));
    bool bEnabled = (strncmp(dhcpv6Enable,"1",1)==0?true:false);
#endif

    /* TODO: static route */

    syscfg_get(NULL, "router_adv_enable", ra_en, sizeof(ra_en));
    if (strcmp(ra_en, "1") != 0) {
        fclose(fp);
        return 0;
    }
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    sysevent_get(sefd, setok, "previous_ipv6_prefix", orig_prefix, sizeof(orig_prefix));
    sysevent_get(sefd, setok, "ipv6_prefix_prdtime", preferred_lft, sizeof(preferred_lft));
    sysevent_get(sefd, setok, "ipv6_prefix_vldtime", valid_lft, sizeof(valid_lft));
#else
    sysevent_get(sefd, setok, "ipv6_prefix", prefix, sizeof(prefix));
    sysevent_get(sefd, setok, "previous_ipv6_prefix", orig_prefix, sizeof(orig_prefix));
#ifndef _HUB4_PRODUCT_REQ_
    sysevent_get(sefd, setok, "current_lan_ipv6address", lan_addr, sizeof(lan_addr));
#else
    if(strncmp(prefix, orig_prefix, 64) == 0) {
        strncpy(orig_prefix, "", sizeof(orig_prefix));
        sysevent_set(sefd, setok, "previous_ipv6_prefix", orig_prefix, 0);
    }
    sysevent_get(sefd, setok, "previous_ipv6_prefix_vldtime", prev_valid_lft, sizeof(prev_valid_lft));
    /* As per Sky requirement, hub should advertise lan bridge's ULA address as DNS address for lan clients as part of RA.
       In case the ULA is not available, lan bridge's LL address can be advertise as DNS address.
    */
    sysevent_get(sefd, setok, "ula_address", lan_addr, sizeof(lan_addr));
    sysevent_get(sefd, setok, "ula_prefix", lan_addr_prefix, sizeof(lan_addr_prefix));
#endif
#ifdef MULTILAN_FEATURE
    sysevent_get(sefd, setok, COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, preferred_lft, sizeof(preferred_lft));
    sysevent_get(sefd, setok, COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, valid_lft, sizeof(valid_lft));
#else
    sysevent_get(sefd, setok, "ipv6_prefix_prdtime", preferred_lft, sizeof(preferred_lft));
    sysevent_get(sefd, setok, "ipv6_prefix_vldtime", valid_lft, sizeof(valid_lft));
#endif
    syscfg_get(NULL, "lan_ifname", lan_if, sizeof(lan_if));
#endif
    if (atoi(preferred_lft) <= 0)
        snprintf(preferred_lft, sizeof(preferred_lft), "300");
    if (atoi(valid_lft) <= 0)
        snprintf(valid_lft, sizeof(valid_lft), "300");

    if ( atoi(preferred_lft) > atoi(valid_lft) )
        snprintf(preferred_lft, sizeof(preferred_lft), "%s",valid_lft);

#if defined (INTEL_PUMA7)
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    sysevent_get(sefd, setok, "wan-status", wan_st, sizeof(wan_st));	
#endif

#ifndef MULTILAN_FEATURE
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    get_active_lanif(sefd, setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sefd, setok, evt_name, lan_if, sizeof(lan_if));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", lan_if);
        sysevent_get(sefd, setok, evt_name, prefix, sizeof(prefix));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-addr", lan_if);
        sysevent_get(sefd, setok, evt_name, lan_addr, sizeof(lan_addr));
#endif
#else
    get_active_lanif(sefd, setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sefd, setok, evt_name, lan_if, sizeof(lan_if));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", lan_if);
        sysevent_get(sefd, setok, evt_name, prefix, sizeof(prefix));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-addr", lan_if);
        sysevent_get(sefd, setok, evt_name, lan_addr, sizeof(lan_addr));
#endif
    fprintf(fp, "# Based on prefix=%s, old_previous=%s, LAN IPv6 address=%s\n", 
            prefix, orig_prefix, lan_addr);

#if defined(_COSA_FOR_BCI_)
    if ((strlen(prefix) || strlen(orig_prefix)) && bEnabled)
#else
#ifndef _HUB4_PRODUCT_REQ_
    if (strlen(prefix) || strlen(orig_prefix))
#else
    if (strlen(prefix) || strlen(orig_prefix) || strlen(lan_addr_prefix))
#endif
#endif
	{
		char val_DNSServersEnabled[ 32 ];

#ifdef _HUB4_PRODUCT_REQ_
        syscfg_get(NULL, "dhcpv6s00::servertype", server_type, sizeof(server_type));
        if (strncmp(server_type, "1", 1) == 0) {
            syscfg_set(NULL, "router_managed_flag", "1");
        }
        else {
            syscfg_set(NULL, "router_managed_flag", "0");
        }
        syscfg_set(NULL, "router_other_flag", "1");
        syscfg_commit();
#endif
        fprintf(fp, "interface %s\n", lan_if);
        fprintf(fp, "   no ipv6 nd suppress-ra\n");
#ifdef _HUB4_PRODUCT_REQ_
        if (strlen(prefix) && (strncmp(server_type, "2", 1) == 0))
        {
            fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);
        }
        else if(strlen(prefix)) {
            fprintf(fp, "   ipv6 nd prefix %s 0 0\n", prefix);
        }
        if (strlen(orig_prefix)) {
            fprintf(fp, "   ipv6 nd prefix %s %s 0\n", orig_prefix, prev_valid_lft); //Previous prefix with '0' as the preferred time value
        }

        if (strlen(lan_addr_prefix) && (strncmp(server_type, "2", 1) == 0))
        {
            fprintf(fp, "   ipv6 nd prefix %s\n", lan_addr_prefix);
        }
        else if (strlen(lan_addr_prefix)) {
            fprintf(fp, "   ipv6 nd prefix %s 0 0\n", lan_addr_prefix);
        }
#else
        if (strlen(prefix))
        {
#if defined (INTEL_PUMA7)
            //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
            if (strcmp(wan_st, "stopped") == 0)
            	fprintf(fp, "   ipv6 nd prefix %s %s 0\n", prefix, valid_lft);
            else
            {
#endif
            fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);
#if defined (INTEL_PUMA7)
            }
#endif
        }
        if (strlen(orig_prefix))
            fprintf(fp, "   ipv6 nd prefix %s 300 0\n", orig_prefix);
#endif//_HUB4_PRODUCT_REQ_
#if defined (INTEL_PUMA7)
        //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
        // Read ra_interval from syscfg.db
        syscfg_get(NULL, "ra_interval", ra_interval, sizeof(ra_interval));
        if (strlen(ra_interval) > 0)
        {
            fprintf(fp, "   ipv6 nd ra-interval %s\n", ra_interval);
        } else
        {
            fprintf(fp, "   ipv6 nd ra-interval 30\n"); //Set ra-interval to default 30 secs as per Erouter Specs.
        }
#else
#ifndef _HUB4_PRODUCT_REQ_
        fprintf(fp, "   ipv6 nd ra-interval 3\n");
#else
        fprintf(fp, "   ipv6 nd ra-interval 180\n");
#endif //_HUB4_PRODUCT_REQ_
#endif

#if defined (INTEL_PUMA7)
        //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
        if (strcmp(wan_st, "stopped") == 0)
        	fprintf(fp, "   ipv6 nd ra-lifetime 0\n");
        else
        {
#endif
        fprintf(fp, "   ipv6 nd ra-lifetime 180\n");
#if defined (INTEL_PUMA7)
        }
#endif

        syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
        if (strcmp(m_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd managed-config-flag\n");

        syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
        if (strcmp(o_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        syscfg_get(NULL, "dhcpv6s_enable", dh6s_en, sizeof(dh6s_en));
        if (strcmp(dh6s_en, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        fprintf(fp, "   ipv6 nd router-preference medium\n");

	// During captive portal no need to pass DNS
	// Check the reponse code received from Web Service
   	if((responsefd = fopen(networkResponse, "r")) != NULL) 
   	{
       		if(fgets(responseCode, sizeof(responseCode), responsefd) != NULL)
       		{
		  	iresCode = atoi(responseCode);
          	}
            fclose(responsefd); /*RDKB-7136, CID-33268, free resource after use*/
            responsefd = NULL;
   	}
        syscfg_get( NULL, "redirection_flag", buf, sizeof(buf));
    	if( buf != NULL )
    	{
		if ((strncmp(buf,"true",4) == 0) && iresCode == 204)
		{
			inCaptivePortal = 1;        		
		}
	}
		/* Static DNS for DHCPv6 
		  *   dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers 
		  *   dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled
		  */
		memset( val_DNSServersEnabled, 0, sizeof( val_DNSServersEnabled ) );
		syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", val_DNSServersEnabled, sizeof(val_DNSServersEnabled));
		
		if( ( val_DNSServersEnabled[ 0 ] != '\0' ) && \
			 ( 0 == strcmp( val_DNSServersEnabled, "1" ) )
		   )
		{
			StaticDNSServersEnabled = 1;
		}

// Modifying rdnss value to fix the zebra config.
	if( ( inCaptivePortal != 1 )  && \
		( StaticDNSServersEnabled != 1 )
	  )
	{
		if (strlen(lan_addr))
            			fprintf(fp, "   ipv6 nd rdnss %s 86400\n", lan_addr);
	}
        /* static IPv6 DNS */
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION          
            snprintf(rec, sizeof(rec), "dhcpv6spool%d0::optionnumber", i);
            syscfg_get(NULL, rec, val, sizeof(val));
#else
        syscfg_get(NULL, "dhcpv6spool00::optionnumber", val, sizeof(val));
#endif
        nopt = atoi(val);
        for (j = 0; j < nopt; j++) {
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION              
             memset(name_servs, 0, sizeof(name_servs));
#endif
            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::bEnabled", j); /*RDKB-12965 & CID:-34147*/
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 1)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Tag", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 23)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::PassthroughClient", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) > 0)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Value", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) == 0)
                continue;

            for (start = val; (tok = strtok_r(start, ", \r\t\n", &sp)); start = NULL) {
                snprintf(name_servs + strlen(name_servs), 
                        sizeof(name_servs) - strlen(name_servs), "%s ", tok);
            }
        }

	if(inCaptivePortal != 1)
	{
			/* Static DNS Enabled case */
			if( 1 == StaticDNSServersEnabled )
			{
				memset( name_servs, 0, sizeof( name_servs ) );
				syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers", name_servs, sizeof(name_servs));
				fprintf(stderr,"%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__, 
																			   __LINE__,
																			   StaticDNSServersEnabled,
																			   name_servs );
			}
			else
			{
				/* DNS from WAN (if no static DNS) */
				if (strlen(name_servs) == 0) {
					sysevent_get(sefd, setok, "ipv6_nameserver", name_servs + strlen(name_servs), 
						sizeof(name_servs) - strlen(name_servs));
				}
			}

			for (start = name_servs; (tok = strtok_r(start, " ", &sp)); start = NULL)
			{
			// Modifying rdnss value to fix the zebra config.
        		fprintf(fp, "   ipv6 nd rdnss %s 86400\n", tok);
			}
#ifdef MULTILAN_FEATURE
                        if (atoi(valid_lft) <= 3*atoi(ra_interval))
                        {
                             // According to RFC8106 section 5.2 dnssl lifttime must be atleast 3 time MaxRtrAdvInterval.
                             dnssllft = 3*atoi(ra_interval);
                             snprintf(dnssl_lft, sizeof(dnssl_lft), "%d", dnssllft);
                        }
                        else
                        {
                             snprintf(dnssl_lft, sizeof(dnssl_lft), "%s", valid_lft);
                        }
                        sysevent_get(sefd, setok, "ipv6_dnssl", dnssl, sizeof(dnssl));
                        for(start = dnssl; (tok = strtok_r(start, " ", &sp)); start = NULL)
                        {
                              fprintf(fp, "   ipv6 nd dnssl %s %s\n", tok, dnssl_lft);
                        }

#endif
		}
	}
    

    fprintf(fp, "interface %s\n", lan_if);
    fprintf(fp, "   ip irdp multicast\n");
#ifndef MULTILAN_FEATURE
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION        
    }
#endif
#else
}
#endif

#ifndef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
char cmd[100];
char out[100];
char *token = NULL; 
char s[2] = ",";
char *pt;
char pref_rx[16];
int pref_len = 0;
memset(out,0,sizeof(out));
memset(pref_rx,0,sizeof(pref_rx));
sysevent_get(sefd, setok,"lan_prefix_v6", pref_rx, sizeof(pref_rx));
syscfg_get(NULL, "IPv6subPrefix", out, sizeof(out));
pref_len = atoi(pref_rx);
if(pref_len < 64)
{
if(!strncmp(out,"true",strlen(out)))
{
	memset(out,0,sizeof(out));
	memset(cmd,0,sizeof(cmd));
	syscfg_get(NULL, "IPv6_Interface", out, sizeof(out));
	pt = out;
	while((token = strtok_r(pt, ",", &pt)))
	{
        fprintf(fp, "interface %s\n", token);
        fprintf(fp, "   no ipv6 nd suppress-ra\n");
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd, "%s%s",token,"_ipaddr_v6");
		memset(prefix,0,sizeof(prefix));
		sysevent_get(sefd, setok, cmd, prefix, sizeof(prefix));
        if (strlen(prefix) != 0)
            fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);

        fprintf(fp, "   ipv6 nd ra-interval 3\n");
        fprintf(fp, "   ipv6 nd ra-lifetime 180\n");

        syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
        if (strcmp(m_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd managed-config-flag\n");

        syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
        if (strcmp(o_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        syscfg_get(NULL, "dhcpv6s_enable", dh6s_en, sizeof(dh6s_en));
        if (strcmp(dh6s_en, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");

        fprintf(fp, "   ipv6 nd router-preference medium\n");
    	if(inCaptivePortal != 1)
        {
                        /* Static DNS Enabled case */
                        if( 1 == StaticDNSServersEnabled )
                        {
                                memset( name_servs, 0, sizeof( name_servs ) );
                                syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers", name_servs, sizeof(name_servs));
                                fprintf(stderr,"%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                           __LINE__,
                                                                                                                                                           StaticDNSServersEnabled,
                                                                                                                                                           name_servs );
                        }
                        else
                        {
                                /* DNS from WAN (if no static DNS) */
                                if (strlen(name_servs) == 0) {
                                        sysevent_get(sefd, setok, "ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                                }
                        }

                        for (start = name_servs; (tok = strtok_r(start, " ", &sp)); start = NULL)
                        {
                        // Modifying rdnss value to fix the zebra config.
                        fprintf(fp, "   ipv6 nd rdnss %s 86400\n", tok);
                        }
         }

	fprintf(fp, "interface %s\n", token);
    	fprintf(fp, "   ip irdp multicast\n");
	}
	memset(out,0,sizeof(out));
}
}
#endif
    fclose(fp);
    return 0;
}

static int gen_ripd_conf(int sefd, token_t setok)
{
    /* should be similar to CosaDmlGenerateRipdConfigFile(), 
     * but there're too many dependencies for that function.
     * It's not good, but DM will generate that file */
    return 0;
}

static int radv_start(struct serv_routed *sr)
{
#if defined(_COSA_FOR_BCI_)
    char dhcpv6Enable[8]={0};
#endif
    /* XXX: 
     * 1) even IPv4 only zebra should start (ripd need it) !
     * 2) IPv6-only do not use wan-status  */
#if 0
    char rtmod[16];

    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    if (atoi(rtmod) != 2 && atoi(rtmod) != 3) { /* IPv6 or Dual-Stack */
        fprintf(stderr, "%s: last_erouter_mode %s\n", __FUNCTION__, rtmod);
        return 0;
    }

    if (!sr->lan_ready || !sr->wan_ready) {
        fprintf(stderr, "%s: LAN or WAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#else
    if (!sr->lan_ready) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#endif

    if (gen_zebra_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to save zebra config\n", __FUNCTION__);
        return -1;
    }

    daemon_stop(ZEBRA_PID_FILE, "zebra");
#if defined(_COSA_FOR_BCI_)
    syscfg_get(NULL, "dhcpv6s00::serverenable", dhcpv6Enable , sizeof(dhcpv6Enable));
    bool bEnabled = (strncmp(dhcpv6Enable,"1",1)==0?true:false);

    vsystem("zebra -d -f %s -u root -P 0", ZEBRA_CONF_FILE);
    printf("DHCPv6 is %s. Starting zebra Process\n", (bEnabled?"Enabled":"Disabled"));
#else
    vsystem("zebra -d -f %s -u root -P 0", ZEBRA_CONF_FILE);
#endif

    return 0;
}

static int radv_stop(struct serv_routed *sr)
{
    return daemon_stop(ZEBRA_PID_FILE, "zebra");
}

static int radv_restart(struct serv_routed *sr)
{
    if (radv_stop(sr) != 0)
        fprintf(stderr, "%s: radv_stop error\n", __FUNCTION__);

    return radv_start(sr);
}

static int rip_start(struct serv_routed *sr)
{
    char enable[16];

    if (!serv_can_start(sr->sefd, sr->setok, "rip"))
        return -1;
#ifndef _HUB4_PRODUCT_REQ_
    if (!sr->lan_ready || !sr->wan_ready) {
        fprintf(stderr, "%s: LAN or WAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#else
    if (!sr->lan_ready) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#endif//_HUB4_PRODUCT_REQ_
    syscfg_get(NULL, "rip_enabled", enable, sizeof(enable));
    if (strcmp(enable, "1") != 0) {
        fprintf(stderr, "%s: RIP not enabled\n", __FUNCTION__);
        return 0;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "starting", 0);

    if (gen_ripd_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to generate ripd config\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    if (vsystem("ripd -d -f %s -u root", RIPD_CONF_FILE) != 0) {
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "started", 0);
    return 0;
}

static int rip_stop(struct serv_routed *sr)
{
    if (!serv_can_stop(sr->sefd, sr->setok, "rip"))
        return -1;

    sysevent_set(sr->sefd, sr->setok, "rip-status", "stopping", 0);

    if (daemon_stop(RIPD_PID_FILE, "ripd") != 0) {
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "rip-status", "stopped", 0);
    return 0;
}

static int rip_restart(struct serv_routed *sr)
{
    if (rip_stop(sr) != 0)
        fprintf(stderr, "%s: rip_stop error\n", __FUNCTION__);

    return rip_start(sr);
}

static int serv_routed_start(struct serv_routed *sr)
{
    char status[16], enable[16], rtmod[16];
    char prefix[64];

    /* state check */
    if (!serv_can_start(sr->sefd, sr->setok, "routed"))
        return -1;

    if (!sr->lan_ready) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return -1;
    }
#ifndef _HUB4_PRODUCT_REQ_
    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    if (atoi(rtmod) != 2) { /* IPv4-only or Dual-Stack */
        if (!sr->wan_ready) {
            fprintf(stderr, "%s: IPv4-WAN is not ready !\n", __FUNCTION__);
            return -1;
        }
    } else { /* IPv6-only */
        sysevent_get(sr->sefd, sr->setok, "ipv6_prefix", prefix, sizeof(prefix));
        if (strlen(prefix) == 0) {
            fprintf(stderr, "%s: IPv6-WAN is not ready !\n", __FUNCTION__);
            return -1;
        }
    }
#endif//_HUB4_PRODUCT_REQ_
    sysevent_set(sr->sefd, sr->setok, "routed-status", "starting", 0);

    /* RA daemon */
    if (radv_start(sr) != 0) {
        fprintf(stderr, "%s: radv_start error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* RIP daemon */
    if (rip_start(sr) != 0) {
        fprintf(stderr, "%s: rip_start error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* route and policy routes */
    if (route_set(sr) != 0) {
        fprintf(stderr, "%s: route_set error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    /* nfq & firewall */
    if (fw_restart(sr) != 0) {
        fprintf(stderr, "%s: fw_restart error\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "routed-status", "error", 0);
        return -1;
    }

    sysevent_set(sr->sefd, sr->setok, "routed-status", "started", 0);
    return 0;
}

static int serv_routed_stop(struct serv_routed *sr)
{
    if (!serv_can_stop(sr->sefd, sr->setok, "routed"))
        return -1;

    sysevent_set(sr->sefd, sr->setok, "routed-status", "stopping", 0);

    if (route_unset(sr) != 0)
        fprintf(stderr, "%s: route_unset error\n", __FUNCTION__);

    if (rip_stop(sr) != 0)
        fprintf(stderr, "%s: rip_stop error\n", __FUNCTION__);

    if (radv_stop(sr) != 0)
        fprintf(stderr, "%s: radv_stop error\n", __FUNCTION__);

    if (fw_restart(sr) != 0)
        fprintf(stderr, "%s: fw_restart error\n", __FUNCTION__);

    sysevent_set(sr->sefd, sr->setok, "routed-status", "stopped", 0);
    return 0;
}

static int serv_routed_restart(struct serv_routed *sr)
{
    if (serv_routed_stop(sr) != 0)
        fprintf(stderr, "%s: serv_routed_stop error\n", __FUNCTION__);

    return serv_routed_start(sr);
}

static int serv_routed_init(struct serv_routed *sr)
{
    char wan_st[16], lan_st[16];
    char buf[16];

    memset(sr, 0, sizeof(struct serv_routed));

    if ((sr->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT, 
                    SE_VERSION, PROG_NAME, &sr->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }

    if (route_enable() != 0) {
        fprintf(stderr, "%s: faild to restore Routing Enabled state\n", __FUNCTION__);
    }

    sysevent_get(sr->sefd, sr->setok, "wan-status", wan_st, sizeof(wan_st));
    if (strcmp(wan_st, "started") == 0)
        sr->wan_ready = true;

    sysevent_get(sr->sefd, sr->setok, "lan-status", lan_st, sizeof(lan_st));
    if (strcmp(lan_st, "started") == 0)
        sr->lan_ready = true;

    return 0;
}

static int serv_routed_term(struct serv_routed *sr)
{
    sysevent_close(sr->sefd, sr->setok);
    return 0;
}

struct cmd_op {
    const char  *cmd;
    int         (*exec)(struct serv_routed *sr);
    const char  *desc;
};

static struct cmd_op cmd_ops[] = {
    {"start",       serv_routed_start,  "start service route daemons"},
    {"stop",        serv_routed_stop,   "stop service route daemons"},
    {"restart",     serv_routed_restart,"restart service route daemons"},
    {"route-set",   route_set,      "set route entries"},
    {"route-unset", route_unset,    "unset route entries"},
    {"rip-start",   rip_start,      "start RIP daemon"},
    {"rip-stop",    rip_stop,       "stop RIP daemon"},
    {"rip-restart", rip_restart,    "restart RIP daemon"},
    {"radv-start",  radv_start,     "start RA daemon"},
    {"radv-stop",   radv_stop,      "stop RA daemon"},
    {"radv-restart",radv_restart,   "restart RA daemon"},
};

static void usage(void)
{
    int i;

    fprintf(stderr, "USAGE\n");
    fprintf(stderr, "    %s COMMAND\n", PROG_NAME);
    fprintf(stderr, "COMMANDS\n");
    for (i = 0; i < NELEMS(cmd_ops); i++)
        fprintf(stderr, "    %-20s%s\n", cmd_ops[i].cmd, cmd_ops[i].desc);
}

int main(int argc, char *argv[])
{
    int i;
    struct serv_routed sr;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_routed_init(&sr) != 0)
        exit(1);

    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] EXEC: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&sr) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_routed_term(&sr) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}
