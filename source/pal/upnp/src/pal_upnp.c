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
 *    FileName:    pal_upnp.c
 *      Author:    Barry Wang (bowan@cisco.com)
 *        Date:    2009-05-05
 * Description:    PAL UPnP abstract interfaces
 *****************************************************************************/
/*$Id: pal_upnp.c,v 1.4 2009/05/19 07:41:12 bowan Exp $
 *
 *$Log: pal_upnp.c,v $
 *Revision 1.4  2009/05/19 07:41:12  bowan
 *change some comments and data type as per common type definition
 *
 *Revision 1.3  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.2  2009/05/13 08:37:11  bowan
 *Change the header file seq to avoid the compile error of definition confict.
 *
 *Revision 1.1  2009/05/13 07:56:33  bowan
 *no message
 *
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <upnp/upnp.h>
#include "pal_upnp.h" 
#include <upnp/upnptools.h>
#include <stdio.h>
#include <string.h>

#ifdef PAL_DEBUG
#define pal_debug(fmt, args...) fprintf(stdout, "Debug[%s,%3d]: "fmt, __FILE__,__LINE__, ##args)
#else
#define pal_debug(fmt, args...)
#endif

struct _pal_error_string{
    INT32 rc;                     /* error code */
    const CHAR *rc_str;        /* error description */
};

/*error message for common upnp process*/
struct _pal_error_string g_pal_error_message[] = { 
    {PAL_UPNP_E_SUCCESS, "PAL_UPNP_E_SUCCESS"},
    {PAL_UPNP_E_INVALID_HANDLE, "PAL_UPNP_E_INVALID_HANDLE"},
    {PAL_UPNP_E_INVALID_PARAM, "PAL_UPNP_E_INVALID_PARAM"},
    {PAL_UPNP_E_OUTOF_HANDLE, "PAL_UPNP_E_OUTOF_HANDLE"},
    {PAL_UPNP_E_OUTOF_CONTEXT, "PAL_UPNP_E_OUTOF_CONTEXT"},
    {PAL_UPNP_E_OUTOF_MEMORY, "PAL_UPNP_E_OUTOF_MEMOR"},
    {PAL_UPNP_E_INIT, "PAL_UPNP_E_INIT"},
    {PAL_UPNP_E_BUFFER_TOO_SMALL, "PAL_UPNP_E_BUFFER_TOO_SMALL"},
    {PAL_UPNP_E_INVALID_DESC, "PAL_UPNP_E_INVALID_DESC"},
    {PAL_UPNP_E_INVALID_URL, "PAL_UPNP_E_INVALID_URL"},
    {PAL_UPNP_E_INVALID_SID, "PAL_UPNP_E_INVALID_SID"},
    {PAL_UPNP_E_INVALID_DEVICE, "PAL_UPNP_E_INVALID_DEVICE"},
    {PAL_UPNP_E_INVALID_SERVICE, "PAL_UPNP_E_INVALID_SERVICE"},
    {PAL_UPNP_E_BAD_RESPONSE, "PAL_UPNP_E_BAD_RESPONSE"},
    {PAL_UPNP_E_BAD_REQUEST, "PAL_UPNP_E_BAD_REQUEST"},
    {PAL_UPNP_E_INVALID_ACTION, "PAL_UPNP_E_INVALID_ACTION"},
    {PAL_UPNP_E_FINISH, "PAL_UPNP_E_FINISH"},
    {PAL_UPNP_E_INIT_FAILED, "PAL_UPNP_E_INIT_FAILED"},
    {PAL_UPNP_E_BAD_HTTPMSG, "PAL_UPNP_E_BAD_HTTPMSG"},
    {PAL_UPNP_E_NETWORK_ERROR, "PAL_UPNP_E_NETWORK_ERROR"},
    {PAL_UPNP_E_SOCKET_WRITE, "PAL_UPNP_E_SOCKET_WRITE"},
    {PAL_UPNP_E_SOCKET_READ, "PAL_UPNP_E_SOCKET_READ"},
    {PAL_UPNP_E_SOCKET_BIND, "PAL_UPNP_E_SOCKET_BIND"},
    {PAL_UPNP_E_SOCKET_CONNECT, "PAL_UPNP_E_SOCKET_CONNECT"},
    {PAL_UPNP_E_OUTOF_SOCKET, "PAL_UPNP_E_OUTOF_SOCKET"},
    {PAL_UPNP_E_LISTEN, "PAL_UPNP_E_LISTEN"},
    {PAL_UPNP_E_EVENT_PROTOCOL, "PAL_UPNP_E_EVENT_PROTOCOL"},
    {PAL_UPNP_E_SUBSCRIBE_UNACCEPTED, "PAL_UPNP_E_SUBSCRIBE_UNACCEPTED"},
    {PAL_UPNP_E_UNSUBSCRIBE_UNACCEPTED, "PAL_UPNP_E_UNSUBSCRIBE_UNACCEPTED"},
    {PAL_UPNP_E_NOTIFY_UNACCEPTED, "PAL_UPNP_E_NOTIFY_UNACCEPTED"},
    {PAL_UPNP_E_INTERNAL_ERROR, "PAL_UPNP_E_INTERNAL_ERROR"},
    {PAL_UPNP_E_INVALID_ARGUMENT, "PAL_UPNP_E_INVALID_ARGUMENT"},
    {PAL_UPNP_E_OUTOF_BOUNDS, "PAL_UPNP_E_OUTOF_BOUNDS"},
    {PAL_UPNP_SOAP_E_INVALID_ACTION, "PAL_UPNP_SOAP_E_INVALID_ACTION"},
    {PAL_UPNP_SOAP_E_INVALID_ARGS, "PAL_UPNP_SOAP_E_INVALID_ARGS"},
    {PAL_UPNP_SOAP_E_OUT_OF_SYNC,"PAL_UPNP_SOAP_E_OUT_OF_SYNC"},
    {PAL_UPNP_SOAP_E_INVALID_VAR, "PAL_UPNP_SOAP_E_INVALID_VAR"},
    {PAL_UPNP_SOAP_E_ACTION_FAILED, "PAL_UPNP_SOAP_E_ACTION_FAILED"}
};


