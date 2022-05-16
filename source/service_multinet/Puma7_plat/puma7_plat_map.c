/****************************************************************************
  Copyright 2017 Intel Corporation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
******************************************************************************/

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
#include "service_multinet_handler.h"
#include "service_multinet_plat.h"
#include "service_multinet_swfab_plat.h"
#include "service_multinet_util.h"

#include <stdio.h>
#include <string.h>

#include "puma7_plat_map.h"
#include "puma7_plat_sw.h"
#include <stdlib.h>

#include "service_multinet_swfab_LinIF.h"
#include "service_multinet_swfab_gre.h"

#define MAX_CMD_SIZE 256

int greIfInit(PSWFabHALArg args, int numArgs, BOOL up) {
    char cmdbuf[MAX_CMD_SIZE];
    char *ifname = (char*)args[0].portID;

    /* Handle hotspot bridges differently than generic GRE */
    if (!strcmp("gretap0", ifname))
    {
        snprintf(cmdbuf, sizeof(cmdbuf), "%s create %d \"%s\"", SERVICE_MULTINET_DIR "/handle_gre.sh", args[0].hints.network->inst, (char*)args[0].portID);
        MNET_DEBUG("greIfInit running: %s" COMMA cmdbuf);
        return system(cmdbuf);
    }
    else
#ifdef INTEL_GRE_HOTSPOT
    {
    /* Set the firewall for gre and bring up the Hotspot process if it is a hotspot bridge */
        snprintf(cmdbuf, sizeof(cmdbuf), "sysevent set igre-bringup-gre-hs %d", args[0].hints.network->inst);
        MNET_DEBUG("greIfInit running: %s" COMMA cmdbuf);
        system(cmdbuf);
    }
#endif
        return 0;
}

int ifhandlerNoop(PSWFabHALArg args, int numArgs, BOOL up) {
    return 0;
}

int ifInitAllReady(PSWFabHALArg args, int numArgs, BOOL up) {
    int i;
    for (i = 0; i < numArgs; ++i) {
        args[i].ready = 1;
    }
    return 0;
}

int isEqualStringCompare(void* portIDa, void* portIDb) {
    return (!strcmp((char*)portIDa, (char*) portIDb));
}

int isEqualIntCompare(void* portIDa, void* portIDb) {
    return *((int*)portIDa) == *((int*)portIDb) ? 1 : 0;
}

int stringIDPortIDDirect (void* portID, char* stringbuf, int bufSize) {
    int retval = snprintf(stringbuf, bufSize, "%s", (char*) portID);
    return retval ? retval + 1 : 0; 
}

int eventIDFromStringPortID (void* portID, char* stringbuf, int bufsize) {
    int retval = snprintf(stringbuf, bufsize, "if_%s-status", (char*)portID);
    return retval ? retval + 1 : 0;
}

