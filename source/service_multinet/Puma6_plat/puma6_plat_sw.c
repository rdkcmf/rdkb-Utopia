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
#include "safec_lib_common.h"

#include <stdio.h>
#include <string.h>
#include "secure_wrapper.h"

extern int sysevent_fd_interactive;
extern token_t sysevent_token_interactive;
void delVlan(int net_id, int vlan_id, char *ports_add);
void addVlan(int, int, char*);

#define DEVICE_PROPS_FILE   "/etc/device.properties"
//unused function
#if 0  
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
#endif

int configVlan_ESW(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    PSwPortState portState;
    char ifname[80];
    char temp_ifname[80];
    errno_t  rc = -1;
    memset(ifname, 0, 80);
    memset(temp_ifname, 0, 80);

    for (i = 0; i < numArgs; ++i ) 
	{ 
    	portState = (PSwPortState) args[i].portID;
        stringIDExtSw(portState, temp_ifname, sizeof(temp_ifname));
        if (args[i].vidParams.tagging)
        {
            rc = strcat_s(temp_ifname, sizeof(temp_ifname), "-t");
            ERR_CHK(rc);
        }
        rc = strcat_s(ifname, sizeof(ifname), temp_ifname);
        ERR_CHK(rc);
        if (i < (numArgs - 1)) 
        {
            rc = strcat_s(ifname, sizeof(ifname), " ");
            ERR_CHK(rc);
        }
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
    MNET_DEBUG("configVlan_ESW, command is %s %s %d %d \"%s\"" COMMA SERVICE_MULTINET_DIR "/handle_sw.sh" COMMA up ? "addVlan" : "delVlan" COMMA
                        args[0].hints.network->inst COMMA args[0].vidParams.vid COMMA ifname)
    v_secure_system(SERVICE_MULTINET_DIR "/handle_sw.sh  %s %d %d '%s'",up ? "addVlan" : "delVlan",
                        args[0].hints.network->inst, args[0].vidParams.vid, ifname);
#endif
    return 0;
}

int configVlan_WiFi(PSWFabHALArg args, int numArgs, BOOL up) 
{
    int i;
    char portID[80];
    memset(portID, 0, 80);
    errno_t  rc = -1;

    for (i = 0; i < numArgs; ++i ) 
    { 
        rc = strcat_s(portID, sizeof(portID), (char*)args[i].portID);
        ERR_CHK(rc);
        if (args[i].vidParams.tagging){
            rc = strcat_s(portID, sizeof(portID), "-t");
            ERR_CHK(rc);
        }
        if (i < (numArgs - 1)){
            rc = strcat_s(portID, sizeof(portID), " ");
            ERR_CHK(rc);
        }
    }
  
#if defined(_COSA_INTEL_XB3_ARM_)
    if (up)
    {
		MNET_DEBUG("Adding ATOM ports:%s\n" COMMA portID)
		addVlan(args[0].hints.network->inst, args[0].vidParams.vid, portID);
    }
    else
    {
        MNET_DEBUG("Deleting ATOM ports:%s\n" COMMA portID)
		delVlan(args[0].hints.network->inst, args[0].vidParams.vid, portID);
    }
#else 
    //Rag: netid and vlanid is same for all the args, so index zero is being used. 
    MNET_DEBUG("configVlan_WiFi, portId is:%s command is %s %s %d %d \"%s\"" COMMA portID COMMA SERVICE_MULTINET_DIR "/handle_wifi.sh" COMMA up ? "addVlan" : "delVlan" COMMA
                        args[0].hints.network->inst COMMA args[0].vidParams.vid COMMA portID);
    v_secure_system(SERVICE_MULTINET_DIR "/handle_wifi.sh %s %d %d '%s'",up ? "addVlan" : "delVlan",
                        args[0].hints.network->inst, args[0].vidParams.vid, portID);
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
    char ifname[80];
    
    for (i = 0; i < numArgs; ++i ) 
	{
        portState = (PSwPortState) args[i].portID;
        stringIDIntSw(portState, ifname, sizeof(ifname));
        
#if defined(_COSA_INTEL_XB3_ARM_)
		if (up)
		{
			if (args[i].vidParams.tagging)
			{
				rc = strcat_s(ifname, sizeof(ifname),"-t");
				ERR_CHK(rc);   
			}
           	MNET_DEBUG("Adding Internal switch ports:%s\n" COMMA ifname)
			addVlan(args[i].hints.network->inst, args[i].vidParams.vid, ifname);
		}
		else
		{
           	MNET_DEBUG("Delete Internal switch ports:%s\n" COMMA ifname)
			delVlan(args[i].hints.network->inst, args[i].vidParams.vid, ifname);
		}
#else
		
    	v_secure_system(SERVICE_MULTINET_DIR "/handle_sw.sh %s %d %d '%s%s'",up ? "addVlan" : "delVlan",
                                args[i].hints.network->inst, args[i].vidParams.vid, ifname, args[i].vidParams.tagging ? "-t" : "");
#endif
	}
	return 0;
}
