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
#include "print_uptime.h"
#include <telemetry_busmessage_sender.h>

#define THIS            "/usr/bin/service_dhcp"
#define LAN_IF_NAME     "brlan0"
#define XHS_IF_NAME     "brlan1"

#define IPV4_TSIP_PREFIX    "dmsb.truestaticip"
#define IPV4_TSIP_ASNPREFIX "dmsb.truestaticip.Asn"
#define IPV4_TSIP_ENABLE    "Enable"
#define IPV4_TSIP_IP        "Ipaddress"
#define IPV4_TSIP_SUBNET    "Subnetmask"
#define IPV4_TSIP_GATEWAY   "Gateway"

#define MAX_TS_ASN_COUNT    64

extern int g_iSyseventfd;
extern token_t g_tSysevent_token;

extern void executeCmd(char *);
extern unsigned int mask2cidr(char *subnetMask);
extern unsigned int countSetBits(int byte);
extern void subnet(char *ipv4Addr, 
                   char *ipv4Subnet, char *subnet);
extern void get_device_props();

//=======================
//service_ipv4.sh conversion
//======================
void remove_config(int l3_inst)
{
    	fprintf(stderr, "ipv4 remove_config\n");

	char l_cCur_Ipv4_Addr[16] = {0}, l_cCur_Ipv4_Subnet[16] = {0};
	char l_cIfName[16] = {0}, l_cSysevent_Cmd[255] = {0}, l_cSubnet[16] = {0};
	int l_iRT_Table, l_iCIDR;	
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);    
    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cCur_Ipv4_Addr, sizeof(l_cCur_Ipv4_Addr));    
    
    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4subnet", l3_inst);   
    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cCur_Ipv4_Subnet, sizeof(l_cCur_Ipv4_Subnet));

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ifname", l3_inst);
	sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cIfName, sizeof(l_cIfName));

	l_iRT_Table = l3_inst + 10;
	l_iCIDR = mask2cidr(l_cCur_Ipv4_Subnet);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip addr del %s/%d dev %s", l_cCur_Ipv4_Addr, l_iCIDR, l_cIfName);
    
	executeCmd(l_cSysevent_Cmd);	
 
	// TODO: Fix this static workaround. Should have configurable routing policy.
	subnet(l_cCur_Ipv4_Addr, l_cCur_Ipv4_Subnet, l_cSubnet);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip rule del from %s lookup %d", l_cCur_Ipv4_Addr, l_iRT_Table);
	executeCmd(l_cSysevent_Cmd);	

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip rule del iif %s lookup erouter", l_cIfName);
	executeCmd(l_cSysevent_Cmd);	
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip rule del iif %s lookup %d", l_cIfName, l_iRT_Table);
	executeCmd(l_cSysevent_Cmd);	

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip route del table %d %s/%d dev %s", l_iRT_Table, l_cSubnet, l_iCIDR, l_cIfName);
	executeCmd(l_cSysevent_Cmd);	
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
			 "ip route del table all_lans %s/%d dev %s", l_cSubnet, l_iCIDR, l_cIfName);
	executeCmd(l_cSysevent_Cmd);	

    // del 161/162 port from brlan0 interface when it is teardown
	/*if (!strncmp(l_cIfName, LAN_IF_NAME, 6))
	{
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "snmppa_socket_entry", "delete", 0);
    }*/

    //END ROUTING TODO
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);    
	sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "", 0);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4subnet", l3_inst);    
	sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "", 0);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4_static", l3_inst);    
	sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "", 0);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);    
	sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "down", 0);

    fprintf(stderr, "ipv4 remove_config complete\n");

}

void teardown_instance(int l3_inst)
{
    fprintf(stderr, "ipv4 teardown instance, instance is %d\n",l3_inst);

	char l_cAsyncIDString[16] = {0}, l_cIpv4_Instances[8] = {0};
	char l_cLower[8] = {0}, l_cActiveInstances[8] = {0};
	char l_cSysevent_Cmd[255] = {0};
	char *l_cToken = NULL;
	async_id_t l_sAsyncID;

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-lower", l3_inst);
    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cLower, sizeof(l_cLower));


	if ( '\0' != l_cLower[0] )	
	{
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-l2async", l3_inst);
		sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
					 l_cAsyncIDString, sizeof(l_cAsyncIDString));

    	sscanf(l_cAsyncIDString, "%d %d", &l_sAsyncID.trigger_id, &l_sAsyncID.action_id);

    	sysevent_rmcallback(g_iSyseventfd, g_tSysevent_token, l_sAsyncID);

		remove_config(l3_inst);
		
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-l2async", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "", 0);

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-lower", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "", 0);

		sysevent_get(g_iSyseventfd, g_tSysevent_token, "ipv4-instances", 
					 l_cIpv4_Instances, sizeof(l_cIpv4_Instances));

		l_cToken = strtok(l_cIpv4_Instances, " ");
		while (l_cToken != NULL)
		{
			if (l3_inst != atoi(l_cToken))
			{
                		strncat(l_cActiveInstances,l_cToken,sizeof(l_cActiveInstances) - strlen(l_cActiveInstances) - 1);
			}
            		l_cToken = strtok(NULL, " ");
	
		}
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "ipv4-instances", l_cActiveInstances, 0);	
    }	
}

