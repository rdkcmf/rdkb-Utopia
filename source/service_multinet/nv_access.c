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
#include "service_multinet_nv.h"
#include <stdio.h>
#include <string.h>

char* typeStrings[] = {
    "SW", "Gre", "Link", "Eth", "WiFi"
};


int nv_get_members(PL2Net net, PMember memberList, int numMembers) {
    int i;
    char cmdBuff[512];
    char valBuff[256];
    int offset = 0;
    FILE* psmcliOut;
    char* ifToken, *dash;
    
    int actualNumMembers = 0;
    
    offset += snprintf(cmdBuff, sizeof(cmdBuff),"psmcli get -e");
    
    for (i = 0; i < sizeof(typeStrings)/sizeof(*typeStrings); ++i) {
        MNET_DEBUG("nv_get_members, adding lookup string index %d. offset=%d\n" COMMA i COMMA offset)
        offset += snprintf(cmdBuff+offset, sizeof(cmdBuff)-offset, " X dmsb.l2net.%d.Members.%s", net->inst, typeStrings[i]);
        MNET_DEBUG("nv_get_members, current lookup offset: %d, string: %s\n" COMMA offset COMMA cmdBuff)
    }
     
    psmcliOut = popen(cmdBuff, "r");
    
    for(i = 0; fgets(valBuff, sizeof(valBuff), psmcliOut); ++i) {
        MNET_DEBUG("nv_get_members, current lookup line %s, i=%d\n" COMMA valBuff COMMA i)
        ifToken = strtok(valBuff+2, "\" \n");
        while(ifToken) { //FIXME: check for memberList overflow
            MNET_DEBUG("nv_get_members, current lookup token %s\n" COMMA ifToken)
            if ((dash = strchr(ifToken, '-'))){
                *dash = '\0';
                memberList[actualNumMembers].bTagging = 1;
            } else {
                memberList[actualNumMembers].bTagging = 0;
            }
            memberList[actualNumMembers].interface->map = NULL; // No mapping has been performed yet
            strcpy(memberList[actualNumMembers].interface->name, ifToken);
            strcpy(memberList[actualNumMembers++].interface->type->name, typeStrings[i]);
            if (dash) *dash = '-'; // replace character just in case it would confuse strtok
            
            ifToken = strtok(NULL, "\" \n");
        }
    }
    
    return actualNumMembers;
    
}

int nv_get_bridge(int l2netInst, PL2Net net) {
    char cmdBuff[512];
    char valBuff[80];
    char tmpBuf[15];
    FILE* psmcliOut;
    snprintf(cmdBuff, sizeof(cmdBuff), 
            "psmcli get -e X dmsb.l2net.%d.Name X dmsb.l2net.%d.Vid X dmsb.l2net.%d.Enable", 
             l2netInst, l2netInst, l2netInst);
    psmcliOut = popen(cmdBuff, "r");
    
    fgets(valBuff, sizeof(valBuff), psmcliOut);
    *strrchr(valBuff, '"') = '\0';
    sscanf(valBuff, "X=\"%s\n", net->name); 
    
    fgets(valBuff, sizeof(valBuff), psmcliOut);
    sscanf(valBuff, "X=\"%d\"\n", &net->vid);
    
    fgets(valBuff, sizeof(valBuff), psmcliOut);
    sscanf(valBuff, "X=\"%s\"\n", tmpBuf);
    if(!strcmp("FALSE", tmpBuf)) {
        net->bEnabled = 0;
    } else {
        net->bEnabled = 1;
    }
    
    net->inst = l2netInst;
    
    pclose(psmcliOut);
    
    return 0;
}
