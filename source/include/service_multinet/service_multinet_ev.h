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
#ifndef MNET_EV_H
#define MNET_EV_H

#include "service_multinet_base.h"
#include "sysevent/sysevent.h"

#define MNET_STATUS_FORMAT(x) "multinet_%d-status", x
#define MNET_IFSTATUS_FORMAT(x) "if_%s-status", x
#define MNET_IFSTATUS_ASYNCID_FORMAT(x) "ifstatus_%s_async", x
#define MNET_IFSTATUS_ASYNCVAL_FORMAT(x) "%d %d", (x).action_id, (x).trigger_id
#define MNET_NAME_FORMAT(x) "multinet_%d-name", x


extern token_t sysevent_token_interactive;
extern int sysevent_fd_interactive;
/**
 * Register the current running process for the given interface event name. If a status value name
 * and bool pointer are provided, the current status will be polled after registration and filled into
 * the bool variable to handle any registration / initialization race conditions. 
 * 
 * This function must register in the appropriate way based on execution style of the process.
 */
int ev_register_ifstatus(PL2Net net, PMember member,  char* ifStatusEventName, char* ifStatusValueName, BOOL* readyFlag); 

/** 
 * Unregister the current running process for the given interface event name. 
 */
int ev_unregister_ifstatus(PL2Net net, char* ifStatusEventName); 


/** 
 * Announce to the system the status of a given l2net.
 */
int ev_set_netStatus(PL2Net net, enum service_status status); 

/** Initialize the event system. isDaemon should be initialized before this is called.
 */
int ev_init(void); 

int ev_string_to_status(char* stringStatus, SERVICE_STATUS* status);

int ev_firewall_restart(void);

int ev_set_name(PL2Net net);

//Private----------

/** This function registers the current handler for the specified event. Function must register
 * appropriately based on the calling style of the handler and capabilities of the underlying eventing
 * system. Calling style should be indicated during lib init.
 */ 
//int ev_register_event(char* eventName); 

#endif
