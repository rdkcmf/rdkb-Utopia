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

/*
 *    FileName:    igd_platform_independent_inf.c
 *      Author:    Andy Liu(zhihliu@cisco.com) Tao Hong(tahong@cisco.com)
 *                 Jianrong(jianxiao@cisco.com)Lipin Zhou(zlipin@cisco.com)	
 *        Date:    2009-05-03
 * Description:    Implementation including all Product-related functions
 *****************************************************************************/
/*$Id: igd_platform_independent_inf.c,v 1.15 2009/05/27 03:18:47 zlipin Exp $
 *
 *$Log: igd_platform_independent_inf.c,v $
 *Revision 1.15  2009/05/27 03:18:47  zlipin
 *portmapping updated.
 *
 *Revision 1.14  2009/05/27 03:08:43  tahong
 *delete stub code in
 *IGD_pii_request_connection() and IGD_pii_force_termination()
 *
 *Revision 1.13  2009/05/26 09:59:55  zhangli
 *Completed the cleanup activity
 *
 *Revision 1.11  2009/05/22 05:43:49  zlipin
 *Get rid of two useless function.
 *
 *Revision 1.10  2009/05/22 05:39:13  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.9  2009/05/21 07:58:27  zhihliu
 *update PII interface
 *
 *Revision 1.8  2009/05/21 06:26:51  jianxiao
 *add IGD_pii_get_wan_device_number/IGD_pii_get_wan_connection_device_number interface
 *
 *Revision 1.7  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.5  2009/05/14 02:39:57  jianxiao
 *Modify the interface of PAL_xml_node_GetFirstbyName
 *
 *Revision 1.4  2009/05/14 02:07:49  tahong
 *in the comment, "pii.c"===> "igd_platform_independent_inf.c"
 *
 *Revision 1.3  2009/05/14 02:05:03  jianxiao
 *Add the function IGD_pii_get_uuid
 *
 *Revision 1.2  2009/05/14 01:45:35  jianxiao
 *Add the functions for WANCommonInterfaceConfig and WANEthernetLinkConfig
 *
 *Revision 1.1  2009/05/13 08:57:08  tahong
 *create orignal version
 *
 *
 **/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <utctx/utctx_api.h>
#include <utapi/utapi.h>
#include <ccsp_syslog.h>
#include "syscfg/syscfg.h"

#include "pal_log.h"
#include "igd_platform_independent_inf.h"

int Utopia_UpdateDynPortMapping_WithoutFirewallRestart (int index, portMapDyn_t *pmap);
/************************************************************
 * Function: IGD_pii_get_serial_number
 *
 *  Parameters: 
 *		NONE
 * 
 *  Description:
 *	  Get the serial number of the your product.
 *	  It will be used in the description file of the IGD device
 *
 *  Return Values: CHAR*
 *	   The serial number of the IGD. NULL if failure.
 ************************************************************/
CHAR* IGD_pii_get_serial_number(VOID)
{
    static char prodSn[128] = {'\0'};
    /* TODO: to be implemented by OEM
	ProductionDb_RetrieveAccess();
    ProdDb_GetSerialNumber(prodSn);*/
    return prodSn;
	//return "123456789001";
}

/************************************************************
 * Function: IGD_pii_get_uuid
 *
 *  Parameters: 
 *	  uuid: OUT. The UUID of one new device.
 * 	
 *  Description:
 *	  Get the UUID for one new device. 
 *	   
 *	  According to the UNnP spec, the different device MUST have the different 
 *	  UUID. Our IGD stack will call this function to get one new UUID when 
 *	  create one new device. That means, this function MUST return the different 
 *	  UUID when it is called every time. And one method to create UUID is provided 
 *	  in the "igd_platform_independent_inf.c". 
 *	 
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/
// One method to create UUID for the different device
// It uses the MAC address of the interface that UNnP IGD run on as the input

#define PII_MAC_ADDRESS_LEN	6
#define PII_IF_NAME_LEN	16
#define WAN_UUID_INDEX_NUM 26
LOCAL INT32 _pii_get_if_MacAddress(IN const CHAR *ifName, INOUT CHAR MacAddress[PII_MAC_ADDRESS_LEN])
{
  	struct ifreq ifr;
  	INT32 fd;
  	INT32 ret = -1;

  	if(NULL == ifName)
    	return ret;

  	if((fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
   		return ret;
  
  	if(fd >= 0 )
  	{
	/* CID 135603 : BUFFER_SIZE_WARNING */
        strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name)-1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    	ifr.ifr_addr.sa_family = AF_INET;
    	if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
    	{
      		memcpy(MacAddress, &ifr.ifr_ifru.ifru_hwaddr.sa_data, PII_MAC_ADDRESS_LEN);
      		ret = 0;
    	}
  	}

  	close(fd);
  	return ret;
}

INT32 IGD_pii_get_uuid(INOUT CHAR *uuid)
{
	LOCAL CHAR base_uuid[UPNP_UUID_LEN_BY_VENDER];
	CHAR uuid_mac_part[PII_MAC_ADDRESS_LEN];
	LOCAL BOOL get_global_uuid_once=BOOL_FALSE;
	UtopiaContext utctx;
	char igd_upnp_interface[10];

	if(uuid == NULL)
		return -1;

	if(!get_global_uuid_once)
	{
		Utopia_Init(&utctx);
		Utopia_RawGet(&utctx,NULL,"lan_ifname",igd_upnp_interface,sizeof(igd_upnp_interface));
        Utopia_Free(&utctx, FALSE);
		if(_pii_get_if_MacAddress(igd_upnp_interface,uuid_mac_part))
		{
			printf("PII get MAC fail\n");
			return -1;
		}
		snprintf(base_uuid,UPNP_UUID_LEN_BY_VENDER,"uuid:ebf5a0a0-1dd1-11b2-a90f-%02x%02x%02x%02x%02x%02x",
								(UINT8)uuid_mac_part[0],(UINT8)uuid_mac_part[1],
								(UINT8)uuid_mac_part[2],(UINT8)uuid_mac_part[3],
								(UINT8)uuid_mac_part[4],(UINT8)uuid_mac_part[5]);
		get_global_uuid_once = BOOL_TRUE;
	}
	else
	{
		base_uuid[WAN_UUID_INDEX_NUM] = base_uuid[WAN_UUID_INDEX_NUM]+1;
		if(base_uuid[WAN_UUID_INDEX_NUM]>'f')
			base_uuid[WAN_UUID_INDEX_NUM]='1';
	}
	strncpy(uuid,base_uuid, UPNP_UUID_LEN_BY_VENDER);
	return 0;
}

