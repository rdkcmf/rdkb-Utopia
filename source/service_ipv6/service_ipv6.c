/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

/**
 * IPv6 Enhancement:
 *      Customer-Facing Ipv6 Provisoning of CPE devices
 *      Support IPv6 prefix delegation
 *      DHCPv6 server functions separated from PAM
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
#include <netinet/in.h>
#include <signal.h>
#include "util.h"
#include <fcntl.h>
#include "autoconf.h"
#include "secure_wrapper.h"

#ifdef MULTILAN_FEATURE
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#endif

#define PROG_NAME       "SERVICE-IPV6"

#ifdef MULTILAN_FEATURE
#define CCSP_SUBSYS                 "eRT."
#define L3_DM_PREFIX                "dmsb.l3net."
#define L3_DM_IPV6_ENABLE_PREFIX    "IPv6Enable"
#define L3_DM_ETHLINK_PREFIX        "EthLink"
#define ETHLINK_DM_PREFIX           "dmsb.EthLink."
#define ETHLINK_DM_L2NET_PREFIX     "l2net"
#define CCSP_CR_COMPONENT_ID        "eRT.com.cisco.spvtg.ccsp.CR"
#define L3_DM_PRIMARY_INSTANCE      "dmsb.MultiLAN.PrimaryLAN_l3net"

static void* bus_handle = NULL;
const char* const service_ipv6_component_id = "ccsp.ipv6";

#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(bus_handle, CCSP_SUBSYS, name, NULL, &(str))
#define PSM_VALUE_GET_INS(name, pIns, ppInsArry) PsmGetNextLevelInstances(bus_handle, CCSP_SUBSYS, name, pIns, ppInsArry)
#endif

#define PROVISIONED_V6_CONFIG_FILE  "/tmp/ipv6_provisioned.config"
#define CLI_RECEIVED_OPTIONS_FILE   "/tmp/.dibbler-info/client_received_options"
#define DHCPV6_SERVER               "dibbler-server"
#define DHCPV6S_PID_FILE            "/var/lib/dibbler/server.pid"
#define DHCPV6S_CONF_FILE           "/etc/dibbler/server.conf"
#define DHCPV6S_NAME                "dhcpv6s"

#ifndef MULTILAN_FEATURE
#define MAX_LAN_IF_NUM              3
#else
#define MAX_LAN_IF_NUM             64
#define CMD_BUF_SIZE              255
#define MAX_ACTIVE_INSTANCE        64

#define EROUTER_EVENT_LOG           "/var/log/event/eventlog"
#define EROUTER_NO_PREFIX_MESSAGE   "/usr/bin/logger -p local4.crit \"72002001 -  LAN Provisioning No Prefix available for eRouter interface\""
#endif

#if defined(MULTILAN_FEATURE)
#define BRIDGE_MODE_STRLEN          4
#endif

/*dhcpv6 client dm related sysevent*/
#define COSA_DML_DHCPV6_CLIENT_IFNAME                 "erouter0"
#define COSA_DML_DHCPV6C_PREF_SYSEVENT_NAME           "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_v6pref"
#define COSA_DML_DHCPV6C_PREF_IAID_SYSEVENT_NAME      "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_iaid"
#define COSA_DML_DHCPV6C_PREF_T1_SYSEVENT_NAME        "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_t1"
#define COSA_DML_DHCPV6C_PREF_T2_SYSEVENT_NAME        "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_t2"
#define COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_pretm"
#define COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_pref_vldtm"

#define COSA_DML_DHCPV6C_ADDR_SYSEVENT_NAME           "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_v6addr"
#define COSA_DML_DHCPV6C_ADDR_IAID_SYSEVENT_NAME      "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_addr_iaid"
#define COSA_DML_DHCPV6C_ADDR_T1_SYSEVENT_NAME        "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_addr_t1"
#define COSA_DML_DHCPV6C_ADDR_T2_SYSEVENT_NAME        "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_addr_t2"
#define COSA_DML_DHCPV6C_ADDR_PRETM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_addr_pretm"
#define COSA_DML_DHCPV6C_ADDR_VLDTM_SYSEVENT_NAME     "tr_"COSA_DML_DHCPV6_CLIENT_IFNAME"_dhcpv6_client_addr_vldtm"

/*erouter topology mode*/
enum tp_mod {
    TPMOD_UNKNOWN,
    FAVOR_DEPTH,
    FAVOR_WIDTH,
};

struct serv_ipv6 {
    int         sefd;
    int         setok;

    bool        wan_ready;

    char        mso_prefix[INET6_ADDRSTRLEN];
    enum tp_mod tpmod;
};

typedef struct ia_info {
    union {
        char v6addr[INET6_ADDRSTRLEN];
        char v6pref[INET6_ADDRSTRLEN];
    } value;

    char t1[32], t2[32], iaid[32], pretm[32], vldtm[32];
    int len;
} ia_na_t, ia_pd_t;

/*dhcpv6 server type*/
enum {
    DHCPV6S_TYPE_STATEFUL = 1,
    DHCPV6S_TYPE_STATELESS,
};

typedef struct dhcpv6s_cfg {
    int     enable;
    int     pool_num;
    int     server_type;
} dhcpv6s_cfg_t;

typedef struct dhcpv6s_pool_opt {
    int     tag;
    int     enable;
    char    pt_client[128]; /*pass through client*/
}dhcpv6s_pool_opt_t;

typedef struct dhcpv6s_pool_cfg {
    int     index;
    int     enable;
    char    interface[32];
    int     rapid_enable;
    int     unicast_enable;
    int     iana_enable;
    int     iana_amount;
    int     eui64_enable;
    signed long     lease_time;
    int     iapd_enable;
    char    ia_prefix[INET6_ADDRSTRLEN];
    char    prefix_range_begin[64];
    char    prefix_range_end[64];
    int     opt_num;
    int     X_RDKCENTRAL_COM_DNSServersEnabled;	
    char    X_RDKCENTRAL_COM_DNSServers[256];
    dhcpv6s_pool_opt_t *opts;
} dhcpv6s_pool_cfg_t;

typedef struct dhcpv6_recv_option {
    unsigned int    tag;
    char            value[1024];
}dhcpv6_recv_option_t;

struct dhcpv6_tag {
    int     tag;
    char    *opt_str;
};

typedef struct ipv6_prefix {
    char value[INET6_ADDRSTRLEN];
    int  len;
    //int  b_used;
} ipv6_prefix_t;

typedef struct pd_pool {
    char start[INET6_ADDRSTRLEN];
    char end[INET6_ADDRSTRLEN];
    int  prefix_length;
    int  pd_length;
} pd_pool_t;

struct dhcpv6_tag tag_list[] =
{
    {17, "vendor-spec"},
    {21, "sip-domain"},
    {22, "sip-server"},
    {23, "dns-server"},
    {24, "domain"},
    {27, "nis-server"},
    {28, "nis+-server"},
    {29, "nis-domain"},
    {30, "nis+-domain"},
    {31, "ntp-server"},
    {42, "time-zone"}
};

#define DHCPV6S_SYSCFG_GETS(unique_name, table1_name, table1_index, table2_name, table2_index, parameter, out) \
{ \
    char ns[128] = {0}; \
    snprintf(ns, sizeof(ns), "%s%s%lu%s%lu", unique_name, table1_name, (unsigned long)table1_index, table2_name, (unsigned long)table2_index); \
    syscfg_get(ns, parameter, out, sizeof(out)); \
} \

#define DHCPV6S_SYSCFG_GETI(unique_name, table1_name, table1_index, table2_name, table2_index, parameter, out) \
{ \
    char ns[128] = {0}; \
    char val[32] = {0}; \
    snprintf(ns, sizeof(ns), "%s%s%lu%s%lu", unique_name, table1_name, (unsigned long)table1_index, table2_name, (unsigned long)table2_index); \
    syscfg_get(ns, parameter, val, sizeof(val)); \
    if ( strcmp(val,"4294967295") == 0) out = -1; \
    else if ( val[0] ) out = atoi(val); \
} \


