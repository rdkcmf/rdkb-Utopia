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
 *    FileName:    igd_platform_independent_inf.h
 *      Author:    Andy Liu(zhihliu@cisco.com) Tao Hong(tahong@cisco.com)
 *                 Jianrong(jianxiao@cisco.com)Lipin Zhou(zlipin@cisco.com)	 
 *        Date:    2009-05-03
 * Description:    Header file including all Product-related macro and functions
 *****************************************************************************/
/*$Id: igd_platform_independent_inf.h,v 1.10 2009/05/26 09:58:28 zhangli Exp $
 *
 *$Log: igd_platform_independent_inf.h,v $
 *Revision 1.10  2009/05/26 09:58:28  zhangli
 *Completed the cleanup activity
 *
 *Revision 1.9  2009/05/22 05:37:34  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.8  2009/05/21 07:58:13  zhihliu
 *update PII interface
 *
 *Revision 1.7  2009/05/21 06:25:35  jianxiao
 *Modified some define of MACRO and add IGD_pii_get_wan_device_number/IGD_pii_get_wan_connection_device_number interface
 *
 *Revision 1.6  2009/05/15 09:25:11  tahong
 *use MACRO to construct description file of wan connection file
 *
 *Revision 1.5  2009/05/15 05:40:45  jianxiao
 *Update for integration
 *
 *Revision 1.4  2009/05/14 02:36:19  jianxiao
 *Add the wan_connect
 *
 *Revision 1.3  2009/05/14 02:05:18  jianxiao
 *Add the function IGD_pii_get_uuid
 *
 *Revision 1.2  2009/05/14 01:47:12  jianxiao
 *Add some macro for description file
 *
 *Revision 1.1  2009/05/13 08:57:38  tahong
 *create orignal version
 *
 *
 **/


#ifndef IGD_PLATFORM_INDEPENDENT_INF_H
#define IGD_PLATFORM_INDEPENDENT_INF_H

#include "pal_def.h"
#include "igd_platform_dependent_inf.h"
#include "autoconf.h"


/***********************************************************************
* (1) Product-related macro
*     Notes: All the below macro should be modified based on your needs 
*     when you port IGD to your products
************************************************************************/

// The name of the lan interface that the IGD will be run on 
//#define IGD_UPNP_INTERFACE "lan0"
//Now pulled from syscfg

// The plarform-related info that will be used in the the description file of IGD device

#define WANDEVICE_FRIENDLY_NAME 	        "WANDevice:1"
#define WAN_CONNECTION_DEVICE_FRIENDLY_NAME 	"WANConnectionDevice:1"
#define LANDEVICE_FRIENDLY_NAME 	        "LANDevice:1"

#ifndef INTEL_PUMA7
#define ROOT_FRIENDLY_NAME 			CONFIG_VENDOR_MODEL
#undef MODULE_DESCRIPTION
#define MODULE_DESCRIPTION 		        CONFIG_VENDOR_MODEL
#undef MODULE_NAME
#define MODULE_NAME 				CONFIG_VENDOR_MODEL
#undef MODULE_NUMBER
#define MODULE_NUMBER 				CONFIG_VENDOR_MODEL
#undef UPC
#define UPC 					CONFIG_VENDOR_MODEL
#endif
/***********************************************************************
* (2) Product-related functions
*     Notes: All the below functions should be implemented based on your   
*     products when you port IGD to them
************************************************************************/
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
extern CHAR* IGD_pii_get_serial_number(VOID);

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
#define UPNP_UUID_LEN_BY_VENDER 42 
extern INT32 IGD_pii_get_uuid(OUT CHAR *uuid);

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
extern INT32 IGD_pii_get_wan_device_number(VOID);

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
extern INT32 IGD_pii_get_wan_connection_device_number(IN INT32 wan_device_index);

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
extern INT32 IGD_pii_get_wan_ppp_service_number(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex);

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
extern INT32 IGD_pii_get_wan_ip_service_number(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex);

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
// The valid value of the input parameter,"ServiceType" 
#define SERVICETYPE_IP     (1)
#define SERVICETYPE_PPP    (2)

