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

/**********************************************************************
 * FileName:   igd_service_lanhostconfigmgmt.c
 * Author:      Sridhar Ramaswamy(srramasw@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD LANHostConfigManagement service
 *****************************************************************************/
/*$Id: igd_service_lanhostconfigmgmt.c,v 1.1 2010/01/11 09:42:29 jianxiao Exp $
 *
 *Revision 1.1  2010/01/11 03:13:02  srramasw
 *create orignal version
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pal_upnp_device.h"
#include "pal_upnp.h"
#include "pal_def.h"
#include "pal_log.h"
#include "igd_platform_independent_inf.h"
#include "igd_utility.h"

#define LANHOSTCONFIGMGMT_SERVICE_ID "urn:upnp-org:serviceId:LANHostCfg1"
#define LANHOSTCONFIGMGMT_SERVICE_TYPE "urn:schemas-upnp-org:service:LANHostConfigManagement:1"
#define	STATUS_STRING_LEN	16

enum LANHostConfigMgmt_service_state_variables_index
{
    DHCPServerConfigurable_index,
        DHCPRelay_index,
        SubnetMask_index,
        IPRouters_index,
        DNSServers_index,
        DomainName_index,
        MinAddress_index,
        MaxAddress_index,
    ReservedAddresses_index,
};

LOCAL INT32 _igd_get_DHCPServerConfigurable (INOUT struct action_event *event);
LOCAL INT32 _igd_get_DHCPRelay (INOUT struct action_event *event);
LOCAL INT32 _igd_get_SubnetMask (INOUT struct action_event *event);
LOCAL INT32 _igd_get_DomainName (INOUT struct action_event *event);
LOCAL INT32 _igd_get_DNSServers (INOUT struct action_event *event);
LOCAL INT32 _igd_get_AddressRange (INOUT struct action_event *event);
LOCAL INT32 _igd_get_ReservedAddresses (INOUT struct action_event *event);
LOCAL INT32 _igd_nope_success (INOUT struct action_event *event);
LOCAL INT32 _igd_get_IPRoutersList (INOUT struct action_event *event);

/*
 * Only selected LAN methods are enabled (to avoid creating a security vulnerability)
 * All set, delete actions methods are stubbed out; only get methods are supported
 */
LOCAL struct upnp_action LANHostConfigMgmt_actions[] =
{
	{"GetDHCPServerConfigurable", _igd_get_DHCPServerConfigurable},
	{"SetDHCPServerConfigurable", _igd_nope_success},
	{"GetDHCPRelay",              _igd_get_DHCPRelay},
	{"SetDHCPRelay",              _igd_nope_success},
	{"GetSubnetMask",             _igd_get_SubnetMask},
	{"SetSubnetMask",             _igd_nope_success},
	{"GetIPRoutersList",          _igd_get_IPRoutersList},
	{"SetIPRouter",               _igd_nope_success},
	{"DeleteIPRouter",            _igd_nope_success},
	{"GetDomainName",             _igd_get_DomainName},
	{"SetDomainName",             _igd_nope_success},
	{"GetAddressRange",           _igd_get_AddressRange},
	{"SetAddressRange",           _igd_nope_success},
	{"GetReservedAddresses",      _igd_get_ReservedAddresses},
	{"SetReservedAddress",        _igd_nope_success},
	{"DeleteReservedAddress",     _igd_nope_success},
	{"GetDNSServers",             _igd_get_DNSServers},
	{"SetDNSServer",              _igd_nope_success},
	{"DeleteDNSServer",           _igd_nope_success},
	{NULL, NULL}
};

LOCAL const CHAR * LANHostConfigMgmt_variables_name[] = 
{
    "DHCPServerConfigurable",
    "DHCPRelay",
    "SubnetMask",
    "IPRouters",
    "DNSServers",
    "DomainName",
    "MinAddress",
    "MaxAddress",
    "ReservedAddresses",
    NULL
};

/************************************************************
 * Function: _igd_service_LANHostConfigMgmt_desc_file 
 *
 *  Parameters:	
 *      fp: Input/Output. the description file pointer.
 * 
 *  Description:
 *      This functions generate the description file of the LANHostConfigManagement service.
 *
 *  Return Values: INT32
 *      0 if successful ,-1 for error
 ************************************************************/ 
LOCAL INT32 _igd_service_LANHostConfigMgmt_desc_file(INOUT FILE *fp)
{
	LOCAL INT32 service_index=0;
	if(fp==NULL)
		return -1;

	fprintf(fp, "<service>\n");
	fprintf(fp, "<serviceType>%s</serviceType>\n",LANHOSTCONFIGMGMT_SERVICE_TYPE);
	fprintf(fp, "<serviceId>%s</serviceId>\n",LANHOSTCONFIGMGMT_SERVICE_ID);
	fprintf(fp, "<SCPDURL>/LANHostConfigManagementSCPD.xml</SCPDURL>\n");
	fprintf(fp, "<controlURL>/upnp/control/LANHostConfigManagement%d</controlURL>\n",service_index);
	fprintf(fp, "<eventSubURL>/upnp/event/LANHostConfigManagement%d</eventSubURL>\n",service_index);
	fprintf(fp, "</service>\n");
	service_index++;
	return 0;
}

