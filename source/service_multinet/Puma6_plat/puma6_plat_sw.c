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

#define DEVICE_PROPS_FILE   "/etc/device.properties"
  
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

int configVlan_ESW(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    PSwPortState portState;
    char cmdBuff[128];
    char ifname[80];
    char temp_ifname[80];
    memset(ifname, 0, 80);
    memset(temp_ifname, 0, 80);

    for (i = 0; i < numArgs; ++i ) 
	{ 
    	portState = (PSwPortState) args[i].portID;
        stringIDExtSw(portState, temp_ifname, sizeof(temp_ifname));
        if (args[i].vidParams.tagging)
        	strcat(temp_ifname, "-t");

        strcat(ifname, temp_ifname);
		if (i < (numArgs - 1)) 
            strcat(ifname, " ");
    }
#if defined(_COSA_INTEL_XB3_ARM_)
	if (up)
	{
		MNET_DEBUG("Adding External ports:%s\n" COMMA ifname)
		addVlan(args[0].hints.network->inst, args[0].vidParams.vid, ifname);
	}
	else
	{
		MNET_DEBUG("Deleting External ports:%s\n" COMMA ifname)
        delVlan(args[0].hints.network->inst, args[0].vidParams.vid, ifname);
	}
#else
    //Rag: netid and vlanid is same for all the args, so index zero is being used.
    sprintf(cmdBuff, "%s %s %d %d \"%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", up ? "addVlan" : "delVlan", 
			args[0].hints.network->inst, args[0].vidParams.vid, ifname);

    MNET_DEBUG("configVlan_ESW, command is %s\n" COMMA cmdBuff)
    system(cmdBuff);
#endif
    return 0;
}

int configVlan_WiFi(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    char cmdBuff[128];
    char portID[80];
    memset(portID, 0, 80);

    for (i = 0; i < numArgs; ++i ) 
	{ 
    	strcat(portID, (char*)args[i].portID);
        if (args[i].vidParams.tagging)
        	strcat(portID, "-t");

		if (i < (numArgs - 1))
            strcat(portID, " ");
    }
  
#if defined(_COSA_INTEL_XB3_ARM_)
    if (up)
    {
		MNET_DEBUG("Adding ATOM ports:%s\n" COMMA portID)
		addVlan(args[0].hints.network->inst, args[0].vidParams.vid, portID);
    }
    else
    {
        MNET_DEBUG("Deleting ATOM ports\n" COMMA portID)
		delVlan(args[0].hints.network->inst, args[0].vidParams.vid, portID);
    }
#else 
    //Rag: netid and vlanid is same for all the args, so index zero is being used. 
    sprintf(cmdBuff, "%s %s %d %d \"%s\"", SERVICE_MULTINET_DIR "/handle_wifi.sh", up ? "addVlan" : "delVlan", 
			args[0].hints.network->inst, args[0].vidParams.vid, portID);

    MNET_DEBUG("configVlan_WiFi, portId is:%s command is %s\n" COMMA portID COMMA cmdBuff)
    system(cmdBuff);
#endif
    return 0;
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

// This function is called for configuring MOCA ports 
// For brlan0 it is untagged port sw_5
// For brlan1 it is tagged port sw_5-t
// For brlan2 and brlan3 this function is not called 
int configVlan_ISW(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    PSwPortState portState;
    char cmdBuff[128];
    char ifname[80];
    
    for (i = 0; i < numArgs; ++i ) 
	{
        portState = (PSwPortState) args[i].portID;
        stringIDIntSw(portState, ifname, sizeof(ifname));
        
#if defined(_COSA_INTEL_XB3_ARM_)
		if (up)
		{
			if (args[i].vidParams.tagging)
            	strcat(ifname, "-t");

           	MNET_DEBUG("Adding Internal switch ports:%s\n" COMMA ifname)
			addVlan(args[i].hints.network->inst, args[i].vidParams.vid, ifname);
		}
		else
		{
           	MNET_DEBUG("Delete Internal switch ports:%s\n" COMMA ifname)
			delVlan(args[i].hints.network->inst, args[i].vidParams.vid, ifname);
		}
#else
		sprintf(cmdBuff, "%s %s %d %d \"%s%s\"", SERVICE_MULTINET_DIR "/handle_sw.sh", up ? "addVlan" : "delVlan", 
				args[i].hints.network->inst, args[i].vidParams.vid, ifname, args[i].vidParams.tagging ? "-t" : "");
    	system(cmdBuff);
#endif
	}
    return 0;
}
