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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include "utapi.h"
#include "utapi_util.h"
#include "utapi_tr_dhcp.h"
#include "DM_TR181.h"
#include <arpa/inet.h>
#include "safec_lib_common.h"

#ifdef SKY_RDKB
#define DNSMASQ_LEASE_CONFIG_FILE "/nvram/dnsmasq.leases"
#define BUF_LEN 200
#endif

static int g_IndexMapServerPool[MAX_NUM_INSTANCES+1] = {-1};
static int g_IndexMapStaticAddr[MAX_NUM_INSTANCES+1] = {-1};

int Utopia_GetDhcpServerEnable(UtopiaContext *ctx, unsigned char *bEnabled)
{
    int iVal = -1;
#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    Utopia_GetInt(ctx, UtopiaValue_DHCP_ServerEnabled, &iVal);

    *bEnabled = (0 == iVal) ? FALSE : TRUE; 

    return SUCCESS;
}

int Utopia_SetDhcpServerEnable(UtopiaContext *ctx, unsigned char bEnabled)
{
    int iVal = -1;
#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** with !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    iVal = (FALSE == bEnabled) ? 0 : 1;
    UTOPIA_SETINT(ctx, UtopiaValue_DHCP_ServerEnabled, iVal);
    return SUCCESS;
}
int Utopia_GetNumberOfDhcpV4ServerPools()
{

#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    return DHCPV4_NUM_SERVER_POOLS;
}

int Utopia_GetDhcpV4ServerPoolEntry(UtopiaContext *ctx, unsigned long ulIndex, void *pEntry)
{
    if((NULL == ctx) || (NULL == pEntry)){
        return ERR_INVALID_ARGS;
    }
    /*char insNum[STR_SZ] = {'\0'};*/
    unsigned long ulInstanceNum = 0;
#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

   dhcpV4ServerPoolEntry_t *pEntry_t = (dhcpV4ServerPoolEntry_t *)pEntry;
#if 0
   if(Utopia_Get(ctx,UtopiaValue_DHCP_ServerPool_InsNum,insNum,STR_SZ)) {
       ulInstanceNum = atol(insNum);
       g_IndexMapServerPool[ulInstanceNum] = ulIndex;
       pEntry_t->Cfg.InstanceNumber = ulInstanceNum;
   }
#else
	g_IndexMapServerPool[1] = ulIndex;
       pEntry_t->Cfg.InstanceNumber = 1;
#endif
   Utopia_GetDhcpV4ServerPoolCfg(ctx, &(pEntry_t->Cfg));
   Utopia_GetDhcpV4ServerPoolInfo(ctx, ulInstanceNum, &(pEntry_t->Info));
   return SUCCESS;
}

