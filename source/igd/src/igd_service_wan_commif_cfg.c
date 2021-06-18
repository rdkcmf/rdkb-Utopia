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
 * FileName:   igd_service_layer3_fwd.c
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD WANCommonInterfaceConfig service
 *****************************************************************************/
/*$Id: igd_service_wan_commif_cfg.c,v 1.7 2009/05/26 09:42:03 jianxiao Exp $
 *
 *$Log: igd_service_wan_commif_cfg.c,v $
 *Revision 1.7  2009/05/26 09:42:03  jianxiao
 *Modify the  function IGD_pii_get_common_link_properties
 *
 *Revision 1.6  2009/05/21 06:31:06  jianxiao
 *Change the interface of PII
 *
 *Revision 1.5  2009/05/21 01:59:32  jianxiao
 *Support two or more  WANCommonInterfaceConfig services in difference WANConnectionDevice
 *
 *Revision 1.4  2009/05/15 05:42:14  jianxiao
 *Add event handler
 *
 *Revision 1.3  2009/05/14 02:39:59  jianxiao
 *Modify the interface of PAL_xml_node_GetFirstbyName
 *
 *Revision 1.2  2009/05/14 01:46:27  jianxiao
 *Change the included header name, the function name
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
#include "pal_upnp.h"
#include "pal_def.h"
#include "pal_log.h"
#include "igd_utility.h"
#include "igd_platform_independent_inf.h"

#define WANCOMMONINTERFACECONFIG_SERVICE_ID "urn:upnp-org:serviceId:WANCommonIFC1"
#define WANCOMMONINTERFACECONFIG_SERVICE_TYPE "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"
#define COMMONLINKROPERTIES_PARA_NUM 4
#define	PROPERTIES_STRING_LEN	16
#define COMMON_LINK_PROPERTIES_EVENT_NUM (1)
#define WANCOMMIFCFG_MAX_EVENT_NUM 1

enum CommonLinkProperties_service_state_variables_index
{
    WANAccessType_index=0,
	Layer1UpstreamMaxBitRate_index,
	Layer1DownstreamMaxBitRate_index,
	PhysicalLinkStatus_index,
	TotalBytesSent_index,
	TotalBytesReceived_index,
	TotalPacketsSent_index,
    TotalPacketsReceived_index
};

LOCAL INT32 _igd_get_CommonLinkProperties (IN struct action_event *event);
LOCAL INT32 _igd_get_TotalBytesSent (IN struct action_event *event);
LOCAL INT32 _igd_get_TotalBytesReceived (IN struct action_event *event);
LOCAL INT32 _igd_get_TotalPacketsSent (IN struct action_event *event);
LOCAL INT32 _igd_get_TotalPacketsReceived (IN struct action_event *event);


LOCAL struct upnp_action WANCommonInterfaceConfig_actions[] =
{
	{"GetCommonLinkProperties", _igd_get_CommonLinkProperties },
	{"GetTotalBytesSent", _igd_get_TotalBytesSent },
	{"GetTotalBytesReceived", _igd_get_TotalBytesReceived },
	{"GetTotalPacketsSent", _igd_get_TotalPacketsSent },
	{"GetTotalPacketsReceived", _igd_get_TotalPacketsReceived },
	{NULL, NULL}
};
LOCAL const CHAR * WANCommonInterfaceConfig_status_variables_name[] = 
{
    "WANAccessType",
	"Layer1UpstreamMaxBitRate",
	"Layer1DownstreamMaxBitRate",
	"PhysicalLinkStatus",
	"TotalBytesSent",
	"TotalBytesReceived",
	"TotalPacketsSent",
	"TotalPacketsReceived",
    NULL
};
LOCAL const CHAR * WANCommonInterfaceConfig_event_variables_name[] = 
{
        "PhysicalLinkStatus",
    NULL
};
/************************************************************
 * Function: _igd_service_WANCommonInterfaceConfig_desc_file 
 *
 *  Parameters:	
 *      fp: Input/Output. the description file pointer.
 * 
 *  Description:
 *      This functions generate the description file of WANCommonInterfaceConfig service.
 *
 *  Return Values: INT32
 *      0 if successful ,-1 for error
 ************************************************************/ 
