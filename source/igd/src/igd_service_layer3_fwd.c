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
 * FileName:   igd_service_layer3_fwd.c
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD Layer3Forwarding service
 *****************************************************************************/
/*$Id: igd_service_layer3_fwd.c,v 1.6 2009/05/26 09:41:32 jianxiao Exp $
 *
 *$Log: igd_service_layer3_fwd.c,v $
 *Revision 1.6  2009/05/26 09:41:32  jianxiao
 *Modify the  MACRO UPNP_UUID_LEN_BY_VENDER
 *
 *Revision 1.5  2009/05/21 06:31:06  jianxiao
 *Change the interface of PII
 *
 *Revision 1.4  2009/05/15 05:42:06  jianxiao
 *Add event handler
 *
 *Revision 1.3  2009/05/14 02:39:59  jianxiao
 *Modify the interface of PAL_xml_node_GetFirstbyName
 *
 *Revision 1.2  2009/05/14 01:46:12  jianxiao
 *Change the included header name
 *
 *Revision 1.1  2009/05/13 03:13:02  jianxiao
 *create orignal version
 *

 *
 **/
#include <stdio.h>
#include "pal_upnp_device.h"
#include "pal_log.h"
#include "igd_platform_independent_inf.h"
#include "igd_utility.h"

#define LAYER3FORWARDING_SERVICE_ID "urn:upnp-org:serviceId:L3Forwarding1"
#define LAYER3FORWARDING_SERVICE_TYPE "urn:schemas-upnp-org:service:Layer3Forwarding:1"
#define DEFAULT_CONNECTION_SERVICE_PARA_NUM 1
#define LAYER3FWD_MAX_EVENT_NUM 1

extern struct upnp_device IGD_device;
LOCAL INT32 _igd_set_DefaultConnectionService (INOUT struct action_event *event);
LOCAL INT32 _igd_get_DefaultConnectionService (INOUT struct action_event *event);
LOCAL VOID l3fwding_conn_service_init(IN struct upnp_service  *ps);

LOCAL struct upnp_variable DefaultConnectionService_variable[]=
{
		{"DefaultConnectionService",""},
		{NULL, ""}
};

LOCAL struct upnp_action Layer3Forwarding_actions[] =
{
	{"SetDefaultConnectionService", _igd_set_DefaultConnectionService },
	{"GetDefaultConnectionService", _igd_get_DefaultConnectionService },
	{NULL, NULL}
};

struct upnp_service Layer3Forwarding_service = {		
		.type				= "urn:schemas-upnp-org:service:Layer3Forwarding:1",
        .serviceID 			= LAYER3FORWARDING_SERVICE_ID,
        .actions   			= Layer3Forwarding_actions,
        .state_variables 	= DefaultConnectionService_variable,        
        .event_variables 	= DefaultConnectionService_variable
};
/************************************************************
 * Function: _igd_service_Layer3Forwarding_desc_file 
 *
 *  Parameters:	
 *      fp: Input/Output. the description file pointer.
 * 
 *  Description:
 *      This functions generate the description file of Layer3Forwarding.
 *
 *  Return Values: INT32
 *      0 if successful ,-1 for error
 ************************************************************/ 