static SWFabHAL halList[] = {
    {HAL_NOOP, ifhandlerNoop, ifhandlerNoop, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
    {HAL_WIFI, ifhandlerNoop, configVlan_WiFi, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
    {HAL_ESW, ifhandlerNoop, configVlan_ESW, isEqualIntCompare, stringIDExtSw, eventIDSw},
    {HAL_GRE, greIfInit, configVlan_GRE, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
    {HAL_LINUX, ifhandlerNoop/*linuxIfInit*/, configVlan_puma7, isEqualStringCompare, stringIDPortIDDirect, eventIDFromStringPortID},
};

static PlatformPort wifiPortList_1[] = {
    {(void*)PUMA7_WIFI_PREFIX_1 "0", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "1", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "2", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "3", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "4", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "5", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "6", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "7", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "8", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "9", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "10", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "11", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "12", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "13", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "14", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_1 "15", ENTITY_AP, halList + HAL_WIFI, 0}
};

static PlatformPort wifiPortList_2[] = {
    {(void*)PUMA7_WIFI_PREFIX_2 "0", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "1", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "2", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "3", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "4", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "5", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "6", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "7", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "8", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "9", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "10", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "11", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "12", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "13", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "14", ENTITY_AP, halList + HAL_WIFI, 0},
    {(void*)PUMA7_WIFI_PREFIX_2 "15", ENTITY_AP, halList + HAL_WIFI, 0}
};

SwPortState extSwIDs[] = {
    {0, "sw_1", 0, {0},0,0},
    {1, "sw_2", 0, {0},0,0},
    {2, "sw_3", 0, {0},0,0},
    {3, "sw_4", 0, {0},0,0},
    {0, "sw_5", 0, {0},0,0},
    {1, "sw_6", 0, {0},0,0},
    {2, "sw_7", 0, {0},0,0},
    {3, "sw_8", 0, {0},0,0}
};

static PlatformPort accessSwPortList[] = {
    {(void*)(extSwIDs + 0), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 1
    {(void*)(extSwIDs + 1), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 2
    {(void*)(extSwIDs + 2), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 3
    {(void*)(extSwIDs + 3), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 4
    {(void*)(extSwIDs + 4), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 5
    {(void*)(extSwIDs + 5), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 6
    {(void*)(extSwIDs + 6), ENTITY_ESW, halList + HAL_ESW, 0}, //SW port 7
    {(void*)(extSwIDs + 7), ENTITY_ESW, halList + HAL_ESW, 0}  //SW port 8
};

static PlatformPort grePortList[] = {
    {(void*)"gretap0", ENTITY_NP, halList + HAL_GRE, 1},
    {(void*)"gretap1", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap2", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap3", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap4", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap5", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap6", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap7", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap8", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap9", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap10", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap11", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap12", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap13", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap14", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap15", ENTITY_NP, halList + HAL_GRE, 0},
    {(void*)"gretap16", ENTITY_NP, halList + HAL_GRE, 0}
};

/** Platform-specific network ports */
static PlatformPort nsgmii0Port = {(void*)"nsgmii0", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort nsgmii1Port = {(void*)"nsgmii1", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort nrgmii2Port = {(void*)"nrgmii2", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort nrgmii3Port = {(void*)"nrgmii3", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort brWanPort = {(void*)"lbr0", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort mocaPort = {(void*)"nmoca0", ENTITY_NP, halList + HAL_LINUX, 0};
static PlatformPort lanMgmt = {(void*)"llan0", ENTITY_NP, halList + HAL_LINUX, 0};

//Public platform specific implementation

/** No implicit ports on Puma 7 */
int plat_addImplicitMembers(PL2Net nv_net, PMember memberBuf) {
    return 0;
}

/** This is a platform specific mapping between DM ports and physical ports. 
 * Integrators may utilize any approach to expedite mapping, such as port naming conventions.
 */
int mapToPlat(PNetInterface iface) {
    int portIndex = 0;

    iface->map = NULL;

    MNET_DEBUG("%s: Looking up interface %s\n" COMMA __FUNCTION__ COMMA iface->name);

    if (!strcmp("WiFi", iface->type->name)) {
        if(sscanf(iface->name, PUMA7_WIFI_PREFIX_1 "%d", &portIndex) > 0) {
            iface->map = wifiPortList_1 + portIndex;
        }
        if(sscanf(iface->name, PUMA7_WIFI_PREFIX_2 "%d", &portIndex) > 0) {
            iface->map = wifiPortList_2 + portIndex;
        }
    } else if (!strcmp("SW", iface->type->name)) {
        if(   (sscanf(iface->name, "sw_%d", &portIndex) > 0)
           || (sscanf(iface->name, "eth%d", &portIndex) > 0)
          ) {
            iface->map = accessSwPortList + (portIndex - 1);
        }
    } else if (!strcmp("Moca", iface->type->name)) {
        iface->map = &mocaPort;
    } else if (!strcmp("Gre", iface->type->name)) {
        if(sscanf(iface->name, "gretap%d", &portIndex) > 0) {
            iface->map = grePortList + portIndex;
        }
    } else if (!strcmp("Eth", iface->type->name)) {
        if (strstr(iface->name, "lbr0")) {
            iface->map = &brWanPort;
        }
        else if (strstr(iface->name, "nsgmii0")) {
            iface->map = &nsgmii0Port;
        }
        else if (strstr(iface->name, "nsgmii1")) {
            iface->map = &nsgmii1Port;
        }
        else if (strstr(iface->name, "nrgmii2")) {
            iface->map = &nrgmii2Port;
        }
        else if (strstr(iface->name, "nrgmii3")) {
            iface->map = &nrgmii3Port;
        }
        else if (strstr(iface->name, "llan0")) {
            iface->map = &lanMgmt;
        }
    }
    
    if (iface->map == NULL) {
        MNET_DEBUG("%s: Interface %s, no mapping found!\n" COMMA __FUNCTION__ COMMA iface->name);
    }

    return 0;
}

PPlatformPort plat_mapFromString(char* portIdString) {
    int portIndex;

    MNET_DEBUG("%s: Looking up interface %s\n" COMMA __FUNCTION__ COMMA portIdString);

    if (strstr(portIdString, PUMA7_WIFI_PREFIX_1 "")) {
        sscanf(portIdString, PUMA7_WIFI_PREFIX_1 "%d", &portIndex);
        return wifiPortList_1 + portIndex;
    } else if (strstr(portIdString, PUMA7_WIFI_PREFIX_2 "")) {
        sscanf(portIdString, PUMA7_WIFI_PREFIX_2 "%d", &portIndex);
        return wifiPortList_1 + portIndex;
    } else if (strstr(portIdString, "sw_")) {
        sscanf(portIdString, "sw_%d", &portIndex);
        return (accessSwPortList + (portIndex - 1));
    } else if (strstr(portIdString, "gretap")) {
        sscanf(portIdString, "gretap%d", &portIndex);
        return  (grePortList + portIndex);
    } else if (strstr(portIdString, "lbr0")) {
        return &brWanPort;
    } else if (strstr(portIdString, "nmoca0")) {
        return &mocaPort;
    } else if (strstr(portIdString, "nsgmii0")) {
        return &nsgmii0Port;
    } else if (strstr(portIdString, "nsgmii1")) {
        return &nsgmii1Port;
    } else if (strstr(portIdString, "nrgmii2")) {
        return &nrgmii2Port;
    } else if (strstr(portIdString, "nrgmii3")) {
        return &nrgmii3Port;
    } else if (strstr(portIdString, "llan0")) {
        return &lanMgmt;
    } 

    MNET_DEBUG("%s: Interface %s, no mapping found!\n" COMMA __FUNCTION__ COMMA portIdString);

    return NULL;
}

PEntityPathDeps* depArray  = 
(PEntityPathDeps[3]) {
        (EntityPathDeps[3]){{ {0}, 0, NULL }},
        (EntityPathDeps[2]){{ {0}, 0, NULL }},
        (EntityPathDeps[1]){{ {0}, 0, NULL }}
    };


PEntityPathDeps getPathDeps(int entityLow, int entityHigh) {
    return &depArray[entityLow-1][entityHigh-1-entityLow];
}

