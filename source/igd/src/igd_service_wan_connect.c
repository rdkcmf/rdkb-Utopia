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

/*
 *    FileName:    igd_service_wan_connect.c
 *      Author:    Andy Liu(zhihliu@cisco.com)
 *                     Tao Hong(tahong@cisco.com)
 *        Date:    2009-05-03
 * Description:    WAN IP/PPP connection service implementation code of UPnP IGD project
 * sa record:  http://wwwin-ses.cisco.com/sa_web_results_sh_linksys/tahong.5_7_2009igd.2/
 *****************************************************************************/
/*$Id: igd_service_wan_connect.c,v 1.8 2009/05/27 02:39:41 zhihliu Exp $
 *
 *$Log: igd_service_wan_connect.c,v $
 *Revision 1.8  2009/05/27 02:39:41  zhihliu
 *always clear OutStr before using it
 *
 *Revision 1.7  2009/05/27 02:13:55  zlipin
 *"IGD_pii_get_portmapping_entry_num" interface changed.
 *
 *Revision 1.6  2009/05/22 05:40:10  zlipin
 *Adjust the PII "PortMapping" module interface
 *
 *Revision 1.5  2009/05/21 07:58:27  zhihliu
 *update PII interface
 *
 *Revision 1.4  2009/05/19 07:07:42  tahong
 *1, PPPOE===>PPPoE
 *2, PPOE_Relay ===> PPPoE_Relay
 *3, add global index into controlURL and eventSubURL
 *
 *Revision 1.3  2009/05/15 08:00:21  bowan
 *1st Integration
 *
 *Revision 1.2  2009/05/14 02:04:12  tahong
 *modify according to Zhou Lipin's file name and function name
 *
 *Revision 1.1  2009/05/13 08:57:17  tahong
 *create orignal version
 *
 *
 **/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "igd_utility.h"
#include "pal_log.h"

#include "igd_platform_independent_inf.h"

#include "igd_service_wan_connect.h"
#include "igd_action_port_mapping.h"

#ifndef LOG_ENTER_FUNCTION
#define LOG_ENTER_FUNCTION  PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "entering %s", __func__)
#endif

#ifndef LOG_LEAVE_FUNCTION
#define LOG_LEAVE_FUNCTION  PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "leaving %s", __func__)
#endif

enum enum_ppp_or_ip{
    enum_ppp_service_type = 0,
    enum_ip_service_type
};


#define WAN_PPP_CONNECTION_SERVICE_PREFIX "urn:upnp-org:serviceId:WANPPPConn"
#define WAN_IP_CONNECTION_SERVICE_PREFIX "urn:upnp-org:serviceId:WANIPConn"



#define ERROR_CODE_402 (402)
#define ERROR_CODE_501 (501)
#define ERROR_CODE_703 (703)
#define ERROR_CODE_704 (704)
#define ERROR_CODE_706 (706)
#define ERROR_CODE_710 (710)
#define ERROR_CODE_711 (711)

LOCAL error_pair ErrorPair[] = {
    { ERROR_CODE_402, "Invalid Args" },
    { ERROR_CODE_501, "Action Failed" },
    { ERROR_CODE_703, "InactiveConnectionStateRequired" },
    { ERROR_CODE_704, "ConnectionSetupFailed" },
    { ERROR_CODE_706, "ConnectionNotConfigured" },
    { ERROR_CODE_710, "InvalidConnectionType" },
    { ERROR_CODE_711, "ConnectionAlreadyTerminated" },
    { 0, NULL }
};

LOCAL VOID _set_err_str(INT32 code, CHAR *dest)
{
    INT32 i = 0;

    while (0 != ErrorPair[i].code) {
        if (code == ErrorPair[i].code) {
            strncpy(dest, ErrorPair[i].desc, strlen(ErrorPair[i].desc)+1);
            break;
        }

        i++;
    }

    return;
}

LOCAL const CHAR *IpConnTypes[] = {
    "Unconfigured",
    "IP_Routed",
    "IP_Bridged",
    NULL
};

LOCAL const CHAR *PppConnTypes[] = {
    "Unconfigured",
    "IP_Routed",
    "DHCP_Spoofed",
    "PPPoE_Bridged",
    "PPTP_Relay",
    "L2TP_Relay",
    "PPPoE_Relay",
    NULL
};

LOCAL BOOL _is_valid_possible_connection_types(CHAR *ctype, CHAR *stype)
{
    INT32 i = 0;
    CHAR **p = NULL;

    if (strcmp(WAN_PPP_CONNECTION_SERVICE_TYPE,stype) == 0)
        p = (CHAR **)PppConnTypes;
    else if (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,stype) == 0)
        p = (CHAR **)IpConnTypes;
    else
        return BOOL_FALSE;

    while (NULL != p[i]) {
        if (0 == strcmp(p[i], ctype))
            return BOOL_TRUE;

        i++;
    }

    return BOOL_FALSE;
}