uint64_t helper_ntoh64(const uint64_t *inputval)
{
    uint64_t returnval;
    uint8_t *data = (uint8_t *)&returnval;

    data[0] = *inputval >> 56;
    data[1] = *inputval >> 48;
    data[2] = *inputval >> 40;
    data[3] = *inputval >> 32;
    data[4] = *inputval >> 24;
    data[5] = *inputval >> 16;
    data[6] = *inputval >> 8;
    data[7] = *inputval >> 0;

    return returnval;
}
uint64_t helper_hton64(const uint64_t *inputval)
{
    return (helper_ntoh64(inputval));
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
static int mbus_get(char *path, char *val, int size)
{
    int                      compNum = 0;
    int                      valNum = 0;
    componentStruct_t        **ppComponents = NULL;
    parameterValStruct_t     **parameterVal = NULL;
    char                     *ppDestComponentName = NULL;
    char                     *ppDestPath = NULL;
    char                     *paramNames[1];

    if (!path || !val || size < 0)
        return -1;

    if (!bus_handle) {
         fprintf(stderr, "DBUS not connected\n");
         return -1;
    }

    if (CcspBaseIf_discComponentSupportingNamespace(bus_handle, CCSP_CR_COMPONENT_ID, path, CCSP_SUBSYS, &ppComponents, &compNum) != CCSP_SUCCESS) {
        fprintf(stderr, "failed to find component for %s \n", path);
        return -1;
    }
    ppDestComponentName = ppComponents[0]->componentName;
    ppDestPath = ppComponents[0]->dbusPath;
    paramNames[0] = path;

    if(CcspBaseIf_getParameterValues(bus_handle, ppDestComponentName, ppDestPath, paramNames, 1, &valNum, &parameterVal) != CCSP_SUCCESS) {
        fprintf(stderr, "failed to get value for %s \n", path);
        free_componentStruct_t(bus_handle, compNum, ppComponents);
        return -1;
    }

    if(valNum >= 1) {
        strncpy(val, parameterVal[0]->parameterValue, size);
        free_parameterValStruct_t(bus_handle, valNum, parameterVal);
        free_componentStruct_t(bus_handle, compNum, ppComponents);
    }
    return 0;
}
#endif

static int get_dhcpv6s_conf(dhcpv6s_cfg_t *cfg)
{
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "", 0, "", 0, "serverenable", cfg->enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "", 0, "", 0, "poolnumber", cfg->pool_num);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "", 0, "", 0, "servertype", cfg->server_type);

    return 0;
}

static int get_dhcpv6s_pool_cfg(struct serv_ipv6 *si6, dhcpv6s_pool_cfg_t *cfg)
{   
    int i = 0;
    dhcpv6s_pool_opt_t *p_opt = NULL;
    char buf[64] = {0};
#ifdef MULTILAN_FEATURE
    FILE *fp = NULL;
    char dml_path[CMD_BUF_SIZE] = {0};
    char iface_name[64] = {0};
#endif
    char l_cSecWebUI_Enabled[8] = {0};
    syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
    if (!strncmp(l_cSecWebUI_Enabled, "true", 4))	
    {
        syscfg_set(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", "1");
        syscfg_commit();
    }
    else
    {
        syscfg_set(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", "0");
        syscfg_commit();
    } 

    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "bEnabled", cfg->enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "RapidEnable", cfg->rapid_enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "UnicastEnable", cfg->unicast_enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "IANAEnable", cfg->iana_enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "IANAAmount", cfg->iana_amount);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "IAPDEnable", cfg->iapd_enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "EUI64Enable", cfg->eui64_enable);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "LeaseTime", cfg->lease_time);
    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "X_RDKCENTRAL_COM_DNSServersEnabled", cfg->X_RDKCENTRAL_COM_DNSServersEnabled);

#ifdef MULTILAN_FEATURE
#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "IAInterface", iface_name);
#else
    DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "Interface", iface_name);
#endif
#else
    DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "IAInterface", cfg->interface);
#endif
    DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "PrefixRangeBegin", cfg->prefix_range_begin);
    DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "PrefixRangeEnd", cfg->prefix_range_end);
	DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "", 0, "X_RDKCENTRAL_COM_DNSServers", cfg->X_RDKCENTRAL_COM_DNSServers);

#ifdef MULTILAN_FEATURE
    /* get Interface name from data model: Device.IP.Interface.%d.Name*/
    snprintf(dml_path, sizeof(dml_path), "%sName", iface_name);
    if (mbus_get(dml_path, cfg->interface, sizeof(cfg->interface)) != 0)
        return -1;
#endif

    /*get interface prefix*/
    snprintf(buf, sizeof(buf), "ipv6_%s-prefix", cfg->interface);
    sysevent_get(si6->sefd, si6->setok, buf, cfg->ia_prefix, sizeof(cfg->ia_prefix));

    DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "", 0, "optionnumber", cfg->opt_num);
    
    p_opt = (dhcpv6s_pool_opt_t *)calloc(cfg->opt_num, sizeof(*p_opt));
    if (p_opt == NULL) {
        fprintf(stderr, "calloc mem for pool options failed!\n");
        return -1;
    }

    for(; i < cfg->opt_num; i++) {
        DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "option", i, "bEnabled", (p_opt + i)->enable);
        DHCPV6S_SYSCFG_GETI(DHCPV6S_NAME, "pool", cfg->index, "option", i, "Tag", (p_opt + i)->tag);
        DHCPV6S_SYSCFG_GETS(DHCPV6S_NAME, "pool", cfg->index, "option", i, "PassthroughClient", (p_opt + i)->pt_client);
    }
    cfg->opts = p_opt;

    return 0;
}

static int get_ia_info(struct serv_ipv6 *si6, char *config_file, ia_na_t *iana, ia_pd_t *iapd)
{
    int  fd = 0;
    char config[1024] = {0};
    char *p= NULL;
    char action[64] = {0};
    
    if(iana == NULL || iapd == NULL)
        return -1;
#ifdef _CBR_PRODUCT_REQ_ 
	sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_T1_SYSEVENT_NAME, action, sizeof(action));
	if(action[0]!='\0')
	{
		if(!strcmp(action,"'\\0'"))
			strcpy(iapd->t1,"0");
		else
			strcpy(iapd->t1,strtok (action,"'"));
	}
	sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_T2_SYSEVENT_NAME, action, sizeof(action));
	if(action[0]!='\0')
	{
		if(!strcmp(action,"'\\0'"))
			strcpy(iapd->t2,"0");
		else
			strcpy(iapd->t2,strtok (action,"'"));
	}
	sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, action, sizeof(action));
	if(action[0]!='\0')
	{
		if(!strcmp(action,"'\\0'"))
			strcpy(iapd->pretm,"0");
		else
			strcpy(iapd->pretm,strtok (action,"'"));
	}
	sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME, action, sizeof(action));	
	if(action[0]!='\0')
	{
		if(!strcmp(action,"'\\0'"))
			strcpy(iapd->vldtm,"0");
		else
			strcpy(iapd->vldtm,strtok (action,"'"));
	}
#else
    fd = open(config_file, O_RDWR);

    if (fd < 0) {
        fprintf(stderr, "open file %s failed!\n", config_file);
        return -1;
    }

    memset(config, 0, sizeof(config));
    read(fd, config, sizeof(config));
    if (!strncmp(config, "dibbler-client", strlen("dibbler-client"))) 
    {

        /*the format is :
          add 2000::ba7a:1ed4:99ea:cd9f :: 0 t1
          action, address, prefix, pref_len 3600
          now action only supports "add", "del"*/

        p = config + strlen("dibbler-client");
        while(isblank(*p)) p++;

        //fprintf(stderr, "%s -- %d !!! get configs from v6 client: %s \n", __FUNCTION__, __LINE__,p);

        if (sscanf(p, "%63s %63s %s %s %s %s %s %63s %d %s %s %s %s %s", 
                    action, iana->value.v6addr, iana->iaid, iana->t1, iana->t2, iana->pretm, iana->vldtm,
                    iapd->value.v6pref, &iapd->len, iapd->iaid, iapd->t1, iapd->t2, iapd->pretm, iapd->vldtm ) == 14) {
            fprintf(stderr, "Get the IA_NA and IA_PD info: ");
            fprintf(stderr, "IA_NA:%s %s %s %s %s %s, IA_PD:%s %d %s %s %s %s\n",
                    iana->value.v6addr, iana->iaid, iana->t1, iana->t2, iana->pretm, iana->vldtm,
                    iapd->value.v6pref, iapd->len, iapd->t1, iapd->t2, iapd->pretm, iapd->vldtm);
        } else {
            fprintf(stderr, "Get the IA_NA and IA_PD failed.\n");
	    close(fd); /*RDKB-12965 & CID:-34141*/
            return -1;
        }
    } else {
	close(fd); /*RDKB-12965 & CID:-34141*/
        return -1;
    }

#if 1
    /*client v6 address*/
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_SYSEVENT_NAME,       iana->value.v6addr, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_IAID_SYSEVENT_NAME,  iana->iaid, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_T1_SYSEVENT_NAME,    iana->t1, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_T2_SYSEVENT_NAME,    iana->t2, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_PRETM_SYSEVENT_NAME, iana->pretm, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_ADDR_VLDTM_SYSEVENT_NAME, iana->vldtm, 0);
   /*v6 prefix*/
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_SYSEVENT_NAME,       iapd->value.v6pref, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_IAID_SYSEVENT_NAME,  iapd->iaid, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_T1_SYSEVENT_NAME,    iapd->t1, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_T2_SYSEVENT_NAME,    iapd->t2, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, iapd->pretm, 0);
   sysevent_set(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME, iapd->vldtm, 0);
