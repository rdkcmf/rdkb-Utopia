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
 *    FileName:    upnp_device.c
 *      Author:    Jerry Liu(zhiyliu@cisco.com)
 *        Date:    2009-04-15
 * Description:    UPnP Device template implementation code of UPnP IGD project
 *****************************************************************************/
/*$Id: pal_upnp_device.c,v 1.2 2009/05/15 08:00:21 bowan Exp $
 *
 *$Log: pal_upnp_device.c,v $
 *Revision 1.2  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.1  2009/05/14 01:52:13  zhiyliu
 *init version
 *
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "pal_upnp.h"
#include "pal_xml.h"
#include "pal_upnp_device.h"
#include "pal_log.h"

#define UPNP_DEVICE_INITED		1
#define UPNP_DEVICE_NOT_INIT	0

#define DEFAULT_MAX_CONTENT_LENGTH 16000
#define DEFAULT_WEB_DIR "/var/upnp"
#define DEFAULT_DEVICE_DESC_NAME "upnp_device_desc.xml"
#define DEFAULT_TIMEOUT_VALUE 1800


LOCAL pal_upnp_device_handle device_handle;

LOCAL struct upnp_device *upnp_device;

LOCAL INT32 upnp_initStatus = UPNP_DEVICE_NOT_INIT;


/************************************************************
 * Function: _find_device
 *
 *  Parameters:	
 *      device: 		Input. upnp device list. 
 *      service: 		Input. the specific device name. 
 * 
 *  Description:
 *     This function find the spicific device in the device list.  
 *
 *  Return Values: struct upnp_device *
 *      upnp device pointer, NULL means find none
 ************************************************************/ 
LOCAL struct upnp_device *_find_device(struct upnp_device *device,  CHAR *device_name)
{
	struct upnp_device *dev = device;
	
	if(NULL == dev)
		return NULL;

	while(dev)
	{
	
		if (strncmp(dev->udn, device_name, strlen(device_name)) == 0)
			return dev;
		
		dev = dev->next;
	}

	return NULL;
}


/************************************************************
 * Function: _find_service
 *
 *  Parameters:	
 *      device: 		Input. upnp device pointer. 
 *      service: 		Input. upnp service ID. 
 * 
 *  Description:
 *     This function find the spicific service in one device's service list.  
 *
 *  Return Values: struct upnp_service *
 *      upnp service pointer, NULL means find none
 ************************************************************/ 
LOCAL struct upnp_service *_find_service(struct upnp_device *device,  CHAR *serviceID)
{
	struct upnp_service *event_service;
	INT32 i = 0;

	if(NULL == device)
		return NULL;
	
	while (event_service = device->services[i], event_service != NULL) 
	{
		if (strncmp(event_service->serviceID, serviceID, strlen(serviceID)) == 0)
			return event_service;
		
		i++;
	}
	return NULL;
}



/************************************************************
 * Function: _find_action
 *
 *  Parameters:	
 *      device: 		Input. upnp service pointer. 
 *      service: 		Input. the specific upnp action name. 
 * 
 *  Description:
 *     This function find the spicific action in the service's action list.  
 *
 *  Return Values: struct upnp_action *
 *      upnp action pointer, NULL means find none
 ************************************************************/ 
LOCAL struct upnp_action *_find_action(struct upnp_service *event_service, CHAR *action_name)
{
	struct upnp_action *event_action;
	INT32 i = 0;
	if(NULL == event_service || NULL == action_name)
		return NULL;
	event_action = &(event_service->actions[i]);
	while (event_action != NULL && event_action->action_name != NULL) 
	{
		if (strncmp(event_action->action_name, action_name, strlen(action_name)) == 0)
			return event_action;
		i++;
        	event_action = &(event_service->actions[i]);
	}
	return NULL;
}



