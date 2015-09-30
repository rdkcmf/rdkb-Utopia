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
 *    FileName:    igd_device_wan_connect.c
 *      Author:    Tao Hong(tahong@cisco.com)
 *        Date:    2009-05-03
 * Description:    WAN connection Device implementation code of UPnP IGD project
  * sa record:  http://wwwin-ses.cisco.com/sa_web_results_sh_linksys/tahong.5_7_2009igd.2/
 *****************************************************************************/
/*$Id: igd_device_wan_connect.c,v 1.5 2009/05/21 06:29:48 jianxiao Exp $
 *
 *$Log: igd_device_wan_connect.c,v $
 *Revision 1.5  2009/05/21 06:29:48  jianxiao
 *Modify the macro to IGD_pii_get_serial_number
 *
 *Revision 1.4  2009/05/15 09:25:37  tahong
 *use MACRO to construct description file of wan connection file
 *
 *Revision 1.3  2009/05/15 05:41:58  jianxiao
 *Add event handler
 *
 *Revision 1.2  2009/05/14 01:44:26  jianxiao
 *Change init interface of  WANEthernetLinkConfig
 *
 *Revision 1.1  2009/05/13 08:56:54  tahong
 *create orignal version
 *
 *
 **/

//#include <assert.h>

#include <stdlib.h>

#include "igd_utility.h"
#include "pal_log.h"

#include "igd_platform_independent_inf.h"

#include "igd_service_wan_connect.h"

#ifndef LOG_ENTER_FUNCTION
#define LOG_ENTER_FUNCTION  PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "entering %s", __func__)
#endif

#ifndef LOG_LEAVE_FUNCTION
#define LOG_LEAVE_FUNCTION  PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "leaving %s", __func__)
#endif

#define WANCONNECTIONSERVICEUPDATE_SECOND 2 
#define PORTMAPPING_LEASE_UPDATE 1 /* changed to support decrementing leaseDuration every second */

#define WAN_CONNECTION_DEVICE_TYPE "urn:schemas-upnp-org:device:WANConnectionDevice:1"

extern struct upnp_service* IGD_service_WANEthernetLinkConfigInit(IN VOID* input_index_struct, INOUT FILE *fp);
extern VOID IGD_service_WANEthernetLinkConfigEventHandler(IN struct upnp_device  *pdevice,IN struct upnp_service  *pservice);
extern VOID IGD_update_wan_connection_service(struct upnp_device  *pd, struct upnp_service  *ps);
extern VOID IGD_update_pm_lease_time(struct upnp_device *pd, struct upnp_service *ps);
LOCAL INT32 _wan_connection_device_destroy(struct upnp_device *device);

