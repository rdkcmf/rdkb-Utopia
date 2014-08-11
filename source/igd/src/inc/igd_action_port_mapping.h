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
 *    FileName:    igd_action_port_mapping.h
 *      Author:    Lipin Zhou(zlipin@cisco.com)
 *        Date:    2009-04-30
 * Description:    IGD port map implementation of UPnP IGD project
 *****************************************************************************/
/*$Id: igd_action_port_mapping.h,v 1.4 2009/05/22 05:36:58 zlipin Exp $
 *
 *$Log: igd_action_port_mapping.h,v $
 *Revision 1.4  2009/05/22 05:36:58  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.3  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.2  2009/05/14 05:56:05  zlipin
 *update the included file name
 *
 *Revision 1.1  2009/05/14 01:58:26  zlipin
 *First version
 *
 *
 **/
 
#ifndef _IGD_ACTION_PORT_MAPPING_
#define _IGD_ACTION_PORT_MAPPING_

/***************************including***************************/
#include "igd_platform_independent_inf.h"
#include "pal_log.h"
#include "pal_upnp_device.h"
#include "pal_xml2s.h"
#include "pal_def.h"
/***************************including end***********************/


/***************************data structure***********************/

#define IGD_GENERAL_ERROR           -1

typedef struct _genPortMapIndex{
    PAL_XML2S_FDMSK fieldMask;
    
    #define MASK_OF_PORTMAP_INDEX    0x00000001

    UINT16 portMapIndex;
}genPortMapIndex;

typedef struct _PORT_MAP_INDEX{
    PAL_XML2S_FDMSK fieldMask;
    
    #define MASK_OF_INDEX_REMOTE_HOST       0x00000001
    #define MASK_OF_INDEX_EXTERNAL_PORT    0x00000002
    #define MASK_OF_INDEX_PROTOCOL              0x00000004
    
    CHAR    *remoteHost;
    UINT16  externalPort;
    CHAR    *pmProtocol;
}PORT_MAP_INDEX, *PPORT_MAP_INDEX;

typedef struct _PORT_MAP_ENTRY{
    PAL_XML2S_FDMSK fieldMask;
    
    #define MASK_OF_ENTRY_REMOTE_HOST       0x00000001
    #define MASK_OF_ENTRY_EXTERNAL_PORT    0x00000002
    #define MASK_OF_ENTRY_PROTOCOL              0x00000004
    #define MASK_OF_ENTRY_INTERNAL_PORT    0x00000008
    #define MASK_OF_ENTRY_INTERNAL_CLIENT 0x00000010
    #define MASK_OF_ENTRY_ENABLED                0x00000020
    #define MASK_OF_ENTRY_DESCRIPTION         0x00000040
    #define MASK_OF_ENTRY_LEASE_TIME           0x00000080
    
    CHAR    *remoteHost;
    UINT16  externalPort;
    CHAR    *pmProtocol;
    UINT16  internalPort;
    CHAR    *internalClient;
    BOOL     pmEnabled;
    CHAR    *pmDescription;
    UINT32  pmLeaseTime;
}PORT_MAP_ENTRY, *PPORT_MAP_ENTRY;

/***************************data structure end********************/


/***************************interface function***************************/

 /************************************************************
 * Function: IGD_get_NATRSIP_status
 *
 *  Parameters:	
 *      event: 		INOUT.  action request from upnp template 
 * 
 *  Description:
 *     This function process the action "IGD_get_NATRSIP_status".  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_get_NATRSIP_status(INOUT struct action_event *event);

 /************************************************************
 * Function: IGD_get_GenericPortMapping_entry
 *
 *  Parameters:	
 *      event: 		INOUT.  action request from upnp template 
 * 
 *  Description:
 *     This function process the action "IGD_get_GenericPortMapping_entry".  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_get_GenericPortMapping_entry(INOUT struct action_event *event);

 /************************************************************
 * Function: IGD_get_SpecificPortMapping_entry
 *
 *  Parameters:	
 *      event: 		INOUT.  action request from upnp template 
 * 
 *  Description:
 *     This function process the action "IGD_get_SpecificPortMapping_entry".  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_get_SpecificPortMapping_entry(INOUT struct action_event *event);

 /************************************************************
 * Function: IGD_add_PortMapping
 *
 *  Parameters:	
 *      event: 		INOUT.  action request from upnp template 
 * 
 *  Description:
 *     This function process the action "IGD_add_PortMapping".  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_add_PortMapping(INOUT struct action_event *event);

 /************************************************************
 * Function: IGD_delete_PortMapping
 *
 *  Parameters:	
 *      event: 		INOUT.  action request from upnp template 
 * 
 *  Description:
 *     This function process the action "IGD_delete_PortMapping".  
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/  
INT32 IGD_delete_PortMapping(INOUT struct action_event *event);
 
/***************************interface function end***********************/

#endif  //_IGD_ACTION_PORT_MAPPING_