/************************************************************
 * Function: upnp_ControlGetVar_request
 *
 *  Parameters:	
 *      cgv_event: 		Input. upnp control get variable request event. 
 * 
 *  Description:
 *     This function handles a upnp control get variable request event.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/ 
LOCAL INT32 _handle_ControGetVar_request(pal_upnp_state_var_request *cgv_event)
{
    INT32 getvar_succeeded = 0;
	struct upnp_device *dev;
	struct upnp_service *srv;
	struct upnp_variable *var;
	INT32 result = -1;
	INT32 i = 0;

	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp device udn: %s\n", cgv_event->dev_udn);
	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp service: %s\n", cgv_event->service_id);

    cgv_event->CurrentVal = NULL;

	dev = _find_device(upnp_device, cgv_event->dev_udn);
	if(NULL == dev)
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Unknown device '%s'\n", cgv_event->dev_udn);
		return result;
	}
	
	srv = _find_service(dev, cgv_event->service_id);
	if (NULL == srv) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Unknown service '%s'\n", cgv_event->service_id);
		return result;
	}

	pthread_mutex_lock(&(srv->service_mutex));

	//check variable name
	while (var = &(srv->state_variables[i]), var->name != NULL) 
	{
		if( strcmp( cgv_event->statvar_name,var->name) == 0) 
		{
        	getvar_succeeded = 1;
			cgv_event->CurrentVal = PAL_xml_escape(srv->state_variables[i].value, 0);
			break;
		}
		i++;
    }

    if( getvar_succeeded ) 
        cgv_event->error_code= PAL_UPNP_E_SUCCESS;
    else 
	{
        PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Error in UPNP_CONTROL_GET_VAR_REQUEST callback:\n");
        PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Unknown variable name = %s\n",cgv_event->statvar_name);
        cgv_event->error_code= 404;
        strncpy(cgv_event->err_str, "Invalid Variable", PAL_UPNP_LINE_SIZE);
    }

    pthread_mutex_unlock(&(srv->service_mutex));

    if(PAL_UPNP_E_SUCCESS == cgv_event->error_code)
		result = 0;

	return result;
}


/************************************************************
 * Function: upnp_subscription_request
 *
 *  Parameters:	
 *      sr_event: 		Input. upnp subscription request event. 
 * 
 *  Description:
 *     This function handles a upnp subscription request event.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/ 
LOCAL INT32 _handle_subscription_request(pal_upnp_subscription_request *sr_event)
{
	struct upnp_device *dev;
	struct upnp_service *srv;
	INT32 i=0;
	INT32 eventVarNum = 0;
	const CHAR **eventvar_names;
	struct upnp_variable *var;
	CHAR **eventvar_values;
	INT32 ret;
	INT32 result = -1;

	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp device udn: %s\n", sr_event->UDN);
	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp service: %s\n", sr_event->ServiceId);

	dev = _find_device(upnp_device, sr_event->UDN);
	if (NULL == dev) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "%s: Unknown device '%s'\n", sr_event->UDN);
		return result;
	}
	
	srv = _find_service(dev, sr_event->ServiceId);
	if (NULL == srv) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "%s: Unknown service '%s'\n", sr_event->ServiceId);
		return result;
	}

	pthread_mutex_lock(&(srv->service_mutex));

	/* generate list of eventable variables */
	while(var = &(srv->event_variables[i]), var->name != NULL) 
	{
		eventVarNum++;
		i++;
	}
	eventvar_names = malloc((eventVarNum+1) * sizeof(const CHAR *));
	if(NULL == eventvar_names)
	{
		pthread_mutex_unlock(&(srv->service_mutex));
		return result;
	}
	
	eventvar_values = malloc((eventVarNum+1) * sizeof(const CHAR *));
	if(NULL == eventvar_values)
	{
		free(eventvar_names);
		pthread_mutex_unlock(&(srv->service_mutex));
		return result;
	}

	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "%d evented variables\n", eventVarNum);

	for(i=0; i<eventVarNum; i++) 
	{
		eventvar_names[i] = srv->event_variables[i].name;
		eventvar_values[i] = PAL_xml_escape(srv->event_variables[i].value, 0);
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "Evented:'%s' = '%s'\n", eventvar_names[i], eventvar_values[i]);
	}
	eventvar_names[i] = NULL;
	eventvar_values[i] = NULL;

	ret = PAL_upnp_accept_subscription(device_handle,
			       						sr_event->UDN, sr_event->ServiceId,
			       						(const CHAR **)eventvar_names,
			       						(const CHAR **)eventvar_values,
			       						eventVarNum,
			       						sr_event->Sid);
	if (PAL_UPNP_E_SUCCESS == ret)
	{
		result = 0;
	}

	pthread_mutex_unlock(&(srv->service_mutex));

	for(i=0; i<eventVarNum; i++) 
	{
		free(eventvar_values[i]);
	}
	free(eventvar_names);
	free(eventvar_values);

	return result;
}