#endif
#endif	
    return 0;
}

static int get_prefix_info(const char *prefix,  char *value, unsigned int val_len, unsigned int *prefix_len)
{
    int i;

    i = strlen(prefix);

    while((prefix[i-1] != '/') && (i > 0)) i--;

    if (i == 0) {
        fprintf(stderr, "[%s] error, there is not '/' in prefix:%s\n", __FUNCTION__, prefix);
        return -1;
    }

    if (prefix_len != NULL)
        *prefix_len = atoi(&prefix[i]);

    if (value != NULL) {
        memset(value, 0, val_len);
        strncpy(value, prefix, i-1);
    }

    //fprintf(stderr, "[%s] prefix:%s length: %d.\n",__FUNCTION__, value != NULL ? value : "null", *prefix_len);

    return 0;
}

/* get the interfaces which need to assign /64 interface-prefix
 * suppose we currently use syscfg "lan_pd_interfaces" to represent the interfaces need to prefix delegation
 */
static int get_active_lanif(struct serv_ipv6 *si6, unsigned int insts[], unsigned int *num)
{
    char active_insts[32] = {0};
    char lan_pd_if[128] = {0};
    char *p = NULL;
    int i = 0;
    char if_name[16] = {0};
    char buf[64] = {0};
#ifdef MULTILAN_FEATURE

    int l_iRet_Val = 0;
    int idx = 0;
    int len = 0;
    unsigned int l3net_count = 0;
    unsigned int *l3net_ins = NULL;
    unsigned char psm_param[CMD_BUF_SIZE] = {0};
    unsigned char active_if_list[CMD_BUF_SIZE] = {0};
    char *psm_get = NULL;
    ipv6_prefix_t mso_prefix;
    int delta_bits = 0;
    unsigned int max_active_if_count = 0;
    int primary_l3_instance = 0;

#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
    syscfg_get(NULL, "lan_pd_interfaces", lan_pd_if, sizeof(lan_pd_if));
    if (lan_pd_if[0] == '\0') {
        *num = 0;
        return *num;
    }

    sysevent_get(si6->sefd, si6->setok, "multinet-instances", active_insts, sizeof(active_insts));
    p = strtok(active_insts, " ");

    while (p != NULL) {
        snprintf(buf, sizeof(buf), "multinet_%s-name", p);
        sysevent_get(si6->sefd, si6->setok, buf, if_name, sizeof(if_name));
        if (strstr(lan_pd_if, if_name)) { /*active interface and need prefix delegation*/
            insts[i] = atoi(p);
            i++;
        }

        p = strtok(NULL, " ");
    }
#else
    /* Get active bridge count from PSM */
    if (!bus_handle) {
        fprintf(stderr, "DBUS not connected, returning \n");
        bus_handle = NULL;
        *num = 0;
        return *num;
    }

    if (get_prefix_info(si6->mso_prefix, mso_prefix.value, sizeof(mso_prefix.value), &mso_prefix.len) != 0) {
        *num = 0;
        return *num;
    }

    /* Get max count of IPv6 enabled interfaces, from received MSO prefix*/
    if((delta_bits = 64 - mso_prefix.len) >= 0) {
        max_active_if_count = 1 << delta_bits;
    }

    l_iRet_Val = PSM_VALUE_GET_INS(L3_DM_PREFIX , &l3net_count, &l3net_ins);

    /* Get primary L3 network instance */
    snprintf(psm_param, sizeof(psm_param), "%s", L3_DM_PRIMARY_INSTANCE);
    l_iRet_Val = PSM_VALUE_GET_STRING(psm_param, psm_get);
    if((l_iRet_Val == CCSP_SUCCESS) && (psm_get != NULL)) {
        primary_l3_instance = atoi(psm_get);
    }

    if((l_iRet_Val == CCSP_SUCCESS) && (l3net_count > 0)) {

        for(idx = 0; (idx < l3net_count) && (i < max_active_if_count); idx++) {
            snprintf(psm_param, sizeof(psm_param), "%s%d.%s", L3_DM_PREFIX , l3net_ins[idx], L3_DM_IPV6_ENABLE_PREFIX);
            l_iRet_Val = PSM_VALUE_GET_STRING(psm_param, psm_get);

            if((l_iRet_Val == CCSP_SUCCESS) && (psm_get != NULL) && (l3net_ins[idx] == primary_l3_instance || !strncmp(psm_get, "true", 4) || !strncmp(psm_get, "1", 1))) {
                Ansc_FreeMemory_Callback(psm_get);
                psm_get = NULL;

                snprintf(psm_param, sizeof(psm_param), "%s%d.%s", L3_DM_PREFIX , l3net_ins[idx], L3_DM_ETHLINK_PREFIX);
                l_iRet_Val = PSM_VALUE_GET_STRING(psm_param, psm_get);

                if((l_iRet_Val == CCSP_SUCCESS) && (psm_get != NULL)) {
                    snprintf(psm_param, sizeof(psm_param), "%s%s.%s", ETHLINK_DM_PREFIX, psm_get, ETHLINK_DM_L2NET_PREFIX);
                    Ansc_FreeMemory_Callback(psm_get);
                    psm_get = NULL;

                    l_iRet_Val = PSM_VALUE_GET_STRING(psm_param, psm_get);

                    if((l_iRet_Val == CCSP_SUCCESS) && (psm_get != NULL)) {
                        insts[i++] = atoi(psm_get);
                        Ansc_FreeMemory_Callback(psm_get);
                        psm_get = NULL;
                    }
                }
            }
            else if (psm_get != NULL) {
                Ansc_FreeMemory_Callback(psm_get);
                psm_get = NULL;
            }
        }
        Ansc_FreeMemory_Callback(l3net_ins);
        l3net_ins = NULL;
    }
    *num = i;

    for(idx = 0; idx < *num; idx++) {
        len += snprintf(active_if_list+len, sizeof(active_if_list)-len, "%d ", insts[idx]);
    }
    /* Set active IPv6 instances */
    sysevent_set(si6->sefd, si6->setok, "ipv6_active_inst", active_if_list, 0);
#endif


#else
    syscfg_get(NULL, "lan_pd_interfaces", lan_pd_if, sizeof(lan_pd_if));
    if (lan_pd_if[0] == '\0') {
        *num = 0;
        return *num;
    }

    sysevent_get(si6->sefd, si6->setok, "multinet-instances", active_insts, sizeof(active_insts));
    p = strtok(active_insts, " ");

    while (p != NULL) {
        snprintf(buf, sizeof(buf), "multinet_%s-name", p);
        sysevent_get(si6->sefd, si6->setok, buf, if_name, sizeof(if_name));
        if (strstr(lan_pd_if, if_name)) { /*active interface and need prefix delegation*/
            insts[i] = atoi(p);
            i++;
        }

        p = strtok(NULL, " ");
    }

    *num = i;
#endif

    return *num;
}

static int get_pd_pool(struct serv_ipv6 *si6, pd_pool_t *pool)
{
    char evt_val[256] = {0};

    sysevent_get(si6->sefd, si6->setok, "ipv6_subprefix-start", evt_val, sizeof(evt_val));
    if (evt_val[0] != '\0')
        strcpy(pool->start, evt_val);
    else 
        return -1;

    sysevent_get(si6->sefd, si6->setok, "ipv6_subprefix-end", evt_val, sizeof(evt_val));
    if (evt_val[0] != '\0')
        strcpy(pool->end, evt_val);
    else 
        return -1;

    sysevent_get(si6->sefd, si6->setok, "ipv6_prefix-length", evt_val, sizeof(evt_val));
    if (evt_val[0] != '\0')
        pool->prefix_length = atoi(evt_val);
    else 
        return -1;

    sysevent_get(si6->sefd, si6->setok, "ipv6_pd-length", evt_val, sizeof(evt_val));
    if (evt_val[0] != '\0')
        pool->pd_length = atoi(evt_val);
    else 
        return -1;

    return 0;
}

/*
 * Break the prefix provisoned from wan to sub-prefixes based on favor width/depth and topology mode
 */