/************************************************************
 * Function: IGD_pii_get_wan_device_number
 *
 *  Parameters: 
 *		NONE
 * 
 *  Description:
 *	  Get the instance number of the WANDevice in IGD device
 *
 *  Return Values: INT32
 *	  The instance number of the WAN device. -1 if failure.
 ************************************************************/
INT32 IGD_pii_get_wan_device_number(VOID)
{
	return 1;
}

/************************************************************
 * Function: IGD_pii_get_lan_device_number
 *
 *  Parameters: 
 *		NONE
 * 
 *  Description:
 *	  Get the instance number of the LANDevice in IGD device
 *
 *  Return Values: INT32
 *	  The instance number of the LAN device. -1 if failure.
 ************************************************************/
INT32 IGD_pii_get_lan_device_number(VOID)
{
	return 1;
}

/************************************************************
 * Function: IGD_pii_get_wan_connection_device_number
 *
 *  Parameters: 
 *    WanDeviceIndex:  IN. Index of WANDevice, range:1-Number of WANDevice
 * 
 *  Description:
 *	  Get the instance number of the WANConnectionDevice
 *	  in one WANDevice specified by the input device index.
 *
 *  Return Values: INT32
 *	   The instance number of the WANConnectionDevice. -1 if failure.
 ************************************************************/
INT32 IGD_pii_get_wan_connection_device_number(IN INT32 wan_device_index)
{
	(void) wan_device_index;

	return 1;
}

/************************************************************
 * Function: IGD_pii_get_wan_ppp_service_number
 *
 *  Parameters:
 *      WanDeviceIndex:            IN. Index of WANDevice, range:1-Number of WANDevice
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice
 *
 *  Description:
 *	  Get the instance number of the WANPPPConnectionService 
 *	  in one WANConnectionDevice specified by the input device index
 *
 *  Return Values: INT32
 *    The instance number of WANPPPConnectionService, -1 if failure.
 ************************************************************/   
INT32 IGD_pii_get_wan_ppp_service_number(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex)
{
	(void) WanDeviceIndex;
	(void) WanConnectionDeviceIndex;

	return 0;    /* for USGv2 no PPP connection */
}	

/************************************************************
 * Function: IGD_pii_get_wan_ip_service_number
 *
 *  Parameters:
 *      WanDeviceIndex:            IN. Index of WANDevice, range:1-Number of WANDevice
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice
 *
 *  Description:
 *	  Get the instance number of the WANIPConnectionService 
 *	  in one WANConnectionDevice specified by the input device index
 *
 *  Return Values: INT32
 *    The instance number of WANIPConnectionService, -1 if failure.
 ************************************************************/ 									
INT32 IGD_pii_get_wan_ip_service_number(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex)
{
	(void) WanDeviceIndex;
	(void) WanConnectionDeviceIndex;

	return 1;
}

/************************************************************
 * Function: IGD_pii_get_possible_connection_types
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice.
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService                                                minimum value is 1.
 *      ServiceType:                         IN. Type of WANXXXXConnection. 
 *      ConnectionTypesList:             OUT. List of possible connection types, a comma-separated
 *                                                  string.One example for WANIPConnection is "Unconfigured,IP_Routed,IP_Bridged".
 *
 *  Description:
 *      Get the list of possbile connection types of one WAN(IP/PPP)ConnectionService 
 *      specified by the input device index and service type
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/ 
LOCAL CHAR ipconntype[32] = {0};

#define IPCONNTYPELIST "Unconfigured,IP_Routed"

INT32 IGD_pii_get_possible_connection_types(IN INT32 WanDeviceIndex,
                                                                                        IN INT32 WanConnectionDeviceIndex,
                                                                                        IN INT32 WanConnectionServiceIndex,
                                                                                        IN INT32 ServiceType,
                                                                                        OUT CHAR *ConnectionTypesList)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    if (ConnectionTypesList) {
        strcpy(ConnectionTypesList, IPCONNTYPELIST);
    }

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_connection_status
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN connection service.
 *      ConnectionType:                      OUT. Current connection status.
 *
 *  Description:
 *      Get the current connection status of one WAN(IP/PPP)ConnectionService 
 *      specified by the input device index and service type 
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService      
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/
INT32 IGD_pii_get_connection_status(IN INT32 WanDeviceIndex,
                                                                        IN INT32 WanConnectionDeviceIndex,
                                                                        IN INT32 WanConnectionServiceIndex,
                                                                        IN INT32 ServiceType,
                                                                        OUT CHAR *ConnectionStatus)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    /*
     * TODO: verify if these requests need to throttled to 
     *       avoid too many sysevent requests?
     */

    wanConnectionStatus_t wan;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    bzero(&wan, sizeof(wanConnectionStatus_t));

    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetWANConnectionStatus(&ctx, &wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan connection info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    switch (wan.status) {
    case WAN_CONNECTED:
        strcpy(ConnectionStatus, "Connected");
        break;
    case WAN_CONNECTING:
        strcpy(ConnectionStatus, "Connecting");
        break;
    case WAN_DISCONNECTING:
        strcpy(ConnectionStatus, "Disconnecting");
        break;
    case WAN_DISCONNECTED:
        strcpy(ConnectionStatus, "Disconnected");
        break;
    default:
        strcpy(ConnectionStatus, "Unconfigured");
    }

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_connection_type
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanWAN(IP/PPP)ConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN connection service.
 *      ConnectionType:                      OUT. Current connection type.
 *
 *  Description:
 *      Get the current connection type of one WAN(IP/PPP)ConnectionService 
 *      specified by the input device index and service type
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService    
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/										  
INT32 IGD_pii_get_connection_type(IN INT32 WanDeviceIndex,
                                                                    IN INT32 WanConnectionDeviceIndex,
                                                                    IN INT32 WanConnectionServiceIndex,
                                                                    IN INT32 ServiceType,
                                                                    OUT CHAR *ConnectionType)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    strcpy(ConnectionType, "IP_Routed");
    return 0;
}
		