LOCAL INT32 _igd_service_WANCommonInterfaceConfig_desc_file(INOUT FILE *fp)
{
	LOCAL INT32 service_index=0;
	if(fp==NULL)
		return -1;
	fprintf(fp, "<service>\n");
      fprintf(fp, "<serviceType>%s</serviceType>\n",WANCOMMONINTERFACECONFIG_SERVICE_TYPE);
      fprintf(fp, "<serviceId>%s</serviceId>\n",WANCOMMONINTERFACECONFIG_SERVICE_ID);
	  fprintf(fp, "<SCPDURL>/WANCommonInterfaceConfigSCPD.xml</SCPDURL>\n");
      fprintf(fp, "<controlURL>/upnp/control/WANCommonInterfaceConfig%d</controlURL>\n",service_index);
      fprintf(fp, "<eventSubURL>/upnp/event/WANCommonInterfaceConfig%d</eventSubURL>\n",service_index);
    fprintf(fp, "</service>\n");
	service_index++;
	return 0;
}
/************************************************************
* Function: _igd_service_WANCommonInterfaceConfig_destroy
*
*  Parameters: 
*	   pservice:		   IN. the service pointer. 
* 
*  Description:
*	  This function destroy the service WANCommonInterfaceConfig.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_service_WANCommonInterfaceConfig_destroy(IN struct upnp_service *pservice)
{
	/* pservice->serviceID is a 'const CHAR *' type and required to remove
	   const before call free() function */
	CHAR * serviceID = (CHAR *)NULL;

	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Destroy WANDevice WANCommonInterfaceConfig\n");
	if(pservice==NULL)
		return -1;

	serviceID = (CHAR *)pservice->serviceID; /*RDKB-7140, CID-33052, use after null check */
	SAFE_FREE(pservice->type);
	SAFE_FREE(serviceID);
	SAFE_FREE(pservice->state_variables);
	SAFE_FREE(pservice->event_variables);
	SAFE_FREE(pservice->private);
	pthread_mutex_destroy(&pservice->service_mutex);
	SAFE_FREE(pservice);
	return 0;
}
/************************************************************
* Function: IGD_service_WANCommonInterfaceConfigInit
*
*  Parameters: 
*	   input_index_struct:		   IN. the device index struct. 
*	   fp:   INOUT. the description file pointer. 
* 
*  Description:
*	  This function initialize the service WANCommonInterfaceConfig.  
*
*  Return Values: struct upnp_service*
*	   The service pointer if successful else NULL.
************************************************************/
struct upnp_service* IGD_service_WANCommonInterfaceConfigInit(IN VOID* input_index_struct, INOUT FILE *fp)
{	
	INT32 i;
	struct upnp_service *WANCommonInterfaceConfig_service=NULL;
	
	PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Initilize WANCommonInterfaceConfig of WANDevice %d\n",((struct device_and_service_index*)input_index_struct)->wan_device_index);
	WANCommonInterfaceConfig_service=(struct upnp_service *)calloc(1,sizeof(struct upnp_service));
	if(WANCommonInterfaceConfig_service==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,upnp_service!\n");
		return NULL;
	}