static int divide_ipv6_prefix(struct serv_ipv6 *si6)
{
    ipv6_prefix_t       mso_prefix;
    ipv6_prefix_t       *sub_prefixes = NULL;
    unsigned int        enabled_iface_num = 0; 
    unsigned int        l2_insts[MAX_LAN_IF_NUM] = {0};
    unsigned char       prefix[sizeof(struct in6_addr)];
    unsigned char       buf[sizeof(struct in6_addr)];
    int                 delta_bits = 0;
    unsigned int        sub_prefix_num = 0;
    unsigned int        iface_prefix_num = 0;
    int                 i;
    ipv6_prefix_t       *p_prefix = NULL;
    int                 bit_boundary = 0;
    unsigned long long  sub_prefix, tmp_prefix; //64 bits
    char                iface_prefix[INET6_ADDRSTRLEN]; //for iface prefix str
    char                evt_name[64];
    char                evt_val[64];
    char                iface_name[64];
    int                 used_sub_prefix_num = 0; 

    sysevent_set(si6->sefd, si6->setok, "ipv6_prefix-divided", "", 0);

    if (get_prefix_info(si6->mso_prefix, mso_prefix.value, sizeof(mso_prefix.value), &mso_prefix.len) != 0) {
        return -1;
    }
    
    if ((delta_bits = 64 - mso_prefix.len) < 0) {
        fprintf(stderr, "invalid prefix.\n");
        return -1;
    }

    if (inet_pton(AF_INET6, mso_prefix.value, prefix) <= 0) {
        fprintf(stderr, "prefix inet_pton error!.\n");
        return -1;
    }

    get_active_lanif(si6, l2_insts, &enabled_iface_num);
    if (enabled_iface_num == 0) {
        fprintf(stderr, "no enabled lan interfaces.\n");
        return -1;
    }

    if (enabled_iface_num > (1 << delta_bits)) {
        fprintf(stderr, "mso prefix is too small to address all of its interfaces.\n");
        return -1;
    }

	printf("mso_prefix.value %s \n",mso_prefix.value);
	printf("mso_prefix.len %d \n",mso_prefix.len);
	printf("si6->tpmod %d \n",si6->tpmod);

    /* divide base on mso prefix len and topology mode
     *  1) prefix len > 56 && topology mode = "favor depth", divide on 2 bit boundaries to 4 sub-prefixes.
     *  2) prefix len > 56 && topology mode = "favor width", divide on 3 bit boundaries to 8 sub-prefixes.
     *  3) prefix len <= 56 && topology mode = "favor depth", divide on 3 bit boundaries to 8 sub-prefixes.
     *  4) prefix len <= 56 && topology mode = "favor width", divide on 4 bit boundaries to 16 sub-prefixes.
     *  5) if prefix is to small to divide in the manner described, divided into as many /64 sub-prefixes as possible and log a message.
     * */
    /*get boundary*/
    if (mso_prefix.len > 56) {
        if (si6->tpmod == FAVOR_DEPTH) {
            bit_boundary = (delta_bits < 2) ? delta_bits : 2;
        } else if (si6->tpmod == FAVOR_WIDTH) {
            bit_boundary = (delta_bits < 3) ? delta_bits : 3;
        }
    }
    else {
        if (si6->tpmod == FAVOR_DEPTH) {
            bit_boundary = (delta_bits < 3) ? delta_bits : 3;
        } else if(si6->tpmod == FAVOR_WIDTH) {
            bit_boundary = (delta_bits < 4) ? delta_bits : 4;
        }
    }
    
    /*divide to sub-prefixes*/
    sub_prefix_num = 1 << bit_boundary;
    sub_prefixes = (ipv6_prefix_t *)calloc(sub_prefix_num, sizeof(ipv6_prefix_t));
    if (sub_prefixes == NULL) {
        fprintf(stderr, "calloc mem for sub-prefixes failed.\n");
        return -1;
    }

    p_prefix = sub_prefixes;

    memcpy((void *)&tmp_prefix, (void *)prefix, 8); // the first 64 bits of mso prefix value
#ifdef _CBR_PRODUCT_REQ_
	tmp_prefix = helper_ntoh64(&tmp_prefix); // The memcpy is copying in reverse order due to LEndianess
#endif
#ifdef MULTILAN_FEATURE
    tmp_prefix &= htobe64((~0) << delta_bits);
    for (i = 0; i < sub_prefix_num; i ++) {
        sub_prefix = tmp_prefix | htobe64(i << (delta_bits - bit_boundary));
#else
    tmp_prefix &= ((~0) << delta_bits);
    for (i = 0; i < sub_prefix_num; i ++) {
        sub_prefix = tmp_prefix | (i << (delta_bits - bit_boundary));
#endif
        memset(buf, 0, sizeof(buf));
#ifdef _CBR_PRODUCT_REQ_	
		sub_prefix = helper_hton64(&sub_prefix);// The memcpy is copying in reverse order due to LEndianess
#endif
        memcpy((void *)buf, (void *)&sub_prefix, 8);
        inet_ntop(AF_INET6, buf, p_prefix->value, INET6_ADDRSTRLEN);
        p_prefix->len = mso_prefix.len + bit_boundary;
        //p_prefix->b_used = 0;

        fprintf(stderr, "sub-prefix:%s/%d\n", p_prefix->value, p_prefix->len);

        p_prefix++;
    }

    /*break the first sub-prefix to interface prefix for lan interface*/
    iface_prefix_num = (1 << delta_bits) / (sub_prefix_num); /*determine the iface prefix num for each sub-prefix*/
   
    p_prefix = sub_prefixes;
    inet_pton(AF_INET6, p_prefix->value, prefix);
    memcpy((void *)&tmp_prefix, (void *)prefix, 8); //the first 64 bits of the first sub-prefix
#ifdef _CBR_PRODUCT_REQ_
	tmp_prefix = helper_ntoh64(&tmp_prefix); // The memcpy is copying in reverse order due to LEndianess
#endif
    for (i = 0; i < enabled_iface_num; i++) {
        //p_prefix->b_used = 1;
        memset(buf, 0, sizeof(buf));
#ifdef _CBR_PRODUCT_REQ_
		tmp_prefix = helper_hton64(&tmp_prefix);// The memcpy is copying in reverse order due to LEndianess
#endif		
        memcpy((void *)buf, (void *)&tmp_prefix, 8);
        inet_ntop(AF_INET6, buf, iface_prefix, INET6_ADDRSTRLEN);
        strcat(iface_prefix, "/64");

        /*set related sysevent*/
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(si6->sefd, si6->setok, evt_name, iface_name, sizeof(iface_name));/*interface name*/
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", iface_name);
        sysevent_set(si6->sefd, si6->setok, evt_name, iface_prefix, 0);

        fprintf(stderr, "interface-prefix %s:%s\n", iface_name, iface_prefix);

#ifdef MULTILAN_FEATURE
        tmp_prefix += htobe64(1);
#else
        tmp_prefix++;
#endif
    }

    /*last set sub-prefix related sysevent*/
    used_sub_prefix_num = enabled_iface_num / iface_prefix_num;
    if ((enabled_iface_num % iface_prefix_num) != 0 )
        used_sub_prefix_num += 1;
    if (used_sub_prefix_num < sub_prefix_num) {
        sysevent_set(si6->sefd, si6->setok, "ipv6_subprefix-start", sub_prefixes[used_sub_prefix_num].value, 0);
        sysevent_set(si6->sefd, si6->setok, "ipv6_subprefix-end", sub_prefixes[sub_prefix_num-1].value, 0);
    } else {
        sysevent_set(si6->sefd, si6->setok, "ipv6_subprefix-start", "", 0);
        sysevent_set(si6->sefd, si6->setok, "ipv6_subprefix-end", "", 0);
    }
    snprintf(evt_val, sizeof(evt_val), "%d", mso_prefix.len);
    sysevent_set(si6->sefd, si6->setok, "ipv6_prefix-length", evt_val, 0);
    snprintf(evt_val, sizeof(evt_val), "%d", mso_prefix.len + bit_boundary);
    sysevent_set(si6->sefd, si6->setok, "ipv6_pd-length", evt_val, 0);

    sysevent_set(si6->sefd, si6->setok, "ipv6_prefix-divided", "ready", 0);

    if (sub_prefixes != NULL)
        free(sub_prefixes);

    return 0;
}

/*compute global ipv6 based on /64 interface-prefix*/
int compute_global_ip(char *prefix, char *if_name, char *ipv6_addr, unsigned int addr_len)
{
    unsigned int    length           = 0;
    char            globalIP[INET6_ADDRSTRLEN] = {0};
#ifdef MULTILAN_FEATURE
    char            inet_v6Address[INET6_ADDRSTRLEN] = {0};
#endif
    unsigned int    i                = 0;
    unsigned int    j                = 0;
    unsigned int    k                = 0;
    char            out[256]         = {0};
    char            tmp[8]           = {0};
    char            mac[32]          = {0};


    strcpy(globalIP, prefix);

    /* Prepare the first part. */

    i = strlen(globalIP);

    while((globalIP[i-1] != '/') && (i>0)) i--;

    if (i == 0) {
        fprintf(stderr, "[%s] error, there is not '/' in prefix:%s\n", __FUNCTION__, prefix);
        return -1;
    }

    length = atoi(&globalIP[i]);

    if (length > 64) {
        fprintf(stderr, "[%s] error, length is bigger than 64. prefix:%s, length:%d\n", __FUNCTION__, prefix, length);
        return -1;
    }

    globalIP[i-1] = '\0';

    i = i-1;

    if ((globalIP[i-1]!=':') && (globalIP[i-2]!=':')) {
        fprintf(stderr, "[%s] error, there is not '::' in prefix:%s\n", __FUNCTION__, prefix);
        return -1;
    }

    j = i-2;
    k = 0;
    while (j>0) {
        if (globalIP[j-1] == ':')
            k++;
        j--;
    }

    if (k == 3) {
        globalIP[i-1] = '\0';
        i = i - 1;
    }

    //fprintf(stderr, "the first ipv6 addr part is:%s\n", globalIP);

    /* prepare second part */
    if (iface_get_hwaddr(if_name, mac, sizeof(mac)) != 0) {
        fprintf(stderr, "get the mac of %s error!\n", if_name);
        return -1;
    }

    /* switch 7bit to 1*/
    tmp[0] = mac[1];

    k = strtol(tmp, (char **)NULL, 16);

    k = k ^ 0x2;
    if ( k < 10 )
        k += '0';
    else
        k += 'A' - 10;

    mac[1] = k;
    mac[17] = '\0';

    //00:50:56: FF:FE:  92:00:22
    strncpy(out, mac, 9);
    out[9] = '\0';
    strcat(out, "FF:FE:");
    strcat(out, mac+9);

    for(k = 0, j = 0; out[j]; j++) {
        if (out[j] == ':')
            continue;
        globalIP[i++] = out[j];
        if (++k == 4) {
            globalIP[i++] = ':';
            k = 0;
        }
    }

    globalIP[i-1] = '\0';

    fprintf(stderr, "the global ipv6 address is:%s\n", globalIP);
#ifdef MULTILAN_FEATURE
    inet_pton(AF_INET6, globalIP, inet_v6Address);
    inet_ntop(AF_INET6, inet_v6Address, ipv6_addr, addr_len);
#else
    strncpy(ipv6_addr, globalIP, addr_len);
#endif

    return 0;
}

#if defined (MULTILAN_FEATURE)
 /*
 *Report that one LAN didn't get an IPv6 prefix
 */
static void report_no_prefix(int i)
{
    (void)i;

    vsystem("%s %d", EROUTER_NO_PREFIX_MESSAGE, i);
}

/*
 *In case prefix assignment completely fails, report failure for all LANs
 */
static void report_no_lan_prefixes(struct serv_ipv6 *si6)
{
    unsigned int enabled_iface_num = 0;
    unsigned int l2_insts[MAX_LAN_IF_NUM] = {0};
    int i = 0;

    if (si6 == NULL)
        return;

    get_active_lanif(si6, l2_insts, &enabled_iface_num);
    for (i=0; i < enabled_iface_num; i++) {
        report_no_prefix(l2_insts[i]);
    }
}
#endif

/*
 *Assign IPv6 address for lan interface from the corresponding interface-prefix
 */
static int lan_addr6_set(struct serv_ipv6 *si6)
{
    unsigned int l2_insts[MAX_LAN_IF_NUM] = {0};
#if defined(MULTILAN_FEATURE)
    unsigned int primary_l2_instance = 0;
#endif
    char iface_name[16] = {0};
    unsigned int enabled_iface_num = 0;
    int i = 0;
    char evt_name[64] = {0};
    char evt_val[64] = {0};
#ifdef MULTILAN_FEATURE
    char action[64] = {0};
    char cmd[CMD_BUF_SIZE] = {0};
    char iapd_preftm[64] = {0};
    char iapd_vldtm[64] = {0};
#endif
    char iface_prefix[INET6_ADDRSTRLEN] = {0};
    unsigned int prefix_len = 0;
    char ipv6_addr[INET6_ADDRSTRLEN] = {0};
#if defined(MULTILAN_FEATURE)
    char bridge_mode[BRIDGE_MODE_STRLEN]={0};
#endif
    
   
    /*
     * divide the Operator-delegated prefix to sub-prefixes
     */
    if (divide_ipv6_prefix(si6) != 0) {
        fprintf(stderr, "divide the operator-delegated prefix to sub-prefix error.\n");
        sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        return -1;
    }
 
    sysevent_get(si6->sefd, si6->setok, "ipv6_prefix-divided", evt_val, sizeof(evt_val));
    if (strcmp(evt_val, "ready")) {
        fprintf(stderr, "[%s] ipv6 prefix is not divided.\n", __FUNCTION__);
        return -1;
    }

    get_active_lanif(si6, l2_insts, &enabled_iface_num);
    if (enabled_iface_num == 0) {
        fprintf(stderr, "no active lan interface.\n");
        return -1;
    }

#if defined(MULTILAN_FEATURE)
    /*Get the primary L2 instance number*/
    snprintf(evt_name, sizeof(evt_name), "primary_lan_l2net");
    sysevent_get(si6->sefd, si6->setok, evt_name, iface_name, sizeof(iface_name));
    /*Read a bounded number of characters from the string returned by sysevent_get*/
    snprintf(cmd, sizeof(cmd), "%%%dd", (sizeof(cmd) - 1));
    sscanf(iface_name, cmd, &primary_l2_instance);
#endif

    for (; i < enabled_iface_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(si6->sefd, si6->setok, evt_name, iface_name, sizeof(iface_name));/*interface name*/
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", iface_name);
        sysevent_get(si6->sefd, si6->setok, evt_name, iface_prefix, sizeof(iface_prefix));

        /*enable ipv6 link local*/
        v_secure_system("ip -6 link set dev %s up", iface_name);
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/autoconf", iface_name, "1");
#ifndef MULTILAN_FEATURE
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", iface_name, "1");
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", iface_name, "0");
#endif
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/forwarding", iface_name, "1");

        sysevent_set(si6->sefd, si6->setok, "ipv6_linklocal", "up", 0);

#if defined(MULTILAN_FEATURE)
        syscfg_get(NULL, "bridge_mode", bridge_mode, sizeof(bridge_mode));

        /*If in bridge mode and interface is brlan0, do not assign IPv6 address to it*/
        if((l2_insts[i] == primary_l2_instance) && !strcmp(bridge_mode, "2"))
        {
            continue;
        }
#endif
        /*construct global ipv6 for lan interface*/
        compute_global_ip(iface_prefix, iface_name, ipv6_addr, sizeof(ipv6_addr));

#ifdef MULTILAN_FEATURE
        if(ipv6_addr[0] == '\0') {
            report_no_prefix(l2_insts[i]);
           continue;
        }
#endif

#if defined(MULTILAN_FEATURE)
        /*If this is the primary LAN instance, set sysevent key lan_ipaddr_v6 to this IPv6 address*/
        if (l2_insts[i] == primary_l2_instance) {
            snprintf(evt_name, sizeof(evt_name), "lan_ipaddr_v6");
            sysevent_set(si6->sefd, si6->setok, evt_name, ipv6_addr, 0);
        }
#endif

        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-addr", iface_name);
        sysevent_set(si6->sefd, si6->setok, evt_name, ipv6_addr, 0);

        get_prefix_info(iface_prefix, NULL, 0, &prefix_len);

#ifdef MULTILAN_FEATURE
        sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, action, sizeof(action));
        if(action[0]!='\0') {
            if(!strcmp(action,"'\\0'"))
                strncpy(iapd_preftm, "forever", sizeof(iapd_preftm));
            else
                strncpy(iapd_preftm, strtok (action,"'"), sizeof(iapd_preftm));
        }
        sysevent_get(si6->sefd, si6->setok, COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME, action, sizeof(action));
        if(action[0]!='\0') {
            if(!strcmp(action,"'\\0'"))
                strncpy(iapd_vldtm, "forever", sizeof(iapd_vldtm));
            else
                strncpy(iapd_vldtm, strtok (action,"'"), sizeof(iapd_vldtm));
        }

        snprintf(cmd, CMD_BUF_SIZE, "ip -6 addr add %s/%d dev %s valid_lft %s preferred_lft %s",
                ipv6_addr, prefix_len, iface_name, iapd_vldtm, iapd_preftm);

        vsystem(cmd);
        bzero(ipv6_addr, sizeof(ipv6_addr));
#else
        if (v_secure_system("ip -6 addr add %s/%d dev %s valid_lft forever preferred_lft forever", 
                    ipv6_addr, prefix_len, iface_name) != 0) {
            fprintf(stderr, "%s set ipv6 addr error.\n", iface_name);
            return -1;
        }
#endif
    }

    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    /*start zebra*/
    sysevent_set(si6->sefd, si6->setok, "zebra-restart", NULL, 0);
	
    return 0;
}