int Utopia_GetDhcpV4ServerPoolCfg(UtopiaContext *ctx, void *pCfg)
{
    int iVal = -1;
    int rc = -1;
    char strVal[STR_SZ] = {'\0'};
    char propagate_ns[STR_SZ] = {'\0'};
    char block_nat_redir[STR_SZ] = {'\0'};
    char prefixIP[IPADDR_SZ] = {'\0'};
    char completeIP[IPADDR_SZ+90] = {'\0'};
    errno_t  safec_rc = -1;

    bool_t propagate_wan_dns = FALSE;
    lanSetting_t lan;
    dhcpServerInfo_t dhcps;
    if((NULL == ctx) || (NULL == pCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK)
    {
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    dhcpV4ServerPoolCfg_t *cfg_t = (dhcpV4ServerPoolCfg_t *)pCfg;
    
    Utopia_Get(ctx,UtopiaValue_DHCP_ServerPool_Alias,cfg_t->Alias,sizeof(cfg_t->Alias));
    if(!strlen(cfg_t->Alias)){
        safec_rc = strcpy_s(cfg_t->Alias,sizeof(cfg_t->Alias),"ServerPool-1");
        ERR_CHK(safec_rc);
    }
    rc = Utopia_GetLanSettings(ctx, &lan);
    if (SUCCESS != rc) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error: Can not get LAN Parameters - Using Default Values!!!", __FUNCTION__);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        cfg_t->IPRouters[0].Value = inet_addr("192.168.1.1");
        cfg_t->SubnetMask.Value = inet_addr("255.255.255.0");
        safec_rc = strcpy_s(cfg_t->DomainName, sizeof(cfg_t->DomainName), "utopia.net");
        ERR_CHK(safec_rc);
        safec_rc = strcpy_s(cfg_t->Interface,sizeof(cfg_t->Interface), "brlan0");
        ERR_CHK(safec_rc);
    }else
    {
        cfg_t->IPRouters[0].Value = inet_addr(lan.ipaddr);
        cfg_t->SubnetMask.Value = inet_addr(lan.netmask);
        safec_rc = strcpy_s(cfg_t->DomainName, sizeof(cfg_t->DomainName), lan.domain);
        ERR_CHK(safec_rc);
        safec_rc = strcpy_s(cfg_t->Interface, sizeof(cfg_t->Interface), lan.ifname);
        ERR_CHK(safec_rc);
    }
 
    /* strip out the last octet of LAN IP*/
    char* pch= strrchr(lan.ipaddr, '.');
    if(pch) {
        /* copies the first three numbers of the IP */
        *pch=0;
        safec_rc = strcpy_s(prefixIP, sizeof(prefixIP), lan.ipaddr);
        ERR_CHK(safec_rc);
    }

    rc = Utopia_GetDHCPServerSettings (ctx, &dhcps);
    if (SUCCESS != rc) {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error: Can not get DHCP Parameters - Using Default Values!!!", __FUNCTION__);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        cfg_t->MinAddress.Value = inet_addr("192.168.1.100");
        cfg_t->MaxAddress.Value = inet_addr("192.168.1.149");
        cfg_t->LeaseTime = 86400;
        
    }else
    {
        if(strlen(dhcps.DHCPIPAddressStart) <= 3){        /* just last octet */
            safec_rc = sprintf_s(completeIP, sizeof(completeIP), "%s.%s",prefixIP,dhcps.DHCPIPAddressStart);
            if(safec_rc < EOK)
            {
                ERR_CHK(safec_rc);
            }
            cfg_t->MinAddress.Value = inet_addr(completeIP);

            /* We only have MaxNum's in syscfg - We need to derive Max Address from that */
            if(0 == dhcps.DHCPMaxUsers) {
                dhcps.DHCPMaxUsers = 50; /* Default Value */
            }
            iVal = atoi(dhcps.DHCPIPAddressStart);
            iVal = (iVal + dhcps.DHCPMaxUsers) - 1; /* Max user includes start and end address */
            safec_rc = sprintf_s(strVal, sizeof(strVal),"%d",iVal);
            if(safec_rc < EOK)
            {
                ERR_CHK(safec_rc);
            }
            safec_rc = sprintf_s(completeIP, sizeof(completeIP),"%s.%s",prefixIP,strVal);
            if(safec_rc < EOK)
            {
                ERR_CHK(safec_rc);
            }
            cfg_t->MaxAddress.Value = inet_addr(completeIP);
        }else{
            cfg_t->MinAddress.Value = inet_addr(dhcps.DHCPIPAddressStart);
            cfg_t->MaxAddress.Value = inet_addr(dhcps.DHCPIPAddressEnd);
        }
        /* format lease time */
        if(0 == strlen(dhcps.DHCPClientLeaseTime)) {
            safec_rc = strcpy_s(dhcps.DHCPClientLeaseTime, sizeof(dhcps.DHCPClientLeaseTime), "24h");/*Default Value */
            ERR_CHK(safec_rc);
        }
        memset(strVal,0,STR_SZ);
	/* CID 135591 : Destination buffer too small */
        strncpy(strVal, dhcps.DHCPClientLeaseTime, sizeof(strVal)-1);
	strVal[sizeof(strVal)-1] = '\0';

        /* lease time default is 24h, otherwise convert to minutes */
        if ( strVal[strlen(strVal)-1] == 'd'){/*added by soyou to support day*/
            strVal[strlen(strVal)-1] = '\0';
            cfg_t->LeaseTime = atol(strVal) * 86400; /* 86400=60*60*24 */
        }else if ( strVal[strlen(strVal)-1] == 'h') {
            strVal[strlen(strVal)-1] = '\0';
            cfg_t->LeaseTime = atol(strVal) * 60 * 60; /* We are disaplying lease time in seconds */
        } else if ( strVal[strlen(strVal)-1] == 'm')
        {
            strVal[strlen(strVal)-1] = '\0';
            cfg_t->LeaseTime = atol(strVal) * 60;
        } else
        {
            cfg_t->LeaseTime = atol(dhcps.DHCPClientLeaseTime);
        }

        cfg_t->DNSServersEnabled = dhcps.StaticNameServerEnabled;
        if(0 == strlen(dhcps.StaticNameServer1)) {
            Utopia_RawGet(ctx,NULL,"dhcp_server_propagate_wan_nameserver",propagate_ns,sizeof(propagate_ns));
            Utopia_Get(ctx,UtopiaValue_Firewall_BlockNatRedir,block_nat_redir,sizeof(block_nat_redir));
            if(((0 != strlen(propagate_ns)) && (1 == atoi(propagate_ns))) || ((0 != strlen(block_nat_redir)) && (1 == atoi(block_nat_redir)))) {
                Utopia_Get(ctx,UtopiaValue_WAN_NameServer1,dhcps.StaticNameServer1,sizeof(dhcps.StaticNameServer1));
                if(0 != strlen(dhcps.StaticNameServer1)) {
                    cfg_t->DNSServers[0].Value = inet_addr(dhcps.StaticNameServer1);
                }
                propagate_wan_dns = TRUE;
            } else
            {
                /* No DNS server is set and the flag to propagate wan nameservers is off, so assume we are DNS Proxy */
                cfg_t->DNSServers[0].Value = cfg_t->IPRouters[0].Value;
            }
        } else
        {
            cfg_t->DNSServers[0].Value = inet_addr(dhcps.StaticNameServer1);
        }
        if(0 == strlen(dhcps.StaticNameServer2)) {
            if(TRUE == propagate_wan_dns) {
                Utopia_Get(ctx,UtopiaValue_WAN_NameServer2,dhcps.StaticNameServer2,sizeof(dhcps.StaticNameServer2));
                if(0 != strlen(dhcps.StaticNameServer2)) {
                    cfg_t->DNSServers[1].Value = inet_addr(dhcps.StaticNameServer2);
                }
            }
        } else
        {
            cfg_t->DNSServers[1].Value = inet_addr(dhcps.StaticNameServer2);
        }
        if(0 == strlen(dhcps.StaticNameServer3)) {
            if(TRUE == propagate_wan_dns) {
                Utopia_Get(ctx,UtopiaValue_WAN_NameServer3,dhcps.StaticNameServer3,sizeof(dhcps.StaticNameServer3));
                if(0 != strlen(dhcps.StaticNameServer3)) {
                    cfg_t->DNSServers[2].Value = inet_addr(dhcps.StaticNameServer3);
                }
            }
        } else
        {
            cfg_t->DNSServers[2].Value = inet_addr(dhcps.StaticNameServer3);
        }
    }

    iVal = -1;
    Utopia_GetInt(ctx, UtopiaValue_DHCP_ServerEnabled, &iVal);
    cfg_t->bEnabled = (0 == iVal) ? FALSE : TRUE;
    cfg_t->Order = 1; /* As we support only one server pool, the priority is always 1 */    

    return SUCCESS;
}

int Utopia_GetDhcpV4ServerPoolInfo(UtopiaContext *ctx, unsigned long ulInstanceNumber, void *pInfo)
{
    token_t        se_token;
    char           dhcp_status[STR_SZ];
    int            se_fd = -1;
    int            iVal = -1;

    if (NULL == ctx || NULL == pInfo) {
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    dhcpV4ServerPoolInfo_t *info_t = (dhcpV4ServerPoolInfo_t *)pInfo;

    se_fd = s_sysevent_connect(&se_token);
    if (0 > se_fd) {
        return ERR_SYSEVENT_CONN;
    }

    memset(dhcp_status,0,sizeof(dhcp_status));
    /* Get DHCP Server Status */
    sysevent_get(se_fd, se_token, "dhcp_server-status", dhcp_status, sizeof(dhcp_status));
    Utopia_GetInt(ctx, UtopiaValue_DHCP_ServerEnabled, &iVal);

    if (0 == strcmp(dhcp_status, "started")) {
        info_t->Status = DHCP_SERVER_POOL_STATUS_Enabled;
    }else if (0 == strcmp(dhcp_status, "error")) {
        if(iVal)
            info_t->Status = DHCP_SERVER_POOL_STATUS_Error_Misconfigured; /*It is enabled but still is stopped */
        else
            info_t->Status = DHCP_SERVER_POOL_STATUS_Disabled; /* It is disabled */
    } else {
        info_t->Status = DHCP_SERVER_POOL_STATUS_Error;
	if (0 == strcmp(dhcp_status, "stopped"))
        {
	    info_t->Status = DHCP_SERVER_POOL_STATUS_Disabled; /* It is disabled */
	}
	if (0 == strlen(dhcp_status))
	{
	    if(iVal)
	    {
	        info_t->Status = DHCP_SERVER_POOL_STATUS_Enabled;
            }   
	    else
	    {
	        info_t->Status = DHCP_SERVER_POOL_STATUS_Disabled; /* It is disabled */
	    }
	}
    }

    return SUCCESS;
}

int Utopia_ValidateLanDhcpPoolRange(UtopiaContext *ctx, const unsigned long minaddr, const unsigned long maxaddr)
{
    int rc = SUCCESS;
    lanSetting_t lan;
    unsigned long lan_ip, lan_netmask;

    if (NULL == ctx)
        return ERR_INVALID_ARGS;

    rc = Utopia_GetLanSettings(ctx, &lan);
    if (rc != SUCCESS)
        return rc;

    lan_ip = inet_addr(lan.ipaddr);
    lan_netmask = inet_addr(lan.netmask);

    if (IsLoopback(minaddr) || 
        IsMulticast(minaddr)||
        IsBroadcast(minaddr, lan_ip, lan_netmask) || 
        IsNetworkAddr(minaddr, lan_ip, lan_netmask)|| 
        !IsSameNetwork(minaddr, lan_ip, lan_netmask)|| 
        minaddr == lan_ip){
        return ERR_INVALID_PORT_RANGE;
    }

    if (IsLoopback(maxaddr) || 
        IsMulticast(maxaddr)||
        IsBroadcast(maxaddr, lan_ip, lan_netmask) || 
        IsNetworkAddr(maxaddr, lan_ip, lan_netmask)|| 
        !IsSameNetwork(maxaddr, lan_ip, lan_netmask)|| 
        minaddr == lan_ip){
        return ERR_INVALID_PORT_RANGE;
    }

    if (ntohl(minaddr) > ntohl(maxaddr))
        return ERR_INVALID_PORT_RANGE;

    return SUCCESS;
}

int Utopia_SetDhcpV4ServerPoolCfg(UtopiaContext *ctx, void *pCfg)
{
    int rc = -1;
    errno_t  safec_rc = -1;
    struct in_addr addrVal;
    /*char strVal[STR_SZ] = {'\0'};*/
    /*char completeIP[IPADDR_SZ] = {'\0'};*/

    /*lanSetting_t lan;*/
    dhcpServerInfo_t dhcps;
    if((NULL == ctx) || (NULL == pCfg)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(safec_rc < EOK)
    {
        ERR_CHK(safec_rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    dhcpV4ServerPoolCfg_t *cfg_t = (dhcpV4ServerPoolCfg_t *)pCfg;

    /* Set Alias if it has a value */
    if(strlen(cfg_t->Alias))
        UTOPIA_SET(ctx,UtopiaValue_DHCP_ServerPool_Alias,cfg_t->Alias);

    /* Order, Interface and Enable are read-only and hence we ignore thenm */

    /* Set all the LAN Parameters first */
    /*
    strcpy(lan.ipaddr, inet_ntoa(cfg_t->IPRouters[0].Value));
    strcpy(lan.netmask,inet_ntoa(cfg_t->SubnetMask.Value));
    strcpy(lan.domain,cfg_t->DomainName);
    rc = Utopia_SetLanSettings(ctx, &lan);
    if (SUCCESS != rc)
    {
        sprintf(ulog_msg, "%s: Error: Can not set LAN Parameters !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }
    */

    /* Now set the DHCP values */

    /* First fill WINS Server and Enable from syscfg as we are not getting it from Data Model */
    Utopia_Get(ctx, UtopiaValue_DHCP_WinsServer, dhcps.WINSServer, IPHOSTNAME_SZ);
    UTOPIA_GETBOOL(ctx, UtopiaValue_DHCP_ServerEnabled, &dhcps.enabled);
    dhcps.enabled = cfg_t->bEnabled;
#if 0
    /* Min Address is stored as the number in the last octet of Min address */
    strcpy(completeIP,inet_ntoa(cfg_t->MinAddress.Value));
    strcpy(dhcps.DHCPIPAddressStart,strrchr(completeIP,'.'));
    /* strrchr returns with a dot, so we need to shift it */
    strcpy(dhcps.DHCPIPAddressStart,(dhcps.DHCPIPAddressStart+1));

    /* Max Address is stored as Max number of users */
    memset(completeIP,0,IPADDR_SZ);
    strcpy(completeIP,inet_ntoa(cfg_t->MaxAddress.Value)); 
    strcpy(strVal,strrchr(completeIP,'.'));
    /* strrchr returns with a dot, so we need to shift it */
    dhcps.DHCPMaxUsers = (atoi(strVal+1) - atoi(dhcps.DHCPIPAddressStart)) + 1; /* Num users include start and end address, so increment */
#else
    rc = Utopia_ValidateLanDhcpPoolRange(ctx, cfg_t->MinAddress.Value, cfg_t->MaxAddress.Value);
    if (rc != SUCCESS)
        return rc;
    addrVal.s_addr = cfg_t->MinAddress.Value;
    strncpy(dhcps.DHCPIPAddressStart, inet_ntoa(addrVal), sizeof(dhcps.DHCPIPAddressStart)-1);
    addrVal.s_addr = cfg_t->MaxAddress.Value;
    strncpy(dhcps.DHCPIPAddressEnd, inet_ntoa(addrVal), sizeof(dhcps.DHCPIPAddressEnd)-1);
    dhcps.DHCPMaxUsers = ntohl(cfg_t->MaxAddress.Value) - ntohl(cfg_t->MinAddress.Value) + 1;
#endif

    safec_rc = sprintf_s(dhcps.DHCPClientLeaseTime,sizeof(dhcps.DHCPClientLeaseTime),"%d",cfg_t->LeaseTime);
    if(safec_rc < EOK)
    { 
        ERR_CHK(safec_rc);
    }
  
    dhcps.StaticNameServerEnabled = cfg_t->DNSServersEnabled;
    if(cfg_t->DNSServers[0].Value > 0){
        addrVal.s_addr = cfg_t->DNSServers[0].Value;
        safec_rc = strcpy_s(dhcps.StaticNameServer1,sizeof(dhcps.StaticNameServer1),inet_ntoa(addrVal));
        ERR_CHK(safec_rc);
    }
    else
    {
        safec_rc = strcpy_s(dhcps.StaticNameServer1, sizeof(dhcps.StaticNameServer1),"");
        ERR_CHK(safec_rc);
    }
    if(cfg_t->DNSServers[1].Value > 0){
        addrVal.s_addr = cfg_t->DNSServers[1].Value;
        safec_rc = strcpy_s(dhcps.StaticNameServer2, sizeof(dhcps.StaticNameServer2),inet_ntoa(addrVal));
        ERR_CHK(safec_rc);
    }
    else
    {
        safec_rc = strcpy_s(dhcps.StaticNameServer2, sizeof(dhcps.StaticNameServer2),"");
        ERR_CHK(safec_rc);
    }
    if(cfg_t->DNSServers[2].Value > 0){
        addrVal.s_addr = cfg_t->DNSServers[2].Value;
        safec_rc = strcpy_s(dhcps.StaticNameServer3, sizeof(dhcps.StaticNameServer3),inet_ntoa(addrVal));
        ERR_CHK(safec_rc);
    }
    else
    {
        safec_rc = strcpy_s(dhcps.StaticNameServer3, sizeof(dhcps.StaticNameServer3),"");
        ERR_CHK(safec_rc);
    }

    rc = Utopia_SetDHCPServerSettings (ctx, &dhcps);
    if (SUCCESS != rc)
    {
        safec_rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error: Can not set DHCP Parameters !!!", __FUNCTION__);
        if(safec_rc < EOK)
        { 
            ERR_CHK(safec_rc);
        }
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
    }

    return SUCCESS;
}

int Utopia_SetDhcpV4ServerPoolValues(UtopiaContext *ctx, unsigned long ulIndex, unsigned long ulInstanceNumber, char *pAlias)
{

    if((NULL == ctx) || (NULL == pAlias)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    g_IndexMapServerPool[ulInstanceNumber] = ulIndex;

    UTOPIA_SETINT(ctx,UtopiaValue_DHCP_ServerPool_InsNum,ulInstanceNumber);
    if(strlen(pAlias))
        UTOPIA_SET(ctx,UtopiaValue_DHCP_ServerPool_Alias,pAlias);

    return SUCCESS;

}

int Utopia_GetDhcpV4SPool_NumOfStaticAddress(UtopiaContext *ctx,unsigned long ulPoolInstanceNumber)
{
    int count = 0;
    if(NULL == ctx){
        return count;
    }

#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    Utopia_GetDHCPServerStaticHostsCount(ctx,&count);

    return count;

}

int Utopia_GetDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber,unsigned long ulIndex, void *pSAddr)
{
    if((NULL == ctx) || (NULL == pSAddr)){
        return ERR_INVALID_ARGS;
    }

    errno_t rc = -1;
#ifdef _DEBUG_
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
  
   dhcpV4ServerPoolStaticAddress_t *pSAddr_t = (dhcpV4ServerPoolStaticAddress_t *)pSAddr; 
   if(0 != Utopia_GetIndexedInt(ctx,UtopiaValue_DHCP_StaticHost_InsNum,ulIndex + 1, (int *)&pSAddr_t->InstanceNumber)) {
       pSAddr_t->InstanceNumber = 0;
       rc = strcpy_s(pSAddr_t->Alias,sizeof(pSAddr_t->Alias), "");
       ERR_CHK(rc);
   } else {
        Utopia_GetIndexed(ctx,UtopiaValue_DHCP_StaticHost_Alias,ulIndex + 1,  (char *)&pSAddr_t->Alias, sizeof(pSAddr_t->Alias));
        if (pSAddr_t->InstanceNumber > MAX_NUM_INSTANCES) {
            rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: Error: pSAddr_t->InstanceNumber greater than MAX_NUM_INSTANCES(%d) !!!", __FUNCTION__, MAX_NUM_INSTANCES);
            if(rc < EOK)
            {
                ERR_CHK(rc);
            }
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_INVALID_ARGS;
        }
        g_IndexMapStaticAddr[pSAddr_t->InstanceNumber] = ulIndex;
   }
   
   Utopia_GetDhcpV4SPool_SAddressByIndex(ctx,ulIndex,pSAddr_t); 
    
    return SUCCESS;
}

int Utopia_GetDhcpV4SPool_SAddressByInsNum(UtopiaContext *ctx, unsigned long ulClientInstanceNumber, void *pSAddr)
{
    unsigned long ulIndex = 0;
    if((NULL == ctx) || (NULL == pSAddr)){
        return ERR_INVALID_ARGS;
    }

#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif
    
    dhcpV4ServerPoolStaticAddress_t *pSAddr_t = (dhcpV4ServerPoolStaticAddress_t *)pSAddr; 
    if(0 == pSAddr_t->InstanceNumber)
        return ERR_INVALID_ARGS;
    ulIndex = g_IndexMapStaticAddr[pSAddr_t->InstanceNumber];
    Utopia_GetDhcpV4SPool_SAddressByIndex(ctx,ulIndex,pSAddr_t); 
    return SUCCESS;
}

int is_ipv4_addr_invalid(unsigned int addr)
{
	if((addr==0)||(ntohl(addr)>=0xE0000000))
		return(1);
	return(0);
}

int is_mac_addr_invalid(unsigned char *pMac)
{
	unsigned char zeroMac[6] = {0,0,0,0,0,0};

	if(!memcmp(pMac,zeroMac,6) || (pMac[0] & 1))
		return(1);
	return(0);
}

extern int Utopia_GetDhcpV4SPool_SAddressByIndex(UtopiaContext *ctx, unsigned long ulIndex, dhcpV4ServerPoolStaticAddress_t *pSAddr_t);

int is_ipv4_addr_in_sa_list(UtopiaContext *ctx, unsigned int addr)
{
	int count=0, i;
	dhcpV4ServerPoolStaticAddress_t saAddr;

	Utopia_GetDHCPServerStaticHostsCount(ctx,&count);
	for(i=0;i<count;i++){
		saAddr.Yiaddr.Value = 0;
		Utopia_GetDhcpV4SPool_SAddressByIndex(ctx, i, &saAddr);
		if(saAddr.Yiaddr.Value == addr)
			return(1);
	}
	return(0);
}

int is_mac_addr_in_sa_list(UtopiaContext *ctx, unsigned char *pMac)
{
	int count=0, i;
	dhcpV4ServerPoolStaticAddress_t saAddr;

	Utopia_GetDHCPServerStaticHostsCount(ctx,&count);
	for(i=0;i<count;i++){
		Utopia_GetDhcpV4SPool_SAddressByIndex(ctx, i, &saAddr);
		if(!memcmp(pMac,saAddr.Chaddr,6))
			return(1);
	}
	return(0);
}

#ifdef SKY_RDKB
/*
 * RM15984: Check the given string is available in dnsmasq configuration file. (/nvram/dnsmasq.leases).
 * This is required to add the reserve static IP configuration. Before update reservation details
 * check the IP is already leased/provided from device.
 */
int is_addr_in_dnsmasq_lease_list (const char *addr, const unsigned char *pMac)
{
    FILE *fp = NULL;
    char buf[BUF_LEN] = {0};
    int ret = FALSE;
    char macStr[MACADDR_SZ] = {0};
    errno_t  rc = -1;

    if ((fp=fopen(DNSMASQ_LEASE_CONFIG_FILE, "r")) == NULL )
    {
        return ret;
    }

    while (fgets(buf, sizeof(buf), fp)!= NULL)
    {
        if(strstr(buf,addr))
        {
            ret = TRUE;
            /* This is required to reserve leased IP for the same device */
            rc = sprintf_s(macStr,sizeof(macStr),"%02x:%02x:%02x:%02x:%02x:%02x",
                pMac[0],pMac[1],pMac[2],pMac[3],pMac[4],pMac[5]);
            if(rc < EOK)
            {
                ERR_CHK(rc);
            }
            if(strstr(buf,macStr))
            {
                ret = FALSE;
            }
            break;
        }
    }

    fclose(fp);

    return ret;
}
#endif //END SKY_RDKB

int Utopia_AddDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, void *pSAddr)
{
    char strVal[STR_SZ] = {'\0'}; 
    char macAddress[32] = {'\0'}; 
    char tbuf[BUF_SZ] = {'\0'}; 
    /*int ip[4] = {0,0,0,0};*/
    unsigned long ulIndex = 0;
    int count = 0;
    struct in_addr addrVal;
    errno_t   rc = -1;
    if((NULL == ctx) || (NULL == pSAddr)){
        return ERR_INVALID_ARGS;
    }
#ifdef _DEBUG_
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    dhcpV4ServerPoolStaticAddress_t *pSAddr_t = (dhcpV4ServerPoolStaticAddress_t *)pSAddr; 
    if((0 == pSAddr_t->InstanceNumber)||is_mac_addr_invalid(pSAddr_t->Chaddr)||is_ipv4_addr_invalid(pSAddr_t->Yiaddr.Value))
        return ERR_INVALID_ARGS;
     addrVal.s_addr = pSAddr_t->Yiaddr.Value;
     rc = strcpy_s(strVal,sizeof(strVal),inet_ntoa(addrVal));
     ERR_CHK(rc);

#ifdef SKY_RDKB
     /* RM15984: "Add Device with Reserved IP" functionality is not working as expected.
     * In this case we can add already leased IP for a connected client as reserved IP for another device from GUI.
     * To resolve this we need to check the configured IP address is in dnsmasq lease table before update the
     * reservation configuration.*/
    if(is_ipv4_addr_in_sa_list(ctx, pSAddr_t->Yiaddr.Value) || is_mac_addr_in_sa_list(ctx,pSAddr_t->Chaddr)
            || is_addr_in_dnsmasq_lease_list(strVal,pSAddr_t->Chaddr))
           return(ERR_INVALID_ARGS);
#else

    if(is_ipv4_addr_in_sa_list(ctx, pSAddr_t->Yiaddr.Value) || is_mac_addr_in_sa_list(ctx,pSAddr_t->Chaddr))
	return(ERR_INVALID_ARGS);
#endif //END SKY_RDKB

    Utopia_GetDHCPServerStaticHostsCount(ctx,&count);
    ulIndex = count; 
    g_IndexMapStaticAddr[pSAddr_t->InstanceNumber] = ulIndex;

    /*sscanf(strVal,"%d.%d.%d.%d", ip,ip+1,ip+2,ip+3);*/
    /* Retrieve MAC address properly */
    rc = sprintf_s(macAddress, sizeof(macAddress),"%02x:%02x:%02x:%02x:%02x:%02x",pSAddr_t->Chaddr[0],pSAddr_t->Chaddr[1],pSAddr_t->Chaddr[2],pSAddr_t->Chaddr[3],pSAddr_t->Chaddr[4],pSAddr_t->Chaddr[5]);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    /*snprintf(tbuf, sizeof(tbuf), "%s%c%d%c%s", macAddress, DELIM_CHAR,
                                                   ip[3], DELIM_CHAR,
                                                   pSAddr_t->DeviceName);*/
    /* put whole IP into StaticHost token */
    rc = sprintf_s(tbuf, sizeof(tbuf), "%s%c%s%c%s", macAddress, DELIM_CHAR,
                                                   strVal, DELIM_CHAR,
                                                   pSAddr_t->DeviceName);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost, ulIndex + 1, tbuf);
    count = count + 1; /* New Static Address addition */
    UTOPIA_SETINT(ctx, UtopiaValue_DHCP_NumStaticHosts, count);

    Utopia_SetIndexedInt(ctx, UtopiaValue_DHCP_StaticHost_InsNum, ulIndex + 1,pSAddr_t->InstanceNumber);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost_Alias, ulIndex + 1, pSAddr_t->Alias);
    Utopia_set_lan_host_comments(ctx,pSAddr_t->Chaddr, pSAddr_t->comments);
    return SUCCESS;
}

int Utopia_DelDhcp4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, unsigned long ulInstanceNumber)
{
    int count = 0;
    unsigned long ulIndex = 0;
    dhcpV4ServerPoolStaticAddress_t sAddr;

    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }
#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if(0 == ulInstanceNumber)
        return ERR_INVALID_ARGS;
    ulIndex = g_IndexMapStaticAddr[ulInstanceNumber];

    Utopia_GetDHCPServerStaticHostsCount(ctx,&count);

    count = count - 1; /* InstanceNumber deletion */
    UTOPIA_SETINT(ctx, UtopiaValue_DHCP_NumStaticHosts, count);
    Utopia_UnsetIndexed(ctx, UtopiaValue_DHCP_StaticHost, ulIndex + 1);
    if(count != 0)
    {
       ulIndex++;
       for(;ulIndex <= count; ulIndex++)
       {
          Utopia_GetDhcpV4SPool_SAddressByIndex(ctx,ulIndex,&sAddr);
          Utopia_GetIndexedInt(ctx,UtopiaValue_DHCP_StaticHost_InsNum,ulIndex + 1, (int *)&sAddr.InstanceNumber);
          Utopia_GetIndexed(ctx,UtopiaValue_DHCP_StaticHost_Alias,ulIndex + 1,  (char *)&sAddr.Alias, sizeof(sAddr.Alias));

          g_IndexMapStaticAddr[sAddr.InstanceNumber] = ulIndex - 1;

          Utopia_SetDhcpV4SPool_SAddress(ctx,ulPoolInstanceNumber,&sAddr);
          Utopia_SetIndexedInt(ctx, UtopiaValue_DHCP_StaticHost_InsNum, ulIndex ,sAddr.InstanceNumber);
          UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost_Alias, ulIndex , sAddr.Alias);
        }
        Utopia_UnsetIndexed(ctx, UtopiaValue_DHCP_StaticHost,ulIndex);
    }
    return SUCCESS;
}

