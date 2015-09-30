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
 * FileName:   igd_device_wan.c
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD WANDevice
 *****************************************************************************/
/*$Id: igd_device_wan.c,v 1.5 2009/05/26 09:40:55 jianxiao Exp $
 *
 *$Log: igd_device_wan.c,v $
 *Revision 1.5  2009/05/26 09:40:55  jianxiao
 *Modify the function IGD_pii_get_uuid
 *
 *Revision 1.4  2009/05/21 06:31:06  jianxiao
 *Change the interface of PII
 *
 *Revision 1.3  2009/05/15 05:41:50  jianxiao
 *Add event handler
 *
 *Revision 1.2  2009/05/14 02:39:43  jianxiao
 *Modify the interface of the template
 *
 *Revision 1.1  2009/05/13 03:13:02  jianxiao
 *create orignal version
 *

 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pal_upnp_device.h"
#include "pal_def.h"
#include "pal_log.h"
#include "pal_kernel.h"
#include "igd_platform_independent_inf.h"
#include "igd_utility.h"

extern struct upnp_service* IGD_service_WANCommonInterfaceConfigInit(IN VOID* input_index_struct, INOUT FILE *fp);
extern VOID IGD_service_WANCommonInterfaceConfigEventHandler(IN struct upnp_device  *pdevice,IN struct upnp_service  *pservice);
extern struct upnp_device *IGD_wan_connection_device_init (IN VOID* input_index_struct,IN const CHAR *udn,INOUT FILE *wan_desc_file);
extern VOID IGD_WANCommonInterfaceConfig_eventvariables_init(struct upnp_service *ps);

/************************************************************
 * Function: _igd_wan_device_desc_file 
 *
 *  Parameters:	
 *      uuid: Input. the uuid of the WANDevice.
 *      fp: Input/Output. the description file pointer.
 * 
 *  Description:
 *      This functions generate the description file of the WANDevice.
 *
 *  Return Values: INT32
 *      0 if successful ,-1 for error
 ************************************************************/ 
LOCAL INT32 _igd_wan_device_desc_file(INOUT FILE *fp,IN const CHAR *uuid)
{
	if(fp==NULL)
		return -1;
	fprintf(fp, "<device>\n");
		fprintf(fp, "<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\n");
		fprintf(fp, "<friendlyName>%s</friendlyName>\n",WANDEVICE_FRIENDLY_NAME);
		fprintf(fp, "<manufacturer>%s</manufacturer>\n",MANUFACTURER);
		fprintf(fp, "<manufacturerURL>%s</manufacturerURL>\n",MANUFACTURER_URL);
		fprintf(fp, "<modelDescription>%s</modelDescription>\n",MODULE_DESCRIPTION);
		fprintf(fp, "<modelName>%s</modelName>\n",MODULE_NAME);
		fprintf(fp, "<modelNumber>%s</modelNumber>\n",MODULE_NUMBER);
		fprintf(fp, "<modelURL>%s</modelURL>\n",MODULE_URL);
		fprintf(fp, "<serialNumber>%s</serialNumber>\n",IGD_pii_get_serial_number());
		fprintf(fp, "<UDN>%s</UDN>\n", uuid);
		fprintf(fp, "<UPC>%s</UPC>\n",UPC);
		fprintf(fp, "<serviceList>\n");
	return 0;
}
/************************************************************
* Function: _igd_wan_device_destroy
*
*  Parameters: 
*	   pdevice:		   IN. Upnp device pointer. 
* 
*  Description:
*	  This function destroy the WANDevice.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_wan_device_destroy (IN struct upnp_device *pdevice)
{
	INT32 i=0;

	if(NULL == pdevice)
		return -1;
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Destroy WANDevice\n");
	if(pdevice->services)
	{
		while(pdevice->services[i])
		{
			if(pdevice->services[i]->destroy_function)
				pdevice->services[i]->destroy_function(pdevice->services[i]);
			i++;
		}
	}
	SAFE_FREE(pdevice->services);
	SAFE_FREE(pdevice);
	return 0;
}

/************************************************************
* Function: IGD_device_WANDeviceInit
*
*  Parameters: 
*	   input_index_struct:		   IN. the device index struct. 
*	   udn:   IN. the uuid of the WANDevice. 
*	   fp:   INOUT. the description file pointer. 
* 
*  Description:
*	  This function initialize the WANDevice.  
*
*  Return Values: struct upnp_service*
*	   The device pointer if successful else NULL.
************************************************************/
struct upnp_device * IGD_device_WANDeviceInit(IN VOID * input_index_struct, IN const CHAR *udn, INOUT FILE *fp)
{
	struct upnp_device *wan_connection_device=NULL;
	struct upnp_device *wandevice=NULL;
	struct upnp_device *next_device=NULL;
	struct upnp_service *WANCommonInterfaceConfig_service=NULL;
	struct device_and_service_index wan_index;
	INT32 wan_connection_index=1;
	INT32 wan_connection_device_number = 0;
	CHAR device_udn[UPNP_UUID_LEN_BY_VENDER];
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Initilize WANDevice %d\n",((struct device_and_service_index*)input_index_struct)->wan_device_index);
	wandevice=(struct upnp_device *)calloc(1,sizeof(struct upnp_device));
	if(wandevice==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,wandevice!\n");
		return NULL;
	}
	