void sync_tsip () 
{
	char l_cNv_Tsip_Enable[8] = {0}, l_cNvTsip_IpAddr[16] = {0};
	char l_cNvTsip_IpSubnet[16] = {0}, l_cNvTsip_Gateway[16] = {0};
	char l_cSubnet[16] = {0}, l_cSysevent_Cmd[255] = {0}, l_cPsm_Parameter[255] = {0};
    char *l_cpPsm_Get = NULL;
		
	int l_iNv_Tsip_Enable = 0, l_iCIDR, l_iRet_Val = 0;
	l_iNv_Tsip_Enable = atoi(l_cNv_Tsip_Enable);

	//psm get dmsb.truestaticip.Enable
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "%s.%s", IPV4_TSIP_PREFIX, IPV4_TSIP_ENABLE);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
	if (CCSP_SUCCESS == l_iRet_Val)
    {
		if (l_cpPsm_Get != NULL)
		{
			strncpy(l_cNv_Tsip_Enable, l_cpPsm_Get, sizeof(l_cNv_Tsip_Enable));
        	Ansc_FreeMemory_Callback(l_cpPsm_Get);
	        l_cpPsm_Get = NULL;
		}
		else
		{
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
		}
    }
    else
    {
        fprintf(stderr, "Error:%d while getting parameter:%s\n", 
				l_iRet_Val, l_cPsm_Parameter);
    }

	//psm get dmsb.truestaticip.Ipaddress
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "%s.%s", IPV4_TSIP_PREFIX, IPV4_TSIP_IP);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS == l_iRet_Val)
	{
		if (l_cpPsm_Get != NULL)
	    {
    	    strncpy(l_cNvTsip_IpAddr, l_cpPsm_Get, sizeof(l_cNvTsip_IpAddr));
	        Ansc_FreeMemory_Callback(l_cpPsm_Get);
    	    l_cpPsm_Get = NULL;
		}
		else
		{
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
		}
    }
    else
    {
        fprintf(stderr, "Error:%d while getting parameter:%s\n", 
				l_iRet_Val, l_cPsm_Parameter);
    } 

	//psm get dmsb.truestaticip.Subnetmask
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "%s.%s", IPV4_TSIP_PREFIX, IPV4_TSIP_SUBNET);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS == l_iRet_Val) 
	{
		if (l_cpPsm_Get != NULL)
	    {
            /* CID 162994: BUFFER_SIZE_WARNING */
            strncpy(l_cNvTsip_IpSubnet, l_cpPsm_Get, sizeof(l_cNvTsip_IpSubnet)-1);
	    l_cNvTsip_IpSubnet[sizeof(l_cNvTsip_IpSubnet)-1] = '\0';
	        Ansc_FreeMemory_Callback(l_cpPsm_Get);
    	    l_cpPsm_Get = NULL;
		}
		else
		{	
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
		}
    }
    else
    {
        fprintf(stderr, "Error:%d while getting parameter:%s\n", 
				l_iRet_Val, l_cPsm_Parameter);
    }

	//psm get dmsb.truestaticip.Gateway
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
			 "%s.%s", IPV4_TSIP_PREFIX, IPV4_TSIP_GATEWAY);

    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS == l_iRet_Val)
	{
		if (l_cpPsm_Get != NULL)
	    {
            /* CID 135638 : BUFFER_SIZE_WARNING */
    	    strncpy(l_cNvTsip_Gateway, l_cpPsm_Get, sizeof(l_cNvTsip_Gateway)-1);
	    l_cNvTsip_Gateway[sizeof(l_cNvTsip_Gateway)-1] = '\0';
	        Ansc_FreeMemory_Callback(l_cpPsm_Get);
    	    l_cpPsm_Get = NULL;
		}
		else
		{
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
		}
    }
    else
    {
        fprintf(stderr, "Error:%d while getting parameter:%s\n", 
				l_iRet_Val, l_cPsm_Parameter);
    }

	fprintf(stderr, "Syncing from PSM True Static IP Enable:%s, IP:%s, SUBNET:%s, GATEWAY:%s\n", 
		   l_cNv_Tsip_Enable, l_cNvTsip_IpAddr, l_cNvTsip_IpSubnet, l_cNvTsip_Gateway);

    // apply the new original true static ip
	if (0 != l_cNv_Tsip_Enable[0] && 0 != l_iNv_Tsip_Enable)
	{
		l_iCIDR = mask2cidr(l_cNvTsip_IpSubnet);
		subnet(l_cNvTsip_IpAddr, l_cNvTsip_IpSubnet, l_cSubnet);

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip addr add %s/%d broadcast + dev %s", 
				 l_cNvTsip_IpAddr, l_iCIDR, LAN_IF_NAME);

        executeCmd(l_cSysevent_Cmd);

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip route add table 14 %s/%d dev %s", 
				 l_cSubnet, l_iCIDR, LAN_IF_NAME);

        executeCmd(l_cSysevent_Cmd);

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip route add table all_lans %s/%d dev %s", 
				 l_cSubnet, l_iCIDR, LAN_IF_NAME);

        executeCmd(l_cSysevent_Cmd);
    }
}

