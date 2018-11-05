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
 *    FileName:    upnp_device.h
 *      Author:    Jerry Liu(zhiyliu@cisco.com)
 *        Date:    2009-04-15
 * Description:    UPnP device template head code of UPnP IGD project
 *****************************************************************************/
/*$Id: pal_upnp_device.h,v 1.2 2009/05/15 08:00:21 bowan Exp $
 *
 *$Log: pal_upnp_device.h,v $
 *Revision 1.2  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.1  2009/05/14 01:52:28  zhiyliu
 *init version
 *
 *
 **/

#ifndef _UPNP_DEVICE_H
#define _UPNP_DEVICE_H

#include <pthread.h>
#include "pal_upnp.h"

#define UDT_LNAME	"UDT"

struct action_event 
{
	pal_upnp_action_request *request;
	struct upnp_service *service;
};

struct upnp_action 
{
	const CHAR *action_name;
	INT32 (*callback)(struct action_event *);
};

struct upnp_variable
{
	const CHAR 	*name;									//state variable name
	CHAR 		value[PAL_UPNP_LINE_SIZE];				//state variable value
};

struct upnp_device 
{
	INT32 (*init_function)(VOID);						//device init function
	INT32 (*destroy_function)(struct upnp_device *);	//device destroy function
	CHAR udn[PAL_UPNP_LINE_SIZE];						//UPnP device UDN
	struct upnp_service **services;						//UPnP device service array, NULL as the end
	struct upnp_device *next;							//UPnP device list
};

struct upnp_service 
{
	pthread_mutex_t service_mutex;						//service mutex
	INT32 (*destroy_function)(struct upnp_service *);	//service destroy function
	CHAR *type;									        //UPnP service type
	const CHAR *serviceID;								//UPnP service ID
	struct upnp_action *actions;						//UPnP service action array, NULL as the end
	struct upnp_variable *state_variables;				//UPnP service state variable array, NULL as the end
	struct upnp_variable *event_variables;				//UPnP service event variable array, NULL as the end
	VOID *private;										//service private variable
};


/************************************************************
 * Function: PAL_upnp_device_init
 *
 *  Parameters:	
 *      device: 		Input. upnp device pointer. 
 *      ip_address: 	Input. Local IP Address. 
 *           				If input is NULL, an appropriate IP address will be automatically selected.
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
extern INT32 PAL_upnp_device_init(IN struct upnp_device *device, 
							IN CHAR *ip_address, 
							IN UINT32 port, 
							IN UINT32 timeout,
							IN const CHAR *desc_doc_name,
							IN const CHAR *web_dir_path);


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
extern INT32 PAL_upnp_device_destroy(IN struct upnp_device *device);


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
extern pal_upnp_device_handle PAL_upnp_device_getHandle(VOID);


#endif /* _UPNP_DEVICE_H */