/************************************************************
 * Function: _set_connection_type
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - set appropriate connection type as required.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
LOCAL INT32 _set_connection_type(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    pal_xml_node *pnode = NULL;
    CHAR *type = NULL;
    CHAR connectionstatus[PAL_UPNP_LINE_SIZE] = {0};
    INT32 rc = -1;

    LOG_ENTER_FUNCTION;

    /*get input parameter ConnectionType*/
    pnode = PAL_xml_node_GetFirstbyName(event->request->action_request,
                                           "NewConnectionType",
                                           NULL);
    if (NULL == pnode)
    {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_xml_node_get_first_by_name() fail");
        goto out;
    }
    type = PAL_xml_node_get_value(pnode);
    if (NULL == type) {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_xml_node_get_value() fail");
        goto out;
    }

    // check connection type validity
    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "connection type to be set: %s\n", type);
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");
        goto out;
    }
    if (BOOL_FALSE == _is_valid_possible_connection_types(type, event->service->type)) {
        rc = ERROR_CODE_402;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "invalid connection type");
        goto out;
    }

    // check current connection status
    strncpy(connectionstatus, event->service->state_variables[ConnectionStatus_index].value, strlen(event->service->state_variables[ConnectionStatus_index].value)+1);
    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "current connection status: %s", connectionstatus);
    if ((0 != strcmp(connectionstatus, "Unconfigured")) &&
        (0 != strcmp(connectionstatus, "Disconnected"))) 
    {
        rc = ERROR_CODE_703;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "inactive connection state required (703)");
        goto out;
    }

    //set new connection type
    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "start to set connection type (%s)", type);
    rc = IGD_pii_set_connection_type(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                 type);
    if (0 != rc)
    {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_set_connection_type() fail");
        goto out;
    }
    strncpy(event->service->state_variables[ConnectionType_index].value, type, strlen(type)+1);

    // construct action response
    rc = PAL_upnp_make_action((VOID**)&event->request->action_result,
                              "SetConnectionType",
                              event->service->type,
                              0,
                              NULL,
                              PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail");
    }
    return rc;

out:
    event->request->action_result = NULL;
    _set_err_str(rc, event->request->error_str);
    event->request->error_code = rc;
        
    LOG_LEAVE_FUNCTION;
    return rc;
}