/************************************************************
 * Function: IGD_pii_set_connection_type
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      ConnectionType:                      IN. The connection type that will be set.
 *
 *  Description:
 *      Set the current connection type of one WAN(IP/PPP)connectionService  
 *      specified by the input device index and service type 
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService 
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/								 
INT32 IGD_pii_set_connection_type(IN INT32 WanDeviceIndex,
                                                                    IN INT32 WanConnectionDeviceIndex,
                                                                    IN INT32 WanConnectionServiceIndex,
                                                                    IN INT32 ServiceType,
                                                                    IN CHAR *ConnType)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    strcpy(ipconntype, ConnType);

    return 0;
}	

/************************************************************
 * Function: IGD_pii_request_connection
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      ConnectionType:                      IN. The connection type that will be set.
 *
 *  Description:
 *      Request to initiate the connection of WAN(IP/PPP)connectionService 
 *      specified by the input device index and service type 
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/									
INT32 IGD_pii_request_connection(IN INT32 WanDeviceIndex,
                                                                IN INT32 WanConnectionDeviceIndex,
                                                                IN INT32 WanConnectionServiceIndex,
                                                                IN INT32 ServiceType)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    return 0;
}	

/************************************************************
 * Function: IGD_pii_force_termination
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      ConnectionType:                      IN. The connection type that will be set.
 *
 *  Description:
 *     Force to terminate the connection of WAN(IP/PPP)connectionService  
 *     specified by the input device index and service type 
 *     Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService 
 *      
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/									 
INT32 IGD_pii_force_termination(IN INT32 WanDeviceIndex,
                                                            IN INT32 WanConnectionDeviceIndex,
                                                            IN INT32 WanConnectionServiceIndex,
                                                            IN INT32 ServiceType)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    UtopiaContext ctx;
    int terminate_allowed = 0;

    if (Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Lock acquired ", __FUNCTION__);
        terminate_allowed = Utopia_IGDInternetDisbleAllowed(&ctx);
        Utopia_Free(&ctx, 0);
        PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
    }

    if (!terminate_allowed) {
        PAL_LOG("igd_platform", "debug", "%s: IGD force-termination is not allowed, return action error", __FUNCTION__);
        return 1;
    }

    if (SUCCESS != Utopia_WANConnectionTerminate()) {
        PAL_LOG("igd_platform", "debug", "%s: Error terminating wan connection ", __FUNCTION__);
        return 1;
    }

    return 0;
}										 

/************************************************************
 * Function: IGD_pii_get_external_ip
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      ExternalIp:                            OUT. External IP address in string format.
 *
 *  Description:
 *      Get current external IP address used by NAT for the connection of WAN(IP/PPP)connectionService 
 *      specified by the input device index and service type        
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService 
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_external_ip(IN INT32 WanDeviceIndex,
                                                        IN INT32 WanConnectionDeviceIndex,
                                                        IN INT32 WanConnectionServiceIndex,
                                                        IN INT32 ServiceType,
                                                        OUT CHAR *ExternalIp)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    wanConnectionStatus_t wan;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    bzero(&wan, sizeof(wanConnectionStatus_t));
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetWANConnectionStatus(&ctx, &wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan connection info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    strncpy(ExternalIp, wan.ip_address, IPV4_ADDR_LEN);

    return 0;
}


/************************************************************
 * Function: IGD_pii_get_link_layer_max_bitrate
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      UpRate:                                OUT. Maximum upstream bitrate, it has a static value once a connection is setup.
 *      DownRate:                            OUT. Maximum downstream bitrate, it has a static value once a connection is setup.
 *
 *  Description:
 *      Get the link layer maximum bitrates(upstream and downstream) for the connection of WAN(IP/PPP)connectionService 
 *      specified by the input device index and service type 
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService 
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_link_layer_max_bitrate(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex,
                                                                            IN INT32 WanConnectionServiceIndex,
                                                                            IN INT32 ServiceType,
                                                                            OUT CHAR *UpRate,
                                                                            OUT CHAR *DownRate)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    strcpy(UpRate, "10000000");
    strcpy(DownRate, "10000000");
    return 0;
}

/************************************************************
 * Function: IGD_pii_get_up_time
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      UpTime:                               OUT. The time in seconds that this connection has stayed up.
 *
 *  Description:
 *      Get the time in seconds that the connection has stayed up.
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_pii_get_up_time(IN INT32 WanDeviceIndex,
                                                    IN INT32 WanConnectionDeviceIndex,
                                                    IN INT32 WanConnectionServiceIndex,
                                                    IN INT32 ServiceType,
                                                    OUT CHAR *UpTime)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    wanConnectionStatus_t wan;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    bzero(&wan, sizeof(wanConnectionStatus_t));
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetWANConnectionStatus(&ctx, &wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan connection info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    sprintf(UpTime, "%ld", wan.uptime);

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_NAT_RSIP_status
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      NATEnable:                               OUT. Value=1(NAT is enabled) or 0(NAT is disabled)
 *      RSIPAvailable:                           OUT. Value=1(RSIP is supported) or 0(RSIP isn't supported)
 *       
 *  Description:
 *      Get the current state of NAT and RSIP for the connection of WAN(IP/PPP)connectionService 
 *      specified by the input device index and service type
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService    
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
 INT32 
IGD_pii_get_NAT_RSIP_status( IN INT32 WanDeviceIndex,
                        IN INT32 WanConnectionDeviceIndex,
                        IN INT32 WanConnectionServiceIndex, 
                        IN INT32 ServiceType,
                        OUT BOOL *natStatus, 
                        OUT BOOL *rsipStatus 
                    )
{
    *natStatus = BOOL_TRUE;
    *rsipStatus = BOOL_FALSE;
    boolean_t natEnable = BOOL_TRUE; /*RDKB-7142, CID-32964; init before use */

    
    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    UtopiaContext ctx;

    if (Utopia_Init(&ctx)) {
        Utopia_GetRouteNAT(&ctx, (napt_mode_t *)&natEnable);
        Utopia_Free(&ctx, 0);
    }

    *natStatus = (natEnable == 1) ? BOOL_TRUE : BOOL_FALSE;

    printf("IGD_pii_get_NAT_RSIP_status is called, %d and %d is returned.\n", *natStatus, *rsipStatus);
    printf("        interface:  %d-%d-%d-%d\n", WanDeviceIndex, WanConnectionDeviceIndex, WanConnectionServiceIndex, ServiceType);
    printf("        %d and %d is returned.\n", *natStatus, *rsipStatus);
    
    return 0;
}