/************************************************************
* Function: _igd_service_LANHostConfigMgmt_destroy
*
*  Parameters: 
*	   pservice:		   IN. the service pointer. 
* 
*  Description:
*	  This function destroy the service WANEthernetLinkConfig.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_service_LANHostConfigMgmt_destroy(IN struct upnp_service *pservice)
{
	/* pservice->serviceID is a 'const CHAR *' type and required to remove
	   const before call free() function */
	CHAR * serviceID = (CHAR *)NULL;

	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Destroy LANDevice LANHostConfigManagement");
	if(pservice==NULL)
		return -1;

	serviceID = (CHAR *)pservice->serviceID; /*RDKB-7141, CID-33136, use after null check*/
	SAFE_FREE(pservice->type);
	SAFE_FREE(serviceID);
	SAFE_FREE(pservice->state_variables);
	SAFE_FREE(pservice->event_variables);
	SAFE_FREE(pservice->private);
	pthread_mutex_destroy(&pservice->service_mutex);
	SAFE_FREE(pservice);
	return 0;
}

/************************************************************
* Function: IGD_service_LANHostConfigManagementInit
*
*  Parameters: 
*	   input_index_struct:		   IN. the device index struct. 
*	   fp:   INOUT. the description file pointer. 
* 
*  Description:
*	  This function initialize the service LANHostConfigManagement
*
*  Return Values: struct upnp_service*
*	   The service pointer if successful else NULL.
************************************************************/
struct upnp_service* IGD_service_LANHostConfigManagementInit(IN VOID* input_index_struct, INOUT FILE *fp)
{	
	INT32 i;
	struct upnp_service *LANHostConfigMgmt_service=NULL;
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Initilize LANHostConfigManagement of LANDevice %d",
                   ((struct device_and_service_index*)input_index_struct)->lan_device_index);

	LANHostConfigMgmt_service=(struct upnp_service *)calloc(1,sizeof(struct upnp_service));
	if(LANHostConfigMgmt_service==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,upnp_service!\n");
		return NULL;
	}

	if(pthread_mutex_init(&LANHostConfigMgmt_service->service_mutex, NULL ))
	{
            PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "Init mutex fail!\n");
            _igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
            return NULL;
	}
	

	LANHostConfigMgmt_service->type=strdup(LANHOSTCONFIGMGMT_SERVICE_TYPE);
	if(LANHostConfigMgmt_service->type==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"strdup() out of memory,type!\n");
		_igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
		return NULL;
	}
	
	LANHostConfigMgmt_service->serviceID=strdup(LANHOSTCONFIGMGMT_SERVICE_ID);
	if(LANHostConfigMgmt_service->serviceID==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,serviceID!\n");
		_igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
		return NULL;
	}

	LANHostConfigMgmt_service->destroy_function = _igd_service_LANHostConfigMgmt_destroy;
	LANHostConfigMgmt_service->actions = LANHostConfigMgmt_actions;

        LANHostConfigMgmt_service->state_variables = (struct upnp_variable *)calloc(sizeof(LANHostConfigMgmt_variables_name)/sizeof(CHAR *),sizeof(struct upnp_variable));
	if (!LANHostConfigMgmt_service->state_variables)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory,state_variables!\n");
		_igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
		return NULL;
	}
	for(i=0; LANHostConfigMgmt_variables_name[i]!= NULL; i++) {
		LANHostConfigMgmt_service->state_variables[i].name = LANHostConfigMgmt_variables_name[i];
	}

	LANHostConfigMgmt_service->private=(struct device_and_service_index *)calloc(1,sizeof(struct device_and_service_index));
	if(LANHostConfigMgmt_service->private==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory!\n");
		_igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
		return NULL;
	}
	memcpy(LANHostConfigMgmt_service->private, input_index_struct, sizeof(struct device_and_service_index));
	if(_igd_service_LANHostConfigMgmt_desc_file(fp))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"create LANHostConfigManagement description file fail!\n");
		_igd_service_LANHostConfigMgmt_destroy(LANHostConfigMgmt_service);
		return NULL;
	}
	
	return LANHostConfigMgmt_service;
}