/************************************************************
 * Function: upnp_action_request
 *
 *  Parameters:	
 *      ar_event: 		Input. upnp action request event. 
 * 
 *  Description:
 *     This function handles a upnp action request event.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/ 
LOCAL INT32 _handle_action_request(pal_upnp_action_request *ar_event)
{
	struct upnp_device *event_device;
	struct upnp_service *event_service;
	struct upnp_action *event_action;

	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp device udn: %s\n", ar_event->dev_udn);
	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp service: %s\n", ar_event->service_id);
	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp action: %s\n", ar_event->action_name);

	event_device = _find_device(upnp_device, ar_event->dev_udn);
	event_service = _find_service(event_device, ar_event->service_id);
	event_action = _find_action(event_service, ar_event->action_name);

	if (NULL == event_action) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Unknown action '%s' for service '%s'\n", 
			ar_event->action_name, ar_event->service_id);
		ar_event->action_result = NULL;
		ar_event->error_code = 401;
		return -1;
	}
	
	if (event_action->callback) 
	{
		struct action_event event;
		INT32 rc;
		event.request = ar_event;
		event.service = event_service;

		pthread_mutex_lock(&(event_service->service_mutex));
		rc = (event_action->callback)(&event);
		pthread_mutex_unlock(&(event_service->service_mutex));
		
		if (PAL_UPNP_E_SUCCESS == rc) 
		{
			ar_event->error_code = PAL_UPNP_E_SUCCESS;
			PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "Action was a success!\n");
		}
		
		if (NULL == ar_event->action_result)
		{
			PAL_upnp_make_action(&(ar_event->action_result),
								ar_event->action_name,
						   		event_service->type, 
						   		0, 
						   		NULL,
						   		PAL_UPNP_ACTION_RESPONSE);
		}
	} 
	else 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "Got a valid action, but no handler defined!\n");
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "  ActionName: '%s'\n", ar_event->action_name);
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "  DevUDN:     '%s'\n", ar_event->dev_udn);
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "  ServiceID:  '%s'\n", ar_event->service_id);
		ar_event->error_code = 0;
	}

	return 0;
}


/************************************************************
 * Function: upnp_event_handler
 *
 *  Parameters:	
 *      EventType: 		Input. upnp event type. 
 *      event: 			Input. upnp event. 
 *      Cookie: 		Input. Reserved.
 * 
 *  Description:
 *     This function handles a upnp event.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/  
LOCAL INT32 upnp_event_handler(Upnp_EventType EventType, VOID *event, VOID *Cookie)
{
	switch (EventType) 
	{
	case UPNP_CONTROL_ACTION_REQUEST:
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "action request\n");
		_handle_action_request(event);
		break;
	case UPNP_CONTROL_GET_VAR_REQUEST:
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "control get variable request\n");
        _handle_ControGetVar_request(event);
		break;
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "event subscription request\n");
		_handle_subscription_request(event);
		break;

	default:
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Unknown event type: %d\n", EventType);
		break;
	}
	return 0;
}


/************************************************************
 * Function: PAL_upnp_device_init
 *
 *  Parameters:	
 *      device: 		Input. upnp device pointer. 
 *      if_name:     	Input. Interface name. 
 *           				If input is NULL, an appropriate interface will be automatically selected.
 *      port: 			Input . Local Port to listen for incoming connections.
 *           				If input is NULL, a appropriate port will be automatically selected.     
 *      timeout: 		Input. upnp device alive timeout value.
 *      desc_doc_name: 	Input. Device description file name.   
 *      web_dir_path: 	Input. Local path for device description file. 
 * 
 *  Description:
 *     This function initialize a upnp device.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/  
INT32 PAL_upnp_device_init(struct upnp_device *device, 
						CHAR *if_name, 
						UINT32 port, 
						UINT32 timeout,
						const CHAR *desc_doc_name,
               			const CHAR *web_dir_path)
{
	INT32 ret;
	INT32 result = -1;

	if(upnp_initStatus != UPNP_DEVICE_NOT_INIT)
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "try to call upnp_device_init() twice\n");
		return result;
	}

	PAL_LOG_REGISTER(UDT_LNAME, NULL);

	if(NULL == desc_doc_name){
		desc_doc_name = DEFAULT_DEVICE_DESC_NAME;
	}
	if(NULL == web_dir_path){
		web_dir_path = DEFAULT_WEB_DIR;
	}
	if (device->init_function)
	{
		ret = device->init_function();
		if (ret != 0) 
		{
			return result;
		}
	}

	upnp_device = device;

	/* init upnp device*/
	ret = PAL_upnp_init(if_name, port);
	if (PAL_UPNP_E_SUCCESS != ret) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "UpnpInit() Error: %d\n", ret);
		PAL_upnp_finish();
		return result;
	}

	/*ret = UpnpSetMaxContentLength(3*DEFAULT_MAX_CONTENT_LENGTH);
    if(PAL_UPNP_E_SUCCESS != ret)
	{
        PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Error setting UPnP MaxContentLength: %d", ret);
		PAL_upnp_finish();
		return result;
    }*/




    port = PAL_upnp_get_port();
    PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "UPnP Initialized\n ip_address= %s\tip6_address = %s\tport = %d\n", PAL_upnp_get_ipaddress(), PAL_upnp_get_ip6address(), port);
    PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "description file: http://%s:%d/%s\n", PAL_upnp_get_ipaddress(), port, desc_doc_name);
    PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "Specifying the webserver root directory: %s\n", web_dir_path);  

	/* register root device*/		
	ret = PAL_upnp_register_root_device(web_dir_path,
										desc_doc_name,
										upnp_event_handler,
										&device, 
										&device_handle);
	if(PAL_UPNP_E_SUCCESS != ret) 
	{
    	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "UpnpRegisterRootDevice() failed\n");		
		PAL_upnp_finish();
		return result;
	}

	/* send advertisement*/
	if(0 == timeout)
		timeout = DEFAULT_TIMEOUT_VALUE;
	PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_INFO, "upnp device alive timeout: %d\n", timeout);
	
	ret = PAL_upnp_send_advertisement(device_handle, timeout);
	if (PAL_UPNP_E_SUCCESS != ret) 
	{
		PAL_LOG(UDT_LNAME, PAL_LOG_LEVEL_WARNING, "Error sending advertisements: %d\n", ret);
		PAL_upnp_finish();
		return result;
	}

	upnp_initStatus = UPNP_DEVICE_INITED;

	result = 0;
	return result;
}


/************************************************************
 * Function: PAL_upnp_device_destroy
 *
 *  Parameters:	
 *      device: 		Input. upnp device pointer. 
 * 
 *  Description:
 *     This function destroy a upnp device.  
 *
 *  Return Values: INT32
 *      0 if successful else error code. More error code is TBD
 ************************************************************/  
INT32 PAL_upnp_device_destroy(struct upnp_device *device)
{
	struct upnp_device *dev = device;
	while(dev)
	{
		struct upnp_device *next = dev->next;
		if(dev->destroy_function)
			(dev->destroy_function)(dev);
		
		dev = next;
	}
	
	PAL_upnp_unregister_root_device(device_handle);
    PAL_upnp_finish();

	device_handle = 0;
	upnp_initStatus = UPNP_DEVICE_NOT_INIT;
	upnp_device = NULL;
    return 0;
}


/************************************************************
 * Function: PAL_upnp_device_getHandle
 *
 *  Parameters:	
 *      None. 
 * 
 *  Description:
 *     This function get device handle.  
 *
 *  Return Values: pal_upnp_device_handle
 *      device handle
 ************************************************************/ 
pal_upnp_device_handle PAL_upnp_device_getHandle(VOID)
{
	return device_handle;
}



