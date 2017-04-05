#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"
#include "lan_handler.h"
#include "util.h"

#define THIS			"/usr/bin/service_dhcp"
#define LAN_IF_NAME     "brlan0"
#define XHS_IF_NAME     "brlan1"

extern int g_iSyseventfd;
extern token_t g_tSysevent_token;

extern void executeCmd(char *);

void bring_lan_up()
{
	char l_cAsyncId[16] = {0}, l_cPsm_Parameter[255] = {0};
	char l_cPrimaryLan_L3Net[8] = {0}, l_cL2Inst[8] = {0}, l_cLan_Brport[8] = {0};
	char l_cHomeSecurity_L3net[8] = {0}, l_cEvent_Name[32] = {0};
	char *l_cpPsm_Get = NULL;
	int l_iRet_Val;
	async_id_t l_sAsyncID;
	char *l_cParam[1] = {0};

	sysevent_get(g_iSyseventfd, g_tSysevent_token, "lan_handler_async", 
				 l_cAsyncId, sizeof(l_cAsyncId));

	fprintf(stderr, "Inside bring_lan_up \n");
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
	char l_cLan_IpAddrv6_prev[64] = {0}, l_cLan_PrefixV6[8] = {0}, l_cLan_IpAddrv6[16] = {0};
	char l_cCur_Ipv4_Addr[16] = {0}, l_cLan_Status[16] = {0}, l_cBrlan0_Mac[32] = {0};
	char l_cLast_Erouter_Mode[8] = {0}, l_cFileName[255] = {0};
	char l_cDsLite_Enabled[8] = {0}, l_cDhcp_Server_Prog[16] = {0};
	char l_cIpv6_Prefix[64] = {0}, l_cLan_Uptime[16] = {0};

	int l_iRes;
	struct sysinfo l_sSysInfo;
	FILE *l_fFp = NULL;	

	fprintf(stderr, "Inside ipv4_status L3Instance:%d L3Status:%s\n", l3_inst, status);
	if (!strncmp(status, "up", 2))
	{	
    	syscfg_get(NULL, "last_erouter_mode", l_cLast_Erouter_Mode, sizeof(l_cLast_Erouter_Mode));
	    fprintf(stderr, "last erouter mode is:%s\n", l_cLast_Erouter_Mode);

		sprintf(l_cSysevent_Cmd, "ipv4_%d-ifname", l3_inst);	
		sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cLanIfName, sizeof(l_cLanIfName));

        // if it's ipv4 only, not enable link local 
        fprintf(stderr, "lan_handler.sh last_erouter_mode:%s\n", l_cLast_Erouter_Mode);
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
    	    	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
        	    	     "ip -6 addr del %s/64 dev %s valid_lft forever preferred_lft forever", 
						 l_cLan_IpAddrv6_prev, l_cLanIfName);

		        executeCmd(l_cSysevent_Cmd);
    	    
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
		fprintf(stderr, "start-misc is:%s\n", l_cStart_Misc);

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
            system("execute_dir /etc/utopia/post.d/");
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
					fprintf(stderr, "Successful in getting %s MAC address:%s\n", LAN_IF_NAME, l_cBrlan0_Mac);
				}
				else
				{
					fprintf(stderr, "Un-Successful in getting %s MAC address\n", LAN_IF_NAME);
				}
            }

			if (is_iface_present(XHS_IF_NAME))
			{
				fprintf(stderr, "%s interface is present call gw_lan_refresh\n", XHS_IF_NAME);
                fprintf(stderr, "LAN HANDLER : Refreshing LAN from handler\n");
                system("gw_lan_refresh&");				
			}
            system("firewall");
            system("execute_dir /etc/utopia/post.d/ restart");
		}
        else
		{
			fprintf(stderr, "LAN HANDLER : Triggering DHCP server using LAN status\n");
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "started", 0);
			fprintf(stderr, "LAN HANDLER : Triggering RDKB_FIREWALL_RESTART\n");
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "firewall-restart", "", 0);
        }
        system("firewall_nfq_handler.sh &");
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
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "lan-status", "stopped", 0);
		}
    }
    fprintf(stderr, "LAN HANDLER : Triggering RDKB_FIREWALL_RESTART after nfqhandler\n");
	sysevent_set(g_iSyseventfd, g_tSysevent_token, "firewall-restart", "", 0);
}
