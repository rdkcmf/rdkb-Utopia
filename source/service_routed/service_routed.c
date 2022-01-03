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
#include "safec_lib_common.h"

#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_) || defined (_CBR2_PRODUCT_REQ_)
#include <sys/stat.h>
#endif

#include "util.h"
#include <telemetry_busmessage_sender.h>
#include "syscfg/syscfg.h"
#ifdef _HUB4_PRODUCT_REQ_
#include "utapi.h"
#include "utapi_util.h"
#include "ccsp_dm_api.h"
#include "ccsp_custom.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#endif
#include "secure_wrapper.h"
#define PROG_NAME       "SERVICE-ROUTED"

#define ZEBRA_PID_FILE  "/var/zebra.pid"
#define RIPD_PID_FILE   "/var/ripd.pid"
#define ZEBRA_CONF_FILE "/var/zebra.conf"

#if defined (_BWG_PRODUCT_REQ_) || defined (ARRIS_XB3_PLATFORM_CHANGES)
#define RIPD_CONF_FILE  "/var/ripd.conf"
#else
#define RIPD_CONF_FILE  "/etc/ripd.conf"
#endif

#define RA_INTERVAL 60

#ifdef _HUB4_PRODUCT_REQ_
#define LAN_BRIDGE "brlan0"
#define CCSP_SUBSYS  "eRT."
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str))
#define PSM_LANMANAGEMENTENTRY_LAN_IPV6_ENABLE "dmsb.lanmanagemententry.lanipv6enable"
#define PSM_LANMANAGEMENTENTRY_LAN_ULA_ENABLE  "dmsb.lanmanagemententry.lanulaenable"
#define SYSEVENT_VALID_ULA_ADDRESS "valid_ula_address"

static void* bus_handle = NULL;
static const char* const service_routed_component_id = "ccsp.routed";
static int getULAAddressFromInterface(char *ulaAddress);
#endif

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

#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_) || defined (_CBR2_PRODUCT_REQ_)
#ifdef _BWG_PRODUCT_REQ_
#define LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#else
#define LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#endif
#define DEG_PRINT(fmt ...)   {\
   FILE *logfp = fopen ( LOG_FILE , "a+");\
   if (logfp)\
   {\
        fprintf(logfp,fmt);\
        fclose(logfp);\
   }\
}\

#define RIPD_CONF_PAM_UPDATE "/tmp/pam_ripd_config_completed"
static int IsFileExists(char *file_name)
{
    struct stat file;
    return (stat(file_name, &file));
}
#endif

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
    t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
    sysevent_set(sr->sefd, sr->setok, "firewall-restart", NULL, 0);
    return 0;
}
#ifdef _HUB4_PRODUCT_REQ_
static int dbusInit( void )
{
    int ret = 0;
    char* pCfg = CCSP_MSG_BUS_CFG;
    if (bus_handle == NULL)
    {
#ifdef DBUS_INIT_SYNC_MODE
        ret = CCSP_Message_Bus_Init_Synced(service_routed_component_id,
                                           pCfg,
                                           &bus_handle,
                                           Ansc_AllocateMemory_Callback,
                                           Ansc_FreeMemory_Callback);
#else
        ret = CCSP_Message_Bus_Init((char *)service_routed_component_id,
                                    pCfg,
                                    &bus_handle,
                                    (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,
                                    Ansc_FreeMemory_Callback);
#endif
        if (ret == -1)
        {
            fprintf(stderr, "DBUS connection error\n");
        }
    }
    return ret;
}

static int getLanIpv6Info(int *ipv6_enable, int *ula_enable)
{
    char *pIpv6_enable, *pUla_enable;

    if(CCSP_SUCCESS != PSM_VALUE_GET_STRING(PSM_LANMANAGEMENTENTRY_LAN_IPV6_ENABLE, pIpv6_enable)) {
        Ansc_FreeMemory_Callback(pIpv6_enable);
        return -1;
    }

    if(CCSP_SUCCESS != PSM_VALUE_GET_STRING(PSM_LANMANAGEMENTENTRY_LAN_ULA_ENABLE, pUla_enable)) {
        Ansc_FreeMemory_Callback(pUla_enable);
        return -1;
    }

    if ( strncmp(pIpv6_enable, "TRUE", 4 ) == 0) {
        *ipv6_enable = TRUE;
    }
    else {
        *ipv6_enable = FALSE;
    }

    if ( strncmp(pUla_enable, "TRUE", 4 ) == 0) {
        *ula_enable = TRUE;
    }
    else {
        *ula_enable = FALSE;
    }

    Ansc_FreeMemory_Callback(pUla_enable);
    Ansc_FreeMemory_Callback(pIpv6_enable);

    return 0;
}
static int getULAAddressFromInterface(char *ulaAddress)
{
    int status = FALSE;
    FILE *fpStream = NULL;
    char cmd[128] = "ifconfig brlan0 | grep inet6 | grep Global| awk '/inet6/{print $3}' | cut -d\"/\" -f1";
    char line[128] = {0};

    fpStream = popen(cmd,"r");
    if (fpStream != NULL)
    {
        while ( NULL != fgets ( line, sizeof (line), fpStream ) )
        {
            char *p = NULL;

            //Removing new line from string
            p = strchr(line, '\n');
            if(p)
            {
                *p = 0;
            }
            if (!strncmp(line, "fd", 2) || !strncmp(line, "fc", 2))
            {
                strncpy(ulaAddress, line, strlen(line)+1);
                status = TRUE;
                break;
            }
            memset (line,0, sizeof(line));
        }
        pclose(fpStream);
    }
    else
    {
        fprintf(stderr, "%s: Unable to open stream \n", __FUNCTION__);
        status = FALSE;
    }
    return status;
}

#endif
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

/* SKYH4-1765: checks the daemon running status */
static int is_daemon_running(const char *pid_file, const char *prog)
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
	return pid;
    }

    return 0;
}