static int lan_addr6_unset(struct serv_ipv6 *si6)
{
    unsigned int l2_insts[MAX_LAN_IF_NUM] = {0};
    char if_name[16] = {0};
    char iface_prefix[INET6_ADDRSTRLEN] = {0};
#if defined (_PROPOSED_BUG_FIX_)
    char old_iface_prefix[INET6_ADDRSTRLEN] = {0};
    char default_lan_ifname[16] = {0};
    char old_lan_iface_prefix[INET6_ADDRSTRLEN] = {0};
#endif
    char iface_addr[INET6_ADDRSTRLEN] = {0};
    int prefix_len;
    unsigned int if_num = 0;
    int i = 0;
    char evt_name[64] = {0};
#if defined(MULTILAN_FEATURE)
    char active_instances[MAX_ACTIVE_INSTANCE] = {0};
    char *bridge_idx = NULL;

    snprintf(evt_name, sizeof(evt_name), "ipv6_active_inst");
    sysevent_get(si6->sefd, si6->setok, evt_name, active_instances, sizeof(active_instances));/*Get last active IPv6 instances*/
    bridge_idx = strtok(active_instances, " ");
    while(bridge_idx) {
        l2_insts[if_num++] = atoi(bridge_idx);
        bridge_idx = strtok(NULL, " ");
    }
#else
    get_active_lanif(si6, l2_insts, &if_num);
    if (if_num == 0) {
        fprintf(stderr, "no active lan interface.\n");
        return -1;
    }
#endif

#if defined (_PROPOSED_BUG_FIX_)
    snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
    sysevent_get(si6->sefd, si6->setok, evt_name, default_lan_ifname, sizeof(if_name));/*interface name*/
    sysevent_get(si6->sefd, si6->setok, "previous_ipv6_prefix", old_iface_prefix, sizeof(old_iface_prefix));
    snprintf(evt_name, sizeof(evt_name), "previous_ipv6_%s-prefix", default_lan_ifname);
    sysevent_get(si6->sefd, si6->setok, evt_name, old_lan_iface_prefix, sizeof(iface_prefix));
    if (strstr(old_iface_prefix, "/")) {
        strncpy(old_iface_prefix, strtok(old_iface_prefix, "/"), sizeof(old_iface_prefix));
    }
    if (strstr(old_lan_iface_prefix, "/")) {
        strncpy(old_lan_iface_prefix, strtok(old_lan_iface_prefix, "/"), sizeof(old_lan_iface_prefix));
    }    
#endif

    for (; i < if_num; i++) {
        snprintf(evt_name, sizeof(evt_name), "multinet_%d-name", l2_insts[i]);
        sysevent_get(si6->sefd, si6->setok, evt_name, if_name, sizeof(if_name));/*interface name*/
#if defined (_PROPOSED_BUG_FIX_)
        if (strlen(old_iface_prefix) && strcmp(old_iface_prefix, old_lan_iface_prefix)) {
            snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", if_name);
            sysevent_get(si6->sefd, si6->setok, evt_name, iface_prefix, sizeof(iface_prefix));
            snprintf(evt_name, sizeof(evt_name), "previous_ipv6_%s-prefix", if_name);
            sysevent_set(si6->sefd, si6->setok, evt_name, iface_prefix, 0);
        }
#endif

        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-prefix", if_name);
        sysevent_get(si6->sefd, si6->setok, evt_name, iface_prefix, sizeof(iface_prefix));
#ifdef MULTILAN_FEATURE
        sysevent_set(si6->sefd, si6->setok, evt_name, "", 0);
#endif
        
        /*del v6 addr*/
        snprintf(evt_name, sizeof(evt_name), "ipv6_%s-addr", if_name);
        sysevent_get(si6->sefd, si6->setok, evt_name, iface_addr, sizeof(iface_addr));
        if (iface_addr[0] != '\0') {
            get_prefix_info(iface_prefix, NULL, 0, &prefix_len);
            v_secure_system("ip -6 addr del %s/%d dev %s", iface_addr, prefix_len, if_name);
        }
#ifdef MULTILAN_FEATURE
        sysevent_set(si6->sefd, si6->setok, evt_name, "", 0);
#else
        sysctl_iface_set("/proc/sys/net/ipv6/conf/%s/disable_ipv6", if_name, "1"); /*this seems not work*/
#endif
        sysevent_set(si6->sefd, si6->setok, "ipv6_linklocal", "down", 0);
    }

    return 0;
}