void sync_tsip_asn () 
{
	char l_cNv_Tsip_Asn_Ip[16] = {0}, l_cNv_Tsip_Asn_Subnet[16] = {0}, l_cSubnet[16] = {0};
	char l_cPsm_Parameter[255] = {0}, l_cSysevent_Cmd[255] = {0};
	unsigned int l_iTs_Asn_Count = 0;
    unsigned int *l_iTs_Asn_Ins = NULL; 
	char* l_cpPsm_Get = NULL;
	int l_iRet_Val = 0, l_iCIDR, l_iIter;

    l_iRet_Val = PSM_VALUE_GET_INS(IPV4_TSIP_ASNPREFIX, &l_iTs_Asn_Count, &l_iTs_Asn_Ins);
    if(l_iRet_Val == CCSP_SUCCESS)
	{
		if (l_iTs_Asn_Count != 0)
		{
    		if(MAX_TS_ASN_COUNT -1  < l_iTs_Asn_Count)
			{
	        	fprintf(stderr, "ERROR Too many Ture static subnet\n");
    	        l_iTs_Asn_Count = MAX_TS_ASN_COUNT -1;
	        }
    	    for(l_iIter = 0; l_iIter < (int)l_iTs_Asn_Count ; l_iIter++)
			{ 
	        	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
						 "%s%d.%s", IPV4_TSIP_ASNPREFIX, 
						 l_iTs_Asn_Ins[l_iIter], IPV4_TSIP_ENABLE);

    	        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get) - CCSP_SUCCESS;
        	    if (l_iRet_Val == 0)
				{
					if (l_cpPsm_Get != NULL)
					{
		            	if(atoi(l_cpPsm_Get) != 1)
						{
        		            Ansc_FreeMemory_Callback(l_cpPsm_Get);
            		        l_cpPsm_Get = NULL; 
                		    continue;
		                }
    		            Ansc_FreeMemory_Callback(l_cpPsm_Get);
        		        l_cpPsm_Get = NULL; 
					}
					else
					{
						fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
					}
            	}
				else
				{
        			fprintf(stderr, "Error:%d while getting parameter:%s\n", 
							l_iRet_Val, l_cPsm_Parameter);
			    }
 
	            snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), 
						 "%s%d.%s", IPV4_TSIP_ASNPREFIX, 
						 l_iTs_Asn_Ins[l_iIter], IPV4_TSIP_IP);

    	        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get) - CCSP_SUCCESS;
        	    if (l_iRet_Val == 0)
				{
					if (l_cpPsm_Get != NULL)
					{
				/*CID 163592 : BUFFER_SIZE */
				strncpy(l_cNv_Tsip_Asn_Ip, l_cpPsm_Get, sizeof(l_cNv_Tsip_Asn_Ip)-1);
				l_cNv_Tsip_Asn_Ip[sizeof(l_cNv_Tsip_Asn_Ip)-1] = '\0';
                                Ansc_FreeMemory_Callback(l_cpPsm_Get);
        		        l_cpPsm_Get = NULL; 
	            	}
					else
					{
						fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
					}
				}
				else
				{
        			fprintf(stderr, "Error:%d while getting parameter:%s\n", 
							l_iRet_Val, l_cPsm_Parameter);
			    }
 	
    	        sprintf(l_cPsm_Parameter,"%s%d.%s", 
						IPV4_TSIP_ASNPREFIX, l_iTs_Asn_Ins[l_iIter], 
						IPV4_TSIP_SUBNET);

        	    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get) - CCSP_SUCCESS;
            	if (l_iRet_Val == 0)
				{
					if(l_cpPsm_Get != NULL)
					{
    	        		strncpy(l_cNv_Tsip_Asn_Subnet, l_cpPsm_Get, 
								sizeof(l_cNv_Tsip_Asn_Subnet));

	        	        Ansc_FreeMemory_Callback(l_cpPsm_Get);
    	        	    l_cpPsm_Get = NULL; 
					}
					else
					{
						fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
					}
	            }
				else
				{
        			fprintf(stderr, "Error:%d while getting parameter:%s\n", 
							l_iRet_Val, l_cPsm_Parameter);
			    }

				l_iCIDR = mask2cidr(l_cNv_Tsip_Asn_Subnet);
				subnet(l_cNv_Tsip_Asn_Ip, l_cNv_Tsip_Asn_Subnet, l_cSubnet);
	
				snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
	    	             "ip addr add %s/%d broadcast + dev %s", 
						 l_cNv_Tsip_Asn_Ip, l_iCIDR, LAN_IF_NAME);

    	    	executeCmd(l_cSysevent_Cmd);

	        	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
    	        	     "ip route add table 14 %s/%d dev %s", 
						 l_cSubnet, l_iCIDR, LAN_IF_NAME);

		        executeCmd(l_cSysevent_Cmd);
		
    		    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
        		         "ip route add table all_lans %s/%d dev %s", 
						 l_cSubnet, l_iCIDR, LAN_IF_NAME);

		        executeCmd(l_cSysevent_Cmd);
    	    }
        	Ansc_FreeMemory_Callback(l_iTs_Asn_Ins);
		}
		else
		{
			fprintf(stderr, "psmcli get of :%s is empty\n", IPV4_TSIP_ASNPREFIX);
		}
    }
	else
	{
    	fprintf(stderr, "Error while getting :%s\n", IPV4_TSIP_ASNPREFIX);
    }
}

