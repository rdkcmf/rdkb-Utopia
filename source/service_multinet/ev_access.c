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
 
#include "service_multinet_lib.h"
#include "service_multinet_ev.h"
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>

token_t sysevent_token_interactive;
int sysevent_fd_interactive;

char* statusStrings[] = {
    "stopped",
    "ready",
    "pending",
    "partial"
};

int ev_init(void) {
    int retcode;
    MNET_DEBUG("Sysevent open\n")
    sysevent_fd_interactive = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "multinet_ev", &sysevent_token_interactive);
    return sysevent_fd_interactive ? 0 : -1;
}

static void include_netInst(PL2Net net) {
    char liveNetBuf[300];
    char outBuf[300];
    char* buf = NULL;
    char* save = NULL;
    char* tok = NULL;
    int match = 0;
    int len = 0;
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, "multinet-instances", liveNetBuf, sizeof(liveNetBuf));
    strcpy(outBuf, liveNetBuf);
    
    for(buf = liveNetBuf; (tok = strtok_r(buf, " ", &save)); buf = NULL) {
        if ( net->inst == atoi(tok) ) {
            match = 1;
            break;
        }
    }
    
    if (!match) {
        len = strlen(outBuf);
        snprintf(outBuf + len, sizeof(outBuf) - len, "%s%d", len ? " " : "", net->inst);
        sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, "multinet-instances", outBuf,0);
    }
    
}

static void exclude_netInst(PL2Net net) {
    char liveNetBuf[300];
    char outBuf[300];
    char* buf = NULL;
    char* save = NULL;
    char* tok = NULL;
    int match = 0;
    int len = 0;
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, "multinet-instances", liveNetBuf, sizeof(liveNetBuf));
    
    for(buf = liveNetBuf; (tok = strtok_r(buf, " ", &save)); buf = NULL) {
        if ( net->inst == atoi(tok) ) {
            match = 1;
        } else {
            len += snprintf(outBuf + len, sizeof(outBuf) - len, "%s%s", len ? " " : "", tok);
        }
    }
    
    if (match)
        sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, "multinet-instances", outBuf,0);
    
}

int ev_set_netStatus(PL2Net net, enum service_status status) {
    char eventName[80];
    
    
    
    //FIXME kludge for localready. current implementation always creates a bridge so this isn't as useful
    snprintf(eventName, sizeof(eventName), "multinet_%d-localready", net->inst);
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, eventName, "1",0);
    
    snprintf(eventName, sizeof(eventName), MNET_STATUS_FORMAT(net->inst));
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, eventName, statusStrings[status],0);
    
    if (status == STATUS_STOPPED) {
        exclude_netInst(net);
    } else {
        include_netInst(net);
    }
    
    return 0;
}

int ev_register_ifstatus(PL2Net net, PMember member, char* ifStatusEventName, char* ifStatusValueName, BOOL* readyFlag) {
    char eventName[80]; //FIXME These array sizes are arbitrary and disconnected
    char netIDString[10];
    char ifNameString[32];
    char ifTypeString[80];
    char asyncIDString[80];
    char valString[80];
    char* params[4];
    async_id_t asyncID;
    int i;
    
    MNET_DEBUG("Enter ev_register_ifstatus\n")
    snprintf(eventName, sizeof(eventName), ifStatusEventName);
    MNET_DEBUG("ev_register_ifstatus: eventName=%s\n" COMMA eventName)
    if(!isDaemon) {
        snprintf(netIDString, sizeof(netIDString), "%d", net->inst);
        MNET_DEBUG("ev_register_ifstatus: netIDString=%s\n" COMMA netIDString)
        snprintf(ifNameString, sizeof(ifNameString), "%s", member->interface->name);
        MNET_DEBUG("ev_register_ifstatus: ifNameString=%s\n" COMMA ifNameString)
        snprintf(ifTypeString, sizeof(ifTypeString), "%s", member->interface->type->name);
        MNET_DEBUG("ev_register_ifstatus: ifTypeString=%s\n" COMMA ifTypeString)
        params[0] = netIDString;
        params[1] = ifNameString;
        params[2] = ifTypeString;
        params[3] = member->bTagging ? "tag" : "untag";
        sysevent_setcallback(sysevent_fd_interactive, sysevent_token_interactive,
                             ACTION_FLAG_NONE,
                             eventName,
                             executableName,
                             4,
                             params,
                             &asyncID  );
        MNET_DEBUG("ev_register_ifstatus: returned from setcallback\n")
        snprintf(asyncIDString, sizeof(asyncIDString), MNET_IFSTATUS_ASYNCID_FORMAT(ifStatusEventName));
        MNET_DEBUG("ev_register_ifstatus: asyncIDString=%s\n" COMMA asyncIDString)
        snprintf(valString, sizeof(valString), MNET_IFSTATUS_ASYNCVAL_FORMAT(asyncID));
        MNET_DEBUG("ev_register_ifstatus: valString=%s\n" COMMA valString)
        
        sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, asyncIDString, valString, 0);
        MNET_DEBUG("ev_register_ifstatus: returned from set async id\n")
        
    } else {
        //TODO:Deferred - Daemon event handling
    }
    
    if (ifStatusValueName && readyFlag) {
        // Poll 
        //ep_get_ifstatus(ifStatusValueName, readyFlag);
        sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, 
                     eventName, 
                     valString,
                     sizeof(valString));
        MNET_DEBUG("ev_register_ifstatus: returned from get current status after registration\n")
        ev_string_to_status(valString, readyFlag);
    }
    
    return 0;
}

int ev_unregister_ifstatus(PL2Net net, char* ifStatusEventName) {
    char paramName[80];
    char asyncString[40];
    async_id_t asyncID;
    
    snprintf(paramName, sizeof(paramName), MNET_IFSTATUS_ASYNCID_FORMAT(ifStatusEventName));
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, 
        paramName, asyncString, sizeof(asyncString));
        
    sscanf(asyncString, MNET_IFSTATUS_ASYNCVAL_FORMAT(asyncID));
    
    sysevent_rmcallback(sysevent_fd_interactive, sysevent_token_interactive,
        asyncID);
}

int ev_firewall_restart(void) {

    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive,
                 "firewall-restart", "", 0);
}

int ev_string_to_status(char* stringStatus, SERVICE_STATUS* status) {
    int i;
    
    for (i = 0; i < 4; ++i ) {  //FIXME Should not use the network status strings. Coincidence that it works.
        if (!strcmp(statusStrings[i], stringStatus)) {
            MNET_DEBUG("ev_string_to_status: about to set status(%p) to %d\n" COMMA status COMMA i)
            *status = i;
            break;
        }
    }
    if (i == 4) {
        MNET_DEBUG("ev_string_to_status: about to set status(%p) to %d\n" COMMA status COMMA STATUS_STOPPED)
        *status = STATUS_STOPPED;
    }
}

int ev_set_name(PL2Net net) {
    char mnetkey[40];
    snprintf(mnetkey, sizeof(mnetkey), MNET_NAME_FORMAT(net->inst));
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, 
                 mnetkey, net->name, 0);
    return 0;
}