/************************************************************
 * Function: _get_connection_type_info
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - get current connection type and list of possible
 *      connection types.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
#define GetConnectionTypeInfo_PARAM_NUM (2)
LOCAL INT32 _get_connection_type_info(INOUT struct action_event *event)
{
    pal_string_pair params[GetConnectionTypeInfo_PARAM_NUM];
    INT32 rc = -1;

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "%s: ConnectionType: %s\nPossibleConnectionTypes: %s\n",
        __FUNCTION__,
        event->service->state_variables[ConnectionType_index].value,
        event->service->state_variables[PossibleConnectionTypes_index].value);

    // construct action response
    params[0].name = "NewConnectionType";
    params[0].value = event->service->state_variables[ConnectionType_index].value;
    params[1].name = "NewPossibleConnectionTypes";
    params[1].value = event->service->state_variables[PossibleConnectionTypes_index].value;
    rc = PAL_upnp_make_action((VOID**)&event->request->action_result,
                              "GetConnectionTypeInfo",
                              event->service->type,
                              GetConnectionTypeInfo_PARAM_NUM,
                              params,
                              PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail");
    }

    return rc;
}

/************************************************************
 * Function: _request_connection
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - issue connection setup procedure.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
LOCAL INT32 _request_connection(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    INT32 rc = -1;

    LOG_ENTER_FUNCTION;

    if (0 == strcmp(event->service->state_variables[ConnectionStatus_index].value, "Unconfigured")) {
        rc = ERROR_CODE_706;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "connection status Unconfigured\n");
        goto out;
    } else if (0 == strcmp(event->service->state_variables[ConnectionStatus_index].value, "Connected")) {
        if (0 != strcmp(event->service->state_variables[ConnectionType_index].value, "IP_Routed")) {
            rc = ERROR_CODE_710;
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Invalid connection type: %s\n", event->service->state_variables[ConnectionType_index].value);
            goto out;
        }
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "start to request connection\n");
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");
        goto out;
    }
    rc = IGD_pii_request_connection(pIndex->wan_device_index,
                                 pIndex->wan_connection_device_index,
                                 pIndex->wan_connection_service_index,
                                 (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP);
    if (0 != rc) {
        rc = ERROR_CODE_704;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_request_connection() fail\n");
        goto out;
    }

    // construct action response
    rc = PAL_upnp_make_action(&event->request->action_result,
                              "RequestConnection",
                              event->service->type,
                              0,
                              NULL,
                              PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail\n");
    }
    return rc;

out:
    event->request->action_result = NULL;
    _set_err_str(rc, event->request->error_str);
    event->request->error_code = rc;

    LOG_LEAVE_FUNCTION;
    return rc;
}

/************************************************************
 * Function: _force_termination
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - force to terminate current connection immediately.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
LOCAL INT32 _force_termination(INOUT struct action_event *event)
{
    struct device_and_service_index *pIndex = NULL;
    INT32 rc = -1;

    LOG_ENTER_FUNCTION;

    if (0 != strcmp(event->service->state_variables[ConnectionType_index].value, "IP_Routed")) {
        rc = ERROR_CODE_710;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Invalid connection type: %s\n", event->service->state_variables[ConnectionType_index].value);
        goto out;
    } else if (0 == strcmp(event->service->state_variables[ConnectionStatus_index].value, 
    "Disconnected")) {
        rc = ERROR_CODE_711;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "Connection already terminated\n");
        goto out;
    }

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "start to force termination\n");
    pIndex = (struct device_and_service_index *)(event->service->private);
    if (NULL == pIndex) {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "pIndex is NULL");
        goto out;
    }
    rc = IGD_pii_force_termination(pIndex->wan_device_index,
                               pIndex->wan_connection_device_index,
                               pIndex->wan_connection_service_index,
                               (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,event->service->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP);

    if (0 != rc) {
        rc = ERROR_CODE_501;
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_force_termination() fail\n");
        goto out;
    }

    // construct action response
    rc = PAL_upnp_make_action(&event->request->action_result,
                              "ForceTermination",
                              event->service->type,
                              0,
                              NULL,
                              PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail\n");
    }
    return rc;

out:
    event->request->action_result = NULL;
    _set_err_str(rc, event->request->error_str);
    event->request->error_code = rc;
        
    LOG_LEAVE_FUNCTION;
    return rc;
}

/************************************************************
 * Function: _get_status_info
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - get current connection status, last connection
 *      error and up time.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
#define GetStatusInfo_PARAM_NUM (3)
LOCAL INT32 _get_status_info(INOUT struct action_event *event)
{
    pal_string_pair params[GetStatusInfo_PARAM_NUM];
    INT32 rc = -1;

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "%s: state variables\nConnectionStatus: %s\nLastConnectionError: %s\nUptime: %s\n",
            __FUNCTION__,
            event->service->state_variables[ConnectionStatus_index].value,
            event->service->state_variables[LastConnectionError_index].value,
            event->service->state_variables[Uptime_index].value);

    params[0].name = "NewConnectionStatus";
    params[0].value = event->service->state_variables[ConnectionStatus_index].value;
    params[1].name = "NewLastConnectionError";
    params[1].value = event->service->state_variables[LastConnectionError_index].value;
    params[2].name = "NewUptime";
    params[2].value = event->service->state_variables[Uptime_index].value;

    // construct action response
    rc = PAL_upnp_make_action((VOID**)&event->request->action_result,
                                                    "GetStatusInfo",
                                                    event->service->type, 
                                                    GetStatusInfo_PARAM_NUM,
                                                    params,
                                                    PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) 
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail");
    }
        
    return rc;
}

/************************************************************
 * Function: _get_external_ip_address
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - get external IP address of IGD.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
#define GetExternalIPAddress_PARAM_NUM (1)
LOCAL INT32 _get_external_ip_address(INOUT struct action_event *event)
{
    pal_string_pair params[GetExternalIPAddress_PARAM_NUM];
    INT32 rc = -1;

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "%s: state variables\nExternalIPAddress: %s\n",
                    __FUNCTION__,
                    event->service->state_variables[ExternalIPAddress_index].value);

    params[0].name = "NewExternalIPAddress";
    params[0].value = event->service->state_variables[ExternalIPAddress_index].value;

    // construct action response
    rc = PAL_upnp_make_action((VOID**)&event->request->action_result,
                                                    "GetExternalIPAddress",
                                                    event->service->type, 
                                                    GetExternalIPAddress_PARAM_NUM,
                                                    params,
                                                    PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail");
    }
        
    return rc;
}

/************************************************************
 * Function: _get_link_layer_max_bit_rates
 *
 *  Parameters:	
 *      event: Input/Output. struct of action_event from PAL UPnP layer.
 *
 *  Description:
 *      UPnP action handler - get upstream max bitrate and downstream
 *      max bitrate.
 *
 *  Return Values: INT32
 *      0 if successful else error code.
 ************************************************************/   