	if(pthread_mutex_init(&WANCommonInterfaceConfig_service->service_mutex, NULL ))
	{
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "Init mutex fail!\n");
        _igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
        return NULL;
	}
	
	WANCommonInterfaceConfig_service->destroy_function = _igd_service_WANCommonInterfaceConfig_destroy;

	WANCommonInterfaceConfig_service->type=(CHAR *)calloc(1,strlen(WANCOMMONINTERFACECONFIG_SERVICE_TYPE)+1);
	if(WANCommonInterfaceConfig_service->type==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,type!\n");
		_igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
		return NULL;
	}
	/* CID 135556 : BUFFER_SIZE_WARNING */
	strncpy(WANCommonInterfaceConfig_service->type, WANCOMMONINTERFACECONFIG_SERVICE_TYPE, strlen(WANCOMMONINTERFACECONFIG_SERVICE_TYPE)+1);
	WANCommonInterfaceConfig_service->type[strlen(WANCOMMONINTERFACECONFIG_SERVICE_TYPE)] = '\0';
	
	WANCommonInterfaceConfig_service->serviceID=(CHAR *)calloc(1,strlen(WANCOMMONINTERFACECONFIG_SERVICE_ID)+1);
	if(WANCommonInterfaceConfig_service->serviceID==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory,serviceID!\n");
		_igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
		return NULL;
	}
	strncpy((CHAR *)WANCommonInterfaceConfig_service->serviceID, WANCOMMONINTERFACECONFIG_SERVICE_ID, strlen(WANCOMMONINTERFACECONFIG_SERVICE_ID)+1);

	WANCommonInterfaceConfig_service->actions = WANCommonInterfaceConfig_actions;

    WANCommonInterfaceConfig_service->state_variables = (struct upnp_variable *)calloc(sizeof(WANCommonInterfaceConfig_status_variables_name)/sizeof(CHAR *),sizeof(struct upnp_variable));
    if (!WANCommonInterfaceConfig_service->state_variables)
    {
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory,state_variables!\n");
        _igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
        return NULL;
    }
    for(i=0; WANCommonInterfaceConfig_status_variables_name[i]!= NULL; i++) {
        WANCommonInterfaceConfig_service->state_variables[i].name = WANCommonInterfaceConfig_status_variables_name[i];
	strncpy(WANCommonInterfaceConfig_service->state_variables[i].value,"",PAL_UPNP_LINE_SIZE);
    }

    WANCommonInterfaceConfig_service->event_variables = (struct upnp_variable *)calloc(sizeof(WANCommonInterfaceConfig_event_variables_name)/sizeof(CHAR *), sizeof(struct upnp_variable));
    if (!WANCommonInterfaceConfig_service->event_variables)
    {
        PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "out of memory,event_variables!\n");
        _igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
        return NULL;
    }
    for(i=0; WANCommonInterfaceConfig_event_variables_name[i]!= NULL; i++) {
        WANCommonInterfaceConfig_service->event_variables[i].name = WANCommonInterfaceConfig_event_variables_name[i];
    }
	
	WANCommonInterfaceConfig_service->private=(struct device_and_service_index *)calloc(1,sizeof(struct device_and_service_index));
	if(WANCommonInterfaceConfig_service->private==NULL)
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"out of memory!\n");
		_igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
		return NULL;
	}
	memcpy(WANCommonInterfaceConfig_service->private, input_index_struct, sizeof(struct device_and_service_index));

	if(_igd_service_WANCommonInterfaceConfig_desc_file(fp))
	{
		PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE,"create WANCommonInterfaceConfig description file fail!\n");
		_igd_service_WANCommonInterfaceConfig_destroy(WANCommonInterfaceConfig_service);
		return NULL;
	}
	
	return WANCommonInterfaceConfig_service;
}
/************************************************************
 * Function: IGD_service_WANCommonInterfaceConfigEventHandler
 *
 *  Parameters:	
 *      pdevice: Input. struct of upnp_device.
 *      pservice: Input. struct of upnp_service.
 *
 *  Description:
*	  handle the event
 *      notification if needed.
 *      This function is called periodically.
 *
 *  Return Values: VOID
 ************************************************************/ 