int Utopia_SetDhcpV4SPool_SAddress(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, void *pSAddr)
{
    unsigned long ulIndex = 0;
    char strVal[STR_SZ] = {'\0'}; 
    char macAddress[32] = {'\0'}; 
    char tbuf[BUF_SZ] = {'\0'}; 
    /*int ip[4] = {0,0,0,0};*/
    struct in_addr addrVal;
    errno_t  rc = -1;

    if(NULL == ctx){
        return ERR_INVALID_ARGS;
    }
#ifdef _DEBUG_
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    dhcpV4ServerPoolStaticAddress_t *pSAddr_t = (dhcpV4ServerPoolStaticAddress_t *)pSAddr;

    if(0 == pSAddr_t->InstanceNumber)
        return ERR_INVALID_ARGS;
    ulIndex = g_IndexMapStaticAddr[pSAddr_t->InstanceNumber];
    Utopia_UnsetIndexed(ctx, UtopiaValue_DHCP_StaticHost, ulIndex + 1);
    addrVal.s_addr = pSAddr_t->Yiaddr.Value;
    rc = strcpy_s(strVal,sizeof(strVal),inet_ntoa(addrVal));
    ERR_CHK(rc);
    /*sscanf(strVal,"%d.%d.%d.%d", ip,ip+1,ip+2,ip+3);*/
    /* Retrieve MAC address properly */
    rc = sprintf_s(macAddress,sizeof(macAddress),"%02x:%02x:%02x:%02x:%02x:%02x",pSAddr_t->Chaddr[0],pSAddr_t->Chaddr[1],pSAddr_t->Chaddr[2],pSAddr_t->Chaddr[3],pSAddr_t->Chaddr[4],pSAddr_t->Chaddr[5]);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    /*snprintf(tbuf, sizeof(tbuf), "%s%c%d%c%s", macAddress, DELIM_CHAR,
                                                   ip[3], DELIM_CHAR,
                                                   pSAddr_t->DeviceName);*/

    snprintf(tbuf, sizeof(tbuf), "%s%c%s%c%s", macAddress, DELIM_CHAR,
                                                   strVal, DELIM_CHAR,
                                                   pSAddr_t->DeviceName);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost, ulIndex + 1, tbuf);
    Utopia_set_lan_host_comments(ctx,pSAddr_t->Chaddr, pSAddr_t->comments);
    return SUCCESS;
}