BOOL apply_config(int l3_inst, char *staticIpv4Addr, char *staticIpv4Subnet)
{
	char l_cCur_Ipv4_Addr[16] = {0}, l_cCur_Ipv4_Subnet[16] = {0}; 
    char l_cIfName[16] = {0}, l_cSysevent_Cmd[255] = {0}, l_cSubnet[16] = {0}; 
	char l_cArp_Ignore_File[64] = {0};
    int l_iRT_Table, l_iCIDR;   
	FILE *l_fArp_Ignore = NULL;
	if (NULL == staticIpv4Addr || 0 == staticIpv4Addr[0])
	{
		fprintf(stderr, "Static IPv4 Address is empty get it from ipv4_%d-ipv4addr\n", 
				l3_inst);

    	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);    
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
					 l_cCur_Ipv4_Addr, sizeof(l_cCur_Ipv4_Addr));    
	}
	else
	{
		fprintf(stderr, "Static IPv4 Address is not empty treating:%s as current IP\n", 
				staticIpv4Addr);

		strncpy(l_cCur_Ipv4_Addr, staticIpv4Addr, 16);
	}
    
	if (NULL == staticIpv4Subnet || 0 == staticIpv4Subnet[0])
	{
		fprintf(stderr, "Static IPv4 Subnet is empty get it from ipv4_%d-ipv4subnet\n", l3_inst);
	    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4subnet", l3_inst);   
    	sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
					 l_cCur_Ipv4_Subnet, sizeof(l_cCur_Ipv4_Subnet));
	}
	else
	{
		fprintf(stderr, "Static IPv4 Subnet is not empty treating:%s as current IP\n", 
				staticIpv4Subnet);

		strncpy(l_cCur_Ipv4_Subnet, staticIpv4Subnet, 16);
	}	

    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ifname", l3_inst);
    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cIfName, sizeof(l_cIfName));

	if (0 == l_cCur_Ipv4_Addr[0] || 0 == l_cCur_Ipv4_Subnet[0])
	{
		fprintf(stderr, "Error during apply_config returning FALSE\n");
    	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);
	    sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "error", 0);

	    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-error", l3_inst);
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, 
					 l_cSysevent_Cmd, "Missing IP or subnet", 0);
        return FALSE;
    }  

	snprintf(l_cArp_Ignore_File, sizeof(l_cArp_Ignore_File), 
			 "/proc/sys/net/ipv4/conf/%s/arp_ignore", l_cIfName);

	l_fArp_Ignore = fopen(l_cArp_Ignore_File, "w");
	if (NULL != l_fArp_Ignore)
	{
		fprintf(l_fArp_Ignore, "%d", 2);
		fclose(l_fArp_Ignore);
	}
	else
	{
		fprintf(stderr, "Error while opening %s\n", l_cArp_Ignore_File);
	}
    l_iRT_Table = l3_inst + 10;
    l_iCIDR = mask2cidr(l_cCur_Ipv4_Subnet);

    //If it's ipv6 only mode, doesn't config ipv4 address. For ipv6 other things, we don't take care.
	if (!strncmp(l_cIfName, LAN_IF_NAME, 6))
	{
		char l_cLast_Erouter_Mode[8] = {0};
    	syscfg_get(NULL, "last_erouter_mode", l_cLast_Erouter_Mode, sizeof(l_cLast_Erouter_Mode));
		if ((!strncmp(l_cLast_Erouter_Mode, "1", 1)) || (!strncmp(l_cLast_Erouter_Mode, "3", 1)))
		{
			snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip addr add %s/%d broadcast + dev %s", l_cCur_Ipv4_Addr, l_iCIDR, l_cIfName);
	        executeCmd(l_cSysevent_Cmd);
        }
	}
    else
	{
        snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                 "ip addr add %s/%d broadcast + dev %s", l_cCur_Ipv4_Addr, l_iCIDR, l_cIfName);
        executeCmd(l_cSysevent_Cmd);
    }  

    
    // TODO: Fix this static workaround. Should have configurable routing policy.
	subnet(l_cCur_Ipv4_Addr, l_cCur_Ipv4_Subnet, l_cSubnet);
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
             "ip rule add from %s lookup %d", 
			 l_cCur_Ipv4_Addr, l_iRT_Table);

    executeCmd(l_cSysevent_Cmd);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
             "ip rule add iif %s lookup erouter", l_cIfName);
	executeCmd(l_cSysevent_Cmd);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
             "ip rule add iif %s lookup %d", 
			 l_cIfName, l_iRT_Table);

	executeCmd(l_cSysevent_Cmd);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
             "ip route add table %d %s/%d dev %s", 
			 l_iRT_Table, l_cSubnet, l_iCIDR, l_cIfName);

    executeCmd(l_cSysevent_Cmd);

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
             "ip route add table all_lans %s/%d dev %s", 
			 l_cSubnet, l_iCIDR, l_cIfName);

    executeCmd(l_cSysevent_Cmd);

    // bind 161/162 port to brlan0 interface
	// del 161/162 port from brlan0 interface when it is teardown
    /*if (!strncmp(l_cIfName, LAN_IF_NAME, 6))
    {
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "ipv4_address", l_cCur_Ipv4_Addr, 0);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "snmppa_socket_entry", "add", 0);
    }*/
	
	// assign lan interface a global ipv6 address
    // I can't find other way to do this. Just put here temporarily.
	char l_cLan_IpAddrv6_prev[64] = {0}, l_cLan_PrefixV6[8] = {0}, l_cLan_IpAddrv6[16] = {0};
	if (!strncmp(l_cIfName, LAN_IF_NAME, 6))
	{
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 "lan_ipaddr_v6_prev", l_cLan_IpAddrv6_prev, 
					 sizeof(l_cLan_IpAddrv6_prev));

    	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 "lan_ipaddr_v6", l_cLan_IpAddrv6, 
					 sizeof(l_cLan_IpAddrv6));

    	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 "lan_prefix_v6", l_cLan_PrefixV6, 
					 sizeof(l_cLan_PrefixV6));

		if (strncmp(l_cLan_IpAddrv6_prev, l_cLan_IpAddrv6, 64))
	        {
                  if (l_cLan_IpAddrv6_prev != NULL)
                  {    
                     snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
                         "ip -6 addr del %s/64 dev %s valid_lft forever preferred_lft forever", 
                                         l_cLan_IpAddrv6_prev, l_cIfName);
 
                     executeCmd(l_cSysevent_Cmd);
                  }

			snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd),
            	     "ip -6 addr add %s/64 dev %s valid_lft forever preferred_lft forever", 
					 l_cLan_IpAddrv6, l_cIfName);

	        executeCmd(l_cSysevent_Cmd);
        }
    }

	if (!strncmp(l_cIfName, LAN_IF_NAME, 6))
	{	
        sync_tsip();
        sync_tsip_asn();
		sysevent_set(g_iSyseventfd, g_tSysevent_token, "wan_staticip-status", "started", 0);
	}

    // END ROUTING TODO
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cCur_Ipv4_Addr, 0);

    snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4subnet", l3_inst);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cCur_Ipv4_Subnet, 0);

	return TRUE;
}