#ifdef MULTILAN_FEATURE
static int get_active_lanif(int sefd, token_t setok, unsigned int *insts, unsigned int *num)
{
    char active_insts[32] = {0};
    char *p = NULL;
    int i = 0;

#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION

    char lan_pd_if[128] = {0};
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

static int route_set(struct serv_routed *sr)
{
#if defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(MULTILAN_FEATURE)
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
    char lan_if[32] = {0};
    int i;

    get_active_lanif(sr->sefd, sr->setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sr->sefd, sr->setok, evt_name, lan_if, sizeof(lan_if));

        v_secure_system("ip -6 rule add iif %s table all_lans", lan_if);
        v_secure_system("ip -6 rule add iif %s table erouter", lan_if);
    }
#endif

#ifdef _HUB4_PRODUCT_REQ_
    /*Clean 'iif brlan0 table erouter' if exist already*/
    vsystem("ip -6 rule del iif brlan0 table erouter");
#endif

#if defined (MULTILAN_FEATURE)
    /* Test to see if the default route for erouter0 is not empty and the default
       route for router table is missing before trying to add a new default route
       for erouter table via erouter0 to prevent vsystem returning error */
    if (vsystem("gw=$(ip -6 route show default dev erouter0 | awk '/via/ {print $3}');"
            "dr=$(ip -6 route show default dev erouter0 table erouter);"
            "if [ \"$gw\" != \"\" -a \"$dr\" = \"\" ]; then"
             "  ip -6 route add default via $gw dev erouter0 table erouter;"
             "fi") != 0)
         return -1;
    return 0;
#else
    if (vsystem("ip -6 rule add iif brlan0 table erouter;"
            "gw=$(ip -6 route show default dev erouter0 | awk '/via/ {print $3}');"
            "if [ \"$gw\" != \"\" ]; then"
            "  ip -6 route add default via $gw dev erouter0 table erouter;"
            "fi") != 0)
        return -1;
    return 0;
#endif
}

static int route_unset(struct serv_routed *sr)
{
#if defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(MULTILAN_FEATURE)
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
    char lan_if[32] = {0};
    int i;

    get_active_lanif(sr->sefd, sr->setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sr->sefd, sr->setok, evt_name, lan_if, sizeof(lan_if));

        v_secure_system("ip -6 rule del iif %s table all_lans", lan_if);
        v_secure_system("ip -6 rule del iif %s table erouter", lan_if);
    }

