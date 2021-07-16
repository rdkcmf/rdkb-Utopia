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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "lan_handler.h"
#include "util.h"
#include <sys/time.h>
#include "print_uptime.h"
#include <telemetry_busmessage_sender.h>
#ifdef FEATURE_SUPPORT_ONBOARD_LOGGING
#include <rdk_debug.h>
#define LOGGING_MODULE "Utopia"
#define OnboardLog(...)     rdk_log_onboard(LOGGING_MODULE, __VA_ARGS__)
#else
#define OnboardLog(...)
#endif
#define THIS			"/usr/bin/service_dhcp"
#define LAN_IF_NAME     "brlan0"
#define XHS_IF_NAME     "brlan1"

#define IPV4_NV_PREFIX  "dmsb.l3net"
#define IPV4_NV_IP      "V4Addr"
#define IPV4_NV_SUBNET  "V4SubnetMask"

#define POSTD_START_FILE "/tmp/.postd_started"

extern int g_iSyseventfd;
extern token_t g_tSysevent_token;

extern void executeCmd(char *);

void get_dateanduptime(char *buffer, int *uptime)
{
    struct 	timeval  tv;
    struct 	tm       *tm;
    struct 	sysinfo info;
    char 	fmt[ 64 ], buf [64];

    sysinfo( &info );
    gettimeofday( &tv, NULL );
    
    if( (tm = localtime( &tv.tv_sec ) ) != NULL)
    {
	strftime( fmt, sizeof( fmt ), "%y%m%d-%T.%%06u", tm );
	snprintf( buf, sizeof( buf ), fmt, tv.tv_usec );
    }
    
    sprintf( buffer, "%s", buf);
    *uptime= info.uptime;
}