void load_static_l3 (int l3_inst) 
{
	char l_cStatic_V4_Addr[16] = {0}, l_cStatic_V4_Subnet[16] = {0};
	char l_cPsm_Parameter[255] = {0};
	char l_cSysevent_Cmd[255] = {0};
	char *l_cpPsm_Get = NULL;
	int l_iRet_Val;
	BOOL l_bApplyConfig_Res;
	int fd = 0;

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.V4Addr", l3_inst);
	l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS == l_iRet_Val)
    {
	if(l_cpPsm_Get != NULL)
        {
                /*CID 135418 : BUFFER_SIZE_WARNING */
                strncpy(l_cStatic_V4_Addr, l_cpPsm_Get, sizeof(l_cStatic_V4_Addr)-1);
                l_cStatic_V4_Addr[sizeof(l_cStatic_V4_Addr)-1] = '\0';
                Ansc_FreeMemory_Callback(l_cpPsm_Get);
                l_cpPsm_Get = NULL;
	}
	else
	{
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
	}
    }    
    else 
    {    
		fprintf(stderr, "Error:%d while getting parameter:%s\n", l_iRet_Val, l_cPsm_Parameter);
    }

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.V4SubnetMask", l3_inst);
	l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS == l_iRet_Val)
	{	
		if (l_cpPsm_Get != NULL)
	    {
            /*CID 135418 : BUFFER_SIZE_WARNING */
            strncpy(l_cStatic_V4_Subnet, l_cpPsm_Get, sizeof(l_cStatic_V4_Subnet)-1);
	    l_cStatic_V4_Subnet[sizeof(l_cStatic_V4_Subnet)-1] = '\0';
	        Ansc_FreeMemory_Callback(l_cpPsm_Get);
    	    l_cpPsm_Get = NULL;
		}
		else
		{
			fprintf(stderr, "psmcli get of :%s is empty\n", l_cPsm_Parameter);
		}
    } 
    else
    { 
		fprintf(stderr, "Error:%d while getting parameter:%s\n", 
				l_iRet_Val, l_cPsm_Parameter);
    }

	//The below code is present in handle_l2_status in shell script 
	if (0 != l_cStatic_V4_Addr[0] && 0 != l_cStatic_V4_Subnet[0])
	{
    	// Apply static config if exists
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4_static", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "1", 0);

		//Passing static IPv4 Address and IPv4 subnet to the apply_config
		//This is different from shell script
		l_bApplyConfig_Res = apply_config(l3_inst, l_cStatic_V4_Addr, l_cStatic_V4_Subnet);
        if (TRUE == l_bApplyConfig_Res)
		{
			snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);
	     		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "up", 0);
		
		       fprintf(stderr, "service_ipv4 : Triggering RDKB_FIREWALL_RESTART\n");
                       t2_event_d("SYS_SH_RDKB_FIREWALL_RESTART", 1);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "firewall-restart", "", 0);	

			if (4 == l3_inst)
			{
            	fprintf(stderr, "IPv4 address is set for %s interface MOCA interface is UP\n",
					    LAN_IF_NAME);
                if (access("/tmp/moca_start", F_OK) == -1 && errno == ENOENT)
                {
                        if((fd = creat("/tmp/moca_start", S_IRUSR | S_IWUSR)) == -1)
                        {
                                fprintf(stderr, "File: /tmp/moca_start creation failed with error:%d\n", errno);
                        }
                        else
                        {
                                close(fd);
                        }
                        print_uptime("boot_to_MOCA_uptime",NULL, NULL);
                }
            }
        }
	}	
    else
	{
    	// Otherwise, announce readiness for provisioning
        // TODO: consider applying dynamic config currently in sysevent
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4_static", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "0", 0);
        
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "unconfigured", 0);
	}
}