static int format_dibbler_option(char *option)
{
    if (option == NULL)
        return -1;

    int i;

    for (i = 0; i < strlen(option); i++) {
        if(option[i] == ' ')
            option[i] = ',';
    }

    return 0;
}
/*
 * Generate the dibbler config:
 *      v6 address range based on the interface-prefix
 *      PD pool based on sub-prefixes
 *      IA-NA/IA-PD lifetime
 *      Options: RDNSS, DNSSL, SNTP, (CONTAINER option)
 */
static int gen_dibbler_conf(struct serv_ipv6 *si6)
{
    dhcpv6s_cfg_t       dhcpv6s_cfg;
    dhcpv6s_pool_cfg_t  dhcpv6s_pool_cfg;
    FILE                *fp = NULL;
    int                 pool_index;
    int                 opt_index;
    dhcpv6s_pool_opt_t  opt;
    int                 tag_index;
    char                prefix_value[64] = {0};
    pd_pool_t           pd_pool;
    ia_na_t             ia_na;
    ia_pd_t             ia_pd;
    char                evt_val[64] = {0};
    int                 ret = 0;
    int                 colon_count = 0;
    char                bridge_mode[4] = {0};
    FILE *ifd=NULL;
    char *HwAdrrPath = "/sys/class/net/brlan0/address";

    sysevent_get(si6->sefd, si6->setok, "ipv6_prefix-divided", evt_val, sizeof(evt_val));
    if (strcmp(evt_val, "ready")) {
       /*
        * Check if delegated prefix is already divided for lan interfaces.
        * If not, then divide the Operator-delegated prefix to sub-prefixes.
        */
        if (divide_ipv6_prefix(si6) != 0) {
            fprintf(stderr, "divide the operator-delegated prefix to sub-prefix error.\n");
            sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
#if defined (MULTILAN_FEATURE)
            report_no_lan_prefixes(si6);
#endif
            return -1;
        }
    }
    memset(&dhcpv6s_pool_cfg, 0, sizeof(dhcpv6s_pool_cfg_t)); /*RDKB-12965 & CID:-34146*/

    fp = fopen(DHCPV6S_CONF_FILE, "w+");
    if (fp == NULL)
        return -1;

    /*Begin write dibbler configurations*/
    fprintf(fp, "log-level 4\n");
   /*Run scipt to config route */
#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_)
    fprintf(fp, "script \"/lib/rdk/server-notify.sh\" \n");
#endif

#ifdef MULTILAN_FEATURE
    fprintf(fp, "reconfigure-enabled 1\n");
#endif

    get_dhcpv6s_conf(&dhcpv6s_cfg);
    if (dhcpv6s_cfg.server_type != DHCPV6S_TYPE_STATEFUL)
        fprintf(fp, "stateless\n");
    
    /*get ia_na & ia_pd info (addr, t1, t2, preftm, vldtm) which passthrough wan*/
    ret = get_ia_info(si6, PROVISIONED_V6_CONFIG_FILE, &ia_na, &ia_pd);

    for (pool_index = 0; pool_index < dhcpv6s_cfg.pool_num; pool_index++) {
        dhcpv6s_pool_cfg.index = pool_index;
        get_dhcpv6s_pool_cfg(si6, &dhcpv6s_pool_cfg);

        if (!dhcpv6s_pool_cfg.enable || dhcpv6s_pool_cfg.ia_prefix[0] == '\0') continue;
    	syscfg_get(NULL, "bridge_mode", bridge_mode, sizeof(bridge_mode));
        if (strcmp(bridge_mode, "2") || strcmp(dhcpv6s_pool_cfg.interface, "brlan0")) {

        fprintf(fp, "iface %s {\n", dhcpv6s_pool_cfg.interface);
        if (dhcpv6s_cfg.server_type != DHCPV6S_TYPE_STATEFUL) goto OPTIONS;

        if (dhcpv6s_pool_cfg.rapid_enable) fprintf(fp, "   rapid-commit yes\n");

#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
        if (dhcpv6s_pool_cfg.unicast_enable) { 
            //fprintf(fp, "  unicast %s\n", ipv6_addr); /*TODO: get ipv6 address*/
        }

        fprintf(fp, "   iface-max-lease %d\n", dhcpv6s_pool_cfg.iana_amount);
#endif

        fprintf(fp, "   preference %d\n", 255);

        if (dhcpv6s_pool_cfg.iana_enable) {
#ifdef MULTILAN_FEATURE
            fprintf(fp, "   subnet %s\n", dhcpv6s_pool_cfg.ia_prefix);
#endif
            fprintf(fp, "   class {\n");
#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
            if (dhcpv6s_pool_cfg.eui64_enable) fprintf(fp, "       share 1000\n");
            fprintf(fp, "       pool %s\n", dhcpv6s_pool_cfg.ia_prefix);
#else
            if (get_prefix_info(dhcpv6s_pool_cfg.ia_prefix, prefix_value, sizeof(prefix_value), NULL) == 0) {

                int count = 0;
                int i = 0;

                while(prefix_value[i]) {
                    if (prefix_value[i] == ':')
                        count++;
                    i++;
                }

                /* delete one last ':' becaues there are 4 parts in this prefix*/
                if (count == 5)
                    prefix_value[strlen(prefix_value)-1] = '\0';

                fprintf(fp, "       pool %s%s - %s%s\n", prefix_value, dhcpv6s_pool_cfg.prefix_range_begin,
                        prefix_value, dhcpv6s_pool_cfg.prefix_range_end);
                colon_count = count;
            }
#endif
            /*lease time*/
            {
                unsigned long t1, t2, pref_time, valid_time;
                if (dhcpv6s_pool_cfg.lease_time <= -1) {
                    t1 = t2 = pref_time = valid_time = 0xFFFFFFFF;
                } else {
                    t1 = dhcpv6s_pool_cfg.lease_time / 2;
                    t2 = (unsigned long)(dhcpv6s_pool_cfg.lease_time * 80.0 /100);
                    pref_time = valid_time = dhcpv6s_pool_cfg.lease_time; 
                }
                fprintf(fp, "       T1 %u\n", t1);
                fprintf(fp, "       T2 %u\n", t2);
                fprintf(fp, "       prefered-lifetime %u\n", pref_time);
                fprintf(fp, "       valid-lifetime %u\n", valid_time);
            }

            fprintf(fp, "   }\n");
        }
        
        if (dhcpv6s_pool_cfg.iapd_enable) {    
            /*pd pool*/
            if(get_pd_pool(si6, &pd_pool) == 0) {
                fprintf(fp, "   pd-class {\n");
#if defined (_CBR_PRODUCT_REQ_) || defined (_BWG_PRODUCT_REQ_)
                fprintf(fp, "       pd-pool %s /%d\n", pd_pool.start, pd_pool.prefix_length);
#else
				fprintf(fp, "       pd-pool %s - %s /%d\n", pd_pool.start, pd_pool.end, pd_pool.prefix_length);
#endif	
                fprintf(fp, "       pd-length %d\n", pd_pool.pd_length);

                if (ret == 0 ) {
                    fprintf(fp, "       T1 %s\n", ia_pd.t1);
                    fprintf(fp, "       T2 %s\n", ia_pd.t2);
                    fprintf(fp, "       prefered-lifetime %s\n", ia_pd.pretm);
                    fprintf(fp, "       valid-lifetime %s\n", ia_pd.vldtm);
                } 

                fprintf(fp, "   }\n");
                printf("%s Fixed prefix_value: %s\n", __func__, prefix_value);
                /*
                client duid B4:2A:0E:11:06:9B
                {
                address 2603:3024:18bf:2000:1::123
                prefix 2603:3024:18bf:2000::/64
                } */
                char dummyAddr[128];
                char HwAddr[24];
                memset( HwAddr, 0, sizeof( HwAddr ) );
                memset( dummyAddr, 0, sizeof( dummyAddr ) );
                strncpy(dummyAddr,prefix_value,sizeof(prefix_value));
                
		dummyAddr[sizeof(dummyAddr)-1] = '\0';
                if( ( ifd = fopen( HwAdrrPath, "r" ) ) != NULL )
                        {
                                if( fgets( HwAddr, sizeof( HwAddr ), ifd ) != NULL )
                                {
                                        fprintf(fp, "client duid %s\n",HwAddr);
                                }
                                fclose(ifd);
                                ifd = NULL;
                        }
                        else
                        fprintf(fp, "client duid 01:02:03:04:05:06\n");

                fprintf(fp, "   {\n");
		if (colon_count == 5)
                {
                	strcat(dummyAddr,":123");
			dummyAddr[sizeof(dummyAddr)-1] = '\0';
			fprintf(fp, "   address %s\n",dummyAddr);
			fprintf(fp, "   prefix %s:/64\n",prefix_value);
                }
                else
		{
                	strcat(dummyAddr,"123");
			dummyAddr[sizeof(dummyAddr)-1] = '\0';
			fprintf(fp, "   address %s\n",dummyAddr);
			fprintf(fp, "   prefix %s/64\n",prefix_value);
		}
                fprintf(fp, "   }\n");
            }
        }

OPTIONS:
        for (opt_index = 0; opt_index < dhcpv6s_pool_cfg.opt_num; opt_index++) {
            opt = dhcpv6s_pool_cfg.opts[opt_index];
            if (!opt.enable) continue;
            for (tag_index = 0; tag_index < NELEMS(tag_list); tag_index++ ) {
                if (tag_list[tag_index].tag == opt.tag) break;
            }
            
            if (tag_index >= NELEMS(tag_list)) continue;

            if (opt.pt_client[0]) {
                if (opt.tag == 23) {//dns
                    char dns_str[256] = {0};

					/* Static DNS */
					if( 1 == dhcpv6s_pool_cfg.X_RDKCENTRAL_COM_DNSServersEnabled )	
					{
						memset( dns_str, 0, sizeof( dns_str ) );
						strcpy( dns_str, dhcpv6s_pool_cfg.X_RDKCENTRAL_COM_DNSServers );
						fprintf(stderr,"%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__, 
																						  __LINE__,
																						  dhcpv6s_pool_cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
																						  dhcpv6s_pool_cfg.X_RDKCENTRAL_COM_DNSServers );
					}
					else
					{
						sysevent_get(si6->sefd, si6->setok, "ipv6_nameserver", dns_str, sizeof(dns_str));
					}
					
                    if (dns_str[0] != '\0') { 
                        format_dibbler_option(dns_str);
                        fprintf(fp, "     option %s %s\n", tag_list[tag_index].opt_str, dns_str);
                    }
                }
                else if (opt.tag == 24) {//domain
                    char domain_str[256] = {0};
                    sysevent_get(si6->sefd, si6->setok, "ipv6_dnssl", domain_str, sizeof(domain_str));
                    if (domain_str[0] != '\0') { 
                        format_dibbler_option(domain_str);
                        fprintf(fp, "     option %s %s\n", tag_list[tag_index].opt_str, domain_str);
                    }
                }
            } else {
                /*TODO:
                 * the configured option value, which is not passed through wan side*/
            }
        } 
        fprintf(fp, "}\n");
        } //closing bracket of if (strcmp(bridge_mode, "2") || strcmp(dhcpv6s_pool_cfg.interface, "brlan0")) {

        if (dhcpv6s_pool_cfg.opts != NULL) {
            free(dhcpv6s_pool_cfg.opts);
	   dhcpv6s_pool_cfg.opts = NULL; /*RDKB-12965 & CID:-34148*/
	}
    }

    fclose(fp);

    return 0;
}

