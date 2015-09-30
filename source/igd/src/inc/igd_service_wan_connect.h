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

/* Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 *
 *
 *    FileName:    igd_service_wan_connect.h
 *      Author:    Andy Liu(zhihliu@cisco.com)
 *                     Tao Hong(tahong@cisco.com)
 *        Date:    2009-05-03
 * Description:    WAN IP/PPP connection service header file of UPnP IGD project
 *****************************************************************************/
/*$Id: igd_service_wan_connect.h,v 1.2 2009/05/22 05:38:06 zlipin Exp $
 *
 *$Log: igd_service_wan_connect.h,v $
 *Revision 1.2  2009/05/22 05:38:06  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.1  2009/05/13 08:57:48  tahong
 *create orignal version
 *
 *
 **/

#ifndef WAN_CONNECTION_SERVICE_H
#define WAN_CONNECTION_SERVICE_H

#include <stdio.h>

#include "pal_upnp_device.h"
#include "pal_def.h"

#define WAN_CONNECTION_DEVICE_LOG_NAME "wan_con"

#define WAN_PPP_CONNECTION_SERVICE_TYPE "urn:schemas-upnp-org:service:WANPPPConnection:1"
#define WAN_IP_CONNECTION_SERVICE_TYPE  "urn:schemas-upnp-org:service:WANIPConnection:1"

enum wan_connection_service_state_variables_index
{
    ConnectionType_index=0,
    PossibleConnectionTypes_index,
    ConnectionStatus_index,
    Uptime_index,
    LastConnectionError_index,
    ExternalIPAddress_index,
    RSIPAvailable_index,
    NATEnabled_index,
    PortMappingNumberOfEntries_index,
    PortMappingEnabled_index,
    PortMappingLeaseDuration_index,
    RemoteHost_index,
    ExternalPort_index,
    InternalPort_index,
    PortMappingProtocol_index,
    InternalClient_index,
    PortMappingDescription_index,
    UpstreamMaxBitRate_index,
    DownstreamMaxBitRate_index
};

// be aligned with wan_connection_service_event_variables_name
enum wan_connection_service_event_variables_index
{
    PossibleConnectionTypes_event_index=0,
    ConnectionStatus_event_index,
    ExternalIPAddress_event_index,
    PortMappingNumberOfEntries_event_index
};

typedef struct{
    INT32 code;
    CHAR *desc;
} error_pair;

/************************************************************
* Function: IGD_wan_ppp_connection_service_init
*
*  Parameters:
*               input_index_struct:    IN.          Wan device index, wan connection device index and wan connection service index
*               wan_desc_file:          INOUT.    The fd to write description file
*  Description:
*     This function is called by IGD_wan_connection_device_init() to initialize a wan ppp connection service instance
*
*  Return Values: struct upnp_service *
*               the initialized wan ppp connection service if successful else NULL
************************************************************/
extern struct upnp_service * IGD_wan_ip_connection_service_init (IN VOID* input_index_struct, INOUT FILE *wan_desc_file);
/************************************************************
* Function: IGD_wan_ip_connection_service_init
*
*  Parameters:
*               input_index_struct:    IN.          Wan device index, wan connection device index and wan connection service index
*               wan_desc_file:          INOUT.    The fd to write description file
*  Description:
*     This function is called by IGD_wan_connection_device_init() to initialize a wan ip connection service instance
*
*  Return Values: struct upnp_service *
*               the initialized wan ip connection service if successful else NULL
************************************************************/
extern struct upnp_service * IGD_wan_ppp_connection_service_init (IN VOID* input_index_struct, INOUT FILE *wan_desc_file);

#endif /* WAN_CONNECTION_SERVICE_H */