VOID IGD_service_WANCommonInterfaceConfigEventHandler(IN struct upnp_device  *pdevice,
                                        IN struct upnp_service  *pservice)
{
		struct device_and_service_index *pIndex = NULL;
		CHAR *var_name[WANCOMMIFCFG_MAX_EVENT_NUM] = {0};
    	CHAR *var_value[WANCOMMIFCFG_MAX_EVENT_NUM] = {0};
		CHAR type[PROPERTIES_STRING_LEN];
		CHAR up[PROPERTIES_STRING_LEN];
		CHAR down[PROPERTIES_STRING_LEN];
		CHAR status[PROPERTIES_STRING_LEN];

		if (NULL == pdevice) {
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "pdevice is NULL");
			return;
		}
		if (NULL == pservice) {
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "pservice is NULL");
			return;
		}
	
		pthread_mutex_lock(&pservice->service_mutex);

		pIndex = (struct device_and_service_index *)(pservice->private);
		if (NULL == pIndex) {
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_FAILURE, "No interface infomation");
			pthread_mutex_unlock(&pservice->service_mutex);
			return;
		}

		if(IGD_pii_get_common_link_properties(pIndex->wan_device_index,type,up,down,status))
		{
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"CommonLinkProperties get fail\n");
			pthread_mutex_unlock(&pservice->service_mutex);
			return;
		}

		if(0!= strcmp(status, pservice->state_variables[PhysicalLinkStatus_index].value))
		{
			strncpy(pservice->state_variables[PhysicalLinkStatus_index].value,status, strlen(status)+1);
			strncpy(pservice->event_variables[0].value,status, strlen(status)+1);
			var_name[0] = (CHAR *)pservice->event_variables[0].name;
            var_value[0] = pservice->event_variables[0].value;
			PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO, "Eventing:%s=%s",var_name[0],var_value[0]);
			if(PAL_upnp_notify (PAL_upnp_device_getHandle(),
                        		(const CHAR *)pdevice->udn,
                        		pservice->serviceID,
                        		(const CHAR **)var_name,
                        		(const CHAR **)var_value,
                        		1))
			{
				PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_WARNING, "PAL_upnp_notify() fail");
			}
		}
		pthread_mutex_unlock(&pservice->service_mutex);
	
		return;
}