static int dhcpv6s_start(struct serv_ipv6 *si6)
{
#ifdef MULTILAN_FEATURE
#if defined(_COSA_FOR_BCI_)
    char dhcpv6Enable[8]={0};
#endif
#else
    #if defined(_COSA_FOR_BCI_)
    char dhcpv6Enable[8]={0};
    #endif
#endif

    if (gen_dibbler_conf(si6) != 0) {
        fprintf(stderr, "%s: fail to generate dibbler config\n", __FUNCTION__);
        return -1;
    }
#ifdef MULTILAN_FEATURE
    daemon_stop(DHCPV6S_PID_FILE, DHCPV6_SERVER);
#else
    daemon_stop(DHCPV6S_PID_FILE, "dibbler");
#endif
#if defined(_COSA_FOR_BCI_)
    syscfg_get(NULL, "dhcpv6s00::serverenable", dhcpv6Enable , sizeof(dhcpv6Enable));
    if (!strncmp(dhcpv6Enable, "0", 1))
    {
       fprintf(stderr, "%s: DHCPv6 Disabled. Dibbler start not required !\n", __FUNCTION__);
       return 0;
    }
#endif
    sleep(1);
    v_secure_system("%s start", DHCPV6_SERVER);
    return 0;
}

static int dhcpv6s_stop(struct serv_ipv6 *si6)
{
#ifdef MULTILAN_FEATURE
    return daemon_stop(DHCPV6S_PID_FILE, DHCPV6_SERVER);
#else
    return daemon_stop(DHCPV6S_PID_FILE, "dibbler");
#endif
}