#elif _HUB4_PRODUCT_REQ_
    vsystem("ip -6 rule del iif brlan0 table erouter");
    if (vsystem("ip -6 route del default dev erouter0 table erouter") != 0) {
        return -1;
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
    char l_cSecWebUI_Enabled[8] = {0};
    syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
    if (!strncmp(l_cSecWebUI_Enabled, "true", 4))	
    {
        syscfg_set(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", "1");
        syscfg_commit();
         
        FILE *fptr = NULL;
        char loc_domain[128] = {0};
        char loc_ip6[256] = {0};
        sysevent_get(sefd, setok, "lan_ipaddr_v6", loc_ip6, sizeof(loc_ip6));
        syscfg_get(NULL, "SecureWebUI_LocalFqdn", loc_domain, sizeof(loc_domain));
        FILE *ptr;
        char buff[10];
        if ((ptr=v_secure_popen("r", "grep %s /etc/hosts",loc_ip6))!=NULL)
        if (NULL != ptr)
        {
            if (NULL == fgets(buff, 9, ptr)) {
                fptr =fopen("/etc/hosts", "a");
                if (fptr != NULL)
                {
                    if ( loc_ip6 != NULL)
                    {
                        if (loc_domain != NULL)
                        {
                            fprintf(fptr, "%s      %s\n",loc_ip6,loc_domain);
                        }
                    }
                    fclose(fptr);
                }
            }
            v_secure_pclose(ptr);
        }
    }
    else
    {
	char l_cDhcpv6_Dns[256] = {0};
        syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers", l_cDhcpv6_Dns, sizeof(l_cDhcpv6_Dns));
        if ( '\0' == l_cDhcpv6_Dns[ 0 ] )
        {
            syscfg_set(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", "0");
            syscfg_commit();
        }
    }
    FILE *fp = NULL;
    char rtmod[16], ra_en[16], dh6s_en[16];
    char ra_interval[8] = {0};
    char name_servs[1024] = {0};
    char dnssl[2560] = {0};
    char dnssl_lft[16];
    unsigned int dnssllft = 0;
    char prefix[64], orig_prefix[64], lan_addr[64];
    char preferred_lft[16], valid_lft[16];
#ifndef _HUB4_PRODUCT_REQ_
    unsigned int rdnsslft = 0;
#endif    
#if defined(MULTILAN_FEATURE)
    char orig_lan_prefix[64];
#endif
    char m_flag[16], o_flag[16];
    char rec[256], val[512];
    char buf[6];
    FILE *responsefd = NULL;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    int inCaptivePortal = 0,inWifiCp=0;
#if defined (_XB6_PROD_REQ_)
    int inRfCaptivePortal = 0;
    char rfCpMode[6] = {0};
    char rfCpEnable[6] = {0};
#endif
    int nopt, j = 0; /*RDKB-12965 & CID:-34147*/
    char lan_if[IFNAMSIZ];
    char *start, *tok, *sp;
    static const char *zebra_conf_base = \
        "!enable password admin\n"
        "!log stdout\n"
        "log file /var/tmp/zebra.log errors\n"
        "table 255\n";
#if defined(MULTILAN_FEATURE) || defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION)
    int i = 0;
    unsigned int l2_insts[4] = {0};
    unsigned int enabled_iface_num = 0;
    char evt_name[64] = {0};
#endif
    int  StaticDNSServersEnabled = 0;
#ifdef _HUB4_PRODUCT_REQ_
    char lan_addr_prefix[64] = {0};
#endif
    char wan_st[16] = {0};

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
    syscfg_get(NULL, "ra_interval", ra_interval, sizeof(ra_interval));
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    sysevent_get(sefd, setok, "previous_ipv6_prefix", orig_prefix, sizeof(orig_prefix));
    sysevent_get(sefd, setok, "ipv6_prefix_prdtime", preferred_lft, sizeof(preferred_lft));
    sysevent_get(sefd, setok, "ipv6_prefix_vldtime", valid_lft, sizeof(valid_lft));
#else
    sysevent_get(sefd, setok, "lan_prefix", prefix, sizeof(prefix));
    sysevent_get(sefd, setok, "previous_ipv6_prefix", orig_prefix, sizeof(orig_prefix));
#ifndef _HUB4_PRODUCT_REQ_
    sysevent_get(sefd, setok, "current_lan_ipv6address", lan_addr, sizeof(lan_addr));
#else
    result = getLanIpv6Info(&ipv6_enable, &ula_enable);
    if(result != 0) {
        fprintf(stderr, "getLanIpv6Info failed");
        return -1;
    }
    sysevent_get(sefd, setok, "previous_ipv6_prefix_vldtime", prev_valid_lft, sizeof(prev_valid_lft));
    /* As per Sky requirement, hub should advertise lan bridge's ULA address as DNS address for lan clients as part of RA.
       In case the ULA is not available, lan bridge's LL address can be advertise as DNS address.
    */
    sysevent_get(sefd, setok, "ula_address", lan_addr, sizeof(lan_addr));

    if (IsValid_ULAAddress(lan_addr) == FALSE)
    {
        char ula_address_brlan[64] = {0};

        if (getULAAddressFromInterface(ula_address_brlan) == TRUE)
        {
            fprintf(stderr, "%s: ula_address_brlan: %s\n", __FUNCTION__, ula_address_brlan);
            sysevent_set(sefd, setok, "ula_address", ula_address_brlan, sizeof(ula_address_brlan));
            sysevent_set(sefd, setok, SYSEVENT_VALID_ULA_ADDRESS, "true", 0);
        }
        else
        {
            sysevent_set(sefd, setok, SYSEVENT_VALID_ULA_ADDRESS, "false", 0);
        }
    }

    if(ula_enable == 1)
        sysevent_get(sefd, setok, "ula_prefix", lan_addr_prefix, sizeof(lan_addr_prefix));
#endif//_HUB4_PRODUCT_REQ_

    // If the current prefix is the same as the previous prefix, no need to advertise the previous one with a lifetime of 0
    if(strncmp(prefix, orig_prefix, 64) == 0) {
        strncpy(orig_prefix, "", sizeof(orig_prefix));
        sysevent_set(sefd, setok, "previous_ipv6_prefix", orig_prefix, 0);
    }

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

    sysevent_get(sefd, setok, "wan-status", wan_st, sizeof(wan_st));
    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));

#if defined(MULTILAN_FEATURE) || defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION)
    get_active_lanif(sefd, setok, l2_insts, &enabled_iface_num);
    for (i = 0; i < enabled_iface_num; i++)
    {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(sefd, setok, evt_name, lan_if, sizeof(lan_if));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", lan_if);
        sysevent_get(sefd, setok, evt_name, prefix, sizeof(prefix));
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-addr", lan_if);
        sysevent_get(sefd, setok, evt_name, lan_addr, sizeof(lan_addr));
#endif
#if defined (_COSA_BCM_MIPS_)
       if (strlen(prefix) == 0)
         {
           sysevent_get(sefd, setok, "lan_prefix", prefix, sizeof(prefix));
         }
#endif

#if defined(MULTILAN_FEATURE)
        snprintf(evt_name, sizeof(evt_name), "previous_ipv6_%s-prefix", lan_if);
        sysevent_get(sefd, setok, evt_name, orig_lan_prefix, sizeof(orig_lan_prefix));

        //If previous prefix is the same as current one, no need to advertise with lifetime 0
        if(strncmp(prefix, orig_lan_prefix, 64) == 0) {
            strncpy(orig_lan_prefix, "", sizeof(orig_prefix));
            snprintf(evt_name, sizeof(evt_name), "previous_ipv6_%s-prefix", lan_if);
            sysevent_set(sefd, setok, evt_name, orig_lan_prefix, 0);
        }
#endif

#if defined (MULTILAN_FEATURE)
        fprintf(fp, "# Based on prefix=%s, old_previous=%s, LAN IPv6 address=%s\n", 
            prefix, orig_lan_prefix, lan_addr);
#else
        fprintf(fp, "# Based on prefix=%s, old_previous=%s, LAN IPv6 address=%s\n", 
            prefix, orig_prefix, lan_addr);
#endif

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
        if(strlen(orig_prefix)) { //SKYH4-1765: we add only the latest prefix data to zebra.conf.
            fprintf(fp, "   ipv6 nd prefix %s %s 0\n", orig_prefix, prev_valid_lft); //Previous prefix with '0' as the preferred time value

            // set previous_ipv6_prefix to EMPTY, since previous_ipv6_prefix pass to zebra for One time only
            strncpy(orig_prefix, "", sizeof(orig_prefix));
            sysevent_set(sefd, setok, "previous_ipv6_prefix", orig_prefix, 0);
        }
        else if (strlen(prefix) && (strncmp(server_type, "2", 1) == 0))
        {
            fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);
        }
        else if(strlen(prefix)) {
            fprintf(fp, "   ipv6 nd prefix %s 0 0\n", prefix);
        }

        if (strlen(lan_addr_prefix) && (strncmp(server_type, "2", 1) == 0))
        {
            fprintf(fp, "   ipv6 nd prefix %s\n", lan_addr_prefix);
        }
        else if (strlen(lan_addr_prefix)) {
            fprintf(fp, "   ipv6 nd prefix %s 0 0\n", lan_addr_prefix);
        }
#else
            //Do not write a config line for the prefix if it's blank
            if (strlen(prefix))
            {
                //If WAN has stopped, advertise the prefix with lifetime 0 so LAN clients don't use it any more
                if (strcmp(wan_st, "stopped") == 0)
                {
                    fprintf(fp, "   ipv6 nd prefix %s 0 0\n", prefix);
                }
                else
                {
                    fprintf(fp, "   ipv6 nd prefix %s %s %s\n", prefix, valid_lft, preferred_lft);
                }
            }

#if defined (MULTILAN_FEATURE)
            if (strlen(orig_lan_prefix))
                fprintf(fp, "   ipv6 nd prefix %s 0 0\n", orig_lan_prefix);
#else
            if (strlen(orig_prefix))
                fprintf(fp, "   ipv6 nd prefix %s 0 0\n", orig_prefix);
#endif

#endif//_HUB4_PRODUCT_REQ_
#if defined (INTEL_PUMA7)
            //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
            // Use ra_interval from syscfg.db
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

#ifndef _HUB4_PRODUCT_REQ_
            /* If WAN is stopped or not in IPv6 or dual stack mode, send RA with router lifetime of zero */
            if ( (strcmp(wan_st, "stopped") == 0) || (atoi(rtmod) != 2 && atoi(rtmod) != 3) )
            {
                fprintf(fp, "   ipv6 nd ra-lifetime 0\n");
            }
            else
            {
                fprintf(fp, "   ipv6 nd ra-lifetime 180\n");
            }
#else
	/* SKYH4-5324 : Selfheal is not working from IPv6 only client.
	 * The Router Life time should not change even after wan disconnection for SKYHUB4.
	 * Requirement of SelfHeal feature */
        fprintf(fp, "   ipv6 nd ra-lifetime 540\n");
#endif //_HUB4_PRODUCT_REQ_

        syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
        if (strcmp(m_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd managed-config-flag\n");
#ifdef _HUB4_PRODUCT_REQ_
            else if (strcmp(m_flag, "0") == 0)
                fprintf(fp, "   no ipv6 nd managed-config-flag\n");
#endif

        syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
        if (strcmp(o_flag, "1") == 0)
            fprintf(fp, "   ipv6 nd other-config-flag\n");
#ifdef _HUB4_PRODUCT_REQ_
            else if (strcmp(o_flag, "0") == 0)
                fprintf(fp, "   no ipv6 nd other-config-flag\n");
#endif
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
#if defined (_COSA_BCM_MIPS_)
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
                 // For CBR platform, the captive portal redirection feature was removed
                 // inWifiCp = 1;
#else
			inWifiCp = 1;
#endif
#else
            inWifiCp = 1;
#endif
		}
	}
#if defined (_XB6_PROD_REQ_)
        syscfg_get(NULL, "enableRFCaptivePortal", rfCpEnable, sizeof(rfCpEnable));
        if(rfCpEnable != NULL)
        {
          if (strncmp(rfCpEnable,"true",4) == 0)
          {
              syscfg_get(NULL, "rf_captive_portal", rfCpMode,sizeof(rfCpMode));
              if(rfCpMode != NULL)
              {
                 if (strncmp(rfCpMode,"true",4) == 0)
                 {
                    inRfCaptivePortal = 1;
                 }
              }
          } 
        }
        if((inWifiCp == 1) || (inRfCaptivePortal == 1))
        {
            inCaptivePortal = 1;
        }
#else
        if(inWifiCp == 1)
           inCaptivePortal = 1;
#endif
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
#ifndef _HUB4_PRODUCT_REQ_
		if (strlen(lan_addr))
#else
                if (strlen(lan_addr) && ula_enable)
#endif
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
                                if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                {
                                    char static_dns[256] = {0};
                                    sysevent_get(sefd, setok, "lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                    fprintf(fp, "   ipv6 nd rdnss %s 86400\n", static_dns);
                                    if (strlen(name_servs) == 0) {
                                        sysevent_get(sefd, setok, "ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                                    }
                                }
                                    
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
#ifdef _HUB4_PRODUCT_REQ_
                        if (0 == strncmp(lan_addr, tok, strlen(lan_addr)))
                        {
                            fprintf(fp, "   ipv6 nd rdnss %s 86400\n", tok);
                        }
#else
                        rdnsslft = 3 * RA_INTERVAL;
                        fprintf(fp, "   ipv6 nd rdnss %s %d\n", tok, rdnsslft);
#endif
                }

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


		}
	}
    

    fprintf(fp, "interface %s\n", lan_if);
    fprintf(fp, "   ip irdp multicast\n");

#if defined(MULTILAN_FEATURE) || defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION)
    } //for (i = 0; i < enabled_iface_num; i++)