#define GetLinkLayerMaxBitRates_PARAM_NUM (2)
LOCAL INT32 _get_link_layer_max_bit_rates(INOUT struct action_event *event)
{
    pal_string_pair params[GetLinkLayerMaxBitRates_PARAM_NUM];
    INT32 rc = -1;

    PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "%s: state variables\nUpstreamMaxBitRate: %s\nDownstreamMaxBitRate: %s\n",
                                            __FUNCTION__,
                                            event->service->state_variables[UpstreamMaxBitRate_index].value,
                                            event->service->state_variables[DownstreamMaxBitRate_index].value);

    params[0].name = "NewUpstreamMaxBitRate";
    params[0].value = event->service->state_variables[UpstreamMaxBitRate_index].value;
    params[1].name = "NewDownstreamMaxBitRate";
    params[1].value = event->service->state_variables[DownstreamMaxBitRate_index].value;

    // construct action response
    rc = PAL_upnp_make_action((VOID**)&event->request->action_result,
                                                    "GetLinkLayerMaxBitRates",
                                                    event->service->type, 
                                                    GetLinkLayerMaxBitRates_PARAM_NUM,
                                                    params,
                                                    PAL_UPNP_ACTION_RESPONSE);
    if (0!=rc) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_make_action() fail");
    }
        
    return rc;
}

/************************************************************
 * Function: WANConnectionServiceUpdate
 *
 *  Parameters:	
 *      ps: Input. struct of upnp_service.
 *
 *  Description:
 *      Internal update function for WANIPConnection and WANPPPConnection,
 *      used to update all internal state variables if necessary, and send event
 *      notification if needed.
 *      This function is called periodically.
 *
 *  Return Values: VOID
 ************************************************************/   
