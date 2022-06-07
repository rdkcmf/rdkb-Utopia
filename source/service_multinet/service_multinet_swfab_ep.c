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
#include "service_multinet_swfab_ep.h"
#include "service_multinet_ep.h"
#include "service_multinet_ev.h"
#include "sysevent/sysevent.h"
#include <stdio.h>
#include <string.h>

#define MULTINET_DEBUG 1

int ep_set_entity_vid_portMembers(int vid, int entity, char* memberPortNames[], int numPorts) {
    char keybuf[80];
    char valbuf[512];
    int i;
    int offset = 0;
    
    valbuf[0]='\0';
    
    snprintf(keybuf, sizeof(keybuf), SWFAB_ENTITY_PORTMEMBER_KEY_FORMAT(vid,entity));
    
    for( i = 0; i < numPorts; ++i) {
        MNET_DEBUG("Writing entity %d member %s\n" COMMA entity COMMA memberPortNames[i])
        offset+= snprintf(valbuf + offset, sizeof(valbuf) - offset, " %s", memberPortNames[i]);
    }
    
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, offset ? valbuf : NULL, 0);
    return 0;
}

int ep_set_entity_vidMembers(int vid, int entities[], int numEntities) {
    char keybuf[80];
    char valbuf[512];
    int i;
    int offset = 0;
    
    snprintf(keybuf, sizeof(keybuf), SWFAB_VID_ENTITYMEMBER_KEY_FORMAT(vid));
    
    for( i = 0; i < numEntities; ++i) {
        offset+= snprintf(valbuf + offset, sizeof(valbuf) - offset, " %d", entities[i]);
    }
    
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, offset ? valbuf : NULL, 0);
    return 0;
}

int ep_set_trunkPort_vid_paths(int vid, char* portName, PEntityPath paths, int numPaths) {
    char keybuf[80];
    char valbuf[512];
    int i;
    int offset = 0;
    valbuf[0]='\0';
    snprintf(keybuf, sizeof(keybuf), SWFAB_PORT_PATHREF_KEY_FORMAT(vid,portName));
    
    for( i = 0; i < numPaths; ++i) {
        offset+= snprintf(valbuf + offset, sizeof(valbuf) - offset, " %d,%d", paths[i].A, paths[i].B);
    }
    
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, offset ? valbuf : NULL, 0);
    return 0;
}
int ep_set_trunkPort_vidMembers(int vid, char* portNames[], int numPorts){
    char keybuf[80];
    char valbuf[512];
    int i;
    int offset = 0;
    
    snprintf(keybuf, sizeof(keybuf), SWFAB_VID_TRUNKMEMBER_KEY_FORMAT(vid));
    
    for( i = 0; i < numPorts; ++i) {
        offset+= snprintf(valbuf + offset, sizeof(valbuf) - offset, " %s", portNames[i]);
    }
    
    sysevent_set(sysevent_fd_interactive, sysevent_token_interactive, keybuf, offset ? valbuf : NULL, 0);
    return 0;
}

//---gets

//FIXME buffer overrun checks

int ep_get_entity_vid_portMembers(int vid, int entity, char* memberPortNames[], int* numPorts, char buf[], int bufSize ) {
    char keybuf[80];
    char valbuf[512];
    char *token;
    int offset = 0;

    memset (keybuf ,0 ,sizeof(keybuf));
    memset (valbuf ,0 ,sizeof(valbuf));
    token = NULL;

    snprintf(keybuf, sizeof(keybuf), SWFAB_ENTITY_PORTMEMBER_KEY_FORMAT(vid,entity));
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, sizeof(valbuf));
	MNET_DEBUG("ep_get_entity_vid_portMembers, vid %d buf %s\n" COMMA vid COMMA valbuf)
	
    token = strtok(valbuf, " ");
    
    *numPorts = 0;
    
    while (token) {
			
	if((long int)numPorts == MAX_ADD_PORTS )
	{
		MNET_DEBUG("ep_get_entity_vid_portMembers  numPorts is exceeded [%ld]\n"COMMA (long int)numPorts)
		break;
	}
        memberPortNames[*numPorts] = buf + offset;
        offset += sprintf(memberPortNames[*numPorts], "%s", token) + 1;
        (*numPorts)++;
        
        token = strtok(NULL, " ");
    }
    return 0;
}
int ep_get_entity_vidMembers(int vid, int entities[], int* numEntities){
    char keybuf[80];
    char valbuf[512];
    char *token;
    
    snprintf(keybuf, sizeof(keybuf), SWFAB_VID_ENTITYMEMBER_KEY_FORMAT(vid));
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, sizeof(valbuf));
	MNET_DEBUG("ep_get_entity_vidMembers, vid %d buf %s\n" COMMA vid COMMA valbuf)

    token = strtok(valbuf, " ");
    
    *numEntities = 0;
    
    while (token) {
        sscanf(token, "%d", entities + *numEntities);
        
        (*numEntities)++;
        
        token = strtok(NULL, " ");
    }
    return 0;
}

int ep_get_trunkPort_vid_paths(int vid, char* portName, PEntityPath paths, int* numPaths){
    char keybuf[80];
    char valbuf[512];
    char *token;
    
    snprintf(keybuf, sizeof(keybuf), SWFAB_PORT_PATHREF_KEY_FORMAT(vid,portName));
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, sizeof(valbuf));
	MNET_DEBUG("ep_get_trunkPort_vid_paths, vid %d buf %s\n" COMMA vid COMMA valbuf)

    token = strtok(valbuf, " ");
    
    *numPaths = 0;
    
    while (token) {
        sscanf(token, "%d,%d", &paths[*numPaths].A, &paths[*numPaths].B);
        (*numPaths)++;
        token = strtok(NULL, " ");
    }
    return 0;
}
int ep_get_trunkPort_vidMembers(int vid, char* portNames[], int* numPorts, char buf[], int bufSize){
    char keybuf[80];
    char valbuf[512];
    char *token;
    int offset = 0;

    memset (keybuf ,0 ,sizeof(keybuf));
    memset (valbuf ,0 ,sizeof(valbuf));	
    token = NULL;

    snprintf(keybuf, sizeof(keybuf), SWFAB_VID_TRUNKMEMBER_KEY_FORMAT(vid));
    
    sysevent_get(sysevent_fd_interactive, sysevent_token_interactive, keybuf, valbuf, sizeof(valbuf));
	MNET_DEBUG("ep_get_trunkPort_vidMembers, vid %d buf %s\n" COMMA vid COMMA valbuf)

    token = strtok(valbuf, " ");
    
    *numPorts = 0;
    while (token) {
			
	if((long int)numPorts == MAX_ADD_PORTS )
	{
		MNET_DEBUG("ep_get_entity_vid_portMembers numPorts is exceeded[%ld]\n"COMMA (long int)numPorts)
		break;
	}
        portNames[*numPorts] = buf + offset;
        offset += sprintf(portNames[*numPorts], "%s", token) + 1;
        (*numPorts)++;
        token = strtok(NULL, " ");
    }
    return 0;
}