// The valid value of the output parameter,"ConnectionTypesList" 
// possible IP connection types
#define IPCONNTYPE_UNCONFIGURED "Unconfigured"
#define IPCONNTYPE_IP_ROUTED        "IP_Routed"
#define IPCONNTYPE_IP_BRIDGED       "IP_Bridged"
// possible PPP connection types
#define PPPCONNTYPE_UNCONFIGURED  "Unconfigured"
#define PPPCONNTYPE_IP_ROUTED         "IP_Routed"
#define PPPCONNTYPE_DHCP_SPOOFED  "DHCP_Spoofed"
#define PPPCONNTYPE_PPPOE_BRIDGED "PPPoE_Bridged"
#define PPPCONNTYPE_PPTP_RELAY        "PPTP_Relay"
#define PPPCONNTYPE_L2TP_RELAY        "L2TP_Relay"
#define PPPCONNTYPE_PPPOE_RELAY     "PPPoE_Relay"   
 
extern INT32 IGD_pii_get_possible_connection_types(IN INT32 WanDeviceIndex,
                                                                                        IN INT32 WanConnectionDeviceIndex,
                                                                                        IN INT32 WanConnectionServiceIndex,
                                                                                        IN INT32 ServiceType,
                                                                                        OUT CHAR *ConnectionTypesList);

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
 
// The valid value of the output parameter,"ConnectionStatus" 
// possible connection status, for both IP and PPP
#define CONNSTATUS_UNCONFIGURED "Unconfigured"
#define CONNSTATUS_CONNECTED        "Connected"
#define CONNSTATUS_DISCONNECTED  "Disconnected"   
 
extern INT32 IGD_pii_get_connection_status(IN INT32 WanDeviceIndex,
                                                                        IN INT32 WanConnectionDeviceIndex,
                                                                        IN INT32 WanConnectionServiceIndex,
                                                                        IN INT32 ServiceType,
                                                                        OUT CHAR *ConnectionStatus);
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
   
// The valid value of the output parameter,"ConnectionType"  
// Same as the output parameter,"ConnectionTypesList", of IGD_pii_get_possible_connection_types()
extern INT32 IGD_pii_get_connection_type(IN INT32 WanDeviceIndex,
                                                                    IN INT32 WanConnectionDeviceIndex,
                                                                    IN INT32 WanConnectionServiceIndex,
                                                                    IN INT32 ServiceType,
                                                                    OUT CHAR *ConnectionType);
                                                                    
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
   
// The valid value of the input parameter,"ConnectionType"  
// Same as the output parameter,"ConnectionTypesList", of IGD_pii_get_possible_connection_types()