/************************************************************
 * Function: PAL_upnp_init 
 *
 *  Parameters:	
 *      if_name: Input. Interface name. 
 *      lo_port: Input . Local Port to listen for incoming connections.
 *           If input is NULL, a appropriate port will be automatically selected. 
 *
 *  Description:
 *      Start UPnP Stack - Initialization.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/   
INT32 PAL_upnp_init(IN const CHAR *if_name, IN UINT16 lo_port)
{
    INT32 ret = 0;
    
    ret = UpnpInit2((const CHAR*)if_name,lo_port);
    
    return ret;
}

/************************************************************
 * Function: PAL_upnp_get_ipaddress 
 *
 *  Parameters:	
 *      
 *  Description:
 *      Gives back the local ipaddress.
 *
 *  Return Values: INT32
 *      return the IP address string on success else NULL of failure
 ************************************************************/ 
CHAR *PAL_upnp_get_ipaddress()
{
    CHAR *ipaddress = NULL;

    ipaddress = UpnpGetServerIpAddress();

    return ipaddress;
}

/************************************************************
 * Function: PAL_upnp_get_ip6address 
 *
 *  Parameters:	
 *      
 *  Description:
 *      Gives back the local ipv6 address.
 *
 *  Return Values: INT32
 *      return the IPv6 address string on success else NULL of failure
 ************************************************************/ 
CHAR *PAL_upnp_get_ip6address()
{
    CHAR *ip6address = NULL;

    ip6address = UpnpGetServerIp6Address();

    return ip6address;
}

/************************************************************
 * Function: PAL_upnp_get_port 
 *
 *  Parameters:	
 *      
 *  Description:
 *      Gives back the port for listening ssdp.
 *
 *  Return Values: UINT16
 *      return the IP address string on success else NULL of failure
 ************************************************************/
UINT16 PAL_upnp_get_port()
{
    UINT16 port = 0;
    
    port = UpnpGetServerPort();

    return port;
}