int Utopia_SetDhcpV4SPool_SAddress_Values(UtopiaContext *ctx, unsigned long ulPoolInstanceNumber, unsigned long ulIndex, unsigned long ulInstanceNumber, char *pAlias)
{
    if((NULL == ctx) || (NULL == pAlias)){
        return ERR_INVALID_ARGS;
    }
#ifdef _DEBUG_
    errno_t  rc = -1;
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    Utopia_SetIndexedInt(ctx, UtopiaValue_DHCP_StaticHost_InsNum, ulIndex + 1,ulInstanceNumber);
    UTOPIA_SETINDEXED(ctx, UtopiaValue_DHCP_StaticHost_Alias, ulIndex + 1, pAlias);

    g_IndexMapStaticAddr[ulInstanceNumber] = ulIndex;
    
    return SUCCESS;
}

int Utopia_GetDhcpV4SPool_SAddressByIndex(UtopiaContext *ctx, unsigned long ulIndex, dhcpV4ServerPoolStaticAddress_t *pSAddr_t)
{
    char strVal[BUF_SZ] = {'\0'};
    /*char temp[STR_SZ] = {'\0'};*/
    /*char ipAddr[IPADDR_SZ] = {'\0'};*/
    char *p, *n ;
    errno_t  rc = -1;
    /*int ip[4] = {0,0,0,0};*/
    int mac[6] = {0,0,0,0,0,0};
    DHCPMap_t dhcp_static_hosts;
    memset(&dhcp_static_hosts, 0x0, sizeof(dhcp_static_hosts)); //CID 162783: Uninitialized scalar variable
#ifdef _DEBUG_
    rc = sprintf_s(ulog_msg, sizeof(ulog_msg), "%s: ********Entered ****** !!!", __FUNCTION__);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
#endif

    if (Utopia_GetIndexed(ctx, UtopiaValue_DHCP_StaticHost, ulIndex + 1, strVal, BUF_SZ)) {
        p = strVal;
        if (NULL != (n = chop_str(p, ','))) {
	    /*CID 162894 :BUFFER_SIZE_WARNING */
            strncpy(dhcp_static_hosts.macaddr, p, sizeof(dhcp_static_hosts.macaddr)-1);
	    dhcp_static_hosts.macaddr[sizeof(dhcp_static_hosts.macaddr)-1] = '\0';
        }
        p = n;
        if (NULL != (n = chop_str(p, ','))) {
            /*
            if (FALSE != IsInteger(p)) {
                dhcp_static_hosts.host_ip = atoi(p);
            }
            */
	    /* CID:135654 BUFFER_SIZE_WARNING */
            strncpy(dhcp_static_hosts.host_ip, p, sizeof(dhcp_static_hosts.host_ip)-1);
	    dhcp_static_hosts.host_ip[sizeof(dhcp_static_hosts.host_ip)-1] = '\0';
        }
        p = n;
        /*CID 69969 : Dereference after null check */
	if(p != NULL) {
	   /* CID:135654 BUFFER_SIZE_WARNING */
           strncpy(dhcp_static_hosts.client_name, p, sizeof(dhcp_static_hosts.client_name)-1);
	   dhcp_static_hosts.client_name[sizeof(dhcp_static_hosts.client_name)-1] = '\0';
        }

        /* Correct the Host IP */
        /*
        UTOPIA_GET(ctx, UtopiaValue_LAN_IPAddr, ipAddr, IPADDR_SZ);
        strcpy(temp, ipAddr);
        sscanf(temp,"%d.%d.%d.%d", ip,ip+1,ip+2,ip+3);

        sprintf(temp, "%d.%d.%d.%d", ip[0],ip[1],ip[2],dhcp_static_hosts.host_ip);
        */
	pSAddr_t->ActiveFlag = TRUE;
        pSAddr_t->bEnabled = TRUE;

        sscanf(dhcp_static_hosts.macaddr,"%x:%x:%x:%x:%x:%x",mac,mac+1,mac+2,mac+3,mac+4,mac+5);
        pSAddr_t->Chaddr[0] = mac[0];
        pSAddr_t->Chaddr[1] = mac[1];
        pSAddr_t->Chaddr[2] = mac[2];
        pSAddr_t->Chaddr[3] = mac[3];
        pSAddr_t->Chaddr[4] = mac[4];
        pSAddr_t->Chaddr[5] = mac[5];

        pSAddr_t->Yiaddr.Value = inet_addr(dhcp_static_hosts.host_ip);
	if(dhcp_static_hosts.client_name != NULL)
    {
        rc = strcpy_s(pSAddr_t->DeviceName,sizeof(pSAddr_t->DeviceName),dhcp_static_hosts.client_name);
        ERR_CHK(rc);
    }
	Utopia_get_lan_host_comments(ctx,pSAddr_t->Chaddr, pSAddr_t->comments);
        return SUCCESS;
    }
    return ERR_INVALID_ARGS;

}
