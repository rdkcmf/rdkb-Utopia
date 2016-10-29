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
 *    FileName:    igd_action_port_mapping.c
 *      Author:    Lipin Zhou(zlipin@cisco.com)
 *        Date:    2009-04-30
 * Description:    IGD port map implementation of UPnP IGD project
 *****************************************************************************/
/*$Id: igd_action_port_mapping.c,v 1.5 2009/05/27 02:14:22 zlipin Exp $
 *
 *$Log: igd_action_port_mapping.c,v $
 *Revision 1.5  2009/05/27 02:14:22  zlipin
 *Fix Warnings.
 *
 *Revision 1.4  2009/05/26 09:54:57  zlipin
 *modifed the PII interface
 *
 *Revision 1.3  2009/05/22 05:38:33  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.2  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.1  2009/05/14 01:58:14  zlipin
 *First version
 *
 *
 **/

#include <string.h>
 
#include "igd_utility.h"
#include "igd_service_wan_connect.h"
#include "igd_action_port_mapping.h"

#define ARRAY_INDEX_INVALID                     713
#define NO_SUCH_ENTRY_IN_ARRAY              714

#define WildCard_NotPermitted_InSrcIP            715
#define WildCard_NotPermitted_InExtPort         716
#define Conflict_In_MappingEntry                       718
#define Same_PortValues_Required                    724
#define Only_Permanent_Leases_Supported      725
#define RemoteHost_OnlySupports_Wildcard      726
#define ExternalPort_OnlySupports_Wildcard     727

#define ARRAY_INDEX_INVALID_STR                 "SpecifiedArrayIndexInvalid"
#define NO_SUCH_ENTRY_IN_ARRAY_STR          "NoSuchEntryInArray"

#define WildCard_NotPermitted_InSrcIP_STR        "The source IP address cannot be wild-carded"
#define WildCard_NotPermitted_InExtPort_STR     "The external port cannot be wild-carded"
#define Conflict_In_MappingEntry_STR                   "The port mapping entry specified conflicts with a mapping assigned previously to another client"
#define Same_PortValues_Required_STR                "Internal and External port values must be the same"
#define Only_Permanent_Leases_Supported_STR  "The NAT implementation only supports permanent lease times on port mappings"
#define RemoteHost_OnlySupports_Wildcard_STR  "RemoteHost must be a wildcard and cannot be a specific IP address or DNS name"
#define ExternalPort_OnlySupports_Wildcard_STR "ExternalPort must be a wildcard and cannot be a specific port value"

#define MAX_NUM_TO_STR_LEN       10

#define PORTMAP_INDEX_FIELD_NUM    3

typedef enum PortMapElem{
    REMOTE_HOST,
    EXTERNAL_PORT,
    PROTOCOL,
    
    INTERNAL_PORT,
    INTERNAL_CLIENT,

    ENABLED,
    PORTMAPPING_DESCRIPTION,
    LEASE_DURATION,

    PORTMAP_ENTRY_FIELD_NUM
} E_PortMapElem;