#define WANCONNECTION_MAX_EVENT_NUM (4)
VOID IGD_update_wan_connection_service(IN struct upnp_device  *pd,
                                        IN struct upnp_service  *ps)
{
    struct device_and_service_index *pIndex = NULL;
    CHAR OutStr[PAL_UPNP_LINE_SIZE] = {0};
    CHAR OutStr2[PAL_UPNP_LINE_SIZE] = {0};

    INT32 eventnum = 0, PortMappingEntryNum;
    CHAR *var_name[WANCONNECTION_MAX_EVENT_NUM] = {0};
    CHAR *var_value[WANCONNECTION_MAX_EVENT_NUM] = {0};

    INT32 rc = -1;

    if (NULL == ps) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "ps is NULL");
        return;
    }

    pthread_mutex_lock(&ps->service_mutex);

    pIndex = (struct device_and_service_index *)(ps->private);
    if (NULL == pIndex) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "ps->private is NULL");
        pthread_mutex_unlock(&ps->service_mutex);
        return;
    }

    if (0 == IGD_pii_get_possible_connection_types(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         OutStr) ) {
        if (0 != strcmp(OutStr, ps->state_variables[PossibleConnectionTypes_index].value)) {
            strncpy(ps->state_variables[PossibleConnectionTypes_index].value, OutStr, strlen(OutStr)+1);
            strncpy(ps->event_variables[PossibleConnectionTypes_event_index].value, OutStr, strlen(OutStr)+1);

            var_name[eventnum] = (CHAR *)ps->event_variables[PossibleConnectionTypes_event_index].name;
            var_value[eventnum] = ps->event_variables[PossibleConnectionTypes_event_index].value;
            eventnum++;
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "PossibleConnectionTypes updated & evented");
        }
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_possible_connection_types() fail");
    }

    OutStr[0] = 0;
    if (0 == IGD_pii_get_connection_status(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         OutStr) ) {
        if (0 != strcmp(OutStr, ps->state_variables[ConnectionStatus_index].value)) {
            strncpy(ps->state_variables[ConnectionStatus_index].value, OutStr, strlen(OutStr)+1);
            strncpy(ps->event_variables[ConnectionStatus_event_index].value, OutStr, strlen(OutStr)+1);

            var_name[eventnum] = (CHAR *)ps->event_variables[ConnectionStatus_event_index].name;
            var_value[eventnum] = ps->event_variables[ConnectionStatus_event_index].value;
            eventnum++;
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ConnectionStatus updated & evented");
        }
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_connection_status() fail");
    }

    OutStr[0] = 0;
    if (0 == IGD_pii_get_external_ip(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         OutStr) ) {
        if (0 != strcmp(OutStr, ps->state_variables[ExternalIPAddress_index].value)) {
            strncpy(ps->state_variables[ExternalIPAddress_index].value, OutStr, strlen(OutStr)+1);
            strncpy(ps->event_variables[ExternalIPAddress_event_index].value, OutStr, strlen(OutStr)+1);

            var_name[eventnum] = (CHAR *)ps->event_variables[ExternalIPAddress_event_index].name;
            var_value[eventnum] = ps->event_variables[ExternalIPAddress_event_index].value;
            eventnum++;
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ExternalIPAddress updated & evented");
        }
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_external_ip() fail");
    }

    OutStr[0] = 0;
    if (0 == IGD_pii_get_portmapping_entry_num(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         &PortMappingEntryNum) ) {
        snprintf(OutStr, PAL_UPNP_LINE_SIZE, "%d", PortMappingEntryNum);
        if (0 != strcmp(OutStr, ps->state_variables[PortMappingNumberOfEntries_index].value)) {
            strncpy(ps->state_variables[PortMappingNumberOfEntries_index].value, OutStr, strlen(OutStr)+1);
            strncpy(ps->event_variables[PortMappingNumberOfEntries_event_index].value, OutStr, strlen(OutStr)+1);

            var_name[eventnum] = (CHAR *)ps->event_variables[PortMappingNumberOfEntries_event_index].name;
            var_value[eventnum] = ps->event_variables[PortMappingNumberOfEntries_event_index].value;
            eventnum++;
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "PortMappingNumberOfEntries updated & evented");
        }
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_PortMappingEntry_num() fail");
    }

    OutStr[0] = 0;
    if (0 == IGD_pii_get_up_time(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         OutStr) ) {
        if (0 != strcmp(OutStr, ps->state_variables[Uptime_index].value))
            strncpy(ps->state_variables[Uptime_index].value, OutStr, strlen(OutStr)+1);
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_up_time() fail");
    }

    OutStr[0] = 0;
    if (0 == IGD_pii_get_connection_type(pIndex->wan_device_index,
                                                         pIndex->wan_connection_device_index,
                                                         pIndex->wan_connection_service_index,
                                                         (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                         OutStr) ) {
        if (0 != strcmp(OutStr, ps->state_variables[ConnectionType_index].value)) {
            strncpy(ps->state_variables[ConnectionType_index].value, OutStr, strlen(OutStr)+1);
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "ConnectionType updated");
        }
    } else {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_connection_type() fail");
    }

    OutStr[0] = 0;
    if (strcmp(WAN_PPP_CONNECTION_SERVICE_TYPE,ps->type) == 0) {
        if (0 == IGD_pii_get_link_layer_max_bitrate(pIndex->wan_device_index,
                                                             pIndex->wan_connection_device_index,
                                                             pIndex->wan_connection_service_index,
                                                             (strcmp(WAN_IP_CONNECTION_SERVICE_TYPE,ps->type) == 0) ? SERVICETYPE_IP : SERVICETYPE_PPP,
                                                             OutStr,
                                                             OutStr2) ) {
            if (0 != strcmp(OutStr, ps->state_variables[UpstreamMaxBitRate_index].value)) {
                strncpy(ps->state_variables[UpstreamMaxBitRate_index].value, OutStr, strlen(OutStr)+1);
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "UpstreamMaxBitRate updated");
            }
            if (0 != strcmp(OutStr2, ps->state_variables[DownstreamMaxBitRate_index].value)) {
                strncpy(ps->state_variables[DownstreamMaxBitRate_index].value, OutStr2, strlen(OutStr2)+1);
                PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "DownstreamMaxBitRate updated");
            }
        } else {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "IGD_pii_get_connection_type() fail");
        }
    }

    // notify if needed
    if (eventnum > 0) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_DEBUG, "event num: %d", eventnum);
        rc = PAL_upnp_notify (PAL_upnp_device_getHandle(),
                        (const CHAR *)pd->udn,
                        ps->serviceID,
                        (const CHAR **)var_name,
                        (const CHAR **)var_value,
                        eventnum);
        if(rc)
        {
            PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_notify() fail, error code=%d", rc);
        }
    }

    pthread_mutex_unlock(&ps->service_mutex);

    return;
}

VOID IGD_update_pm_lease_time(struct upnp_device *pd, struct upnp_service *ps)
{
    if (ps == NULL) {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "ps is NULL");
        return;
    }

    pthread_mutex_lock(&ps->service_mutex);
    Utopia_InvalidateDynPortMappings();
    pthread_mutex_unlock(&ps->service_mutex);

    return;
}

LOCAL struct upnp_action wan_ppp_connection_service_actions[] = {
    {"SetConnectionType", _set_connection_type},
    {"GetConnectionTypeInfo", _get_connection_type_info},
    {"RequestConnection", _request_connection},
    {"ForceTermination", _force_termination},
    {"GetStatusInfo", _get_status_info},
    {"GetLinkLayerMaxBitRates", _get_link_layer_max_bit_rates},
    {"GetExternalIPAddress", _get_external_ip_address},
    {"GetNATRSIPStatus", IGD_get_NATRSIP_status},
    {"GetGenericPortMappingEntry", IGD_get_GenericPortMapping_entry},
    {"GetSpecificPortMappingEntry", IGD_get_SpecificPortMapping_entry},
    {"AddPortMapping", IGD_add_PortMapping},
    {"DeletePortMapping", IGD_delete_PortMapping},
	{NULL, NULL}
};