LOCAL INT32 _igd_service_Layer3Forwarding_desc_file(INOUT FILE *fp)
{
	if(fp==NULL)
		return -1;
	fprintf(fp, "<service>\n");
      fprintf(fp, "<serviceType>%s</serviceType>\n",LAYER3FORWARDING_SERVICE_TYPE);
      fprintf(fp, "<serviceId>%s</serviceId>\n",LAYER3FORWARDING_SERVICE_ID);
	  fprintf(fp, "<SCPDURL>/Layer3ForwardingSCPD.xml</SCPDURL>\n");
      fprintf(fp, "<controlURL>/upnp/control/Layer3Forwarding</controlURL>\n");
      fprintf(fp, "<eventSubURL>/upnp/event/Layer3Forwarding</eventSubURL>\n");
    fprintf(fp, "</service>\n");
	return 0;
}
/************************************************************
* Function: _igd_service_Layer3Forwarding_destroy
*
*  Parameters: 
*	   pservice:		   IN. the service pointer. 
* 
*  Description:
*	  This function destroy the service Layer3Forwarding.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_service_Layer3Forwarding_destroy(IN struct upnp_service *pservice)
{
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Destroy IGD Layer3Forwarding\n");
	if(pservice==NULL)
		return -1;
	pthread_mutex_destroy(&pservice->service_mutex);
	return 0;
}
/************************************************************
* Function: IGD_service_Layer3ForwardingInit
*
*  Parameters: 
*	   input_index_struct:		   IN. the device index struct. 
*	   fp:   INOUT. the description file pointer. 
* 
*  Description:
*	  This function initialize the service Layer3Forwarding.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/ 
INT32 IGD_service_Layer3ForwardingInit(IN VOID* input_index_struct, INOUT FILE *fp)
{	
	(void) input_index_struct;
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Initilize IGD Layer3Forwarding\n");
	if(pthread_mutex_init(&Layer3Forwarding_service.service_mutex, NULL ))
	{
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "Layer3Forwarding Init mutex fail!\n");
        return -1;
	}
	if(_igd_service_Layer3Forwarding_desc_file(fp))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"create Layer3Forwarding description file fail!\n");
		_igd_service_Layer3Forwarding_destroy(&Layer3Forwarding_service);
		return -1;
	}
	Layer3Forwarding_service.destroy_function= _igd_service_Layer3Forwarding_destroy;

    l3fwding_conn_service_init(&Layer3Forwarding_service);
	return 0;
}
/************************************************************
* Function: _igd_check_DefaultConnectionService
*
*  Parameters: 
*
*	   connecion_service_string:   IN. the string be checked. 
* 
*  Description:
*	  This function check the string of DefaultConnectionService.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_check_DefaultConnectionService (IN const CHAR *connecion_service_string)
{	
	struct upnp_device *dev = &IGD_device;
	struct upnp_device *connection_device=NULL;
	struct upnp_service *connection_service=NULL;
	CHAR check_string[PAL_UPNP_NAME_SIZE];
	CHAR udn[UPNP_UUID_LEN_BY_VENDER];
	CHAR *service_ID;
	INT32 i=0;
	BOOL  check_service=BOOL_FALSE;
	
	if(connecion_service_string == NULL)
		return ACTION_FAIL;

	if(strlen(connecion_service_string)<UPNP_UUID_LEN_BY_VENDER)
		return INVALID_DEVICE_UUID;
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"check the string:%s\n",connecion_service_string);
	
	strncpy(check_string,connecion_service_string,strlen(connecion_service_string));
	strncpy(udn,connecion_service_string,UPNP_UUID_LEN_BY_VENDER);
	udn[UPNP_UUID_LEN_BY_VENDER-1]='\0';	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"UUID:%s\n",udn);
	/*check the uuid */
	while(dev)
	{	
		if (strncmp(dev->udn,udn,strlen(dev->udn)) == 0)
		{
			connection_device = dev;
			break;
		}
		dev = dev->next;
	}
	if(NULL == connection_device)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Can't find %s\n",udn);
		return INVALID_DEVICE_UUID;
	}
	/*get the ServiceID*/
	service_ID=strtok(check_string,",");
	if(NULL==service_ID) 
		return INVALID_SERVICE_ID;
	service_ID = (CHAR *)((INT32)connecion_service_string + strlen(service_ID)+1);
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"service ID:%s\n",service_ID);
	/*check the ServiceID */
	dev = &IGD_device;
	while(dev)
	{	
		i=0;
		while (connection_service = dev->services[i], connection_service != NULL) 
		{
			if (strncmp(connection_service->serviceID, service_ID, strlen(connection_service->serviceID)) == 0)
			{
				check_service = BOOL_TRUE;
				goto CHECK_INVALID_CONN_SERVICE_SELECTION;
			}
			i++;
		}
		dev = dev->next;
	}
	return INVALID_SERVICE_ID;
	