/************************************************************
 * Function: PAL_upnp_register_root_device
 *
 *  Parameters:	
 *      lo_path: Input. Local path for device description file.
               This path also is as web root directory.
 *      file_name: Input. File name of device description file.
 *      callback_func: Input. Callback functions for device events      
 *      cookie: Input. Reserved.
 *      handle: Output. Device handle.        
 * 
 *  Description:
 *     This function registers a device application with
 *	the UPnP Library.  A device application cannot make any other API
 *	calls until it registers using this function. 
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/  
INT32 PAL_upnp_register_root_device(IN const CHAR *lo_path,
                              IN const CHAR *file_name,
                              IN pal_upnp_func callback_func,
                              IN const VOID *cookie,
                              OUT pal_upnp_device_handle *handle)
{
    CHAR desc_doc_url[PAL_UPNP_DESC_URL_SIZE];
    CHAR *web_dir_path = NULL;
    CHAR *upnp_server_ip = NULL;
    INT32 ret = 0;


    if (!file_name || !callback_func)
        return -1;

    if (lo_path)
        web_dir_path = (CHAR *)lo_path;
    else
        web_dir_path = PAL_UPNP_DEFAULT_WEB_DIR;

    if ( (upnp_server_ip = UpnpGetServerIpAddress()) == NULL )
    	return -1;

    snprintf(desc_doc_url, PAL_UPNP_DESC_URL_SIZE, "http://%s:%d/%s", upnp_server_ip,
             UpnpGetServerPort(), file_name);

    ret = UpnpSetWebServerRootDir( web_dir_path );
    if (ret != PAL_UPNP_E_SUCCESS){
        return ret;
    }

    ret = UpnpRegisterRootDevice( desc_doc_url,
                                  (Upnp_FunPtr)callback_func,
                                  (VOID *)handle, (UpnpDevice_Handle *)handle);
    return ret;
}


/************************************************************
 * Function: PAL_upnp_unregister_root_device 
 *
 *  Parameters:	
 *      handle: Input. Device handle.        
 * 
 *  Description:
 *     This function unregisters a root device registered with 
 *	PAL_upnp_register_root_device. After this call, the 
 *	pal_upnp_device_handle handle is no longer valid. For all advertisements that 
 *	have not yet expired, the UPnP library sends a device unavailable message 
 *	automatically. 
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_unregister_root_device(IN pal_upnp_device_handle handle)
{
    INT32 ret = 0;

    ret = UpnpUnRegisterRootDevice(handle);

    return ret;
}



/************************************************************
 * Function: PAL_upnp_send_advertisement 
 *
 *  Parameters:	
 *      handle: Input. Handle for device instance.    
 *      expire: Input. Timer of seconds for resending the advertisement.    
 * 
 *  Description:
 *     This function sends the device advertisement. It also schedules a
 *	job for the next advertisement after "expire" time.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_send_advertisement(IN pal_upnp_device_handle handle, IN INT32 expire)
{
    INT32 ret = 0;

    ret = UpnpSendAdvertisement(handle, expire);

    return ret;
}


#ifndef PAL_UPNP_CLIENT_DISABLE

/************************************************************
 * Function: PAL_upnp_register_cp 
 *
 *  Parameters:	
 *      func: Input. Pointer to a function for receiving 
 *		               asynchronous events.    
 *      cookie: Input. Reserved. 
 *      handle: Output. Pointer to a variable to store 
 *		               the new control point handle.  
 * 
 *  Description:
 *     This function registers a control point application with the
 *	UPnP Library.  A control point application cannot make any other API 
 *	calls until it registers using this function.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_register_cp (IN pal_upnp_func func,
                       IN const VOID *cookie,
                       OUT pal_upnp_cp_handle *handle)
{
    INT32 ret = 0;

    ret = UpnpRegisterClient((Upnp_FunPtr)func,cookie,(UpnpClient_Handle *)handle);

    return ret;
}


/************************************************************
 * Function: PAL_upnp_unregister_cp 
 *
 *  Parameters:	
 *      handle: Input. The handle of the control point instance 
 *		               to unregister.   
 * 
 *  Description:
 *      This function unregisters a client registered with 
 *	PAL_upnp_register_cp. After this call, the 
 *	pal_upnp_cp_handle handle is no longer valid. The UPnP Library generates 
 *	no more callbacks after this function returns.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_unregister_cp(IN pal_upnp_cp_handle handle)
{
    INT32 ret = 0;

    ret = UpnpUnRegisterClient(handle);

    return ret;
}


/************************************************************
 * Function: PAL_upnp_search_async 
 *
 *  Parameters:	
 *      handle: Input. The handle of the control point instance 
 *		  max_timeout: Input. Maximum time to wait for the search reply.
 *		  target: Input. Search target string.
 *		  cookie: Input. Reserved. 
 * 
 *  Description:
 *      This function searches for the devices for the provided maximum time.
 *	It is a asynchronous function. It schedules a search job and returns. 
 *	control point is notified about the search results after search timer.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_search_async(IN pal_upnp_cp_handle handle,
                       IN INT32 max_timeout,
                       IN const CHAR *target,
                       IN const VOID *cookie)
{
    INT32 ret = 0;

    ret = UpnpSearchAsync(handle, max_timeout, target, cookie);

    return ret;
}
#endif


/************************************************************
 * Function: PAL_upnp_make_action 
 *
 *  Parameters:	
 *      action: InputOutput. Action buffer.
 *      action_name: Input. Action name.
 *      service_type: Input. Server Type.
 *      nb_params: Input. Number of pairs of parameters.
 *	params: Input. Parameter pairs array.
 *	action_type: Input. Request or response. 
 * 
 *  Description:
 *      This function creates the action(request/response) from the argument
 *      list.This function creates the action request or response if it is a first
 * argument else it will add the argument in the document.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_make_action( INOUT pal_xml_top** action,
                       IN const CHAR *action_name,
                       IN const CHAR *service_type,
                       IN INT32 nb_params,
                       IN const pal_string_pair* params,
                       IN pal_upnp_action_type action_type)
{
    INT32 i = 0;
    INT32 ret = 0;

    if (action == NULL || (nb_params > 0 && params == NULL)) 
        return -1; 

    if (*action == NULL){
        if (PAL_UPNP_ACTION_RESPONSE == action_type){
            *action  = (pal_xml_top*)UpnpMakeActionResponse(action_name, service_type, 0, NULL);
        }else if (PAL_UPNP_ACTION_REQUEST == action_type){
            *action  = (pal_xml_top*)UpnpMakeAction(action_name, service_type, 0, NULL);
        }else{
            return -1;
        }
    }   

    
    for (i = 0; i < nb_params; i++){
        
         if (action_type) {
            ret = UpnpAddToActionResponse((IXML_Document **)action,action_name, service_type, 
            params[i].name, params[i].value);
         } else {
             ret = UpnpAddToAction((IXML_Document **)action,action_name, service_type, 
            params[i].name, params[i].value);
         }

        if (ret != PAL_UPNP_E_SUCCESS){

            if (*action) 
                PAL_xml_top_free(*action);

            return -1;
        }
    }
    
    return ret;
}


#ifndef PAL_UPNP_CLIENT_DISABLE
/************************************************************
 * Function: PAL_upnp_send_action 
 *
 *  Parameters:	
 *      handle: Input. Handle of control point to send action.
 *      action_url: Input. The action URL of service.
 *      service_type: Input. The type of the service.
 *      action: Input. The top structure for action.
 *      response: The top structure for the response 
 *		to the action.  The UPnP Library allocates this buffer
 *		and the caller needs to free it.     
 * 
 *  Description:
 *      This function sends a message to change a state variable in a service.
 *	This is a synchronous call that does not return until the action is
 *	complete.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/                       
INT32 PAL_upnp_send_action( IN pal_upnp_cp_handle handle,
                       IN const CHAR *action_url,
                       IN const CHAR *service_type,
                       IN  pal_xml_top* action,
                       OUT  pal_xml_top **response)
{
    INT32 ret = 0;

    ret = UpnpSendAction(handle, action_url, service_type, NULL, (IXML_Document *)action, (IXML_Document **)response);

    return ret;
}
#endif


/************************************************************
 * Function: PAL_upnp_download_xmldoc 
 *
 *  Parameters:	
 *      url: Input. Device description url for file downloading.
 *      xml_top: Output. The buffer to strore device description file.
 *               The UPnP Library allocates this buffer
 *		and the caller needs to free it  
 * 
 *  Description:
 *      This function sends a message to change a state variable in a service.
 *	This is a synchronous call that does not return until the action is
 *	complete.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/                    
INT32 PAL_upnp_download_xmldoc(IN const CHAR *url, OUT pal_xml_top **xml_top)
{
    INT32 ret = 0;

    ret = UpnpDownloadXmlDoc(url, (IXML_Document **)xml_top);

    return ret;
}


/************************************************************
 * Function: PAL_upnp_resolve_url 
 *
 *  Parameters:	
 *      base_url: Input. Base URL string.
 *      rel_url: Input. Relative URL string.
 *      abs_url: Output. Absolute URL string. 
 * 
 *  Description:
 *      This functions concatinates the base URL and relative URL to generate
*	the absolute URL.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 *      abs_url need to be free after usage.
 ************************************************************/                          