void bring_lan_up()
{
	char l_cAsyncId[16] = {0}, l_cPsm_Parameter[255] = {0};
	char l_cPrimaryLan_L3Net[8] = {0}, l_cL2Inst[8] = {0}, l_cLan_Brport[8] = {0};
	char l_cHomeSecurity_L3net[8] = {0}, l_cEvent_Name[32] = {0};
	char *l_cpPsm_Get = NULL;
	int l_iRet_Val;
	async_id_t l_sAsyncID;
	char *l_cParam[1] = {0};
	int uptime = 0;
	char buffer[64]= { 0 };
	get_dateanduptime(buffer,&uptime);
	print_uptime("Lan_init_start", NULL, NULL);
    OnboardLog("Lan_init_start:%d\n",uptime);
	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_handler_async", 
				 l_cAsyncId, sizeof(l_cAsyncId));

	if (0 == l_cAsyncId[0])
	{
		//L3 Instance
		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.MultiLAN.PrimaryLAN_l3net");
	    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    	if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
	    {    
    	    fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L3INST returned null, retrying\n");
	        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    	    if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        	{
	            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L3INST returned null even after retry, no more retries\n");
    	    }
        	else
	        {
    	        strncpy(l_cPrimaryLan_L3Net, l_cpPsm_Get, sizeof(l_cPrimaryLan_L3Net));
        	    fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L3INST is:%s\n", l_cPrimaryLan_L3Net);
	        }
    	}    
	    else 
    	{    
	        strncpy(l_cPrimaryLan_L3Net, l_cpPsm_Get, sizeof(l_cPrimaryLan_L3Net));
    	    fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L3INST is:%s\n", l_cPrimaryLan_L3Net);
		}

		// L2 Instance
		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.MultiLAN.PrimaryLAN_l2net");
		l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L2INST returned null, retrying\n");
            l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
            if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
            {
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L2INST returned null even after retry, no more retries\n");
            }
            else
            {
                strncpy(l_cL2Inst, l_cpPsm_Get, sizeof(l_cL2Inst));
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L2INST is:%s\n", l_cL2Inst);
            }
        }
        else
        {
            strncpy(l_cL2Inst, l_cpPsm_Get, sizeof(l_cL2Inst));
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : L2INST is:%s\n", l_cL2Inst);
        }		

		// BRPORT
		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.MultiLAN.PrimaryLAN_brport");
		l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : BRPORT returned null, retrying\n");
            l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
            if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
            {
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : BRPORT returned null even after retry, no more retries\n");
            } 
            else
            {
                strncpy(l_cLan_Brport, l_cpPsm_Get, sizeof(l_cLan_Brport));
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : BRPORT is:%s\n", l_cLan_Brport);
            }
        }
        else
        {
            strncpy(l_cLan_Brport, l_cpPsm_Get, sizeof(l_cLan_Brport));
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : BRPORT is:%s\n", l_cLan_Brport);
        }

		//HSINST
		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.MultiLAN.HomeSecurity_l3net");
        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : HSINST returned null, retrying\n");
            l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
            if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
            {
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : HSINST returned null even after retry, no more retries\n");
            } 
            else
            {
                strncpy(l_cHomeSecurity_L3net, l_cpPsm_Get, sizeof(l_cHomeSecurity_L3net));
                fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : HSINST is:%s\n", l_cHomeSecurity_L3net);
            }
        }
        else
        {
            strncpy(l_cHomeSecurity_L3net, l_cpPsm_Get, sizeof(l_cHomeSecurity_L3net));
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : HSINST is:%s\n", l_cHomeSecurity_L3net);
        }
		

		if (0 != l_cPrimaryLan_L3Net[0])
		{
			snprintf(l_cEvent_Name, sizeof(l_cEvent_Name), "ipv4_%s-status", l_cPrimaryLan_L3Net);
			sysevent_setcallback(g_iSyseventfd, g_tSysevent_token, ACTION_FLAG_NONE,
                             	 l_cEvent_Name, THIS, 1, l_cParam, &l_sAsyncID);

			snprintf(l_cAsyncId, sizeof(l_cAsyncId), "%d %d", l_sAsyncID.action_id, l_sAsyncID.trigger_id);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan_handler_async", l_cAsyncId, 0);

			sysevent_set(g_iSyseventfd, g_tSysevent_token, "primary_lan_l3net", l_cPrimaryLan_L3Net, 0);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "primary_lan_l2net", l_cL2Inst, 0);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "primary_lan_brport", l_cLan_Brport, 0);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "homesecurity_lan_l3net", l_cHomeSecurity_L3net, 0);
		}
	}
	else
	{
		fprintf(stderr, "lan_handler_async is not empty returning from bring_lan_up\n");
	}
}