void handle_l2_status (int l3_inst, int l2_inst, char *net_status, int input) 
{
	char l_cLocalReady[8] = {0}, l_cSysevent_Cmd[255] = {0}, l_cIpv4_Status[16] = {0};
	char l_cIfName[16] = {0};
	char l_cL2Inst[8] = {0};

	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "multinet_%d-localready", l2_inst);
    sysevent_get(g_iSyseventfd, g_tSysevent_token, 
				 l_cSysevent_Cmd, l_cLocalReady, sizeof(l_cLocalReady));

	if (((!strncmp(net_status, "partial", 7)) || (!strncmp(net_status, "ready", 5))) &&
        (!strncmp(l_cLocalReady, "1", 1)))
	{
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 l_cSysevent_Cmd, l_cIpv4_Status, sizeof(l_cIpv4_Status));
	
		if ((!strncmp(l_cIpv4_Status, "up", 2)) || (!strncmp(l_cIpv4_Status, "unconfigured", 12)))
		{
			fprintf(stderr, "IPv4 is already prepared so nothing needs to be done\n");
			return;
		}
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "multinet_%d-name", l2_inst);
	    sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 l_cSysevent_Cmd, l_cIfName, sizeof(l_cIfName));

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ifname", l3_inst);
   		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cIfName, 0);

		load_static_l3(l3_inst);
	}
    else
	{
		fprintf(stderr, "Multinet status:%s is neither partial nor ready\n", net_status);
    	// l2 down
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-status", l3_inst);
		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, "pending", 0);

		if (((0 == net_status[0]) || (!strncmp(net_status, "stopped", 7))) && (0 != input))
		{
			/* Changes made to improve XHome and wan uptime*/
			fprintf(stderr, "Setting multinet-up event for %s and %s\n",LAN_IF_NAME, XHS_IF_NAME);
			snprintf(l_cL2Inst, sizeof(l_cL2Inst), "%d", l2_inst);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, "multinet-up", l_cL2Inst, 0);
		}
	}
}