LOCAL struct upnp_action wan_ip_connection_service_actions[] = {
    {"SetConnectionType", _set_connection_type},
    {"GetConnectionTypeInfo", _get_connection_type_info},
    {"RequestConnection", _request_connection},
    {"ForceTermination", _force_termination},
    {"GetStatusInfo", _get_status_info},
    //{"GetLinkLayerMaxBitRates", _get_link_layer_max_bit_rates},
    {"GetExternalIPAddress", _get_external_ip_address},
    {"GetNATRSIPStatus", IGD_get_NATRSIP_status},
    {"GetGenericPortMappingEntry", IGD_get_GenericPortMapping_entry},
    {"GetSpecificPortMappingEntry", IGD_get_SpecificPortMapping_entry},
    {"AddPortMapping", IGD_add_PortMapping},
    {"DeletePortMapping", IGD_delete_PortMapping},
	{NULL, NULL}
};

typedef struct variable_name_and_value_s
{
    const CHAR * name;
    const CHAR * default_value;
}variable_name_and_value_t;

variable_name_and_value_t wan_ppp_connection_service_state_variables[] = 
{
   {"ConnectionType", ""},
   {"PossibleConnectionTypes", ""},
   {"ConnectionStatus", ""},
   {"Uptime", ""},
   {"LastConnectionError", "ERROR_NONE"},
   {"ExternalIPAddress", ""},
   {"RSIPAvailable", ""},
   {"NATEnabled", ""},
   {"PortMappingNumberOfEntries", ""},
   {"PortMappingEnabled", ""},
   {"PortMappingLeaseDuration", ""},
   {"RemoteHost", ""},
   {"ExternalPort", ""},
   {"InternalPort", ""},
   {"PortMappingProtocol", ""},
   {"InternalClient", ""},
   {"PortMappingDescription", ""},
   {"UpstreamMaxBitRate", ""},
   {"DownstreamMaxBitRate", ""},
   {NULL,NULL}
};

variable_name_and_value_t wan_ip_connection_service_state_variables[] = 
{
   {"ConnectionType", "IP_Routed"},
   {"PossibleConnectionTypes", "Unconfigured,IP_Routed"},
   {"ConnectionStatus", ""},
   {"Uptime", ""},
   {"LastConnectionError", "ERROR_NONE"},
   {"ExternalIPAddress", ""},
   {"RSIPAvailable", ""},
   {"NATEnabled", "1"},
   {"PortMappingNumberOfEntries", "0"},
   {"PortMappingEnabled", ""},
   {"PortMappingLeaseDuration", ""},
   {"RemoteHost", ""},
   {"ExternalPort", ""},
   {"InternalPort", ""},
   {"PortMappingProtocol", ""},
   {"InternalClient", ""},
   {"PortMappingDescription", ""},
   //{"UpstreamMaxBitRate", ""},
   //{"DownstreamMaxBitRate", ""},
   {NULL,NULL}
};

variable_name_and_value_t wan_connection_service_event_variables[] = 
{
    {"PossibleConnectionTypes", "Unconfigured,IP_Routed"},
    {"ConnectionStatus", ""},
    {"ExternalIPAddress", ""},
    {"PortMappingNumberOfEntries", "0"},
    {NULL,NULL}
};
#define WAN_PPP_CONNECTION_SERVICE_STATE_VARIABLES_SIZE (sizeof(wan_ppp_connection_service_state_variables)/sizeof(variable_name_and_value_t))
#define WAN_IP_CONNECTION_SERVICE_STATE_VARIABLES_SIZE (sizeof(wan_ip_connection_service_state_variables)/sizeof(variable_name_and_value_t))
#define WAN_CONNECTION_SERVICE_EVENT_VARIABLES_SIZE (sizeof(wan_connection_service_event_variables)/sizeof(variable_name_and_value_t))