/************************************************************
* Function: _igd_get_CommonLinkProperties
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetCommonLinkProperties.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_CommonLinkProperties (INOUT struct action_event *event)
{   
	   struct device_and_service_index local_index;
	   pal_string_pair params[COMMONLINKROPERTIES_PARA_NUM];
	   CHAR type[PROPERTIES_STRING_LEN];
	   CHAR up[PROPERTIES_STRING_LEN];
	   CHAR down[PROPERTIES_STRING_LEN];
	   CHAR status[PROPERTIES_STRING_LEN];
	   
	   event->request->error_code = PAL_UPNP_E_SUCCESS;
	   
	   local_index = *((struct device_and_service_index*)event->service->private);
	   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetCommonLinkProperties of WAN%d\n",local_index.wan_device_index);
	   
	   if(IGD_pii_get_common_link_properties(local_index.wan_device_index,type,up,down,status))
	   {
		   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"Layer3Forwarding action:Action fail\n");
		   event->request->error_code = 501;
		   PAL_upnp_make_action(&event->request->action_result,"GetEthernetLinkStatus",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		   return(event->request->error_code);
	   }
	   
	   params[0].name="NewWANAccessType";
	   params[0].value=type;
	   strncpy(event->service->state_variables[WANAccessType_index].value,type, strlen(type)+1);
	   params[1].name="NewLayer1UpstreamMaxBitRate";
	   params[1].value=up;
	   strncpy(event->service->state_variables[Layer1UpstreamMaxBitRate_index].value,up, strlen(up)+1);
	   params[2].name="NewLayer1DownstreamMaxBitRate";
	   params[2].value=down;
	   strncpy(event->service->state_variables[Layer1DownstreamMaxBitRate_index].value,down, strlen(down)+1);
	   params[3].name="NewPhysicalLinkStatus";
	   params[3].value=status;
	   strncpy(event->service->state_variables[PhysicalLinkStatus_index].value,status, strlen(status)+1);
	   PAL_upnp_make_action(&event->request->action_result,"GetCommonLinkProperties",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,4,params,PAL_UPNP_ACTION_RESPONSE);
	   return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_TotalBytesSent
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetTotalBytesSent.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_TotalBytesSent (INOUT struct action_event *event)
{   
	   struct device_and_service_index local_index;
	   pal_string_pair params[COMMONLINKROPERTIES_PARA_NUM];
	   CHAR bytes_sent[PROPERTIES_STRING_LEN];
	   
	   event->request->error_code = PAL_UPNP_E_SUCCESS;
	   
	   local_index = *((struct device_and_service_index*)event->service->private);
	   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalBytesSent of WAN%d\n",local_index.wan_device_index);
	   
	   /*
	    * IGD_pii_get_traffic_stats(local_index.wan_device_index,bufsz,bytes_sent,bytes_rcvd,pkts_sent,pkts_rcvd)
	    */
	   if(IGD_pii_get_traffic_stats(local_index.wan_device_index, PROPERTIES_STRING_LEN, bytes_sent, NULL, NULL, NULL))
	   {
		   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalBytesSent action:Action fail\n");
		   event->request->error_code = 501;
		   PAL_upnp_make_action(&event->request->action_result,"GetTotalBytesSent",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		   return(event->request->error_code);
	   }
	   
	   params[0].name="NewTotalBytesSent";
	   params[0].value=bytes_sent;
	   strncpy(event->service->state_variables[TotalBytesSent_index].value, bytes_sent, PAL_UPNP_LINE_SIZE);
	   PAL_upnp_make_action(&event->request->action_result,"GetTotalBytesSent",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	   return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_TotalBytesReceived
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetTotalBytesReceived.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_TotalBytesReceived (INOUT struct action_event *event)
{   
	   struct device_and_service_index local_index;
	   pal_string_pair params[COMMONLINKROPERTIES_PARA_NUM];
	   CHAR bytes_rcvd[PROPERTIES_STRING_LEN];
	   
	   event->request->error_code = PAL_UPNP_E_SUCCESS;
	   
	   local_index = *((struct device_and_service_index*)event->service->private);
	   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalBytesReceived of WAN%d\n",local_index.wan_device_index);
	   
	   /*
	    * IGD_pii_get_traffic_stats(local_index.wan_device_index,bufsz,bytes_sent,bytes_rcvd,pkts_sent,pkts_rcvd)
	    */
	   if(IGD_pii_get_traffic_stats(local_index.wan_device_index, PROPERTIES_STRING_LEN, NULL, bytes_rcvd, NULL, NULL))
	   {
		   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalBytesReceived action:Action fail\n");
		   event->request->error_code = 501;
		   PAL_upnp_make_action(&event->request->action_result,"GetTotalBytesReceived",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		   return(event->request->error_code);
	   }
	   
	   params[0].name="NewTotalBytesReceived";
	   params[0].value=bytes_rcvd;
	   strncpy(event->service->state_variables[TotalBytesReceived_index].value, bytes_rcvd, PAL_UPNP_LINE_SIZE);
	   PAL_upnp_make_action(&event->request->action_result,"GetTotalBytesReceived",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	   return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_TotalPacketsSent
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetTotalPacketsSent.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_TotalPacketsSent (INOUT struct action_event *event)
{   
	   struct device_and_service_index local_index;
	   pal_string_pair params[COMMONLINKROPERTIES_PARA_NUM];
	   CHAR pkts_sent[PROPERTIES_STRING_LEN];
	   
	   event->request->error_code = PAL_UPNP_E_SUCCESS;
	   
	   local_index = *((struct device_and_service_index*)event->service->private);
	   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalPacketsSent of WAN%d\n",local_index.wan_device_index);
	   
	   /*
	    * IGD_pii_get_traffic_stats(local_index.wan_device_index,bufsz,bytes_sent,bytes_rcvd,pkts_sent,pkts_rcvd)
	    */
	   if(IGD_pii_get_traffic_stats(local_index.wan_device_index, PROPERTIES_STRING_LEN, NULL, NULL, pkts_sent, NULL))
	   {
		   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalPacketsSent action:Action fail\n");
		   event->request->error_code = 501;
		   PAL_upnp_make_action(&event->request->action_result,"GetTotalPacketsSent",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		   return(event->request->error_code);
	   }
	   
	   params[0].name="NewTotalPacketsSent";
	   params[0].value=pkts_sent;
	   strncpy(event->service->state_variables[TotalPacketsSent_index].value, pkts_sent, PAL_UPNP_LINE_SIZE);
	   PAL_upnp_make_action(&event->request->action_result,"GetTotalPacketsSent",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	   return(event->request->error_code);
}

/************************************************************
* Function: _igd_get_TotalPacketsReceived
*
*  Parameters: 
*
*	   event:   INOUT. the action_event struct of the action. 
* 
*  Description:
*	  This function do the action of GetTotalPacketsReceived.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
LOCAL INT32 _igd_get_TotalPacketsReceived (INOUT struct action_event *event)
{   
	   struct device_and_service_index local_index;
	   pal_string_pair params[COMMONLINKROPERTIES_PARA_NUM];
	   CHAR pkts_rcvd[PROPERTIES_STRING_LEN];
	   
	   event->request->error_code = PAL_UPNP_E_SUCCESS;
	   
	   local_index = *((struct device_and_service_index*)event->service->private);
	   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalPacketsReceived of WAN%d\n",local_index.wan_device_index);
	   
	   /*
	    * IGD_pii_get_traffic_stats(local_index.wan_device_index,bufsz,bytes_sent,bytes_rcvd,pkts_sent,pkts_rcvd)
	    */
	   if(IGD_pii_get_traffic_stats(local_index.wan_device_index, PROPERTIES_STRING_LEN, NULL, NULL, NULL, pkts_rcvd))
	   {
		   PAL_LOG(LOG_IGD_NAME, PAL_LOG_LEVEL_INFO,"GetTotalPacketsReceived action:Action fail\n");
		   event->request->error_code = 501;
		   PAL_upnp_make_action(&event->request->action_result,"GetTotalPacketsReceived",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,0,NULL,PAL_UPNP_ACTION_RESPONSE);
		   return(event->request->error_code);
	   }
	   
	   params[0].name="NewTotalPacketsReceived";
	   params[0].value=pkts_rcvd;
	   strncpy(event->service->state_variables[TotalPacketsReceived_index].value, pkts_rcvd, PAL_UPNP_LINE_SIZE);
	   PAL_upnp_make_action(&event->request->action_result,"GetTotalPacketsReceived",WANCOMMONINTERFACECONFIG_SERVICE_TYPE,1,params,PAL_UPNP_ACTION_RESPONSE);
	   return(event->request->error_code);
}

VOID IGD_WANCommonInterfaceConfig_eventvariables_init(struct upnp_service *ps)
{
    CHAR type[PROPERTIES_STRING_LEN];
    CHAR up[PROPERTIES_STRING_LEN];
    CHAR down[PROPERTIES_STRING_LEN];
    CHAR status[PROPERTIES_STRING_LEN];

    if (NULL == ps){
        PAL_LOG("WANCOMMIFCFG", 1, "service is NULL");
        return;
    }

    pthread_mutex_lock(&ps->service_mutex);

    if (IGD_pii_get_common_link_properties(0, type, up, down, status)){
        PAL_LOG("WANCOMMIFCFG", 1, "CommonLinkProperties get fail");
        pthread_mutex_unlock(&ps->service_mutex);
        return;
    }

    strncpy(ps->event_variables[0].value, status, strlen(status)+1);

    pthread_mutex_unlock(&ps->service_mutex);

    return;
}