// Portmap Entry parameter set
LOCAL CHAR *PM_SET[PORTMAP_ENTRY_FIELD_NUM] = {
    "NewRemoteHost",
    "NewExternalPort",
    "NewProtocol",
    
    "NewInternalPort",
    "NewInternalClient",

    "NewEnabled",
    "NewPortMappingDescription",
    "NewLeaseDuration"
};


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
INT32 IGD_get_NATRSIP_status(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    BOOL natStatus, rsipStatus;
    INT32 ret = PAL_UPNP_E_SUCCESS;
    pal_string_pair response[] = {
        {"NewRSIPAvailable", 0},
        {"NewNATEnabled", 0}
    };

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ENTER %s...", __func__);

    if (!event || !(event->request)) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Input parameter error");

        ret = IGD_GENERAL_ERROR;
        return ret;
    }

    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");

        ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);

        return ret;
    }
    
    ret = IGD_pii_get_NAT_RSIP_status(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 &natStatus, &rsipStatus);

    if(ret != PAL_UPNP_E_SUCCESS)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "get_nat_rsip_status error");

        event->request->error_code = ret;
        event->request->action_result = NULL;
    }else {
        event->request->error_code = PAL_UPNP_E_SUCCESS;

        if(rsipStatus == BOOL_TRUE)
        {
            response[0].value = "1";
        }else {
            response[0].value = "0";
        }
        if(natStatus == BOOL_TRUE)
        {
            response[1].value = "1";
        }else {
            response[1].value = "0";
        }

        ret = PAL_upnp_make_action(&(event->request->action_result), event->request->action_name, 
                                                            event->service->type, 2, response, PAL_UPNP_ACTION_RESPONSE);
        if(ret != PAL_UPNP_E_SUCCESS)
        {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_upnp_make_action error: %d", ret);

            event->request->error_code = ret;
            event->request->action_result = NULL;
        }
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "EXIT %s...", __func__);

    return ret;
}

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
INT32 IGD_get_GenericPortMapping_entry(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    CHAR internalPort[MAX_NUM_TO_STR_LEN], externalPort[MAX_NUM_TO_STR_LEN], leaseTime[MAX_NUM_TO_STR_LEN];
    IGD_PortMapping_Entry portmapEntry;
    genPortMapIndex portmapIndex = {0, 0};
    INT32 entryNum, ret;
    

   PAL_XML2S_TABLE tableGenPorMap[] = {
        {"NewPortMappingIndex", PAL_XML2S_UINT16, XML2S_MSIZE(genPortMapIndex, portMapIndex), NULL, MASK_OF_PORTMAP_INDEX},
        XML2S_TABLE_END
    };
    pal_string_pair response[PORTMAP_ENTRY_FIELD_NUM] = {
        { PM_SET[REMOTE_HOST] ,                       NULL },
        { PM_SET[EXTERNAL_PORT] ,                    NULL },
        { PM_SET[PROTOCOL] ,                             NULL },
        { PM_SET[INTERNAL_PORT] ,                     NULL },
        { PM_SET[INTERNAL_CLIENT] ,                  NULL },
        { PM_SET[ENABLED] ,                               NULL },
        { PM_SET[PORTMAPPING_DESCRIPTION] , NULL },
        { PM_SET[LEASE_DURATION] ,                  NULL }
    };

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ENTER %s...", __func__);

    if (!event || !(event->request)) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Input parameter error");

        ret = IGD_GENERAL_ERROR;
        return ret;
    }
    
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");

        ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);

        return ret;
    }
    
    bzero(&portmapIndex, sizeof(portmapIndex));
    ret = PAL_xml2s_process(event->request->action_request, tableGenPorMap, &portmapIndex);
    if (ret < 0){
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_xml2s_process error");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else {
        ret = IGD_pii_get_portmapping_entry_num(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 &entryNum);
        if(ret == 0)
        {
            if(portmapIndex.portMapIndex >= entryNum)
            {
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "portmap index error");
                
                ret = ARRAY_INDEX_INVALID;
                event->request->error_code = ret;
                strncpy(event->request->error_str, ARRAY_INDEX_INVALID_STR, strlen(ARRAY_INDEX_INVALID_STR) + 1);
            } else {
                ret = IGD_pii_get_portmapping_entry_generic(pIndex->wan_device_index,
                                         pIndex->wan_connection_device_index,
                                         pIndex->wan_connection_service_index,
                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                         portmapIndex.portMapIndex,
                                         &portmapEntry);

                if(ret == 0)
                {
                    event->request->error_code = PAL_UPNP_E_SUCCESS;

                    response[REMOTE_HOST].value = strdup(portmapEntry.remoteHost);

                    snprintf(externalPort, MAX_NUM_TO_STR_LEN, "%d", portmapEntry.externalPort);
                    response[EXTERNAL_PORT].value = externalPort;

                    response[PROTOCOL].value = strdup(portmapEntry.protocol);

                    snprintf(internalPort, MAX_NUM_TO_STR_LEN, "%d", portmapEntry.internalPort);
                    response[INTERNAL_PORT].value = internalPort;

                    response[INTERNAL_CLIENT].value = strdup(portmapEntry.internalClient);

                    if(portmapEntry.enabled == BOOL_TRUE)
                    {
                        response[ENABLED].value = "1";
                    } else {
                        response[ENABLED].value = "0";
                    }

                    response[PORTMAPPING_DESCRIPTION].value = strdup(portmapEntry.description);

                    snprintf(leaseTime, MAX_NUM_TO_STR_LEN, "%d", portmapEntry.leaseTime);
                    response[LEASE_DURATION].value = leaseTime;

                    ret = PAL_upnp_make_action(&(event->request->action_result), event->request->action_name, 
                                    event->service->type, PORTMAP_ENTRY_FIELD_NUM, response, PAL_UPNP_ACTION_RESPONSE);
                    if(ret != PAL_UPNP_E_SUCCESS)
                    {
                        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_upnp_make_action error");

                        event->request->error_code = ret;
                        event->request->action_result = NULL;
                    }

                    if(response[REMOTE_HOST].value)
                    {
                        free(response[REMOTE_HOST].value);
                    }
                    if(response[PROTOCOL].value)
                    {
                        free(response[PROTOCOL].value);
                    }
                    if(response[INTERNAL_CLIENT].value)
                    {
                        free(response[INTERNAL_CLIENT].value);
                    }
                    if(response[PORTMAPPING_DESCRIPTION].value)
                    {
                        free(response[PORTMAPPING_DESCRIPTION].value);
                    }
                }else {  //IGD_pii_get_portmapping_entry_generic error
                    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "IGD_pii_get_portmapping_entry_num error");

                    ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);
                }
            }   // end if (portmapIndex.portMapIndex >= entryNum)
        } else {   //IGD_pii_get_portmapping_entry_num error
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "IGD_pii_get_portmapping_entry_num error");

            ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
            event->request->error_code = ret;
            strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);
        }
        
        PAL_xml2s_free(&portmapIndex, tableGenPorMap);
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "EXIT %s...", __func__);

    return ret;
}

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
INT32 IGD_get_SpecificPortMapping_entry(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    CHAR internalPort[MAX_NUM_TO_STR_LEN], leaseTime[MAX_NUM_TO_STR_LEN];
    IGD_PortMapping_Entry portmapEntry;
    PORT_MAP_INDEX portmapIndex;
    struct in_addr addr;
    INT32 ret;

    PAL_XML2S_TABLE tableSpecPorMap[] = {
        {PM_SET[REMOTE_HOST],    PAL_XML2S_STRING, XML2S_MSIZE(PORT_MAP_INDEX, remoteHost),  NULL, MASK_OF_INDEX_REMOTE_HOST},
        {PM_SET[EXTERNAL_PORT], PAL_XML2S_UINT16,  XML2S_MSIZE(PORT_MAP_INDEX, externalPort), NULL, MASK_OF_INDEX_EXTERNAL_PORT},
        {PM_SET[PROTOCOL],         PAL_XML2S_STRING,  XML2S_MSIZE(PORT_MAP_INDEX, pmProtocol),  NULL, MASK_OF_INDEX_PROTOCOL},
        XML2S_TABLE_END
    };
    pal_string_pair response[PORTMAP_ENTRY_FIELD_NUM - PORTMAP_INDEX_FIELD_NUM] = {
        { PM_SET[INTERNAL_PORT] ,                     NULL },
        { PM_SET[INTERNAL_CLIENT] ,                  NULL },
        { PM_SET[ENABLED] ,                               NULL },
        { PM_SET[PORTMAPPING_DESCRIPTION] , NULL },
        { PM_SET[LEASE_DURATION] ,                  NULL }
    };
    
    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ENTER %s...", __func__);

    if (!event || !(event->request)) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Input parameter error");

        ret = IGD_GENERAL_ERROR;
        return ret;
    }

    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");

        ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);

        return ret;
    }
    
    bzero(&portmapIndex, sizeof(portmapIndex));
    ret = PAL_xml2s_process(event->request->action_request, tableSpecPorMap, &portmapIndex);

    if (ret < 0){
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_xml2s_process error");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else if ((portmapIndex.remoteHost != NULL)
                &&(0 == inet_pton(AF_INET, portmapIndex.remoteHost, &addr))){ 
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "remoteHost format error: x.x.x.x");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else {
        bzero(&portmapEntry, sizeof(portmapEntry));
        if(portmapIndex.remoteHost != NULL)
        {
            strncpy(portmapEntry.remoteHost, portmapIndex.remoteHost, IPV4_ADDR_LEN);
        }
        portmapEntry.externalPort = portmapIndex.externalPort;
        if(portmapIndex.pmProtocol != NULL)
        {
            strncpy(portmapEntry.protocol, portmapIndex.pmProtocol, PORT_MAP_PROTOCOL_LEN);
        }

        ret = IGD_pii_get_portmapping_entry_specific(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 &portmapEntry);
        if(ret == 0)
        {
            event->request->error_code = PAL_UPNP_E_SUCCESS;

            snprintf(internalPort, MAX_NUM_TO_STR_LEN, "%d", portmapEntry.internalPort);
            response[INTERNAL_PORT - PORTMAP_INDEX_FIELD_NUM].value = internalPort;

            response[INTERNAL_CLIENT - PORTMAP_INDEX_FIELD_NUM].value = strdup(portmapEntry.internalClient);

            if(portmapEntry.enabled == BOOL_TRUE)
            {
                response[ENABLED - PORTMAP_INDEX_FIELD_NUM].value = "1";
            } else {
                response[ENABLED - PORTMAP_INDEX_FIELD_NUM].value = "0";
            }
            
            response[PORTMAPPING_DESCRIPTION - PORTMAP_INDEX_FIELD_NUM].value = strdup(portmapEntry.description);

            snprintf(leaseTime, MAX_NUM_TO_STR_LEN, "%d", portmapEntry.leaseTime);
            response[LEASE_DURATION - PORTMAP_INDEX_FIELD_NUM].value = leaseTime;
            
            ret = PAL_upnp_make_action(&(event->request->action_result), event->request->action_name, event->service->type, 
                                            PORTMAP_ENTRY_FIELD_NUM - PORTMAP_INDEX_FIELD_NUM, response, PAL_UPNP_ACTION_RESPONSE);
            if(ret != PAL_UPNP_E_SUCCESS)
            {
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_upnp_make_action error");

                event->request->error_code = ret;
                event->request->action_result = NULL;
            }
            
            if(response[INTERNAL_CLIENT - PORTMAP_INDEX_FIELD_NUM].value)
            {
                free(response[INTERNAL_CLIENT - PORTMAP_INDEX_FIELD_NUM].value);
            }
            if(response[PORTMAPPING_DESCRIPTION - PORTMAP_INDEX_FIELD_NUM].value)
            {
                free(response[PORTMAPPING_DESCRIPTION - PORTMAP_INDEX_FIELD_NUM].value);
            }
            
        } else {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "IGD_pii_get_portmapping_entry_specific error");

            ret = NO_SUCH_ENTRY_IN_ARRAY;
            event->request->error_code = ret;
            strncpy(event->request->error_str, NO_SUCH_ENTRY_IN_ARRAY_STR, sizeof(NO_SUCH_ENTRY_IN_ARRAY_STR)+1);
        }
        
        PAL_xml2s_free(&portmapIndex, tableSpecPorMap);
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "EXIT %s...", __func__);

    return ret;
}

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
INT32 IGD_add_PortMapping(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    IGD_PortMapping_Entry pii_pmEntry;
    PORT_MAP_ENTRY portmapEntry;
    struct in_addr addr;
    INT32 ret;

    PAL_XML2S_TABLE tableAddPorMap[] = {
        {PM_SET[REMOTE_HOST],                       PAL_XML2S_STRING,   XML2S_MSIZE(PORT_MAP_ENTRY, remoteHost),    NULL, MASK_OF_ENTRY_REMOTE_HOST},
        {PM_SET[EXTERNAL_PORT],                    PAL_XML2S_UINT16,    XML2S_MSIZE(PORT_MAP_ENTRY, externalPort),   NULL, MASK_OF_ENTRY_EXTERNAL_PORT},
        {PM_SET[PROTOCOL],                            PAL_XML2S_STRING,   XML2S_MSIZE(PORT_MAP_ENTRY, pmProtocol),     NULL, MASK_OF_ENTRY_PROTOCOL},
        {PM_SET[INTERNAL_PORT],                    PAL_XML2S_UINT16,    XML2S_MSIZE(PORT_MAP_ENTRY, internalPort),     NULL, MASK_OF_ENTRY_INTERNAL_PORT},
        {PM_SET[INTERNAL_CLIENT],                  PAL_XML2S_STRING,   XML2S_MSIZE(PORT_MAP_ENTRY, internalClient),  NULL, MASK_OF_ENTRY_INTERNAL_CLIENT},
        {PM_SET[ENABLED],                               PAL_XML2S_UINT8,     XML2S_MSIZE(PORT_MAP_ENTRY, pmEnabled),      NULL, MASK_OF_ENTRY_ENABLED},
        {PM_SET[PORTMAPPING_DESCRIPTION], PAL_XML2S_STRING,   XML2S_MSIZE(PORT_MAP_ENTRY, pmDescription), NULL, MASK_OF_ENTRY_DESCRIPTION},
        {PM_SET[LEASE_DURATION],                  PAL_XML2S_UINT32,    XML2S_MSIZE(PORT_MAP_ENTRY, pmLeaseTime),  NULL, MASK_OF_ENTRY_LEASE_TIME},
        XML2S_TABLE_END
    };

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ENTER %s...", __func__);

    if (!event || !(event->request)) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Input parameter error");

        ret = IGD_GENERAL_ERROR;
        return ret;
    }
    
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");

        ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);

        return ret;
    }

    bzero(&portmapEntry, sizeof(portmapEntry));
    ret = PAL_xml2s_process(event->request->action_request, tableAddPorMap, &portmapEntry);

    if (ret < 0){
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_xml2s_process error: %d", ret);

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else if (((portmapEntry.remoteHost != NULL)
                &&(0 == inet_pton(AF_INET, portmapEntry.remoteHost, &addr)))
                ||(portmapEntry.internalClient == NULL) /* WANIpConnection v1: internalClient can not be wildcard (i.e. empty string) */
                ||(0 == inet_pton(AF_INET, portmapEntry.internalClient, &addr)) 
                ||(!chkPortMappingClient(portmapEntry.internalClient))){ 
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "remoteHost or internalClient format error: x.x.x.x");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else {
        bzero(&pii_pmEntry, sizeof(pii_pmEntry));

        if(portmapEntry.remoteHost != NULL)
        {
            strncpy(pii_pmEntry.remoteHost, portmapEntry.remoteHost, IPV4_ADDR_LEN);
        }
        pii_pmEntry.externalPort = portmapEntry.externalPort;
        if(portmapEntry.pmProtocol != NULL)
        {
            strncpy(pii_pmEntry.protocol, portmapEntry.pmProtocol, PORT_MAP_PROTOCOL_LEN);
        }
        pii_pmEntry.internalPort = portmapEntry.internalPort;
        if(portmapEntry.internalClient != NULL)
        {
            strncpy(pii_pmEntry.internalClient, portmapEntry.internalClient, IPV4_ADDR_LEN);
        }
        pii_pmEntry.enabled = portmapEntry.pmEnabled;
        if(portmapEntry.pmDescription != NULL)
        {
            strncpy(pii_pmEntry.description, portmapEntry.pmDescription, sizeof(pii_pmEntry.description)-1);
        }

	// Setting the lease time for Port Mapping entry , once lease expires rule will get deleted from iptable
	portmapEntry.pmLeaseTime = 172800;
        pii_pmEntry.leaseTime = portmapEntry.pmLeaseTime;
        ret = IGD_pii_add_portmapping_entry(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 &pii_pmEntry);
        if(ret == 0)
        {
            event->request->error_code = PAL_UPNP_E_SUCCESS;
            ret = PAL_upnp_make_action(&(event->request->action_result), event->request->action_name, 
                            event->service->type, 0, NULL, PAL_UPNP_ACTION_RESPONSE);
            if(ret != PAL_UPNP_E_SUCCESS)
            {
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_upnp_make_action error");

                event->request->error_code = ret;
                event->request->action_result = NULL;
            }
        } else {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "IGD_pii_add_portmapping_entry error");
            switch(ret) 
            { 
                case ERROR_WILDCARD_NOTPERMIT_FOR_SRC_IP: 
                    ret = WildCard_NotPermitted_InSrcIP;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, WildCard_NotPermitted_InSrcIP_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_WILDCARD_NOTPERMIT_FOR_EXTERNAL_PORT: 
                    ret = WildCard_NotPermitted_InExtPort;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, WildCard_NotPermitted_InExtPort_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_CONFLICT_FOR_MAPPING_ENTRY: 
                    ret = Conflict_In_MappingEntry;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, Conflict_In_MappingEntry_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_SAME_PORT_VALUE_REQUIRED: 
                    ret = Same_PortValues_Required;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, Same_PortValues_Required_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_ONLY_PERMANENT_LEASETIME_SUPPORTED: 
                    ret = Only_Permanent_Leases_Supported;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, Only_Permanent_Leases_Supported_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_REMOST_HOST_ONLY_SUPPORT_WILDCARD: 
                    ret = RemoteHost_OnlySupports_Wildcard;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, RemoteHost_OnlySupports_Wildcard_STR, PAL_UPNP_LINE_SIZE);
                    break;
                case ERROR_EXTERNAL_PORT_ONLY_SUPPORT_WILDCARD: 
                    ret = ExternalPort_OnlySupports_Wildcard;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, ExternalPort_OnlySupports_Wildcard_STR, PAL_UPNP_LINE_SIZE);
                    break;
                default:
                    ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
                    event->request->error_code = ret;
                    strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);
                    break;
            }
        }
        
        PAL_xml2s_free(&portmapEntry, tableAddPorMap);
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "EXIT %s...", __func__);

    return ret;
}

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
INT32 IGD_delete_PortMapping(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    PORT_MAP_INDEX portmapIndex;
    struct in_addr addr;
    INT32 ret;
    
    PAL_XML2S_TABLE tableDelPorMap[] = {
        {PM_SET[REMOTE_HOST],    PAL_XML2S_STRING,  XML2S_MSIZE(PORT_MAP_INDEX, remoteHost),  NULL, MASK_OF_INDEX_REMOTE_HOST},
        {PM_SET[EXTERNAL_PORT], PAL_XML2S_UINT16,   XML2S_MSIZE(PORT_MAP_INDEX, externalPort), NULL, MASK_OF_INDEX_EXTERNAL_PORT},
        {PM_SET[PROTOCOL],         PAL_XML2S_STRING,   XML2S_MSIZE(PORT_MAP_INDEX, pmProtocol),  NULL, MASK_OF_INDEX_PROTOCOL},
        XML2S_TABLE_END
    };

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ENTER %s...", __func__);

    if (!event || !(event->request)) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Input parameter error");

        ret = IGD_GENERAL_ERROR;
        return ret;
    }
    
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");

        ret = PAL_UPNP_SOAP_E_ACTION_FAILED;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_ACTION_FAILED), PAL_UPNP_LINE_SIZE);

        return ret;
    }
    
    bzero(&portmapIndex, sizeof(portmapIndex));
    ret = PAL_xml2s_process(event->request->action_request, tableDelPorMap, &portmapIndex);
    if (ret < 0){
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_xml2s_process error");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else if ((portmapIndex.remoteHost != NULL)
                &&(0 == inet_pton(AF_INET, portmapIndex.remoteHost, &addr))){ 
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "remoteHost format error: x.x.x.x");

        ret = PAL_UPNP_SOAP_E_INVALID_ARGS;
        event->request->error_code = ret;
        strncpy(event->request->error_str, PAL_upnp_get_error_message(PAL_UPNP_SOAP_E_INVALID_ARGS), PAL_UPNP_LINE_SIZE);
    } else {

        ret = IGD_pii_del_portmapping_entry(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 portmapIndex.remoteHost,
                                 portmapIndex.externalPort,
                                 portmapIndex.pmProtocol);

        if(ret == 0)
        {
            event->request->error_code = PAL_UPNP_E_SUCCESS;
            ret = PAL_upnp_make_action(&(event->request->action_result), event->request->action_name, 
                            event->service->type, 0, NULL, PAL_UPNP_ACTION_RESPONSE);
            if(ret != PAL_UPNP_E_SUCCESS)
            {
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "PAL_upnp_make_action error");

                event->request->error_code = ret;
                event->request->action_result = NULL;
            }
        } else {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "IGD_pii_del_portmapping_entry error");

            ret = NO_SUCH_ENTRY_IN_ARRAY;
            event->request->error_code = ret;
            strncpy(event->request->error_str, NO_SUCH_ENTRY_IN_ARRAY_STR, strlen(NO_SUCH_ENTRY_IN_ARRAY_STR) + 1);
        }
        
        PAL_xml2s_free(&portmapIndex, tableDelPorMap);
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "EXIT %s...", __func__);

    return ret;
}


