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
   Copyright [2014] [Cisco Systems, Inc.]
 
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

#include <stdio.h>
#include "srvmgr.h"
#ifdef RDKB_EXTENDER_ENABLED
#include <string.h>
#endif
#include <stdlib.h>

const char* SERVICE_NAME            = "hotspot";
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_multinet/handle_gre.sh";
#ifdef INTEL_GRE_HOTSPOT
const char* SERVICE_CUSTOM_EVENTS[] = {
                                        "gre-restart|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "gre-forceRestart|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "snmp_subagent-status|/etc/utopia/service.d/service_multinet/handle_gre.sh",
                                        "hotspot-update_bridges|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "igre-start|/etc/utopia/service.d/service_multinet/service_gre.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "igre-stop|/etc/utopia/service.d/service_multinet/service_gre.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "ipv6_dhcp6_addr|/etc/utopia/service.d/service_multinet/service_gre.sh|NULL",
                                        "igre-bringup-gre-hs|/etc/utopia/service.d/service_multinet/service_gre.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "igre-hotspot-stop|/etc/utopia/service.d/service_multinet/service_gre.sh|NULL|"TUPLE_FLAG_EVENT,
                                        NULL
                                      };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "gre-restart|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "gre-forceRestart|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "snmp_subagent-status|/etc/utopia/service.d/service_multinet/handle_gre.sh",
                                        "hotspot-update_bridges|/etc/utopia/service.d/service_multinet/handle_gre.sh|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        NULL
                                      };
#endif

void srv_register(void) {
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
   system("modprobe brMtuMod");
}

#ifdef RDKB_EXTENDER_ENABLED
void stop_service()
{
    char buf[512];
    memset(buf,0,sizeof(buf));
    snprintf(buf,sizeof(buf),"sh %s %s-stop",SERVICE_DEFAULT_HANDLER,SERVICE_NAME);
    system(buf);
}
#endif

void srv_unregister(void) {
   #ifdef RDKB_EXTENDER_ENABLED
      stop_service();
   #endif
   sm_unregister(SERVICE_NAME);
}

int main(int argc, char **argv)
{
   cmd_type_t choice = parse_cmd_line(argc, argv);
   
   switch(choice) {
      case(nochoice):
      case(start):
         srv_register();
         break;
      case(stop):
         srv_unregister();
         break;
      case(restart):
         srv_unregister();
         srv_register();
         break;
      default:
         printf("%s called with invalid parameter (%s)\n", argv[0], 1==argc ? "" : argv[1]);
   }   
   return(0);
}