/************************************************************
* Function: _igd_get_DHCPServerConfigurable
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetDHCPServerConfigurable.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_DHCPServerConfigurable (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR status[STATUS_STRING_LEN];

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDHCPServerConfigurable of LAN%d\n",local_index.lan_device_index);

        // Status should be 1 (if configurable) or 0 (not configurable)
	if(IGD_pii_get_lan_dhcpserver_configurable(local_index.lan_device_index, status))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDHCPServerConfigurable action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetDHCPServerConfigurable",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewDHCPServerConfigurable";
	params[0].value=status;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[DHCPServerConfigurable_index].value,status, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetDHCPServerConfigurable",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_DHCPRelay
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetDHCPRelay.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_DHCPRelay (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR status[STATUS_STRING_LEN];

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDHCPRelay of LAN%d\n",local_index.lan_device_index);

        // Status should be 1 (if DHCP Relay, i.e router in bridge mode) or
        //                  0 (non DHCP Relay mode, DHCPServer is running in this device)
	if(IGD_pii_get_lan_dhcp_relay_status(local_index.lan_device_index, status))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDHCPRelay action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetDHCPRelay",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewDHCPRelay";
	params[0].value=status;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[DHCPRelay_index].value,status, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetDHCPRelay",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_SubnetMask
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetSubnetMask.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_SubnetMask (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR subnetmask[32];

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetSubnetMask of LAN%d\n",local_index.lan_device_index);

	if(IGD_pii_get_lan_info(local_index.lan_device_index, sizeof(subnetmask), NULL, subnetmask, NULL))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetSubnetMask action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetSubnetMask",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewSubnetMask";
	params[0].value=subnetmask;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[SubnetMask_index].value, subnetmask, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetSubnetMask",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_DomainName
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetDomainName.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_DomainName (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR domain_name[256];

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDomainName of LAN%d\n",local_index.lan_device_index);

	if(IGD_pii_get_lan_info(local_index.lan_device_index, sizeof(domain_name), NULL, NULL, domain_name))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDomainName action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetDomainName",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewDomainName";
	params[0].value=domain_name;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[DomainName_index].value, domain_name, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetDomainName",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_DNSServers
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetDNSServers.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_DNSServers (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR dns_server_list[256] = "";

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDNSServers of LAN%d\n",local_index.lan_device_index);

        // Returns a comma separated list of DNS servers
	if(IGD_pii_get_lan_dns_servers(local_index.lan_device_index, dns_server_list, sizeof(dns_server_list)))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetDNSServers action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetDNSServers",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewDNSServers";
	params[0].value=dns_server_list;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[DNSServers_index].value, dns_server_list, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetDNSServers",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_AddressRange
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetAddressRange.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_AddressRange (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[2];	
#define RANGE_ADDR_BUF_SZ 32
	CHAR min_address[RANGE_ADDR_BUF_SZ] = "";
	CHAR max_address[RANGE_ADDR_BUF_SZ] = "";

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetAddressRange of LAN%d\n",local_index.lan_device_index);

	if(IGD_pii_get_lan_addr_range(local_index.lan_device_index, RANGE_ADDR_BUF_SZ, min_address, max_address))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetAddressRange action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetAddressRange",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewMinAddress";
	params[0].value=min_address;
	params[1].name="NewMaxAddress";
	params[1].value=max_address;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[MinAddress_index].value, min_address, PAL_UPNP_LINE_SIZE);
	strncpy(event->service->state_variables[MaxAddress_index].value, min_address, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetAddressRange",LANHOSTCONFIGMGMT_SERVICE_TYPE,2,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_ReservedAddresses
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetReservedAddresses.  
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_ReservedAddresses (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[2];	
	CHAR reserved_list[256] = "";

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetReservedAddresses of LAN%d\n",local_index.lan_device_index);

        // Returns a comma seperated list of DHCP address reserved (static dhcp reservation)
	if(IGD_pii_get_lan_reserved_addr_list(local_index.lan_device_index, reserved_list, sizeof(reserved_list)))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetReservedAddresses action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetReservedAddresses",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewReservedAddresses";
	params[0].value=reserved_list;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[ReservedAddresses_index].value, reserved_list, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetReservedAddresses",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_IPRoutersList
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function do the action of GetIPRoutersList.  
*         aka DefaultGateway
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_IPRoutersList (INOUT struct action_event *event)
{
	struct device_and_service_index local_index;
	pal_string_pair params[1];	
	CHAR ipaddr[64];

	local_index = *((struct device_and_service_index*)event->service->private);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetIPRoutersList of LAN%d\n",local_index.lan_device_index);

        // to be enhanced when Auto-Bridging feature is available
	if(IGD_pii_get_lan_info(local_index.lan_device_index, sizeof(ipaddr), ipaddr, NULL, NULL))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetIPRoutersList action:Action fail\n");
		strncpy(event->request->error_str, "Action Fail,get status fail",PAL_UPNP_LINE_SIZE);
		event->request->error_code = 501;
		PAL_upnp_make_action(&event->request->action_result,"GetIPRoutersList",LANHOSTCONFIGMGMT_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		return(event->request->error_code);
	}

	params[0].name="NewIPRouters";
	params[0].value=ipaddr;
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	strncpy(event->service->state_variables[IPRouters_index].value, ipaddr, PAL_UPNP_LINE_SIZE);
	PAL_upnp_make_action(&event->request->action_result,"GetIPRoutersList",LANHOSTCONFIGMGMT_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_nope_success
*  Parameters: 
*	   event:   INOUT. the action_event struct of the action. 
*  Description:
*	  This function generic function to implement an action 
*         that is not explicitly implemented for various reasons
*         including some insecure methods like LANDevice set methods
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_nope_success (INOUT struct action_event *event)
{
	event->request->error_code = PAL_UPNP_E_SUCCESS;
	return(event->request->error_code);
}