/************************************************************
* Function: IGD_wan_connection_device_init
*
*  Parameters:
*               input_index_struct:    IN.          Wan device index and wan connection device index
*               udn:                         IN.          The udn assigned to this wan connection device
*               wan_desc_file:          INOUT.    The fd to write description file
*  Description:
*     This function is called by wan_device_init() to initialize a wan connection device instance
*
*  Return Values: struct upnp_device *
*               the initialized wan connection device if successful else NULL
************************************************************/
struct upnp_device *IGD_wan_connection_device_init (IN VOID* input_index_struct,
                                                     IN const CHAR *udn,
                                                     INOUT FILE *wan_desc_file)
{
    INT32 rv = 0;
    struct device_and_service_index* temp_index = NULL;
    struct upnp_device *new_wan_connection_device = NULL;
    INT32 wan_ppp_service_number = 0;
    INT32 wan_ip_service_number = 0;
    INT32 i=0;

    /*register module name and callback to log*/
    PAL_LOG_REGISTER(WAN_CONNECTION_DEVICE_LOG_NAME, NULL);

    LOG_ENTER_FUNCTION;
    IGD_timer_start();

    /*check input parameters*/
    temp_index = (struct device_and_service_index*)input_index_struct;
    if ( (!temp_index)
         ||(temp_index->wan_device_index<0)
         || (temp_index->wan_connection_device_index<0)
         || (!udn)
         || (!wan_desc_file)
         )
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "input parameter error");
        return NULL;
    }

    /*malloc a new_wan_connection_device */
    new_wan_connection_device = (struct upnp_device *)calloc(1, sizeof(struct upnp_device));
    if (!new_wan_connection_device)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory : new_wan_connection_device malloc error");
        return NULL;
    }

    /*assign init_function*/
    /*new_wan_connection_device->init_function = NULL;*/

    /*assign destroy_function*/
    new_wan_connection_device->destroy_function = _wan_connection_device_destroy;
    
    /*assign udn*/
    rv = snprintf(new_wan_connection_device->udn,strlen(udn)+1, "%s", udn);
    if ( rv < 0 )
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "print content to udn error");
        _wan_connection_device_destroy(new_wan_connection_device);
        return NULL;
    }

    /*assign services*/
    wan_ppp_service_number = IGD_pii_get_wan_ppp_service_number(temp_index->wan_device_index, temp_index->wan_connection_device_index);
    wan_ip_service_number = IGD_pii_get_wan_ip_service_number(temp_index->wan_device_index, temp_index->wan_connection_device_index);
    if ( (wan_ppp_service_number<0) || (wan_ip_service_number<0))
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "wan_ppp_service_number or wan_ip_service_number error");
        _wan_connection_device_destroy(new_wan_connection_device);
        return NULL;    
    }
    new_wan_connection_device->services = (struct upnp_service **)calloc(wan_ppp_service_number+wan_ip_service_number+1, sizeof(struct upnp_service*));
    if (!new_wan_connection_device->services)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory, malloc services error!");
        _wan_connection_device_destroy(new_wan_connection_device);
        return NULL;            
    }

    /*generate device description file head*/
    fprintf(wan_desc_file,
"    <device>\n"
"        <deviceType>%s</deviceType>\n"
"        <friendlyName>%s</friendlyName>\n"
"        <manufacturer>%s</manufacturer>\n"
"        <manufacturerURL>%s</manufacturerURL>\n"
"        <modelDescription>%s</modelDescription>\n"
"        <modelName>%s</modelName>\n"
"        <modelNumber>%s</modelNumber>\n"
"        <modelURL>%s</modelURL>\n"
"        <serialNumber>%s</serialNumber>\n"
"        <UDN>%s</UDN>\n"
"        <UPC>%s</UPC>\n"
"        <serviceList>\n",
        WAN_CONNECTION_DEVICE_TYPE,
        WAN_CONNECTION_DEVICE_FRIENDLY_NAME,
        MANUFACTURER,
        MANUFACTURER_URL,
        MODULE_DESCRIPTION,
        MODULE_NAME,
        MODULE_NUMBER,
        MODULE_URL,
        IGD_pii_get_serial_number(),
        udn,
        UPC
        );
    /*init services*/
    for (i=0; i<wan_ppp_service_number+wan_ip_service_number; i++)
    {
        if (i < wan_ppp_service_number)
        {
            temp_index->wan_connection_service_index = i+1;//begin from 1
            new_wan_connection_device->services[i] = IGD_wan_ppp_connection_service_init(temp_index, wan_desc_file);
            IGD_timer_register(new_wan_connection_device, new_wan_connection_device->services[i], IGD_update_wan_connection_service, WANCONNECTIONSERVICEUPDATE_SECOND, timer_function_mode_cycle);
        }
        else if (i < wan_ppp_service_number+wan_ip_service_number)
        {
            temp_index->wan_connection_service_index = i-wan_ppp_service_number+1;//begin from 1
            new_wan_connection_device->services[i] = IGD_wan_ip_connection_service_init(temp_index, wan_desc_file);
            IGD_timer_register(new_wan_connection_device, new_wan_connection_device->services[i], IGD_update_wan_connection_service, WANCONNECTIONSERVICEUPDATE_SECOND, timer_function_mode_cycle);
            IGD_timer_register(new_wan_connection_device, new_wan_connection_device->services[i], IGD_update_pm_lease_time, PORTMAPPING_LEASE_UPDATE, timer_function_mode_cycle);
        }

        if (!new_wan_connection_device->services[i])
        {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "init services error!");
            _wan_connection_device_destroy(new_wan_connection_device);
            return NULL;            
        }
    }
    /*new_wan_connection_device->services[wan_ppp_service_number+wan_ip_service_number+1] == NULL, used in wan_connection_device_destroy() as the guard*/

    /*generate device description file tail*/
    fprintf(wan_desc_file,
"        </serviceList>\n"
"    </device>\n");

    /*assign next = NULL*/
    /*new_wan_connection_device->next = NULL;*/

    LOG_LEAVE_FUNCTION;
    return new_wan_connection_device;
}

/************************************************************
* Function: wan_connection_device_destroy
*
*  Parameters:
*               device:                     IN.    Wan device to be destroyed
*  Description:
*     This function is called by wan_device_destroy() to destroy a wan connection device instance
*
*  Return Values: INT32
*      0 if successful else error code.
************************************************************/
LOCAL INT32 _wan_connection_device_destroy(struct upnp_device *device)
{
    INT32 i=0;

    LOG_ENTER_FUNCTION;
    IGD_timer_stop();

    /*check input parameters*/
    if (!device)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "input parameter error");
        return 0;
    }

    if (device->services)
    {
        while(device->services[i])
        {
            if (device->services[i]->destroy_function)
            {
                device->services[i]->destroy_function(device->services[i]);
            }
            i++;
        }
    }
    free(device->services);

    free(device);

    LOG_LEAVE_FUNCTION;
    return 0;
}