#define WAN_CONNECTION_SERVICE_INDEX_MEX_LENGTH 3
LOCAL INT32 _wan_connection_service_destroy(IN struct upnp_service *service);
/************************************************************
* Function: _wan_connection_service_init
*
*  Parameters:
*               ppp_or_ip:                IN.          ppp or ip
*               input_index_struct:    IN.          Wan device index, wan connection device index and wan connection service index
*               wan_desc_file:          INOUT.    The fd to write description file
*  Description:
*     This function is to initialize a wan ppp or ip connection service instance
*
*  Return Values: struct upnp_service *
*               the initialized wan ppp or ip connection service if successful else NULL
************************************************************/
LOCAL struct upnp_service * _wan_connection_service_init (IN BOOL ppp_or_ip,
                                                             IN VOID* input_index_struct,
                                                             INOUT FILE *wan_desc_file)
{
    INT32 rv = 0;
    struct device_and_service_index* temp_index = NULL;
    struct upnp_service *new_wan_connection_service = NULL;
    INT32 i = 0;
    LOCAL INT32 temp_ppp=0;
    LOCAL INT32 temp_ip=0;

    LOG_ENTER_FUNCTION;

    /*check input parameters*/
    temp_index = (struct device_and_service_index*)input_index_struct;
    if ( ((enum_ppp_service_type != ppp_or_ip)&&(enum_ip_service_type != ppp_or_ip))
          ||(!temp_index)
          || (temp_index->wan_device_index<0)
          || (temp_index->wan_connection_device_index<0)
          || (temp_index->wan_connection_service_index<0)
          || (temp_index->wan_connection_service_index>999)//for simple, WAN_CONNECTION_SERVICE_INDEX_MEX_LENGTH==3 now
          || (!wan_desc_file)
          )
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "input parameter error");
        return NULL;
    }

    /*malloc a new_wan_connection_device */
    new_wan_connection_service = (struct upnp_service *)calloc(1, sizeof(struct upnp_service));
    if (!new_wan_connection_service)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory : new_wan_connection_service malloc error");
        return NULL;
    }

    /*assign service_mutex*/
    if (pthread_mutex_init (&(new_wan_connection_service->service_mutex), NULL) != 0)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "ERROR: service_mutex init error!");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }

    /*assign destroy_function*/
    new_wan_connection_service->destroy_function = _wan_connection_service_destroy;

    /*assign type*/
    if (enum_ppp_service_type == ppp_or_ip)
    {
        new_wan_connection_service->type = WAN_PPP_CONNECTION_SERVICE_TYPE;
    }
    else// if (enum_ip_service_type == ppp_or_ip)
    {
        new_wan_connection_service->type = WAN_IP_CONNECTION_SERVICE_TYPE;
    }

    /*assign serviceID*/
    new_wan_connection_service->serviceID = 
        (CHAR *)calloc
        (   1,
            strlen( (enum_ppp_service_type == ppp_or_ip)?WAN_PPP_CONNECTION_SERVICE_PREFIX : WAN_IP_CONNECTION_SERVICE_PREFIX )+WAN_CONNECTION_SERVICE_INDEX_MEX_LENGTH+1
        );

    if (!(new_wan_connection_service->serviceID))
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory : serviceID malloc error");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }
    rv = snprintf((CHAR *)(new_wan_connection_service->serviceID),
                  strlen( (enum_ppp_service_type == ppp_or_ip)?WAN_PPP_CONNECTION_SERVICE_PREFIX : WAN_IP_CONNECTION_SERVICE_PREFIX )+WAN_CONNECTION_SERVICE_INDEX_MEX_LENGTH+1,
                  "%s%d",
                  ( (enum_ppp_service_type == ppp_or_ip)?WAN_PPP_CONNECTION_SERVICE_PREFIX : WAN_IP_CONNECTION_SERVICE_PREFIX ),
                  temp_index->wan_connection_service_index );
    if ( rv < 0 )
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "print content to serviceID error");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }

    /*assign actions*/
    new_wan_connection_service->actions = (enum_ppp_service_type == ppp_or_ip)? wan_ppp_connection_service_actions : wan_ip_connection_service_actions;

    /*assign state_variables*/
    new_wan_connection_service->state_variables = 
    (struct upnp_variable *)calloc
        (   (enum_ppp_service_type == ppp_or_ip)?WAN_PPP_CONNECTION_SERVICE_STATE_VARIABLES_SIZE : WAN_IP_CONNECTION_SERVICE_STATE_VARIABLES_SIZE,
            sizeof(struct upnp_variable)
        );

    if (!new_wan_connection_service->state_variables)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "out of memory : state_variables malloc error");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }
    if (enum_ppp_service_type == ppp_or_ip)
    {
        for(i=0; wan_ppp_connection_service_state_variables[i].name!= NULL; i++)
        {
            new_wan_connection_service->state_variables[i].name = wan_ppp_connection_service_state_variables[i].name;
            strncpy(new_wan_connection_service->state_variables[i].value, wan_ppp_connection_service_state_variables[i].default_value, strlen(wan_ppp_connection_service_state_variables[i].default_value)+1);
        }
    }
    else// if (enum_ip_service_type == ppp_or_ip)
    {
        for(i=0; wan_ip_connection_service_state_variables[i].name!= NULL; i++)
        {
            new_wan_connection_service->state_variables[i].name = wan_ip_connection_service_state_variables[i].name;
            strncpy(new_wan_connection_service->state_variables[i].value, wan_ip_connection_service_state_variables[i].default_value, strlen(wan_ip_connection_service_state_variables[i].default_value)+1);
        }
    }

    /*assign event_variables*/
    new_wan_connection_service->event_variables = (struct upnp_variable *)calloc(WAN_CONNECTION_SERVICE_EVENT_VARIABLES_SIZE, sizeof(struct upnp_variable));
    if (!new_wan_connection_service->event_variables)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "out of memory : event_variables malloc error");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }
    for(i=0; wan_connection_service_event_variables[i].name!= NULL; i++)
    {
        new_wan_connection_service->event_variables[i].name = wan_connection_service_event_variables[i].name;
        strncpy(new_wan_connection_service->event_variables[i].value, wan_connection_service_event_variables[i].default_value, strlen(wan_connection_service_event_variables[i].default_value)+1);
    }

    /*assign private*/
    new_wan_connection_service->private = calloc(1, sizeof(struct device_and_service_index));
    if (!new_wan_connection_service->private)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "out of memory : private malloc error");
        _wan_connection_service_destroy(new_wan_connection_service);
        return NULL;
    }
    ((struct device_and_service_index *)new_wan_connection_service->private)->wan_device_index = temp_index->wan_device_index;
    ((struct device_and_service_index *)new_wan_connection_service->private)->wan_connection_device_index = temp_index->wan_connection_device_index;
    ((struct device_and_service_index *)new_wan_connection_service->private)->wan_connection_service_index = temp_index->wan_connection_service_index;

    /*generate service description file*/
    fprintf(wan_desc_file,
"       <service>\n"
"           <serviceType>%s</serviceType>\n"
"           <serviceId>%s</serviceId>\n"
"           <SCPDURL>%s</SCPDURL>\n"
"           <controlURL>%s%d</controlURL>\n"
"           <eventSubURL>%s%d</eventSubURL>\n"
"       </service>\n", 
    new_wan_connection_service->type,
    new_wan_connection_service->serviceID,
    ( (enum_ppp_service_type == ppp_or_ip)?"/WANPPPConnectionServiceSCPD.xml" : "/WANIPConnectionServiceSCPD.xml" ),
    ( (enum_ppp_service_type == ppp_or_ip)?"/upnp/control/WANPPPConnection" : "/upnp/control/WANIPConnection" ),( (enum_ppp_service_type == ppp_or_ip)?temp_ppp : temp_ip ),
    ( (enum_ppp_service_type == ppp_or_ip)?"/upnp/event/WANPPPConnection" : "/upnp/event/WANIPConnection" ),( (enum_ppp_service_type == ppp_or_ip)?temp_ppp : temp_ip )
    );


    if (enum_ppp_service_type == ppp_or_ip)
    {
         temp_ppp++;
    }
    else// if (enum_ip_service_type == ppp_or_ip)
    {
         temp_ip++;
    }

    LOG_LEAVE_FUNCTION;
    return new_wan_connection_service;
}

