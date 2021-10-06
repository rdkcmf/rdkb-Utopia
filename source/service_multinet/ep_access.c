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

#include "service_multinet_ep.h"
#include "service_multinet_ev.h"
#if defined (INTEL_PUMA7)
#include "service_multinet_lib.h"
#if defined (MULTILAN_FEATURE)
#include "puma7_plat_sw.h"
#endif
#endif
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "safec_lib_common.h"

int ep_get_allMembers(PL2Net net, PMember live_members, int numMembers){
    char ifnamebuf[32];
    char iflistbuf[512];
    char netmemberskey[32];
    char* ifToken, *dash;
    int curNumMembers = 0;
    char *saveptr1;
    errno_t  rc = -1;
 
    snprintf(netmemberskey, sizeof(netmemberskey), MNET_EP_ALLMEMBERS_KEY_FORMAT(net->inst));
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, netmemberskey, iflistbuf, sizeof(iflistbuf));
    MNET_DEBUG("%s: multinet_%d-allMembers value [%s]\n" COMMA __FUNCTION__ COMMA net->inst COMMA iflistbuf);
 
    //Token holds a complete single interface string
    for (ifToken = strtok_r(iflistbuf, " ", &saveptr1); ifToken != NULL; ifToken = strtok_r(NULL, " ", &saveptr1))
    {
        MNET_DEBUG("%s: Token [%s]\n" COMMA __FUNCTION__ COMMA ifToken);
 
        sscanf(ifToken, MNET_EP_MEMBER_FORMAT(ifnamebuf, live_members[curNumMembers].interface->type->name, (unsigned char *)&live_members[curNumMembers].bReady));
 
#ifdef MULTILAN_FEATURE
#if defined (INTEL_PUMA7)
        //If the interface is ready, verify that it's really connected to this bridge
        MNET_DEBUG("%s: Check whether [%s] (ready %d) is connected\n" COMMA __FUNCTION__ COMMA ifnamebuf COMMA live_members[curNumMembers].bReady);
        if (live_members[curNumMembers].bReady && ep_check_if_really_bridged(net, ifnamebuf)){
            MNET_DEBUG("Port %s NOT really connected to bridge %s\n" COMMA ifnamebuf COMMA net->name);
            continue;
        }
        else {
            MNET_DEBUG("Port %s is already connected to bridge %s\n" COMMA ifnamebuf COMMA net->name);
        }
#endif
#endif

        if ((dash = strchr(ifnamebuf, '-'))){
            *dash = '\0';
            live_members[curNumMembers].bTagging = 1;
            MNET_DEBUG("Port %s is tagged\n" COMMA ifnamebuf);
        } else {
            live_members[curNumMembers].bTagging = 0;
        }
        live_members[curNumMembers].interface->map = NULL;
        rc = strcpy_s(live_members[curNumMembers].interface->name, sizeof(live_members[curNumMembers].interface->name), ifnamebuf);
        ERR_CHK(rc);
        curNumMembers++;
    }
    MNET_DEBUG("%s: Done tokenizing multinet_%d-allMembers\n" COMMA __FUNCTION__ COMMA net->inst);

    return curNumMembers; // TODO, check nv_* to make sure returning number
    
}

int ep_set_allMembers(PL2Net net, PMember members, int numMembers) {
    int i;
    char ifnamebuf[32];
    char iflistbuf[512];
    char netmemberskey[32];
    int offset = 0;
    
    iflistbuf[0] = '\0';
    
    for (i = 0; i < numMembers; ++i) {
#ifdef MULTILAN_FEATURE
        if (members[i].bReady == STATUS_STARTED) {
            MNET_DEBUG("ep_set_allMembers, Writing Member %d," COMMA i);
                MNET_DEBUG(" %s\n" COMMA members[i].interface->name);
                snprintf(ifnamebuf, sizeof(ifnamebuf), "%s%s", members[i].interface->name, members[i].bTagging ? "-t" : "");
                offset += snprintf(iflistbuf + offset, 
                                   sizeof(iflistbuf) - offset, " "
                                   MNET_EP_MEMBER_SET_FORMAT(ifnamebuf, members[i].interface->type->name, members[i].bReady));
        }
        else
        {
            MNET_DEBUG("ep_set_allMembers, Skipping Member %d," COMMA i);
        }
#else
        MNET_DEBUG("ep_set_allMembers, Writing Member %d," COMMA i)
        MNET_DEBUG(" %s\n" COMMA members[i].interface->name)
        snprintf(ifnamebuf, sizeof(ifnamebuf), "%s%s", members[i].interface->name, members[i].bTagging ? "-t" : "");
        offset += snprintf(iflistbuf + offset, 
                           sizeof(iflistbuf) - offset, " "
                           MNET_EP_MEMBER_SET_FORMAT(ifnamebuf, members[i].interface->type->name, members[i].bReady));
#endif
    }
    
    snprintf(netmemberskey, sizeof(netmemberskey), MNET_EP_ALLMEMBERS_KEY_FORMAT(net->inst));
    
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, netmemberskey, offset ? iflistbuf : NULL, 0);
    return 0;
}