/************************************************************
 * Function: IGD_pii_add_portmapping_entry
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      PortmappingEntry: 		           IN.  The portmapping entry to be added.
 * 
 *  Description:
 *     Add a new port mapping or overwrites an existing mapping with the same internal client.  
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService   
 *
 *
 *  Return Values: INT32
 *      0 if successful else error code.               
 ************************************************************/  
INT32 IGD_pii_add_portmapping_entry( IN INT32 WanDeviceIndex,
                                IN INT32 WanConnectionDeviceIndex,
                                IN INT32 WanConnectionServiceIndex, 
                                IN INT32 ServiceType,
                                IN PIGD_PortMapping_Entry portmapEntry
                    )
{
    syslog_systemlog("IGD", LOG_NOTICE, "Add Port mapping %s:%d to %s:%d",
                     portmapEntry->remoteHost, portmapEntry->externalPort,
                     portmapEntry->internalClient, portmapEntry->internalPort);

    PAL_LOG("igd_platform", "debug", "%s: desc %s, ext_port %d, int_port %d ", __FUNCTION__, portmapEntry->description, portmapEntry->externalPort, portmapEntry->internalPort);

    UtopiaContext ctx;
    int rc = 0;
#ifdef _HUB4_PRODUCT_REQ_
    char upnpEnabled[16] = {0};

    syscfg_get(NULL, "upnp_igd_enabled", upnpEnabled, sizeof(upnpEnabled));
    if (0 == strcmp("0", upnpEnabled)) {
        printf("UPnP Feature is not Enabled \n");
        return 1;
    }
#endif
    if (Utopia_Init(&ctx)) {
        int index;
        portMapDyn_t pmap;
        protocol_t proto = (0 == strcasecmp(portmapEntry->protocol, "TCP")) ? TCP : UDP;

        PAL_LOG("igd_platform", "debug", "%s: Lock acquired ", __FUNCTION__);

        if (!Utopia_IGDConfigAllowed(&ctx)) {
            PAL_LOG("igd_platform", "debug", "%s: IGD config disabled in administration, return action error", __FUNCTION__);
            Utopia_Free(&ctx, 0);
            PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
            return 1;
        }

        bzero(&pmap, sizeof(pmap));

        /*
         * check if entry already exist using (RemoteHost, ExternalPort, PortMappingProtocol) tuple
         */
        if (UT_SUCCESS == Utopia_FindDynPortMapping(portmapEntry->remoteHost, 
                                                    portmapEntry->externalPort,
                                                    proto,
                                                    &pmap, &index)) {
            /*
             * if for same internal client, update leasttime and return success
             */
            if (0 == strcmp(portmapEntry->internalClient, pmap.internal_host)) {

                if (portmapEntry->description != NULL) {
                    strncpy(pmap.name, portmapEntry->description, sizeof(pmap.name));
                }
		
                pmap.lease = portmapEntry->leaseTime;

	        if (( portmapEntry->internalPort == pmap.internal_port ) && ( pmap.enabled == (boolean_t) portmapEntry->enabled ))
	        {
	            printf("Internal port is also same, no need to restart firewall\n");
		    (void) Utopia_UpdateDynPortMapping_WithoutFirewallRestart(index, &pmap);
	        } 
	        else
	        {
	            printf("Internal port/ enabled status is different, update dyn port event. Need to restart firewall\n");
	            pmap.enabled = (boolean_t) portmapEntry->enabled;
                    pmap.internal_port = portmapEntry->internalPort;
                    (void) Utopia_UpdateDynPortMapping(index, &pmap);
		}

                rc = 0;
            } else {
                /*
                 * if for different internal client, return error
                 */
                PAL_LOG("igd_platform", "debug", "%s: entry exists for different internal client (error)", __FUNCTION__);
                //rc = 1;
                rc = ERROR_CONFLICT_FOR_MAPPING_ENTRY;
            }
        } else {
            /*
             * Create new entry
             *   for unique ([remote-host], external-port, protocol)
             */
            pmap.enabled = (boolean_t) portmapEntry->enabled;
            if (portmapEntry->description != NULL) {
                strncpy(pmap.name, portmapEntry->description, sizeof(pmap.name));
            }
            pmap.external_port = portmapEntry->externalPort;
            if (portmapEntry->remoteHost != NULL) {
                strncpy(pmap.external_host, portmapEntry->remoteHost, sizeof(pmap.external_host)); 
            }
            pmap.internal_port = portmapEntry->internalPort;
            if (portmapEntry->internalClient != NULL) {
                strncpy(pmap.internal_host, portmapEntry->internalClient, sizeof(pmap.internal_host)); 
            }
            pmap.lease = portmapEntry->leaseTime;
            pmap.protocol = proto;
    
            int st = Utopia_AddDynPortMapping(&pmap);
            if (UT_SUCCESS == st) {
                PAL_LOG("igd_platform", "debug", "%s: successfully added port map entry", __FUNCTION__);
                rc = 0;
            } else {
                PAL_LOG("igd_platform", "debug", "%s: Error, adding port map entry", __FUNCTION__);
                rc = 1; 
            }
        }
        Utopia_Free(&ctx, 0);
        PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
    }

    return rc;
}

