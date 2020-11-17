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
#include "service_multinet_swfab_LinIF.h"
#include <stdio.h>
#include <stdlib.h>

#define MULTINET_DEBUG 1

//TODO:move actual config to nethelper lib
int linuxIfConfigVlan(PSWFabHALArg args, int numArgs, BOOL up) {
    char cmdBuf[1024];
    int offset = 0; 
    int i;

MNET_DEBUG("%s : %d Entry. \n" COMMA __FUNCTION__ COMMA __LINE__)
    for (i = 0; i < numArgs; ++i) {
        if (offset) offset+=snprintf(cmdBuf+offset, 
                                    sizeof(cmdBuf) - offset,
                                    " ; ");
        if (up) {
            if (args[i].vidParams.tagging) {
                offset += snprintf(cmdBuf+offset, 
                                sizeof(cmdBuf) - offset,
                                "vconfig add %s %d ; ", 
                                (char*)args[i].portID, 
                                args[i].vidParams.vid);
            }
            
            offset += snprintf(cmdBuf+offset, 
                               sizeof(cmdBuf) - offset,
                               "brctl addif %s %s", 
                               args[i].hints.network->name,
                               (char*)args[i].portID); 
            
            if (args[i].vidParams.tagging) {
                offset += snprintf(cmdBuf+offset, 
                            sizeof(cmdBuf) - offset,
                            ".%d", 
                            args[i].vidParams.vid); 
            }
            
            offset += snprintf(cmdBuf+offset, 
                               sizeof(cmdBuf) - offset,
                               " ; ifconfig %s up", 
                               (char*)args[i].portID);
            if (args[i].vidParams.tagging) {
                offset += snprintf(cmdBuf+offset, 
                               sizeof(cmdBuf) - offset,
                               " ; ifconfig %s.%d up", 
                               (char*)args[i].portID, 
                               args[i].vidParams.vid);
            }
            
        } else {
            if (args[i].vidParams.tagging) {
                offset += snprintf(cmdBuf+offset, 
                               sizeof(cmdBuf) - offset,
                               "vconfig rem %s.%d", 
                               (char*)args[i].portID, 
                               args[i].vidParams.vid);
            } else {
                offset += snprintf(cmdBuf+offset, 
                               sizeof(cmdBuf) - offset,
                               "brctl delif %s %s", 
                               args[i].hints.network->name, 
                               (char*)args[i].portID);
            }
        }
            
    }

MNET_DEBUG("%s : %d offset is %d \n" COMMA __FUNCTION__ COMMA __LINE__ COMMA offset)

MNET_DEBUG(" CMD : %s \n" COMMA cmdBuf)

    if (offset) 
        system(cmdBuf);
    return 0;
}


//Not used currently
int linuxIfInit(PSWFabHALArg args, int numArgs, BOOL up) {
    
    
    
    return 0;
}