static int dhcpv6s_restart(struct serv_ipv6 *si6)
{
    if (dhcpv6s_stop(si6) != 0)
        fprintf(stderr, "%s: dhcpv6s_stop error\n", __FUNCTION__);

    return dhcpv6s_start(si6);
}


static int serv_ipv6_start(struct serv_ipv6 *si6)
{
    char status[16], enable[16], rtmod[16];
    char prefix[INET6_ADDRSTRLEN];

    /* state check */
    if (!serv_can_start(si6->sefd, si6->setok, "service_ipv6"))
        return -1;


    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    if (atoi(rtmod) != 1) { /* IPv6-only or Dual-Stack */
        if (!si6->wan_ready) {
            fprintf(stderr, "%s: IPv6-WAN is not ready !\n", __FUNCTION__);
            return -1;
        }
    } else {/* IPv4-only */
        return 0;
    }

    sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "starting", 0);

    /* handle logic:
     *  1) divide the Operator-delegated prefix to sub-prefixes
     *  2) further break the first of these sub-prefixes into /64 interface-prefixes for lan interface
     *  3) assign IPv6 address from the corresponding interface-prefix for lan interfaces
     *  4) the remaining sub-prefixes are delegated via DHCPv6 to directly downstream routers
     *  5) Send RA, start DHCPv6 server
     */
	/* For CBR product the lan(brlan0) v6 address set is done as part of PandM process*/
#if !defined(_CBR_PRODUCT_REQ_) && !defined(_BWG_PRODUCT_REQ_)
    if (lan_addr6_set(si6) !=0) {
        fprintf(stderr, "assign IPv6 address for lan interfaces error!\n");
        sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        return -1;
    }
#endif
	
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    /*start zebra*/
    sysevent_set(si6->sefd, si6->setok, "zebra-restart", NULL, 0);
	
    if (dhcpv6s_start(si6) != 0) {
        fprintf(stderr, "start dhcpv6 server error.\n");
        sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        return -1;
    }
#ifdef MULTILAN_FEATURE
    /* Restart firewall to apply ip6tables rules*/
    sysevent_set(si6->sefd, si6->setok, "firewall-restart", NULL, 0);
#endif

    sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "started", 0);
    return 0;
}

static int serv_ipv6_stop(struct serv_ipv6 *si6)
{
    if (!serv_can_stop(si6->sefd, si6->setok, "service_ipv6"))
        return -1;

    sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "stopping", 0);

    if (dhcpv6s_stop(si6) != 0) {
        fprintf(stderr, "stop dhcpv6 server error.\n");
        sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        return -1;
    }
#if !defined(_CBR_PRODUCT_REQ_) && !defined(_BWG_PRODUCT_REQ_)
    if (lan_addr6_unset(si6) !=0) {
        fprintf(stderr, "unset IPv6 address for lan interfaces error!\n");
        sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        return -1;
    }
#endif
    sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "stopped", 0);
    return 0;
}

static int serv_ipv6_restart(struct serv_ipv6 *si6)
{
    if (serv_ipv6_stop(si6) != 0)
        fprintf(stderr, "%s: serv_ipv6_stop error\n", __FUNCTION__);

    return serv_ipv6_start(si6);
}

static int serv_ipv6_init(struct serv_ipv6 *si6)
{
    char wan_st[16], lan_st[16];
    char buf[16];
#ifdef MULTILAN_FEATURE
    int ret = 0;
    char* pCfg = CCSP_MSG_BUS_CFG;
#endif

    memset(si6, 0, sizeof(struct serv_ipv6));

    if ((si6->sefd = sysevent_open(SE_SERV, SE_SERVER_WELL_KNOWN_PORT, 
                    SE_VERSION, PROG_NAME, &si6->setok)) < 0) {
        fprintf(stderr, "%s: fail to open sysevent\n", __FUNCTION__);
        return -1;
    }

    if (syscfg_init() != 0) {
        fprintf(stderr, "%s: fail to init syscfg\n", __FUNCTION__);
        return -1;
    }
#ifdef MULTILAN_FEATURE
    ret = CCSP_Message_Bus_Init(service_ipv6_component_id, pCfg, &bus_handle, Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    if (ret == -1) {
        fprintf(stderr, "%s: DBUS connection failed \n", __FUNCTION__);
        bus_handle = NULL;
        ret -1;
    }
#endif
    syscfg_get(NULL, "last_erouter_mode", buf, sizeof(buf));
    if(atoi(buf) == 1) {/*v4 only*/
        fprintf(stderr, "IPv6 not enabled on board!\n");
#if defined (_PROPOSED_BUG_FIX_)
        //Intel Proposed RDKB Bug Fix
        si6->mso_prefix[0] = '\0';
        si6->wan_ready = false;
        return 0;
#else
        return -1;
#endif
    }

    sysevent_get(si6->sefd, si6->setok, "ipv6_prefix", si6->mso_prefix, sizeof(si6->mso_prefix));
    if (strlen(si6->mso_prefix))
        si6->wan_ready = true;

    sysevent_get(si6->sefd, si6->setok, "erouter_topology-mode", buf, sizeof(buf));
    switch(atoi(buf)) {
        case 1:
            si6->tpmod = FAVOR_DEPTH;
            break;
        case 2:
            si6->tpmod = FAVOR_WIDTH;
            break;
        default:
#ifdef MULTILAN_FEATURE
            fprintf(stderr, "%s: unknown erouter topology mode, settinf default mode to FAVOR_WIDTH \n", __FUNCTION__);
            si6->tpmod = FAVOR_WIDTH;
#else
            fprintf(stderr, "%s: unknown erouter topology mode.\n", __FUNCTION__);
            si6->tpmod = TPMOD_UNKNOWN;
#endif
            break;
    }

    return 0;
}

static int serv_ipv6_term(struct serv_ipv6 *si6)
{
    sysevent_close(si6->sefd, si6->setok);
#ifdef MULTILAN_FEATURE
    if (bus_handle != NULL) {
        fprintf(stderr, "Closing DBUS connection \n");
        CCSP_Message_Bus_Exit(bus_handle);
    }
#endif
    return 0;
}

struct cmd_op {
    const char  *cmd;
    int         (*exec)(struct serv_ipv6 *si6);
    const char  *desc;
};

static struct cmd_op cmd_ops[] = {
    {"start",       serv_ipv6_start,  "start service ipv6"},
    {"stop",        serv_ipv6_stop,   "stop service ipv6"},
    {"restart",     serv_ipv6_restart,"restart service ipv6"},
    {"addr-set",     lan_addr6_set, "set IPv6 address for lan interface"},
    {"addr-unset",     lan_addr6_unset, "unset IPv6 address for lan interface"},
    {"dhcpv6s-start",  dhcpv6s_start,     "start DHCPv6 Sever"},
    {"dhcpv6s-stop",   dhcpv6s_stop,      "stop DHCPv6 Server"},
    {"dhcpv6s-restart",dhcpv6s_restart,   "restart DHCPv6 Server"},
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
    struct serv_ipv6 si6;

    fprintf(stderr, "[%s] -- IN\n", PROG_NAME);

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (serv_ipv6_init(&si6) != 0)
        exit(1);

    for (i = 0; i < NELEMS(cmd_ops); i++) {
        if (strcmp(argv[1], cmd_ops[i].cmd) != 0 || !cmd_ops[i].exec)
            continue;

        fprintf(stderr, "[%s] EXEC: %s\n", PROG_NAME, cmd_ops[i].cmd);

        if (cmd_ops[i].exec(&si6) != 0)
            fprintf(stderr, "[%s]: fail to exec `%s'\n", PROG_NAME, cmd_ops[i].cmd);

        break;
    }
    if (i == NELEMS(cmd_ops))
        fprintf(stderr, "[%s] unknown command: %s\n", PROG_NAME, argv[1]);

    if (serv_ipv6_term(&si6) != 0)
        exit(1);

    fprintf(stderr, "[%s] -- OUT\n", PROG_NAME);
    exit(0);
}