extern INT32 IGD_pii_set_connection_type(IN INT32 WanDeviceIndex,
                                                                    IN INT32 WanConnectionDeviceIndex,
                                                                    IN INT32 WanConnectionServiceIndex,
                                                                    IN INT32 ServiceType,
                                                                    IN CHAR *ConnectionType);
                                                                    
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
extern INT32 IGD_pii_request_connection(IN INT32 WanDeviceIndex,
                                                                IN INT32 WanConnectionDeviceIndex,
                                                                IN INT32 WanConnectionServiceIndex,
                                                                IN INT32 ServiceType);
                                                                
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
extern INT32 IGD_pii_force_termination(IN INT32 WanDeviceIndex,
                                                            IN INT32 WanConnectionDeviceIndex,
                                                            IN INT32 WanConnectionServiceIndex,
                                                            IN INT32 ServiceType);
                                                            
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
extern INT32 IGD_pii_get_external_ip(IN INT32 WanDeviceIndex,
                                                        IN INT32 WanConnectionDeviceIndex,
                                                        IN INT32 WanConnectionServiceIndex,
                                                        IN INT32 ServiceType,
                                                        OUT CHAR *ExternalIp);
                                                        
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
extern INT32 IGD_pii_get_link_layer_max_bitrate(IN INT32 WanDeviceIndex,
                                                                            IN INT32 WanConnectionDeviceIndex,
                                                                            IN INT32 WanConnectionServiceIndex,
                                                                            IN INT32 ServiceType,
                                                                            OUT CHAR *UpRate,
                                                                            OUT CHAR *DownRate);
                                                                            
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
extern INT32 IGD_pii_get_up_time(IN INT32 WanDeviceIndex,
                                                    IN INT32 WanConnectionDeviceIndex,
                                                    IN INT32 WanConnectionServiceIndex,
                                                    IN INT32 ServiceType,
                                                    OUT CHAR *UpTime);


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
extern INT32 IGD_pii_get_NAT_RSIP_status( IN INT32 WanDeviceIndex,
                            IN INT32 WanConnectionDeviceIndex,
                            IN INT32 WanConnectionServiceIndex, 
                            IN INT32 ServiceType,
                            OUT BOOL *NATEnable, 
                            OUT BOOL *RSIPAvailable );      
                    

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
//Structure definition for the output parameter,"PortmappingEntry"
#define PORT_MAP_PROTOCOL_LEN         4
#define PORT_MAP_DESCRIPTION_LEN      128
typedef struct IGD_PortMapping_Entry{
    CHAR        remoteHost[IPV4_ADDR_LEN]; //"RemoteHost"
    UINT16      externalPort;              //"ExternalPort"
    CHAR        protocol[PORT_MAP_PROTOCOL_LEN];  //"PortMappingProtocol"
    UINT16      internalPort;                     //"InternalPort"
    CHAR        internalClient[IPV4_ADDR_LEN];   //"InternalClient"
    BOOL        enabled;                          //"PortMappingEnabled"
    CHAR        description[PORT_MAP_DESCRIPTION_LEN];//"PortMappingDescription"
    UINT32      leaseTime;                            //"PortMappingLeaseDuration"
}IGD_PortMapping_Entry, *PIGD_PortMapping_Entry; 

//Error code
#define ERROR_PORTMAPPING_ADD_FAILED         -501
#define ERROR_WILDCARD_NOTPERMIT_FOR_SRC_IP        -715 //The source IP address cannot be wild-carded(i.e. empty string)
#define ERROR_WILDCARD_NOTPERMIT_FOR_EXTERNAL_PORT -716 //The external port cannot be wild-carded(i.e. 0)
#define ERROR_CONFLICT_FOR_MAPPING_ENTRY           -718 //The port mapping entry specified conflicts with a mapping assigned previously to another client
#define ERROR_SAME_PORT_VALUE_REQUIRED             -724 //Internal and External port values must be the same
#define ERROR_ONLY_PERMANENT_LEASETIME_SUPPORTED   -725 //The NAT implementation only supports permanent lease times on port mappings
#define ERROR_REMOST_HOST_ONLY_SUPPORT_WILDCARD    -726 //RemoteHost must be a wildcard and cannot be a specific IP address or DNS name
#define ERROR_EXTERNAL_PORT_ONLY_SUPPORT_WILDCARD  -727 //ExternalPort must be a wildcard and cannot be a specific port value

extern INT32 IGD_pii_add_portmapping_entry( IN INT32 WanDeviceIndex,
                                	IN INT32 WanConnectionDeviceIndex,
                                	IN INT32 WanConnectionServiceIndex, 
                                	IN INT32 ServiceType,
                                	IN PIGD_PortMapping_Entry PortmappingEntry);
                                
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
extern INT32 IGD_pii_del_portmapping_entry( IN INT32 WanDeviceIndex,
                        	IN INT32 WanConnectionDeviceIndex,
                        	IN INT32 WanConnectionServiceIndex, 
                        	IN INT32 ServiceType,
                        	IN CHAR  *RemoteHost,
                        	IN UINT16  ExternalPort,
                        	IN CHAR  *Protocol);
                        	
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
extern INT32 IGD_pii_get_portmapping_entry_num(IN INT32 WanDeviceIndex,
                                        IN INT32 WanConnectionDeviceIndex,
                                        IN INT32 WanConnectionServiceIndex, 
                                        IN INT32 ServiceType,
                                        OUT INT32 *PortmappingEntryNum);
                                        
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
// Error code 
#define ERROR_SPECIFIED_INDEX_INVALID -713 //The Specified index is out of bounds  
										                         	