INT32 PAL_upnp_resolve_url(IN const CHAR *base_url,
                      IN const CHAR *rel_url,
                      OUT CHAR **abs_url)
{
    INT32 ret = 0;

    *abs_url = (CHAR*)calloc(1, (base_url? strlen(base_url) : 0) + (rel_url? strlen(rel_url) : 0) + 1);

    if( *abs_url == 0 ){
        ret = UPNP_E_OUTOF_MEMORY;
    }
    else{
        
        if(NULL == rel_url)
            ret = UpnpResolveURL (base_url, "/", *abs_url);
        else
            ret= UpnpResolveURL (base_url, rel_url, *abs_url);
        if( ret != PAL_UPNP_E_SUCCESS ){      
            (*abs_url)[0] = '\0';
        }
    }

    return ret;
}


/************************************************************
 * Function: PAL_upnp_finish 
 *
 *  Parameters:	
 *     
 * 
 *  Description:
 *      Stop the UPnP library working and clean resources.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_finish()
{
    INT32 ret = 0;

    ret = UpnpFinish();

    return ret;
}


#ifndef PAL_UPNP_CLIENT_DISABLE
/************************************************************
 * Function: PAL_upnp_subscribe 
 *
 *  Parameters:	
 *      handle: Input. Handle of the control point to register event.
 *      event_url: Input. The URL of the service to subscribe to.
 *      timeout: Input/Output. Pointer to a variable containing the requested 
 *		           subscription time.  Upon return, it contains the
 *		           actual subscription time returned from the service. 
 *		  sid: Output. Pointer to a variable to receive the 
 *		               subscription ID (SID)  
 * 
 *  Description:
 *      This function registers a control point to receive event
 *	notifications from another device.  This operation is synchronous
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_subscribe (IN pal_upnp_cp_handle handle,
                         IN const CHAR *event_url,
                         INOUT INT32 *timeout,
                         OUT pal_upnp_sid sid)
{
    INT32 ret = 0;

    ret = UpnpSubscribe((UpnpClient_Handle)handle, event_url,timeout, sid);
    
    return ret;
}
#endif


/************************************************************
 * Function: PAL_upnp_accept_subscription 
 *
 *  Parameters:	
 *      handle: Input. The handle of the device.
 *      device_id: Input. The device ID of the subdevice of the 
 *		                   service generating the event
 *      service_id: Input.  The unique service identifier of the 
 *		                   service generating the event. 
 *		var_names: Input. Pointer to an array of event variables.
 *      var_vals: Input. Pointer to an array of values for 
 *		                 the event variables.
 *      var_nb: Input. The number of event variables in var_names.
 *      sub_id: Input.  The subscription ID of the newly 
 *		               registered control point.
 * 
 *  Description:
 *      This function accepts a subscription request and sends
 *	out the current state of the eventable variables for a service.  
 *	The device application should call this function when it receives a 
 *	UPNP_EVENT_SUBSCRIPTION_REQUEST callback. This function is sychronous
 *	and generates no callbacks.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
INT32 PAL_upnp_accept_subscription(IN pal_upnp_device_handle handle,
                                 IN const CHAR *device_id,
                                 IN const CHAR *service_id,
                                 IN const CHAR **var_names,
                                 IN const CHAR **var_vals,
                                 IN INT32 var_nb,
                                 IN pal_upnp_sid sub_id)
{
    INT32 ret = 0;

    ret = UpnpAcceptSubscription(handle, device_id, service_id, var_names, var_vals, var_nb, sub_id);
    
    return ret;
}
                                 

/************************************************************
 * Function: PAL_upnp_notify 
 *
 *  Parameters:	
 *      handle: Input. The handle to the device sending the event.
 *      device_id: Input. The device ID of the subdevice of the 
 *		             service generating the event. 
 *		  service_name: Input. The unique identifier of the service 
 *		             generating the event.
 *		  var_name: Input. Pointer to an array of variables that 
 *		           have changed. 
 *		  new_value: Input. Pointer to an array of new values for 
 *		           those variables.  
 *		  var_number: Input. The count of variables included in this 
 *		           notification.
 * 
 *  Description:
 *      This function sends out an event change notification to all
 *	control points subscribed to a particular service.  This function is
 *	synchronous and generates no callbacks.
 *
 *  Return Values: INT32
 *      0 if successful else error code. 
 ************************************************************/
 INT32 PAL_upnp_notify (IN pal_upnp_device_handle handle,
                      IN const CHAR *device_id,
                      IN const CHAR *service_name,
                      IN const CHAR **var_name,
                      IN const CHAR **new_value,
                      IN INT32 var_number)
{
   INT32 ret = 0;

   ret = UpnpNotify(handle, device_id, service_name,var_name, new_value,var_number);

   return ret;
}

/************************************************************
 * Function: PAL_upnp_get_error_message 
 *
 *  Parameters:	
 *     errno: Input, error code.
 * 
 *  Description:
 *     This functions returns the error string mapped to the error code.
 *
 *  Return Values: const CHAR *
 *      return either the right string or "Unknown Error"
 ************************************************************/
 const CHAR *PAL_upnp_get_error_message(IN INT32 errno)
 {
    INT32 i;
    INT32 msg_number = sizeof(g_pal_error_message) / sizeof(g_pal_error_message[0]);

    for(i = 0; i < msg_number; i++ ){
        if (errno == g_pal_error_message[i].rc)
            return g_pal_error_message[i].rc_str;
    }
    return "Unknown Error";
 }

    
//end of pal_upnp.c