/************************************************************
 * Function: IGD_pii_del_portmapping_entry
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
  *     RemoteHost: 		                  IN. Remote host.
 *      ExternalPort: 	                  IN.  External port.
 *      Protocol: 		                  IN.  PortMapping protocol.
 * 
 *  Description:
 *     Delete a previously instantiated port mapping.
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService   
 *
 *  Return Values: INT32
 *      0 if successful else error code.               
 ************************************************************/     
INT32 IGD_pii_del_portmapping_entry( IN INT32 WanDeviceIndex,
                        IN INT32 WanConnectionDeviceIndex,
                        IN INT32 WanConnectionServiceIndex, 
                        IN INT32 ServiceType,
                        IN CHAR *RemoteHost,
                        IN UINT16  ExternalPort,
                        IN CHAR    *Protocol
                    )
{
    syslog_systemlog("IGD", LOG_NOTICE, "Delete Port mapping %s:%d", RemoteHost, ExternalPort);

    PAL_LOG("igd_platform", "debug", "%s: remote-host %s, ext_port %d, protocol %s ", __FUNCTION__, RemoteHost, ExternalPort, Protocol);

    UtopiaContext ctx;
    int st, rc = 1;

    if (Utopia_Init(&ctx)) {
        portMapDyn_t portmap;

        bzero(&portmap, sizeof(portmap));

        PAL_LOG("igd_platform", "debug", "%s: Lock acquired ", __FUNCTION__);

        portmap.external_port = ExternalPort;
        portmap.protocol = (0 == strcasecmp(Protocol, "TCP")) ? TCP : UDP;
        if (RemoteHost) {
            strncpy(portmap.external_host, RemoteHost, sizeof(portmap.external_host));
        }

        st = Utopia_DeleteDynPortMapping(&portmap);
        if (UT_SUCCESS == st) {
            PAL_LOG("igd_platform", "debug", "%s: successfully deleted port map entry", __FUNCTION__);
            rc = 0;
        } else {
            PAL_LOG("igd_platform", "debug", "%s: failed to delete port map entry", __FUNCTION__);
            rc = 1;
        }

        Utopia_Free(&ctx, 0);
        PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
    }

    return rc;
}


 /************************************************************
 * Function: IGD_pii_get_portmapping_entry_num
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:                         IN. Type of WAN(IP/PPP)connectionService.
 *      PortmappingEntryNum: 		                  OUT.  The total number of the PortMapping entry .
 * 
 *  Description:
 *     Get the total number of the PortMapping entry.
 *      Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService       
 *  Return Values: INT32
 *      0 if successful else error code.               
 ************************************************************/          	
INT32 IGD_pii_get_portmapping_entry_num(IN INT32 WanDeviceIndex,
                                        IN INT32 WanConnectionDeviceIndex,
                                        IN INT32 WanConnectionServiceIndex, 
                                        IN INT32 ServiceType,
                                        OUT INT32 *PortmappingEntryNum)
{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;
    (void) WanConnectionServiceIndex;
    (void) ServiceType;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    int totalEntryNum = 0;

    Utopia_GetDynPortMappingCount(&totalEntryNum);
    // PAL_LOG("igd_platform", "debug", "%s: count = %d", __FUNCTION__, totalEntryNum);

    if(PortmappingEntryNum != NULL)
        (*PortmappingEntryNum) = totalEntryNum;

    /*
     * Called roughly once per-second to re-validate existing entries
     */
    /* Register an independent thread on the timer list */
    /*Utopia_InvalidateDynPortMappings();*/

    return 0;
}                                        
                                        