	wandevice->destroy_function=_igd_wan_device_destroy;
	strncpy(wandevice->udn, udn, UPNP_UUID_LEN_BY_VENDER);

	if(_igd_wan_device_desc_file(fp,udn))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"create WANDevice description file fail!\n");
		SAFE_FREE(wandevice);
		return NULL;
	}

	wandevice->services = (struct upnp_service **)calloc(2,sizeof(struct upnp_service *));
	if(wandevice->services==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory!\n");
		SAFE_FREE(wandevice);
		return NULL;
	}
		
	if((WANCommonInterfaceConfig_service=IGD_service_WANCommonInterfaceConfigInit(input_index_struct,fp))==NULL)
    {
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"WANCommonInterfaceConfig init fail, %s");
		SAFE_FREE(wandevice->services);
		SAFE_FREE(wandevice);
        return NULL;
    }
	wandevice->services[0]=WANCommonInterfaceConfig_service;
	wandevice->services[1]=NULL;

    /* init WANCommonInterfaceConfig_service state_variables */
    IGD_WANCommonInterfaceConfig_eventvariables_init(WANCommonInterfaceConfig_service);
    	
	/*register the event handler*/
	IGD_timer_register(wandevice,WANCommonInterfaceConfig_service, IGD_service_WANCommonInterfaceConfigEventHandler, 2, timer_function_mode_cycle);

	fprintf(fp, "</serviceList>\n");
	fprintf(fp, "<deviceList>\n");
	wan_connection_device_number = IGD_pii_get_wan_connection_device_number(((struct device_and_service_index*)input_index_struct)->wan_device_index);
	if(wan_connection_device_number <= 0)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"wan_connection_device_number error:%d\n",wan_connection_device_number);
		SAFE_FREE(wandevice->services);
		SAFE_FREE(wandevice);
        return NULL;
	}
	
	while(wan_connection_index < wan_connection_device_number + 1)
	{
		memset(&wan_index,0,sizeof(struct device_and_service_index));
		wan_index.wan_device_index = ((struct device_and_service_index*)input_index_struct)->wan_device_index;
		wan_index.wan_connection_device_index= wan_connection_index;

		if(IGD_pii_get_uuid(device_udn))
		{
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"Get UUID fail\n");
			goto Destroy_device_recursively;
		}
		wan_connection_device = IGD_wan_connection_device_init((VOID*)(&wan_index),device_udn,fp);
		if(NULL == wan_connection_device)
		{
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"IGD WAN connection device:%d init failed\n", wan_connection_index);
			/*because return NULL so upnp_device_destroy() will not destroy the initialized WANConnectionDevice*/
	Destroy_device_recursively:
			while(wandevice!=NULL)
			{
				next_device=wandevice->next;
				if(wandevice->destroy_function)
					wandevice->destroy_function(wandevice);
				SAFE_FREE(wandevice);
				wandevice = next_device;
			}
			return NULL;
		}
		else
		{
			next_device = wandevice;
			while(next_device->next!=NULL)
				next_device=next_device->next;
			next_device->next= wan_connection_device;
			wan_connection_device->next = NULL;
		}
		wan_connection_index++;
	}
	
	fprintf(fp, "</deviceList>\n");
	fprintf(fp, "</device>\n");
	
	return wandevice;
}


