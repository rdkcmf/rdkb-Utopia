
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
#ifndef MNET_BASE_H
#define MNET_BASE_H

#if 0
#undef BOOL
# define BOOL unsigned char
#else
typedef  unsigned char BOOL;
#endif /* 0 */


#ifndef SERVICE_D_BASE_DIR
#define SERVICE_D_BASE_DIR "/etc/utopia/service.d"
#endif

#define SERVICE_MULTINET_DIR SERVICE_D_BASE_DIR "/service_multinet"

#include <stdio.h>
#include <stdlib.h>

extern FILE *mnetfp;
void multinet_log( char* fmt, ...);
#define MULTINET_DEBUG 1


#ifdef MULTINET_DEBUG
//Usage: MNET_DEBUG("format" COMMA ARG COMMA ARG)
#define COMMA ,
#define MNET_DEBUG(x) multinet_log(x);
#define MNET_DBG_CMD(x) x;
#else 
#define MNET_DEBUG(x)
#define MNET_DBG_CMD(x)
#endif

typedef enum service_status {  //TODO: move to sysevent common
STATUS_STOPPED = 0,
STATUS_STARTED,
STATUS_PENDING,
STATUS_PARTIAL
} SERVICE_STATUS;

typedef enum if_status { // TODO: move to sysevent common
IF_STATUS_DOWN =0,
IF_STATUS_UP
} IF_STATUS;

typedef struct l2net {
    int bEnabled;
    int vid;
    int inst;
    char name[16];
} L2Net, *PL2Net;

typedef struct iftype {
    char name[80];
    void* nvkey;
} IFType, *PIFType;

typedef struct netif {
    PIFType type;
    char name[16];
    char eventName[64];
    BOOL dynamic;
    void* map;
} NetInterface, *PNetInterface;

typedef struct member {
    PNetInterface interface;
    int pvid;
    BOOL bTagging; 
    SERVICE_STATUS bReady;
} Member, *PMember;

#endif
