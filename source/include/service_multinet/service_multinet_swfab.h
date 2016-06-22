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
#ifndef MNET_PLAT_H
#define MNET_PLAT_H

#include "service_multinet_base.h"
#include "service_multinet_handler.h"
//#include "service_multinet_ifplugin.h"
#include "service_multinet_util.h"


#define TRUE 1
#define FALSE 0
#define HAL_MAX_PORTS 32
#define MAX_ADD_PORTS HAL_MAX_PORTS*2
#define MAX_ENTITIES 32
#define MAX_PATHS 128

typedef struct vidParams { 
    int vid;
    int tagging;
    int pvid;
} VLANParams, *PVLANParams;
    
typedef struct portConfig {
    struct platport* platPort;
    VLANParams vidParams;
} PortConfig, *PPortConfig; 

typedef struct pcControl {
    PortConfig config;
    int handled;
} PortConfigControl, *PPortConfigControl;

typedef struct halhints {
    PL2Net network;
    PNetInterface iface;
} HalHints, *PHalHints;

typedef struct halArg {
    void* portID;
    VLANParams vidParams;
    BOOL ready;
     HalHints hints;
} SWFabHALArg, *PSWFabHALArg; 

typedef struct argMemberMap {
    PSWFabHALArg args;
    PMember* members;
    PL2Net network;
} HALArgMemberMap, *PHALArgMemberMap;

typedef int (*IFHandlerFunc)(PSWFabHALArg args, int numArgs, BOOL up);
typedef int (*IsEqualFunc)(void* portIDa, void* portIDb);
typedef int (*IDToStringFunc)(void* portID, char* stringbuf, int bufsize);
//typedef int (*EventStringFunc)(void* portID, char* stringbuf, int bufsize);

typedef struct swfabhal {
    // Used to match. Should be unique for each instance, or initIF and configVlan should be
    // identical across instances.
    int id;
    
    //Should fill the 'ready' field of each member
    IFHandlerFunc initIF;
    
    IFHandlerFunc configVlan;
    
    //Check if two ports are equal. Do not assume all arguements are of the local HALs portID type.
    IsEqualFunc isEqual;
    
    /**Return the number of characters written including null terminator.
     */
    IDToStringFunc stringID;
    IDToStringFunc eventString;
}SWFabHAL, *PSWFabHAL;

typedef struct platport {
    void* portID;
    int entity;
    PSWFabHAL hal;
    /**Dynamic means the availability of this interface may change
     * during the course of runtime, and should be taken into consideration
     * by the bridging framework. A false value here will save some overhead.
     */
    int isDynamic;
    
    /** Shared means this port may be a trunking port for multiple other platform ports in 
     * some configuration. If it can be a shared pathway, mark this true. 
     * 
     * The framework will force this port to tagging mode if this port is a dependency
     * and this flag is set. Otherwise the port will be assigned the tagging mode specified 
     * on the member port.
     */
    //int isShared;
} PlatformPort, *PPlatformPort;





// typedef struct ifTypeHandler {
//     
// } IFTypeHandler, *PIFTypeHandler;

int swfab_addVlan(PL2Net net, PMemberControl members);
int swfab_removeVlan(PL2Net net, PMemberControl members);
int swfab_create(PL2Net net, PMemberControl members);
int swfab_domap(PL2Net net, PMemberControl members);

//-------------
//int isPortEqual(PPlatformPort portA, PPlatformPort portB);
void printPlatport(PPlatformPort port);


#endif
