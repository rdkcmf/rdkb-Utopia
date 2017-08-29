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

 /* This is the main executable for service_multinet.It is implemented as
  * a foreground command line interface to the multinet library. 
  */
 
 #include "service_multinet_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#ifndef SERVICE_MULTINET_EXE_PATH
#define SERVICE_MULTINET_EXE_PATH "/etc/utopia/service.d/service_multinet_exec"
#endif

FILE *mnetfp = NULL;
 
 typedef int (*entryCallback)(char* argv[], int argc);
 
 typedef struct entryCall {
     char call[32];
     entryCallback action;
 } EntryCall;
 
 int handle_up(char* argv[], int argc);
 int handle_down(char* argv[], int argc);
 int handle_start(char* argv[], int argc);
 int handle_stop(char* argv[], int argc);
 int handle_syncMembers(char* argv[], int argc);
 int handle_syncNets(char* argv[], int argc);
 int handle_restart(char* argv[], int argc);
 int handle_ifStatus(char* argv[], int argc);
 int set_multicast_mac(char* argv[], int argc);
 int add_ipc_vlan(char* argv[], int argc);
 int add_radius_vlan(char* argv[], int argc);
 int create_mesh_vlan(char* argv[], int argc);
 
 EntryCall calls[] = {
	 {"multinet-up", handle_up},
	 //{"multinet-ifStatus", handle_ifStatus},
	 {"multinet-down", handle_down},
	 {"multinet-start", handle_start},
	 {"multinet-stop", handle_stop},
	 {"multinet-syncMembers", handle_syncMembers},
	 {"multinet-syncNets", handle_syncNets},
	 {"multinet-restart", handle_restart},
	 {"set_multicast_mac", set_multicast_mac},
	 {"add_ipc_vlan", add_ipc_vlan},
	 {"add_radius_vlan", add_radius_vlan},
	 {"create_mesh_vlan", create_mesh_vlan},
 };
	 
 
 int main(int argc, char* argv[]) {
     int retval;
    int i;
	if (argc < 2) return 1;
        
	if(mnetfp == NULL) {
		mnetfp = fopen ("/rdklogs/logs/MnetDebug.txt", "a+");
	}
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#endif 
        MNET_DEBUG("ENTERED MULTINET APP, argc = %d \n" COMMA argc)
        if((retval = multinet_lib_init(0, SERVICE_MULTINET_EXE_PATH))) {
            printf("%s failed to init multinet lib. code=%d\n" SERVICE_MULTINET_EXE_PATH, retval);
            exit(retval);
        }
	
	for (i =0; i < sizeof(calls) / sizeof(*calls); ++i) {
		if (!strcmp(calls[i].call, argv[1])) {
			calls[i].action(argv, argc);
			if(mnetfp)
		      fclose(mnetfp);
			return 0;
		}
	}
	
	if (argc == 7) {
            //FIXME more input validation
            
            handle_ifStatus(argv, argc);
        }
	
	//printUsage();
	if(mnetfp)
		fclose(mnetfp);
	return 1;
	
}

//exeName, eventName, netIdString
 int handle_up(char* argv[], int argc) {
     MNET_DEBUG("Main: handle_up")
    multinet_bridgeUpInst(atoi(argv[2]), 0);
     return 0;
 }
 int handle_down(char* argv[], int argc) {
     MNET_DEBUG("Main: handle_down")
     multinet_bridgeDownInst(atoi(argv[2]));
     return 0;
 }
 int handle_start(char* argv[], int argc){
     MNET_DEBUG("Main: handle_start")
     multinet_bridgeUpInst(atoi(argv[2]), 1);
     return 0;
 }
 int handle_stop(char* argv[], int argc){
     MNET_DEBUG("Main: handle_stop")
     multinet_bridgeDownInst(atoi(argv[2]));
     return 0;
 }
 int handle_syncMembers(char* argv[], int argc){
     MNET_DEBUG("Main: handle_syncMembers")
     multinet_SyncInst(atoi(argv[2]));
     return 0;
 }
 int handle_syncNets(char* argv[], int argc){
     MNET_DEBUG("Main: handle_syncNets NOT SUPPORTED")
        //deferred
     return 0;
 }
 int handle_restart(char* argv[], int argc){
     MNET_DEBUG("Main: handle_restart")
    multinet_bridgeDownInst(atoi(argv[2]));
    multinet_bridgeUpInst(atoi(argv[2]), 0);
    return 0;
 }
 //exeName, eventName, statusValue, netIdString, ifname, iftype, tag?
 int handle_ifStatus(char* argv[], int argc){
     int l2netInst;
     MNET_DEBUG("Main: handle_ifStatus")
     sscanf(argv[3], "%d", &l2netInst);
     
     multinet_ifStatusUpdate_ids(l2netInst, argv[4], argv[5], argv[2], argv[6]);
     return 0;
 }

 int set_multicast_mac(char* argv[], int argc)
 {
	MNET_DEBUG("Setting Multicast MACs early\n")
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
	setMulticastMac();
#endif
    return 0;
 }

 int add_ipc_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    addIpcVlan();
#endif	
    return 0;
 }

 int add_radius_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    addRadiusVlan();
#endif
    return 0;
 }

 int create_mesh_vlan(char* argv[], int argc)
 {
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_)
    createMeshVlan();
#endif
    return 0;
 }