// This function should only be called when an instance needs to be brought 
// args: l3 instance
void resync_instance (int l3_inst)
{
	char l_cNv_EthLower[8] = {0}, l_cNv_Ip[16] = {0}, l_cNv_Subnet[16] = {0};
	char l_cNv_Enabled[8] = {0}, l_cPsm_Parameter[255] = {0}, l_cSysevent_Cmd[255] = {0};
	char l_cLower[8] = {0}, l_cCur_Ipv4_Addr[16] = {0}, l_cCur_Ipv4_Subnet[16] = {0};
	char l_cIpv4_Static[16] = {0}, l_cIpv4_Instances[8] = {0}, l_cNv_Lower_Status[16] = {0}, l_cIpv4_Instances_new[30];
	char l_cNv_Lower[8] = {0};
	char l_cAsyncId[16] = {0};
	char l_cEvent_Name[32] = {0}, l_cL3Inst[8] = {0};

	char *l_cParam[1] = {0};
	char *l_cpPsm_Get = NULL;

	int l_iRet_Val, l_iL2Inst;
	async_id_t l_sAsyncID;

	fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : In resync_instance to bring up an instance\n");
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.EthLink", l3_inst);
	l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
    {    
		fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ETHLOWER returned null, retrying\n");
		l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
		if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
		{
			fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ETHLOWER returned null ");
			fprintf(stderr, "even after retry, no more retries\n");
		}
		else
		{
			strncpy(l_cNv_EthLower, l_cpPsm_Get, sizeof(l_cNv_EthLower));
			fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ETHLOWER is:%s\n", l_cNv_EthLower);
		}
    }    
	else
	{
		strncpy(l_cNv_EthLower, l_cpPsm_Get, sizeof(l_cNv_EthLower));
		fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ETHLOWER is:%s\n", l_cNv_EthLower);
		
	}

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.V4Addr", l3_inst);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
    {    
        fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_IP returned null, retrying\n");
        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_IP returned null ");
			fprintf(stderr, "even after retry, no more retries\n");
        }
		else
		{
			strncpy(l_cNv_Ip, l_cpPsm_Get, sizeof(l_cNv_Ip));
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_IP is:%s\n", l_cNv_Ip);
		}
    }
	else
	{
		strncpy(l_cNv_Ip, l_cpPsm_Get, sizeof(l_cNv_Ip));
        fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_IP is:%s\n", l_cNv_Ip);
	}

	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.V4SubnetMask", l3_inst);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
   	if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
    {    
        fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_SUBNET returned null, retrying\n");
        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_SUBNET returned null ");
			fprintf(stderr, "even after retry, no more retries\n");
        }
		else
		{
			strncpy(l_cNv_Subnet, l_cpPsm_Get, sizeof(l_cNv_Subnet));
			fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_SUBNET is:%s\n", l_cNv_Subnet);
		}
    }	
	else
	{
		strncpy(l_cNv_Subnet, l_cpPsm_Get, sizeof(l_cNv_Subnet));
		fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_SUBNET is:%s\n", l_cNv_Subnet);
	}	
		
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.l3net.%d.Enable", l3_inst);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
    {    
        fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ENABLED returned null, retrying\n");
        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ENABLED returned null even after retry, no more retries\n");
        }
		else
		{
			strncpy(l_cNv_Enabled, l_cpPsm_Get, sizeof(l_cNv_Enabled));
			fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ENABLED is:%s\n", l_cNv_Enabled);
		}
    }
	else
	{
		strncpy(l_cNv_Enabled, l_cpPsm_Get, sizeof(l_cNv_Enabled));
		fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_ENABLED is:%s\n", l_cNv_Enabled);
	}	

	if (0 == l_cNv_Enabled[0] || !strncmp(l_cNv_Enabled, "false", 5))
	{
		fprintf(stderr, "L3 Instance:%d is not enabled\n", l3_inst);
		teardown_instance(l3_inst); //TODO teardown_instance
		return;
	}

    // Find l2net instance from EthLink instance.
	snprintf(l_cPsm_Parameter, sizeof(l_cPsm_Parameter), "dmsb.EthLink.%s.l2net", l_cNv_EthLower);
    l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
    if (CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
    {
        fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_LOWER returned null, retrying\n");
        l_iRet_Val = PSM_VALUE_GET_STRING(l_cPsm_Parameter, l_cpPsm_Get);
        if(CCSP_SUCCESS != l_iRet_Val || l_cpPsm_Get == NULL)
        {
            fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_LOWER returned null even after retry, no more retries\n");
        }
		else
		{
			strncpy(l_cNv_Lower, l_cpPsm_Get, sizeof(l_cNv_Lower));
			fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_LOWER is:%s\n", l_cNv_Lower);
		}
    }
	else
	{
		strncpy(l_cNv_Lower, l_cpPsm_Get, sizeof(l_cNv_Lower));
		fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : NV_LOWER is:%s\n", l_cNv_Lower);
	}
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-lower", l3_inst);	
	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
				 l_cSysevent_Cmd, l_cLower, 
				 sizeof(l_cLower));	
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4addr", l3_inst);	
	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
				 l_cSysevent_Cmd, l_cCur_Ipv4_Addr, 
				 sizeof(l_cCur_Ipv4_Addr));	
	
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4subnet", l3_inst);	
	sysevent_get(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, 
				 l_cCur_Ipv4_Subnet, sizeof(l_cCur_Ipv4_Subnet));	

    //DEBUG
	fprintf(stderr, "RDKB_SYSTEM_BOOT_UP_LOG : Syncing l3 instance (%d), ", l3_inst);
	fprintf(stderr, "NV_ETHLOWER:%s, NV_LOWER:%s, NV_ENABLED:%s, ", 
			l_cNv_EthLower, l_cNv_Lower, l_cNv_Enabled);

	fprintf(stderr, "NV_IP:%s, NV_SUBNET:%s, LOWER:%s, CUR_IPV4_ADDR:%s, CUR_IPV4_SUBNET:%s\n", 
			l_cNv_Ip, l_cNv_Subnet, l_cLower, l_cCur_Ipv4_Addr, l_cCur_Ipv4_Subnet);

	l_iL2Inst = atoi(l_cNv_Lower);
	if (strncmp(l_cNv_Lower, l_cLower, 1)) //NV_LOWER != LOWER
	{
        //#different lower layer, teardown and switch
		if (0 != l_cLower[0])
		{
            teardown_instance(l3_inst);
        }

		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-lower", l3_inst);
		//sysevent set ipv4_<L3 Instance>-lower NV_LOWER
    	sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cNv_Lower, 0);
		if (0 != l_cNv_Lower[0])
		{
			sysevent_get(g_iSyseventfd, g_tSysevent_token, 
						 "ipv4-instances", l_cIpv4_Instances, 
						 sizeof(l_cIpv4_Instances));
			if (0 != l_cIpv4_Instances[0]){
				snprintf(l_cIpv4_Instances_new, sizeof(l_cIpv4_Instances_new), 
						 "%s %d", l_cIpv4_Instances, l3_inst);
			}
			else
				snprintf(l_cIpv4_Instances_new, sizeof(l_cIpv4_Instances_new), 
						 "%d", l3_inst);

			fprintf(stderr, "IPv4 instances is:%s\n", l_cIpv4_Instances_new);
			sysevent_set(g_iSyseventfd, g_tSysevent_token, 
						 "ipv4-instances", l_cIpv4_Instances_new, 0);

			snprintf(l_cEvent_Name, sizeof(l_cEvent_Name), "multinet_%s-status", l_cNv_Lower);
			snprintf(l_cL3Inst, sizeof(l_cL3Inst), "%d", l3_inst);
			l_cParam[0] = l_cL3Inst;

			sysevent_setcallback(g_iSyseventfd, g_tSysevent_token, ACTION_FLAG_NONE,
                             	 l_cEvent_Name, THIS, 1, l_cParam, &l_sAsyncID);

			snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
					 "ipv4_%d-l2async", l3_inst);
			snprintf(l_cAsyncId, sizeof(l_cAsyncId), 
					 "%d %d", l_sAsyncID.action_id, 
					 l_sAsyncID.trigger_id);

    		sysevent_set(g_iSyseventfd, g_tSysevent_token, l_cSysevent_Cmd, l_cAsyncId, 0);

			snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
					 "multinet_%s-status", l_cNv_Lower);
   			sysevent_get(g_iSyseventfd, g_tSysevent_token, 
						 l_cSysevent_Cmd, l_cNv_Lower_Status, 
						 sizeof(l_cNv_Lower_Status));

			handle_l2_status(l3_inst, l_iL2Inst, l_cNv_Lower_Status, 1);
        }
	}
    else
	{
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-ipv4_static", l3_inst);
		sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 l_cSysevent_Cmd, l_cIpv4_Static, 
					 sizeof(l_cIpv4_Static));

		if ((0 != l_cCur_Ipv4_Addr[0] ) && 
			((!strncmp(l_cIpv4_Static, "1", 1)) || (0 != l_cNv_Ip[0])))
		{
			if ((strncmp(l_cCur_Ipv4_Addr, l_cNv_Ip, 15)) || 
				(strncmp(l_cCur_Ipv4_Subnet, l_cNv_Subnet, 15)))
			{
                // Same lower layer, but static IP info changed.
				remove_config(l3_inst);
				snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
						 "multinet_%s-status", l_cNv_Lower);
     			sysevent_get(g_iSyseventfd, g_tSysevent_token, 
							 l_cSysevent_Cmd, l_cNv_Lower_Status, 
							 sizeof(l_cNv_Lower_Status));

				//4th parameter is not passed in shell script but passing it as zero in C code
				handle_l2_status(l3_inst, l_iL2Inst, l_cNv_Lower_Status, 0);
            }
        }
    }
}