int ep_add_active_net(PL2Net net) { // TODO deferred
    return 0;
}
int ep_rem_active_net(PL2Net net) { // TODO deferred
    return 0;
}

int ep_netIsStarted(int netInst) {
    char eventName[80];
    char statusBuf[32];
    SERVICE_STATUS status;
    int retcode;
    
    snprintf(eventName, sizeof(eventName), MNET_STATUS_FORMAT(netInst));
    
    retcode = sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,eventName, statusBuf, sizeof(statusBuf));
    MNET_DEBUG("ep_netIsStarted sysevent_get retcode %d\n" COMMA retcode)
    ev_string_to_status(statusBuf, &status);
    
    return status;
}

int ep_get_bridge(int l2netinst, PL2Net net) {
    char keybuf[64];
    char valbuf[64];
    net->inst = l2netinst;
    errno_t  rc = -1;
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_VID_FORMAT(l2netinst));
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,keybuf, valbuf, sizeof(valbuf));
    
    if(strlen(valbuf) > 0) {
        net->vid = atoi(valbuf);
    }
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_NAME_FORMAT(l2netinst));
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,keybuf, valbuf, sizeof(valbuf));
    
    if(strlen(valbuf) > 0) {
        rc = strcpy_s(net->name, sizeof(net->name), valbuf);
        ERR_CHK(rc);
    }
    return 0;
}

int ep_get_bridge_mode(void) {
    char keybuf[64];
    char valbuf[64];
    int bridge_mode = 0;

    snprintf(keybuf, sizeof(keybuf), MNET_EP_BRIDGE_MODE_KEY);
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,keybuf, valbuf, sizeof(valbuf));

    if(strlen(valbuf) > 0) {
        bridge_mode = atoi(valbuf);
    }

    return bridge_mode;
}

int ep_set_bridge(PL2Net net) {
    char keybuf[64];
    char valbuf[64];
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_VID_FORMAT(net->inst));
    snprintf(valbuf, sizeof(valbuf), "%d", net->vid);
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, 0);
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_NAME_FORMAT(net->inst));
    snprintf(valbuf, sizeof(valbuf), "%s", net->name);
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, 0);
    return 0;
}

int ep_set_rawString(char* key, char* value) {
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, key, value, 0);
    return 0;
}
int ep_get_rawString(char* key, char* value, int valueSize) {
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,key, value, valueSize);
    return 0;
}

int ep_clear(PL2Net net) {
    char keybuf[64];
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_VID_FORMAT(net->inst));
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, NULL, 0);
    
    snprintf(keybuf, sizeof(keybuf),  MNET_EP_BRIDGE_NAME_FORMAT(net->inst));
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, NULL, 0);
    
    ep_set_allMembers(net, NULL, 0);
    return 0;
}

// int ep_add_members(PL2Net net, PMember members, int numMembers){
//      char memberBuf[512];
//      char keyBuf[80];
//      char* tail;
//      int i;
//      
//      snprintf(keyBuf, sizeof(keyBuf), MNET_EP_ALLMEMBERS_KEY_FORMAT(net->inst));
//      
//      sysevent_get(sysevent_fd_interactive, sysevent_token_interactive,keyBuf, memberBuf, sizeof(memberBuf));
//      
//      tail = strchr(memberBuf, '\0');
//      
//      for( i = 0; i < numMembers; ++i ) {
//          tail += sprintf(tail, " " MNET_EP_MEMBER_FORMAT(members[i].interface->name, members[i].interface->type->name, members[i].bTagging));
//      }
//      
//      sysevent_set(sysevent_fd_interactive, sysevent_token_interactive,keyBuf,memberBuf, 0);
//  } 
//  
//  int ep_rem_members(PL2Net net, PMember members, int numMembers) {
//      Member live_members[16];
//      
//      ep_get_allMembers(net, live_members, sizeof(live_members)/sizeof(*live_members));
//      
//      
//  }