/************************************************************
 * Function: IGD_pii_get_portmapping_entry_generic
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:				IN.  Type of WAN(IP/PPP)connectionService.
 *      PortmappingIndex:		IN. The index of the portmapping entry. Value range: 0-PortmappingEntryNum
 *      PortmappingEntry:		OUT. The portmapping entry.
 * 
 *  Description:
 *     Get one portmapping entry specified by the input index.
 *     Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService   
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/ 										                         	
INT32 IGD_pii_get_portmapping_entry_generic( IN INT32 WanDeviceIndex,
                                IN INT32 WanConnectionDeviceIndex,
                                IN INT32 WanConnectionServiceIndex, 
                                IN INT32 ServiceType,
                                IN INT32 PortmappingIndex,
                                OUT PIGD_PortMapping_Entry PortmappingEntry)
{ 
    printf("IGD_pii_get_portmapping_entry_generic is called.\n");
    printf("        interface:  %d-%d-%d-%d\n", WanDeviceIndex, WanConnectionDeviceIndex, WanConnectionServiceIndex, ServiceType);
    printf("        PortmappingIndex:  %d\n", PortmappingIndex);

    PAL_LOG("igd_platform", "debug", "%s: for index %d", __FUNCTION__, PortmappingIndex);

    /*
     * IGD array runs from 0 to PortMappingNumberOfEntries - 1
     * Utopia API entry run from 1 to PortMappingNumberOfEntries
     * .. so match up accordingly
     */
    UtopiaContext ctx;
    int rc = 0;

    if (Utopia_Init(&ctx)) {
        portMapDyn_t portmap;
        int count = 0, st;

        PAL_LOG("igd_platform", "debug", "%s: Lock acquired ", __FUNCTION__);
        if (UT_SUCCESS != Utopia_GetDynPortMappingCount(&count)) {
            PAL_LOG("igd_platform", "debug", "%s: Lock released 1", __FUNCTION__);
            Utopia_Free(&ctx, 0);
            return 1;
        }
        if (PortmappingIndex < 0 || PortmappingIndex >= count) {
            PAL_LOG("igd_platform", "debug", "%s: Lock released 2", __FUNCTION__);
            Utopia_Free(&ctx, 0);
            return 1;
        }
        bzero(&portmap, sizeof(portMapDyn_t));
        if (UT_SUCCESS != (st = Utopia_GetDynPortMapping(PortmappingIndex+1, &portmap))) {
            PAL_LOG("igd_platform", "debug", "%s: Utopia_GetDynPortMapping failed (rc=%d)", __FUNCTION__, st);
            PAL_LOG("igd_platform", "debug", "%s: Lock released 3", __FUNCTION__);
            Utopia_Free(&ctx, 0);
            return 1;
        }

        PortmappingEntry->enabled = portmap.enabled;
        strncpy(PortmappingEntry->description, portmap.name, PORT_MAP_DESCRIPTION_LEN);
        PortmappingEntry->leaseTime = portmap.lease;       
        if (portmap.protocol == TCP) {
            strcpy(PortmappingEntry->protocol, "TCP");
        } else {
            strcpy(PortmappingEntry->protocol, "UDP");
        } 
        PortmappingEntry->externalPort = portmap.external_port;
        strcpy(PortmappingEntry->remoteHost, portmap.external_host); 
        PortmappingEntry->internalPort = portmap.internal_port;
        strcpy(PortmappingEntry->internalClient, portmap.internal_host); 

        PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
        Utopia_Free(&ctx, 0);
    }

    return rc;
}                                
                                
/************************************************************
 * Function: IGD_pii_get_portmapping_entry_specific
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *      WanConnectionServiceIndex: IN. Index of WAN(IP/PPP)ConnectionService,range:1-Number of WAN(IP/PPP)ConnectionService
 *      ServiceType:				IN.  Type of WAN(IP/PPP)connectionService.
 *      PortmappingEntry:		INOUT. The portmapping entry.
 * 
 *  Description:
 *     Get one portmapping entry specified by the unique tuple of 
 *     RemoteHost,ExteralPort and Protocol in the input parameter,PortmappingEntry
 *      
 *     Related UPnP Device/Service:  WAN(IP/PPP)ConnectionService   
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/ 										                         	
INT32 IGD_pii_get_portmapping_entry_specific( IN INT32 WanDeviceIndex,
                                IN INT32 WanConnectionDeviceIndex,
                                IN INT32 WanConnectionServiceIndex, 
                                IN INT32 ServiceType,
                                INOUT PIGD_PortMapping_Entry PortmappingEntry)
{ 
    printf("IGD_pii_get_portmapping_entry_specific is called.\n");
    printf("        interface:  %d-%d-%d-%d\n", WanDeviceIndex, WanConnectionDeviceIndex, WanConnectionServiceIndex, ServiceType);
    printf("        Remote Host: %s\n", PortmappingEntry->remoteHost);
    printf("        External Port: %d\n", PortmappingEntry->externalPort);
    printf("        PortMapping Protocol: %s\n", PortmappingEntry->protocol);

    PAL_LOG("igd_platform", "debug", "%s: Remote Host: %s, External Port: %d, PortMapping Protocol: %s\n", __FUNCTION__, PortmappingEntry->remoteHost, PortmappingEntry->externalPort, PortmappingEntry->protocol);

    /*
     * IGD array runs from 0 to PortMappingNumberOfEntries - 1
     * syscfg entry run from 1 to PortMappingNumberOfEntries
     * .. so match up accordingly
     *
     */
    UtopiaContext ctx;
    int rc = 1; 

    if (Utopia_Init(&ctx)) {
        int index;
        portMapDyn_t pmap;

        PAL_LOG("igd_platform", "debug", "%s: Lock acquired ", __FUNCTION__);

        bzero(&pmap, sizeof(pmap));

        /*
         * check for entry using (RemoteHost, ExternalPort, PortMappingProtocol) tuple
         */
        protocol_t proto = (0 == strcasecmp(PortmappingEntry->protocol, "TCP")) ? TCP : UDP;
        if (UT_SUCCESS == Utopia_FindDynPortMapping(PortmappingEntry->remoteHost, 
                                                    PortmappingEntry->externalPort,
                                                    proto,
                                                    &pmap, &index)) {
            PortmappingEntry->enabled = pmap.enabled;
            strncpy(PortmappingEntry->description, pmap.name, PORT_MAP_DESCRIPTION_LEN);
            PortmappingEntry->leaseTime = pmap.lease;       
            PortmappingEntry->internalPort = pmap.internal_port;
            strcpy(PortmappingEntry->internalClient, pmap.internal_host); 

            rc = 0;
        } else {
            PAL_LOG("igd_platform", "debug", "%s: couldn't find entry", __FUNCTION__);
            rc = 1;
        }
        Utopia_Free(&ctx, 0);
        PAL_LOG("igd_platform", "debug", "%s: Lock released ", __FUNCTION__);
    }

    return rc;
}								   