void ipv4_up(char *l3_inst)
{
	char l_cLower[8] = {0}, l_cSysevent_Cmd[64] = {0}, l_cL3Net_Status[16] = {0};
	int l_iL3Inst, l_iL2Inst;
	
	l_iL3Inst = atoi(l3_inst);
	snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), "ipv4_%d-lower", l_iL3Inst);	
	sysevent_get(g_iSyseventfd, g_tSysevent_token, 
				 l_cSysevent_Cmd, l_cLower, sizeof(l_cLower));	

	if (0 == l_cLower[0])
	{
		fprintf(stderr, "Lower is empty Calling resync instance with input:%d\n", l_iL3Inst);
		resync_instance(l_iL3Inst);
	}	    
    else
	{
		fprintf(stderr, "Lower is not empty Calling handle_l2_status\n");
		snprintf(l_cSysevent_Cmd, sizeof(l_cSysevent_Cmd), 
				 "multinet_%s-status", l_cLower);

		sysevent_get(g_iSyseventfd, g_tSysevent_token, 
					 l_cSysevent_Cmd, l_cL3Net_Status, 
					 sizeof(l_cL3Net_Status));
		
		l_iL2Inst = atoi(l_cLower);
		handle_l2_status(l_iL3Inst, l_iL2Inst, l_cL3Net_Status, 1);
	}
}
