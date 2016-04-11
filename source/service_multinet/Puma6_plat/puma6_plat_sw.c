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
   Copyright [2015] [Cisco Systems, Inc.]
 
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
#include "service_multinet_base.h"
#include "puma6_plat_sw.h"
#include "puma6_plat_map.h"
#include "service_multinet_ep.h"
#include "sysevent/sysevent.h"
#include "syscfg/syscfg.h"

#include <stdio.h>
#include <string.h>
extern int sysevent_fd_interactive;
extern token_t sysevent_token_interactive;
static unsigned char bLibInited = 0;


static int psm_get_record(const char *name, char *val, int size)
{
    FILE *fp;
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "psmcli get %s", name);

    if ((fp = popen(cmd, "rb")) == NULL) {
        return -1;
    }

    fgets(val, size, fp);

    pclose(fp);
    return 0;
}

int configVlan_ESW(PSWFabHALArg args, int numArgs, BOOL up) {
    int i;
    PSwPortState portState;
    
    char cmdBuff[128];
    char ifname[80];
    char temp_ifname[80];
    char iot_enabled[20];
    memset(ifname, 0, 80);
    memset(temp_ifname, 0, 80);

	if (100 == args[0].vidParams.vid) //brlan0 case
    {   
        for (i = 0; i < numArgs; ++i ) { 
    
            portState = (PSwPortState) args[i].portID;
            stringIDExtSw(portState, ifname, sizeof(ifname));
    
            //#Args: netid, netvid, members...
            sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", up ? "addVlan" : "delVlan", args[i].hints.network->inst, args[i].vidParams.vid, ifname, args[i].vidParams.tagging ? "-t" : "");
    
            system(cmdBuff);
        }
    }   

    else //brlan1 case
    {   
        for (i = 0; i < numArgs; ++i ) { 
            portState = (PSwPortState) args[i].portID;
            stringIDExtSw(portState, temp_ifname, sizeof(temp_ifname));
            if (args[i].vidParams.tagging)
                strcat(temp_ifname, "-t");

            strcat(ifname, temp_ifname);
            strcat(ifname, " ");
        }
        //Rag: netid and vlanid is same for all the args, so index zero is being used.
        sprintf(cmdBuff, "%s %s %d %d \"%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", up ? "addVlan" : "delVlan", args[0].hints.network->inst, args[0].vidParams.vid, ifname);
        MNET_DEBUG("configVlan_ESW, command is %s\n" COMMA cmdBuff)
        system(cmdBuff);

        syscfg_init();
        memset(iot_enabled, 0, sizeof(iot_enabled));
        int rc=syscfg_get(NULL, "lost_and_found_enable", iot_enabled, sizeof(iot_enabled));

        if((iot_enabled != NULL) || (rc != -1))
        {
	    printf("%s , IOT_LOG : iot_enabled is not NULL\n",__FUNCTION__);
              
            if(0==strncmp(iot_enabled,"true",4))
            {
               // Add vlan for IOT 
               memset(cmdBuff, 0, 128);
               sprintf(cmdBuff, "%s %s %d %d \"%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", "addIotVlan", 0, 106,"-t");
               printf("%s, IOT_LOG : Command for IOT %s\n",__FUNCTION__,cmdBuff);
               system(cmdBuff);
               printf("%s, IOT_LOG : Raise sysevent from multinet\n",__FUNCTION__);
               sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, "iot_status","bootup",0);
            }
            else
            {
               printf("%s, IOT_LOG : IOT is disabled\n",__FUNCTION__);

            }
        }
        else
        {
           printf("%s, IOT_LOG : iot_enabled returned NULL\n",__FUNCTION__);
        }
        
    }
    
}

int configVlan_WiFi(PSWFabHALArg args, int numArgs, BOOL up) {
    int i;
    
    char cmdBuff[128];
    char portID[80];
    memset(portID, 0, 80);

	if (100 == args[0].vidParams.vid) 
    {   
        for (i = 0; i < numArgs; ++i ) { 
            //#Args: netid, netvid, members...
            sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_wifi.sh", up ? "addVlan" : "delVlan", args[i].hints.network->inst, args[i].vidParams.vid, (char*)args[i].portID, args[i].vidParams.tagging ? "-t" : "");

            system(cmdBuff);
        }
    }   
    else //blran1 case
    {   
        for (i = 0; i < numArgs; ++i ) { 
            strcat(portID, (char*)args[i].portID);
            if (args[i].vidParams.tagging)
                strcat(portID, "-t");

            strcat(portID, " ");
        }
   
        //Rag: netid and vlanid is same for all the args, so index zero is being used. 
        sprintf(cmdBuff, "%s %s %d %d \"%s\"", SERVICE_MULTINET_DIR "/handle_wifi.sh", up ? "addVlan" : "delVlan", args[0].hints.network->inst, args[0].vidParams.vid, portID);
        MNET_DEBUG("configVlan_WiFi, portId is:%s command is %s\n" COMMA portID COMMA cmdBuff)
        system(cmdBuff);
    }    
}

int stringIDIntSw (void* portID, char* stringbuf, int bufSize) {
    PSwPortState portState = (PSwPortState) portID;
    int retval = snprintf(stringbuf, bufSize, "%s", portState->stringID);
    
    return retval ? retval + 1 : 0;
}

int eventIDSw (void* portID, char* stringbuf, int bufSize) {
    PSwPortState portState = (PSwPortState) portID;
    return (eventIDFromStringPortID((void*)portState->stringID, stringbuf, bufSize));
}

int stringIDExtSw (void* portID, char* stringbuf, int bufSize) {
    PSwPortState portState = (PSwPortState) portID;
    int retval = snprintf(stringbuf, bufSize, "%s", portState->stringID);
    
    return retval ? retval + 1 : 0;
}

int configVlan_ISW(PSWFabHALArg args, int numArgs, BOOL up) {
    int i;
    PSwPortState portState;
    char cmdBuff[128];
    char ifname[80];
    
    for (i = 0; i < numArgs; ++i ) {
        portState = (PSwPortState) args[i].portID;
        stringIDIntSw(portState, ifname, sizeof(ifname));
        
        //#Args: netid, netvid, members...
        sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", up ? "addVlan" : "delVlan", args[i].hints.network->inst, args[i].vidParams.vid, ifname, args[i].vidParams.tagging ? "-t" : "");
        
        system(cmdBuff);
    }
}