void ipv4_status(int l3_inst, char *status)
{
	char l_cSysevent_Cmd[255] = {0}, l_cLanIfName[16] = {0};
	char l_cLan_IpAddrv6_prev[64] = {0}, l_cLan_PrefixV6[8] = {0}; 
	char l_cLan_IpAddrv6[64] = {0};
	char l_cCur_Ipv4_Addr[16] = {0}, l_cLan_Status[16] = {0};
	char l_cBrlan0_Mac[32] = {0};
	char l_cLast_Erouter_Mode[8] = {0}, l_cFileName[255] = {0};
	char l_cDsLite_Enabled[8] = {0}, l_cDhcp_Server_Prog[16] = {0};
	char l_cIpv6_Prefix[64] = {0}, l_cLan_Uptime[16] = {0};
	int l_iRes;
	struct sysinfo l_sSysInfo;
	FILE *l_fFp = NULL;	
	int uptime = 0;
	char buffer[64] = { 0 };
	if (!strncmp(status, "up", 2))
	{	
    	syscfg_get(NULL, "last_erouter_mode", l_cLast_Erouter_Mode, sizeof(l_cLast_Erouter_Mode));

		sprintf(l_cSysevent_Cmd, "ipv4_%d-ifname", l3_inst);	
		sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cLanIfName, sizeof(l_cLanIfName));

        // if it's ipv4 only, not enable link local 
		if (!strncmp(l_cLast_Erouter_Mode, "1", 1))
		{
			//Do not do SLAAC
			snprintf(l_cFileName, sizeof(l_cFileName),
            		 "/proc/sys/net/ipv6/conf/%s/autoconf", l_cLanIfName);

			l_fFp = fopen(l_cFileName, "w");
			if (NULL != l_fFp)
    		{
        		fprintf(l_fFp, "%d", 0);
        		fclose(l_fFp);
    		}
    		else
    		{
        		fprintf(stderr, "Error while opening %s\n", l_cFileName);
    		}	
		}
		else
		{
			snprintf(l_cFileName, sizeof(l_cFileName),
                     "/proc/sys/net/ipv6/conf/%s/autoconf", l_cLanIfName);

			l_fFp = fopen(l_cFileName, "w");
            if (NULL != l_fFp)
            {
                fprintf(l_fFp, "%d", 1);
                fclose(l_fFp);
            }
            else
            {
                fprintf(stderr, "Error while opening %s\n", l_cFileName);
            }

			snprintf(l_cFileName, sizeof(l_cFileName),
                     "/proc/sys/net/ipv6/conf/%s/disable_ipv6", l_cLanIfName);

            //First write 1 to disable_ipv6 and then overwrite it with 0
            l_fFp = fopen(l_cFileName, "w");
            if (NULL != l_fFp)
            {    
                fprintf(l_fFp, "%d", 1);
                fclose(l_fFp);
            }    
            else 
            {    
                fprintf(stderr, "Error while opening %s\n", l_cFileName);
            }    

            //overwriting it with 0
            l_fFp = fopen(l_cFileName, "w");
            if (NULL != l_fFp)
            {
                fprintf(l_fFp, "%d", 0);
                fclose(l_fFp);
            }
            else
            {
                fprintf(stderr, "Error while opening %s\n", l_cFileName);
            }

			snprintf(l_cFileName, sizeof(l_cFileName),
                     "/proc/sys/net/ipv6/conf/%s/forwarding", l_cLanIfName);

			l_fFp = fopen(l_cFileName, "w");
            if (NULL != l_fFp)
            {
                fprintf(l_fFp, "%d", 1);
                fclose(l_fFp);
            }
            else
            {
                fprintf(stderr, "Error while opening %s\n", l_cFileName);
            }
		}

		if (!strncmp(l_cLanIfName, LAN_IF_NAME, 6))
		{
	    	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_ipaddr_v6_prev", 
						 l_cLan_IpAddrv6_prev, sizeof(l_cLan_IpAddrv6_prev));

	    	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_ipaddr_v6", 
						 l_cLan_IpAddrv6, sizeof(l_cLan_IpAddrv6));
	
    		sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_prefix_v6", 
						 l_cLan_PrefixV6, sizeof(l_cLan_PrefixV6));
			if ((strncmp(l_cLan_IpAddrv6_prev, l_cLan_IpAddrv6, 64)) && (0 != l_cLan_IpAddrv6[0]))
			{
                           if (l_cLan_IpAddrv6_prev != NULL)
                           {  
    	    	             snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
        	    	      "ip -6 addr del %s/64 dev %s valid_lft forever preferred_lft forever", 
						 l_cLan_IpAddrv6_prev, l_cLanIfName);

		             executeCmd(l_cSysevent_Cmd);
			   }
    	    
				snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
            		     "ip -6 addr add %s/64 dev %s valid_lft forever preferred_lft forever", 
						 l_cLan_IpAddrv6, l_cLanIfName);
				executeCmd(l_cSysevent_Cmd);

			}
		}
        //sysevent set current_lan_ipaddr `sysevent get ipv4_${INST}-ipv4addr`
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);    
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
					 l_cCur_Ipv4_Addr, sizeof(l_cCur_Ipv4_Addr));    

    	sysevent_set(g_iSyseventfd, g_tSysevent_token, "current_lan_ipaddr", l_cCur_Ipv4_Addr, 0);

		char l_cStart_Misc[16] = {0};
		sysevent_get(g_iSyseventfd, g_tSysevent_token, "start-misc", l_cStart_Misc, sizeof(l_cStart_Misc));

		char l_cCurrentWan_IpAddr[16] = {0};
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, "current_wan_ipaddr", 
					 l_cCurrentWan_IpAddr, sizeof(l_cCurrentWan_IpAddr));    
		
		char l_cParcon_Nfq_Status[16] = {0};
		sysevent_get(g_iSyseventfd, g_tSysevent_token, "parcon_nfq_status",
                     l_cParcon_Nfq_Status, sizeof(l_cParcon_Nfq_Status));

		if ((!strncmp(l_cLast_Erouter_Mode, "2", 1)) && (strncmp(l_cStart_Misc, "ready", 5))) 
		{
			fprintf(stderr, "LAN HANDLER : Triggering DHCP server using LAN status based on RG_MODE:2");
    		sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "started", 0);
            system("firewall");

            if (access(POSTD_START_FILE, F_OK) != 0)
            {
                    char postd_cmd[128] = {0};
                    snprintf(postd_cmd,sizeof(postd_cmd),"touch %s ; execute_dir /etc/utopia/post.d/",POSTD_START_FILE);
                    fprintf(stderr, "[%s] Restarting post.d from ipv4_status\n", __FUNCTION__);
                    system(postd_cmd);
            }		
        }
		else if ((strncmp(l_cStart_Misc, "ready", 5)) && 
				 (0 != l_cCurrentWan_IpAddr[0]) &&
				 (strncmp(l_cCurrentWan_IpAddr, "0.0.0.0", 7)))
		{
			fprintf(stderr, "LAN HANDLER : Triggering DHCP server using LAN status based on start misc\n");
    		sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "started", 0);

			if (strncmp(l_cParcon_Nfq_Status, "started", 7))
			{
				l_iRes = iface_get_hwaddr(LAN_IF_NAME, l_cBrlan0_Mac, sizeof(l_cBrlan0_Mac));
				if (0 == l_iRes)
				{
					fprintf(stderr, "Successful in getting %s MAC address:%s\n", 
									LAN_IF_NAME, l_cBrlan0_Mac);
				}
				else
				{
					fprintf(stderr, "Un-Successful in getting %s MAC address\n", 
									LAN_IF_NAME);
				}
            }

			if (is_iface_present(XHS_IF_NAME))
			{
				fprintf(stderr, "%s interface is present call gw_lan_refresh\n", XHS_IF_NAME);
                fprintf(stderr, "LAN HANDLER : Refreshing LAN from handler\n");
                system("gw_lan_refresh&");				
			}
                system("firewall");

                if (access(POSTD_START_FILE, F_OK) != 0)
                {
                        char postd_cmd[128] = {0};
                        snprintf(postd_cmd,sizeof(postd_cmd),"touch %s ; execute_dir /etc/utopia/post.d/",POSTD_START_FILE);
                        fprintf(stderr, "[%s] Restarting post.d from ipv4_status\n", __FUNCTION__);
                        system(postd_cmd);
                }   
            }
        else
		{
			fprintf(stderr, "LAN HANDLER : Triggering DHCP server using LAN status\n");
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "started", 0);
			fprintf(stderr, "LAN HANDLER : Triggering RDKB_FIREWALL_RESTART\n");
                        t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "firewall-restart", "", 0);
			get_dateanduptime(buffer,&uptime);
			OnboardLog("RDKB_FIREWALL_RESTART:%d\n",uptime);
        }