#endif

#ifndef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
char cmd[100];
char out[100];
char interface_name[32] = {0};
char *token = NULL; 
char *pt;
char pref_rx[16];
int pref_len = 0;
errno_t  rc = -1;
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

        	memset(interface_name,0,sizeof(interface_name));
                #ifdef _COSA_INTEL_XB3_ARM_
                char LnFIfName[32] = {0} , LnFBrName[32] = {0} ;
                syscfg_get( NULL, "iot_ifname", LnFIfName, sizeof(LnFIfName));
                if( (LnFIfName[0] != '\0' ) && ( strlen(LnFIfName) != 0 ) )
                {
                        if (strcmp((const char*)token,LnFIfName) == 0 )
                        {
                                syscfg_get( NULL, "iot_brname", LnFBrName, sizeof(LnFBrName));
                                if( (LnFBrName[0] != '\0' ) && ( strlen(LnFBrName) != 0 ) )
                                {
                                        strncpy(interface_name,LnFBrName,sizeof(interface_name)-1);
                                }
                                else
                                {
                                	strncpy(interface_name,token,sizeof(interface_name)-1);
                                }
                        }
                        else
                        {
                        	strncpy(interface_name,token,sizeof(interface_name)-1);
                        }
                }
                else
                {
                        strncpy(interface_name,token,sizeof(interface_name)-1);
                }
                #else
                        strncpy(interface_name,token,sizeof(interface_name)-1);
                #endif 
        	fprintf(fp, "interface %s\n", interface_name);
        	fprintf(fp, "   no ipv6 nd suppress-ra\n");
		rc = sprintf_s(cmd, sizeof(cmd), "%s%s",interface_name,"_ipaddr_v6");
		if(rc < EOK)
		{
			ERR_CHK(rc);
		}
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
                                if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                {
                                    char static_dns[256] = {0};
                                    sysevent_get(sefd, setok, "lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                    fprintf(fp, "   ipv6 nd rdnss %s 86400\n", static_dns);
                                    /* DNS from WAN (if no static DNS) */
                                    if (strlen(name_servs) == 0) {
                                        sysevent_get(sefd, setok, "ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                                    }
                                 }
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

	fprintf(fp, "interface %s\n", interface_name);
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
#ifdef _HUB4_PRODUCT_REQ_
    int result;
    int ipv6_enable;
    int ula_enable;
#endif
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
#ifdef _HUB4_PRODUCT_REQ_
    result = getLanIpv6Info(&ipv6_enable, &ula_enable);
    if(result != 0) {
        fprintf(stderr, "getLanIpv6Info failed");
        return -1;
    }
    if(ipv6_enable == 0) {
        daemon_stop(ZEBRA_PID_FILE, "zebra");
        return -1;
    }
#endif
    if (gen_zebra_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to save zebra config\n", __FUNCTION__);
        return -1;
    }

#ifdef _HUB4_PRODUCT_REQ_
    /*
     * SKYH4-1765: we do not want to restart the zebra if it is already running,
     * since restarting zebra will leads clear the current zebra counter.
     * So we send SIGUSR1 to zebra to notify 'read the updated zebra.conf'
     */
    int pid = is_daemon_running(ZEBRA_PID_FILE, "zebra");
#ifndef FEATURE_RDKB_WAN_MANAGER
    char dhcpv6_lease[16] = {0};
    sysevent_get(sr->sefd, sr->setok, "dhcpv6_lease", dhcpv6_lease, sizeof(dhcpv6_lease));
    if((pid) && (strncmp(dhcpv6_lease, "expired", sizeof(dhcpv6_lease)) != 0))
#else
    if(pid)
#endif
    {
        kill(pid, SIGUSR1);
        return 0;
    }
#endif
    daemon_stop(ZEBRA_PID_FILE, "zebra");

#if defined(_COSA_FOR_BCI_)
    syscfg_get(NULL, "dhcpv6s00::serverenable", dhcpv6Enable , sizeof(dhcpv6Enable));
    bool bEnabled = (strncmp(dhcpv6Enable,"1",1)==0?true:false);

    v_secure_system("zebra -d -f %s -P 0", ZEBRA_CONF_FILE);
    printf("DHCPv6 is %s. Starting zebra Process\n", (bEnabled?"Enabled":"Disabled"));
#else
    v_secure_system("zebra -d -f %s -P 0", ZEBRA_CONF_FILE);
#endif

    return 0;
}

static int radv_stop(struct serv_routed *sr)
{
    /*
     * SKYH4-1765: we do not want to restart the zebra if it is already running,
     * since restarting zebra will clear the current zebra counter.
     */
#if defined(_HUB4_PRODUCT_REQ_) && ! defined(FEATURE_RDKB_WAN_MANAGER)
    char dhcpv6_lease[16] = {0};
    sysevent_get(sr->sefd, sr->setok, "dhcpv6_lease", dhcpv6_lease, sizeof(dhcpv6_lease));
    if((is_daemon_running(ZEBRA_PID_FILE, "zebra")) && (strncmp(dhcpv6_lease, "expired", sizeof(dhcpv6_lease)) != 0))
#else
    if(is_daemon_running(ZEBRA_PID_FILE, "zebra"))
#endif
    {
        return 0;
    }
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
#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_) || defined (_CBR2_PRODUCT_REQ_)
    char ripd_conf_status[16];
#endif
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

#if defined (_BWG_PRODUCT_REQ_)
sleep(45); /*sleep upto update ripd.conf after reboot*/
#endif

    sysevent_set(sr->sefd, sr->setok, "rip-status", "starting", 0);

    if (gen_ripd_conf(sr->sefd, sr->setok) != 0) {
        fprintf(stderr, "%s: fail to generate ripd config\n", __FUNCTION__);
        sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
        return -1;
    }
#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_) || defined (_CBR2_PRODUCT_REQ_)
	  int retries=0;
    	  while (retries<20)  {
          memset(ripd_conf_status,0,sizeof(ripd_conf_status));
          sysevent_get(sr->sefd, sr->setok, "ripd_conf-status", ripd_conf_status, sizeof(ripd_conf_status));
          if (strcmp((const char*)ripd_conf_status, "ready") == 0) {
              if (!(IsFileExists(RIPD_CONF_PAM_UPDATE) == 0)) {
                    DEG_PRINT("Incomplete ripd conf update \n");
              }
              else  {
                    DEG_PRINT("starting ripd after PAM updates conf \n");
              }
              if (v_secure_system("ripd -d -f %s -u root", RIPD_CONF_FILE) != 0) {
                   sysevent_set(sr->sefd, sr->setok, "rip-status", "error", 0);
                   return -1;
             }
              sysevent_set(sr->sefd, sr->setok, "rip-status", "started", 0);
             break;
          }
          sleep(5);
          retries=retries+1;
     }
#endif
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
#ifndef _HUB4_PRODUCT_REQ_
    char rtmod[16];
    char prefix[64];
#endif

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
        sysevent_get(sr->sefd, sr->setok, "lan_prefix", prefix, sizeof(prefix));
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

    if (radv_restart(sr) != 0)
        fprintf(stderr, "%s: radv_restart error\n", __FUNCTION__);

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
#ifdef _HUB4_PRODUCT_REQ_
    /* dbus init based on bus handle value */
    if(bus_handle ==  NULL)
        dbusInit();

    if(bus_handle == NULL)
    {
        fprintf(stderr, "service_routed, DBUS init error\n");
        return -1;
    }
#endif
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