CHECK_INVALID_CONN_SERVICE_SELECTION:
	/*Check the selected connection service instance can be set as a default connection or not*/
	i=0;
	while (connection_service = connection_device->services[i], connection_service != NULL) 
	{
		if ((strncmp(connection_service->serviceID, service_ID, strlen(connection_service->serviceID)) == 0) && \
			((strncmp("urn:upnp-org:serviceId:WANIPConn", service_ID, strlen("urn:upnp-org:serviceId:WANIPConn")) == 0) || \
			(strncmp("urn:upnp-org:serviceId:WANPPPConn", service_ID, strlen("urn:upnp-org:serviceId:WANPPPConn")) == 0)))
			return 0;		
		i++;
	}	
	return INVALID_CONN_SERVICE_SELECTION;
}
/************************************************************
* Function: _igd_set_DefaultConnectionService
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of SetDefaultConnectionService.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_set_DefaultConnectionService (INOUT struct action_event *event)
{
	CHAR *value=NULL;
	INT32 ret = 0;
	CHAR *var_name[LAYER3FWD_MAX_EVENT_NUM] = {0};
    CHAR *var_value[LAYER3FWD_MAX_EVENT_NUM] = {0};
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:SetDefaultConnectionService\n");
	
	value = PAL_xml_node_get_value(PAL_xml_node_GetFirstbyName(event->request->action_request,"NewDefaultConnectionService",NULL));
	if(value == NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:No NewDefaultConnectionService\n");
		event->request->error_code = INVALID_ARGS;
		goto erro_response;
	}

	ret = _igd_check_DefaultConnectionService(value);
	if(ret == 0)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"NewDefaultConnectionService:%s\n",value);
		strncpy(event->service->state_variables[0].value,value,strlen(value)+1);
		strncpy(event->service->event_variables[0].value,value, strlen(value)+1);
		var_name[0] = (CHAR *)event->service->event_variables[0].name;
        var_value[0] = event->service->event_variables[0].value;
		ret = PAL_upnp_notify(PAL_upnp_device_getHandle(),IGD_device.udn,event->service->serviceID,(const CHAR **)var_name,(const CHAR **)var_value,1);
		if(ret)
		{
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_notify() fail, error code=%d", ret);
		}		
		event->request->error_code = PAL_UPNP_E_SUCCESS;
	}
	else if(INVALID_DEVICE_UUID == ret)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:InvalidDeviceUUID\n");
		strncpy(event->request->error_str, "InvalidDeviceUUID",PAL_UPNP_LINE_SIZE);
		event->request->error_code = INVALID_DEVICE_UUID;
	}
	else if(INVALID_SERVICE_ID == ret)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:InvalidServiceID\n");
		strncpy(event->request->error_str, "InvalidServiceID",PAL_UPNP_LINE_SIZE);
		event->request->error_code = INVALID_SERVICE_ID;
	}
	else if(INVALID_CONN_SERVICE_SELECTION == ret)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:The selected connection service instance cannot be set as a default connection\n");
		strncpy(event->request->error_str, "InvalidConnServiceSelection",PAL_UPNP_LINE_SIZE);
		event->request->error_code = INVALID_CONN_SERVICE_SELECTION;
	}
	else
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:Action fail\n");
		event->request->error_code = ACTION_FAIL;
	}
	
erro_response:
	PAL_upnp_make_action(&event->request->action_result,"SetDefaultConnectionService",LAYER3FORWARDING_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}
/************************************************************
* Function: _igd_get_DefaultConnectionService
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetDefaultConnectionService.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_DefaultConnectionService (IN struct action_event *event)
{
	pal_string_pair params[DEFAULT_CONNECTION_SERVICE_PARA_NUM];

	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:GetDefaultConnectionService\n");
	event->request->error_code = PAL_UPNP_E_SUCCESS;

	params[0].name="NewDefaultConnectionService";
	params[0].value=event->service->state_variables[0].value;	
	PAL_upnp_make_action(&event->request->action_result,"GetDefaultConnectionService",LAYER3FORWARDING_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	return(event->request->error_code);
}

const static char *defaultConnService = "uuid:%s:WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1";
const static char *l3fwd = "layer3forwarding";

LOCAL VOID l3fwding_conn_service_init(IN struct upnp_service  *ps)
{
    CHAR connService[PAL_UPNP_LINE_SIZE] = {0};
    CHAR baseUuid[UPNP_UUID_LEN_BY_VENDER] = {0};

    if (NULL == ps){
        PAL_LOG(l3fwd, 1, "layer3forwarding service is NULL");
        return;
    }

    pthread_mutex_lock(&ps->service_mutex);

    IGD_pii_get_uuid(baseUuid);
    snprintf(connService, sizeof(connService), defaultConnService, baseUuid);
    
    strncpy(ps->state_variables[0].value, connService, strlen(connService));
    strncpy(ps->event_variables[0].value, connService, strlen(connService));

    pthread_mutex_unlock(&ps->service_mutex);
    return;
}