//        system("firewall_nfq_handler.sh &");
    	sysinfo(&l_sSysInfo);
		snprintf(l_cLan_Uptime, sizeof(l_cLan_Uptime), "%ld", l_sSysInfo.uptime);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan_start_time", l_cLan_Uptime, 0);	
          
		if (4 == l3_inst)
		{  
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "ipv4_4_status_configured", "1", 0);
		}


        // disable dnsmasq when ipv6 only mode and DSlite is disabled
		sysevent_get(g_iSyseventfd, g_tSysevent_token, "dslite_enabled",
                     l_cDsLite_Enabled, sizeof(l_cDsLite_Enabled));

		sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcp_server-progress",
                     l_cDhcp_Server_Prog, sizeof(l_cDhcp_Server_Prog));

		sysevent_get(g_iSyseventfd, g_tSysevent_token, "ipv6_prefix",
                     l_cIpv6_Prefix, sizeof(l_cIpv6_Prefix));
		
		fprintf(stderr, "LAN HANDLER : DHCP configuration status got is:%s\n", l_cDhcp_Server_Prog);
		if (!strncmp(l_cLast_Erouter_Mode, "2", 1) && (strncmp(l_cDsLite_Enabled, "1", 1)))
		{
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-stop", "", 0);
		}
		else if ((strncmp(l_cLast_Erouter_Mode, "0", 1)) && 
				 (strncmp(l_cDhcp_Server_Prog, "inprogress", 10)))
		{	
			fprintf(stderr, "LAN HANDLER : Triggering dhcp start based on last erouter mode\n");
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcp_server-start", "", 0);
		}

		if (0 != l_cIpv6_Prefix[0])	
		{
    	    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
        	         "ip -6 route add %s dev %s", 
					 l_cIpv6_Prefix, l_cLanIfName);

	        executeCmd(l_cSysevent_Cmd);
		}
	}	
    else
	{
		sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan-status",
                     l_cLan_Status, sizeof(l_cLan_Status));

		if (!strncmp(l_cLan_Status, "started", 7))
		{
			char bridge_mode[16] = {0};
			static int isBridgeMode;
			sysevent_get(g_iSyseventfd, g_tSysevent_token, "bridge_mode", bridge_mode, sizeof(bridge_mode));
			isBridgeMode        = (0 == strcmp("0", bridge_mode)) ? 0 : 1;
			if(!isBridgeMode)
			{
			    fprintf(stderr, "LAN HANDLER : Device in Router mode and lan-status: stopped\n");
			}
			else
			{
			    fprintf(stderr, "LAN HANDLER : Device in Bridge mode and lan-status: stopped\n");
			}
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "stopped", 0);
		}
    }
    fprintf(stderr, "LAN HANDLER : Triggering RDKB_FIREWALL_RESTART after nfqhandler\n");
    t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "firewall-restart", "", 0);
    get_dateanduptime(buffer,&uptime);
    OnboardLog("RDKB_FIREWALL_RESTART:%d\n",uptime);
    print_uptime("Laninit_complete", NULL, NULL);
    OnboardLog("Lan_init_complete:%d\n", uptime);
    t2_event_d("btime_laninit_split", uptime);
}