/************************************************************
* Function: _wan_connection_service_destroy
*
*  Parameters:
*               service:                     IN.    Wan service to be destroyed
*  Description:
*     This function is called by wan_connection_device_destroy() to destroy a wan connection service instance
*
*  Return Values: INT32
*      0 if successful else error code.
************************************************************/
LOCAL INT32 _wan_connection_service_destroy(IN struct upnp_service *service)
{
    LOG_ENTER_FUNCTION;

    /*check input parameters*/
    if (!service)
    {
        PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "input parameter error");
        return -1;
    }

    /*RDKB-7139, CID-32721, null check before free */
    /*free serviceID*/
    if((CHAR *)service->serviceID)
    {
        free((CHAR *)service->serviceID);
        service->serviceID = NULL;
    }
    /*free state_variables*/
    if(service->state_variables)
    {
        free(service->state_variables);
        service->state_variables = NULL;
    }
    /*free event_variables*/
    if(service->event_variables)
    {
        free(service->event_variables);
        service->event_variables = NULL;
    }

    /*free private*/
    if(service->private)
    {
        free(service->private);
        service->private = NULL;
    }

    /*destroy service_mutex*/
    pthread_mutex_destroy (&(service->service_mutex));

    free(service);

    LOG_LEAVE_FUNCTION;
    return 0;
}

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
struct upnp_service * IGD_wan_ppp_connection_service_init (IN VOID* input_index_struct,
                                                             INOUT FILE *wan_desc_file)
{
    return _wan_connection_service_init(enum_ppp_service_type, input_index_struct, wan_desc_file);
}

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
struct upnp_service * IGD_wan_ip_connection_service_init (IN VOID* input_index_struct,
                                                           INOUT FILE *wan_desc_file)
{
    return _wan_connection_service_init(enum_ip_service_type, input_index_struct, wan_desc_file);
}