/************************************************************
 * Function: IGD_pii_get_ethernet_link_status
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *      WanConnectionDeviceIndex:  IN. Index of WANConnectionDevice, range:1-Number of WANConnectionDevice..
 *	    EthernetLinkStatus:   OUT. The status of the WNA Ethernet link.
 *
 *  Description:
 *	    Get the link status of the Ethernet connection specified by the input device index  
 *      Related UPnP Device/Service:  WANEthernetLinkConfigService
 *       
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/                                                            
INT32 IGD_pii_get_ethernet_link_status(IN INT32 WanDeviceIndex,
													IN INT32 WanConnectionDeviceIndex,
													OUT CHAR *EthernetLinkStatus)

{
    (void) WanDeviceIndex;
    (void) WanConnectionDeviceIndex;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    wanConnectionStatus_t wan;

    bzero(&wan, sizeof(wanConnectionStatus_t));
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetWANConnectionStatus(&ctx, &wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan connection info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    if (0 != wan.phylink_up) {
	strncpy(EthernetLinkStatus,ETHERNETLINKSTATUS_UP,16);
    } else {
	strncpy(EthernetLinkStatus,ETHERNETLINKSTATUS_DOWN,16);
    }

    return 0;
}
/************************************************************
 * Function: IGD_pii_get_common_link_properties
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *	    WanAccessType:   OUT. The type of the WAN access.
 *	    Layer1UpstreamMaxBitRate:   OUT. The MAX upstream theoretical bit rate(in bit/s) for the WAN device. 
 *	    Layer1DownstreamMaxBitRate:   OUT. The MAX downstream theoretical bit rate(in bit/s) for the WAN device.
 *	    PhyscialLinkStatus:   OUT. The state of the physical connection(link) from WANDevice to a connected entity.
 * 
 *  Description:
 *	  Get the common link properties of the WAN device specified by the input device index.
 *    Related UPnP Device/Service:  WANCommonInterfaceConfigService 
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_common_link_properties(IN INT32 WanDeviceIndex,
														OUT CHAR *WanAccessType,
														OUT CHAR *Layer1UpstreamMaxBitRate,
														OUT CHAR *Layer1DownstreamMaxBitRate,
														OUT CHAR *PhyscialLinkStatus)
{
    (void) WanDeviceIndex;

    // PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    wanConnectionStatus_t wan;

    bzero(&wan, sizeof(wanConnectionStatus_t));
    UtopiaContext ctx;

    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetWANConnectionStatus(&ctx, &wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan connection info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    strncpy(WanAccessType,WANACCESSTYPE_ETHERNET,16);
    strncpy(Layer1UpstreamMaxBitRate,"100000000",16);
    strncpy(Layer1DownstreamMaxBitRate,"100000000",16);
    if (0 != wan.phylink_up) {
        strncpy(PhyscialLinkStatus,LINKSTATUS_UP,16);
    } else {
        strncpy(PhyscialLinkStatus,LINKSTATUS_DOWN,16);
    }

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_traffic_stats
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
 *	    WanAccessType:   IN. The type of the WAN access.
 *	    bufsz:           IN. size of output buffer (same for all four params)
 *	    TotalBytesSent:  OUT. Total bytes sent on the WAN device. 
 *	    TotalBytesReceived:  OUT. Total bytes received on the WAN device. 
 *	    TotalPacketsSent:  OUT. Total packets sent on the WAN device. 
 *	    TotalPacketsReceived:  OUT. Total packets received on the WAN device. 
 * 
 *  Description:
 *	  Get the traffice statistics of the WAN device specified by the input device index.
 *    Related UPnP Device/Service:  WANCommonInterfaceConfigService 
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_traffic_stats(IN INT32 WanDeviceIndex,
				IN INT32 bufsz,
				OUT CHAR *TotalBytesSent,
				OUT CHAR *TotalBytesReceived,
				OUT CHAR *TotalPacketsSent,
				OUT CHAR *TotalPacketsReceived)
{
    (void) WanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    wanTrafficInfo_t wan;

    bzero(&wan, sizeof(wanTrafficInfo_t));

    if (UT_SUCCESS != Utopia_GetWANTrafficInfo(&wan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting wan traffic statistics", __FUNCTION__);
        return 1;
    }

    if (TotalBytesSent) {
        snprintf(TotalBytesSent, bufsz, "%d", (unsigned int) wan.bytes_sent);
    }
    if (TotalBytesReceived) {
        snprintf(TotalBytesReceived, bufsz, "%d", (unsigned int) wan.bytes_rcvd);
    }
    if (TotalPacketsSent) {
        snprintf(TotalPacketsSent, bufsz, "%d", (unsigned int) wan.pkts_sent);
    }
    if (TotalPacketsReceived) {
        snprintf(TotalPacketsReceived, bufsz, "%d", (unsigned int) wan.pkts_rcvd);
    }

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_lan_dhcpserver_configurable
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    status:              OUT. status 
 * 
 *  Description:
 *    It is security violation to allow DHCP Server to be configurable using UPnP IGD
 *    currently there is no authentication to protect DHCP server set methods. 
 *    hence return NOT configurable
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_lan_dhcpserver_configurable(IN INT32 LanDeviceIndex, OUT CHAR *status)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);
    strcpy(status, "0");
    return 0;
}

/************************************************************
 * Function: IGD_pii_get_lan_dhcp_relay
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    status:              OUT. status 
 * 
 *  Description:
 *      Checks if we are in bridge mode, if yes return 1
 *             if we are in router mode return 0
 *      to be enhanced as part of LAN Auto-Bridging feature
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_lan_dhcp_relay_status(IN INT32 LanDeviceIndex, OUT CHAR *status)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);
    strcpy(status, "0");
    return 0;
}

/************************************************************
 * Function: IGD_pii_get_lan_info
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    bufsz:              IN. buffer size of OUT params (they are all need to be same size) 
 *	    ipaddr:              OUT. IP address of the LAN device 
 *	    subnet_mask:         OUT. subnet mask address of the device 
 *	    domai_name:          OUT. domain name of the device 
 * 
 *  Description:
 *      Returns various LAN Device settings
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/   
INT32 IGD_pii_get_lan_info(IN INT32 LanDeviceIndex, IN INT32 bufsz, OUT CHAR *ipaddr, OUT CHAR *subnet_mask, OUT CHAR *domain_name)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    lanSetting_t lan;
    bzero(&lan, sizeof(lanSetting_t));

    UtopiaContext ctx;
    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetLanSettings(&ctx, &lan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    if (ipaddr) {
        strncpy(ipaddr, lan.ipaddr, bufsz);
    }
    if (subnet_mask) {
        strncpy(subnet_mask, lan.netmask, bufsz);
    }
    if (domain_name) {
        strncpy(domain_name, lan.domain, bufsz);
    }
    return 0;
}

/************************************************************
 * Function: IGD_pii_get_lan_dns_servers
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    max_list_sz:              IN. buffer size of OUT params (they are all need to be same size) 
 *	    dns_servers:          OUT. comma separated list of dns servers
 * 
 *  Description:
 *      Returns various LAN DNS Servers
 *      Currently system uses router's dns proxy as the LAN's dns server,
 *      so just return LAN default gw address as the DNS server address
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/
INT32 IGD_pii_get_lan_dns_servers(IN INT32 LanDeviceIndex, OUT CHAR *dns_servers, IN INT32 max_list_sz)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    lanSetting_t lan;
    bzero(&lan, sizeof(lanSetting_t));

    UtopiaContext ctx;
    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetLanSettings(&ctx, &lan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    if (dns_servers) {
        strncpy(dns_servers, lan.ipaddr, max_list_sz);
    }

    return 0;
}

/************************************************************
 * Function: IGD_pii_get_lan_addr_range
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    buf_sz:              IN. buffer size of OUT params (they are all need to be same size) 
 *	    min_address:         OUT. start address of the range
 *	    max_address:         OUT. end address of the range
 * 
 *  Description:
 *      Returns various LAN DHCP Server's DHCP address range
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/
INT32 IGD_pii_get_lan_addr_range(IN INT32 LanDeviceIndex, IN INT32 buf_sz, OUT CHAR *min_address, OUT CHAR *max_address)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    dhcpServerInfo_t dhcps;
    lanSetting_t lan;

    bzero(&lan, sizeof(lanSetting_t));
    bzero(&dhcps, sizeof(dhcpServerInfo_t));

    UtopiaContext ctx;
    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetDHCPServerSettings(&ctx, &dhcps)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    if (SUCCESS != Utopia_GetLanSettings(&ctx, &lan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);


    PAL_LOG("igd_platform", "debug", "%s: lan.ipaddr %s", __FUNCTION__, lan.ipaddr);

    int octet1, octet2, octet3, last_octet;
    int ct = sscanf(lan.ipaddr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &last_octet);
    PAL_LOG("igd_platform", "debug", "%s: p [%s], sscanf ct %d", __FUNCTION__, lan.ipaddr, ct);
    if (4 == ct) {
        snprintf(min_address, buf_sz, "%d.%d.%d.%s", octet1, octet2, octet3, dhcps.DHCPIPAddressStart);
        int end_ip_octet = atoi(dhcps.DHCPIPAddressStart) + dhcps.DHCPMaxUsers - 1;
        snprintf(max_address, buf_sz, "%d.%d.%d.%d", octet1, octet2, octet3, end_ip_octet);
    }

    return 0;

}

/************************************************************
 * Function: IGD_pii_get_lan_reserved_addr_list
 *  Parameters:
 *      LanDeviceIndex:          IN. Index of LANDevice, range:1-Number of LANDevice.
 *	    max_list_sz:              IN. buffer size of OUT params (they are all need to be same size) 
 *	    reserved_list:         OUT. comma separated list of reserved DHCP addresses
 * 
 *  Description:
 *      Returns LAN DHCP Server's reserverd DHCP addresses
 *
 *  Return Values: INT32
 *	   0 if successful else error code.
 ************************************************************/