void lan_restart()
{
	char l_cLanIpAddr[16] = {0}, l_cLanNetMask[16] = {0};
	char l_cPsmGetLanIp[16] = {0}, l_cPsmGetLanSubNet[16] = {0};
	char l_cLanInst[8] = {0}, l_cLanRestarted[8] = {0};
	char l_cSysevent_Cmd[255] = {0}, l_cLanIfName[16] = {0};
	char l_cLan_IpAddrv6_prev[64] = {0}, l_cLan_PrefixV6[32] = {0};
	char l_cLan_IpAddrv6[64] = {0}, l_cPsm_Parameter[255] = {0};
	char *l_cpPsm_Get = NULL;
	
	int l_iLanInst, l_iRetVal;

	syscfg_get(NULL, "lan_ipaddr", l_cLanIpAddr, sizeof(l_cLanIpAddr));

	syscfg_get(NULL, "lan_netmask", l_cLanNetMask, sizeof(l_cLanNetMask));

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "primary_lan_l3net", 
				 l_cLanInst, sizeof(l_cLanInst));

	l_iLanInst = atoi(l_cLanInst);

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
			"%s.%s.%s", IPV4_NV_PREFIX, l_cLanInst, IPV4_NV_IP);

    l_iRetVal = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
   	if (CCSP_SUCCESS == l_iRetVal || l_cpPsm_Get != NULL)
	{
	    /*CID 135444 : BUFFER_SIZE_WARNING */
            strncpy(l_cPsmGetLanIp, l_cpPsm_Get, sizeof(l_cPsmGetLanIp)-1);
	    l_cPsmGetLanIp[sizeof(l_cPsmGetLanIp)-1] = '\0';
	}
	else
	{
		fprintf(stderr, "Error:%d while getting:%s or value is empty\n", 
                l_iRetVal, l_cPsm_Parameter);
	}

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
			"%s.%s.%s", IPV4_NV_PREFIX, l_cLanInst, IPV4_NV_SUBNET);

    l_iRetVal = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
   	if (CCSP_SUCCESS == l_iRetVal || l_cpPsm_Get != NULL)
	{
        /* CID 163349 : BUFFER_SIZE */
        strncpy(l_cPsmGetLanSubNet, l_cpPsm_Get, sizeof(l_cPsmGetLanSubNet)-1);
	l_cPsmGetLanSubNet[sizeof(l_cPsmGetLanSubNet)-1] = '\0';
	}
	else
	{
		fprintf(stderr, "Error:%d while getting:%s or value is empty\n", 
                l_iRetVal, l_cPsm_Parameter);
	}

	if ((strncmp(l_cLanIpAddr, l_cPsmGetLanIp, sizeof(l_cLanIpAddr))) ||
		(strncmp(l_cLanNetMask, l_cPsmGetLanSubNet, sizeof(l_cLanNetMask))))
	{
		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
				"%s.%s.%s", IPV4_NV_PREFIX, l_cLanInst, IPV4_NV_IP);

    	l_iRetVal = PSM_VALUE_SET_STRING(l_cPsm_Parameter, l_cLanIpAddr);
   		if (CCSP_SUCCESS == l_iRetVal)
		{
   	    	fprintf(stderr, "Successful in setting:%s\n", 
					l_cPsm_Parameter);
		}
		else
		{
			fprintf(stderr, "Error:%d while Setting:%s\n", 
            	    l_iRetVal, l_cPsm_Parameter);
		}

		snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
				"%s.%s.%s", IPV4_NV_PREFIX, l_cLanInst, IPV4_NV_SUBNET);

    	l_iRetVal = PSM_VALUE_SET_STRING(l_cPsm_Parameter, l_cLanNetMask);
   		if (CCSP_SUCCESS == l_iRetVal)
		{
   	    	fprintf(stderr, "Successful in setting:%s\n", 
					l_cPsm_Parameter);
		}
		else
		{
			fprintf(stderr, "Error:%d while Setting:%s\n", 
            	    l_iRetVal, l_cPsm_Parameter);
		}

        // TODO check for lan network being up ?
		sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 "ipv4-resync", l_cLanInst, 0);
	}
	//handle ipv6 address on brlan0. 
	//Because it's difficult to add ipv6 operation in ipv4 process. 
	//So just put here as a temporary method
	sprintf(l_cSysevent_Cmd, "ipv4_%d-ifname", l_iLanInst);	
	sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cLanIfName, sizeof(l_cLanIfName));

    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_ipaddr_v6_prev", 
                 l_cLan_IpAddrv6_prev, sizeof(l_cLan_IpAddrv6_prev));

    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_ipaddr_v6", 
                 l_cLan_IpAddrv6, sizeof(l_cLan_IpAddrv6));
    
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_prefix_v6", 
                 l_cLan_PrefixV6, sizeof(l_cLan_PrefixV6));

    sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_restarted", 
                 l_cLanRestarted, sizeof(l_cLanRestarted));

    if ((strncmp(l_cLan_IpAddrv6_prev, l_cLan_IpAddrv6, 64)) && 
		(0 != l_cLan_IpAddrv6[0]))
    {
               if ((l_cLan_IpAddrv6_prev != NULL) && (0 != l_cLan_IpAddrv6_prev[0]))
	       {
        	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
            	     "ip -6 addr del %s/64 dev %s valid_lft forever preferred_lft forever", 
                	 l_cLan_IpAddrv6_prev, l_cLanIfName);

	        executeCmd(l_cSysevent_Cmd);
	       }
   
        snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip -6 addr add %s/64 dev %s valid_lft forever preferred_lft forever", 
                 l_cLan_IpAddrv6, l_cLanIfName);

        executeCmd(l_cSysevent_Cmd);
	
    }
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan_restarted", "done", 0);
}