extern INT32 IGD_pii_get_portmapping_entry_generic( IN INT32 WanDeviceIndex,
                                IN INT32 WanConnectionDeviceIndex,
                                IN INT32 WanConnectionServiceIndex, 
                                IN INT32 ServiceType,
                                IN INT32 PortmappingIndex,
                                OUT PIGD_PortMapping_Entry PortmappingEntry);
                                
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
/*Special notes for the INOUT parameter,PortmappingEntry
typedef struct IGD_PortMapping_Entry{
    CHAR        remoteHost[IPV4_ADDR_LEN];        //IN
    UINT16      externalPort;                     //IN
    CHAR        protocol[PORT_MAP_PROTOCOL_LEN];  //IN
    
    UINT16      internalPort;                    //OUT
    CHAR        internalClient[IPV4_ADDR_LEN];   //OUT
    BOOL        enabled;                         //OUT
    CHAR        description[PORT_MAP_DESCRIPTION_LEN];//OUT
    UINT32      leaseTime;                            //OUT
}IGD_PortMapping_Entry, *PIGD_PortMapping_Entry; */
 
//Error code 
#define ERROR_NO_SUCH_ENTRY -714 // The specified value doesn't exist
										                         	
extern INT32 IGD_pii_get_portmapping_entry_specific( IN INT32 WanDeviceIndex,
                                IN INT32 WanConnectionDeviceIndex,
                                IN INT32 WanConnectionServiceIndex, 
                                IN INT32 ServiceType,
                                INOUT PIGD_PortMapping_Entry PortmappingEntry);                                

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
// The valid value of the output parameter,"status"   
#define ETHERNETLINKSTATUS_UP			"Up"
#define ETHERNETLINKSTATUS_DOWN			"Down"
#define ETHERNETLINKSTATUS_UNAVAILABLE	"Unavailable"
extern INT32 IGD_pii_get_ethernet_link_status(IN INT32 WanDeviceIndex,
													IN INT32 WanConnectionDeviceIndex,
													OUT CHAR *EthernetLinkStatus);
													
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
// The valid value of the output parameter,"WanAccessType"  			
#define WANACCESSTYPE_DSL 		"DSL" 
#define WANACCESSTYPE_POTS 		"POTS"
#define WANACCESSTYPE_CABLE 	"Cable"
#define WANACCESSTYPE_ETHERNET 	"Ethernet"

// The valid value of the output parameter,"PhyscialLinkStatus"  	
#define LINKSTATUS_UP 	"Up"
#define LINKSTATUS_DOWN "Down"

extern INT32 IGD_pii_get_common_link_properties(IN INT32 WanDeviceIndex,
														OUT CHAR *WanAccessType,
														OUT CHAR *Layer1UpstreamMaxBitRate,
														OUT CHAR *Layer1DownstreamMaxBitRate,
														OUT CHAR *PhyscialLinkStatus);
														
														




/************************************************************
 * Function: IGD_pii_get_traffic_stats
 *
 *  Parameters:
 *      WanDeviceIndex:                  IN. Index of WANDevice, range:1-Number of WANDevice.
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
				OUT CHAR *TotalPacketsReceived);

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
INT32 IGD_pii_get_lan_dhcpserver_configurable(IN INT32 LanDeviceIndex, OUT CHAR *status);

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
INT32 IGD_pii_get_lan_dhcp_relay_status(IN INT32 LanDeviceIndex, OUT CHAR *status);

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
INT32 IGD_pii_get_lan_info(IN INT32 LanDeviceIndex, IN INT32 bufsz, OUT CHAR *ipaddr, OUT CHAR *subnet_mask, OUT CHAR *domain_name);

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
INT32 IGD_pii_get_lan_dns_servers(IN INT32 LanDeviceIndex, OUT CHAR *dns_servers, IN INT32 max_list_sz);

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
INT32 IGD_pii_get_lan_addr_range(IN INT32 LanDeviceIndex, IN INT32 buf_sz, OUT CHAR *min_address, OUT CHAR *max_address);

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
INT32 IGD_pii_get_lan_reserved_addr_list(IN INT32 LanDeviceIndex, OUT CHAR *reserved_list, IN INT32 max_list_sz);

#endif /*IGD_PLATFORM_INDEPENDENT_INF_H*/

