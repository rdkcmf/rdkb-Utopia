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
 * FileName:   igd_device_lan.c
 * Cloned by: Sridhar Ramaswamy
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD LANDevice
 *****************************************************************************/
/*$Id: igd_device_lan.c,v 1.5 2009/05/26 09:40:55 jianxiao Exp $
 *
 *$Log: igd_device_lan.c,v $
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

extern struct upnp_service* IGD_service_LANHostConfigManagementInit(IN VOID* input_index_struct, INOUT FILE *fp);

/************************************************************
 * Function: _igd_lan_device_desc_file 
 *
 *  Parameters:	
 *      uuid: Input. the uuid of the LANDevice.
 *      fp: Input/Output. the description file pointer.
 * 
 *  Description:
 *      This functions generate the description file of the LANDevice.
 *
 *  Return Values: INT32
 *      0 if successful ,-1 for error
 ************************************************************/ 
LOCAL INT32 _igd_lan_device_desc_file(INOUT FILE *fp,IN const CHAR *uuid)
{
	if(fp==NULL)
		return -1;
	fprintf(fp, "<device>\n");
		fprintf(fp, "<deviceType>urn:schemas-upnp-org:device:LANDevice:1</deviceType>\n");
		fprintf(fp, "<friendlyName>%s</friendlyName>\n",LANDEVICE_FRIENDLY_NAME);
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
* Function: _igd_lan_device_destroy
*
*  Parameters: 
*	   pdevice:		   IN. Upnp device pointer. 
* 
*  Description:
*	  This function destroy the LANDevice.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_lan_device_destroy (IN struct upnp_device *pdevice)
{
	INT32 i=0;

	if(NULL == pdevice)
		return -1;
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Destroy LANDevice\n");
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
* Function: IGD_device_LANDeviceInit
*
*  Parameters: 
*	   input_index_struct:		   IN. the device index struct. 
*	   udn:   IN. the uuid of the LANDevice. 
*	   fp:   INOUT. the description file pointer. 
* 
*  Description:
*	  This function initialize the LANDevice.  
*
*  Return Values: struct upnp_service*
*	   The device pointer if successful else NULL.
************************************************************/
struct upnp_device * IGD_device_LANDeviceInit(IN VOID * input_index_struct, IN const CHAR *udn, INOUT FILE *fp)
{
	struct upnp_device *landevice=NULL;
	struct upnp_service *LANHostConfigManagement_service=NULL;
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Initilize LANDevice %d\n",((struct device_and_service_index*)input_index_struct)->lan_device_index);

	landevice=(struct upnp_device *)calloc(1,sizeof(struct upnp_device));
	if(landevice==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,landevice!\n");
		return NULL;
	}
	
	landevice->destroy_function=_igd_lan_device_destroy;
	strncpy(landevice->udn, udn, UPNP_UUID_LEN_BY_VENDER);

	if(_igd_lan_device_desc_file(fp,udn))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"create LANDevice description file fail!\n");
		SAFE_FREE(landevice);
		return NULL;
	}

	landevice->services = (struct upnp_service **)calloc(2,sizeof(struct upnp_service *));
	if(landevice->services==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory!\n");
		SAFE_FREE(landevice);
		return NULL;
	}
		
	if((LANHostConfigManagement_service=IGD_service_LANHostConfigManagementInit(input_index_struct,fp))==NULL)
    {
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"LANHostConfigManagement init fail, %s");
		SAFE_FREE(landevice->services);
		SAFE_FREE(landevice);
        return NULL;
    }
	landevice->services[0]=LANHostConfigManagement_service;
	landevice->services[1]=NULL;

	
	/*no event handler registered as this device/services doesn't 
         any eventable variables*/

	fprintf(fp, "</serviceList>\n");
	fprintf(fp, "</device>\n");
	
	return landevice;
}