INT32 IGD_pii_get_lan_reserved_addr_list(IN INT32 LanDeviceIndex, OUT CHAR *reserved_list, IN INT32 max_list_sz)
{
    (void) LanDeviceIndex;

    PAL_LOG("igd_platform", "debug", "%s: Enter ", __FUNCTION__);

    DHCPMap_t *dhcp_static_hosts = NULL;
    int        dhcp_static_hosts_count = 0;
    lanSetting_t lan;

    bzero(&lan, sizeof(lanSetting_t));

    UtopiaContext ctx;
    if (!Utopia_Init(&ctx)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting utctx object", __FUNCTION__);
        return 1;
    }
    if (SUCCESS != Utopia_GetLanSettings(&ctx, &lan)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    if (SUCCESS != Utopia_GetDHCPServerStaticHosts(&ctx, &dhcp_static_hosts_count, &dhcp_static_hosts)) {
        PAL_LOG("igd_platform", "debug", "%s: Error, in getting lan settings info", __FUNCTION__);
        Utopia_Free(&ctx, 0);
        return 1;
    }
    Utopia_Free(&ctx, 0);

    PAL_LOG("igd_platform", "debug", "%s: ipaddr [%s], host ct [%d]", __FUNCTION__, lan.ipaddr, dhcp_static_hosts_count);

    int octet1, octet2, octet3, last_octet;
    int i, ct;

    ct = sscanf(lan.ipaddr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &last_octet);
    if (4 == ct) {
        int first = 1;
        char ipaddr[32];
        for (i = 0; i < dhcp_static_hosts_count; i++) {
            if (first) {
                // append a comma
                strncat(reserved_list, ",", max_list_sz);
            } else {
                first = 0;
            }
            PAL_LOG("igd_platform", "debug", "%s: index [%d], name [%s], host_ip [%d], mac [%s]", __FUNCTION__, i, dhcp_static_hosts[i].client_name, dhcp_static_hosts[i].host_ip, dhcp_static_hosts[i].macaddr);
            snprintf(ipaddr, sizeof(ipaddr), "%d.%d.%d.%d", 
                     octet1, octet2, octet3, (int)dhcp_static_hosts[i].host_ip);
            strncat(reserved_list, ipaddr, max_list_sz);
        }
    }

    if (dhcp_static_hosts) {
        free(dhcp_static_hosts);
    }

    return 0;
}
