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
#include <stdlib.h>
#ifdef RDKB_EXTENDER_ENABLED
#include <string.h>
#endif
#include "srvmgr.h"
#include "secure_wrapper.h"

#define SERVICE_NAME "routed"
#define SERVICE_DEFAULT_HANDLER "/etc/utopia/service.d/service_routed.sh"

#ifdef CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "wan-status|/etc/utopia/service.d/service_routed.sh",
                                        "lan-status|/etc/utopia/service.d/service_routed.sh",
                                        "dhcpv6_option_changed|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "ripd-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "zebra-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "staticroute-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        NULL
                                      };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "wan-status|/etc/utopia/service.d/service_routed.sh",
                                        "lan-status|/etc/utopia/service.d/service_routed.sh",
                                        "ipv6_nameserver|/etc/utopia/service.d/service_routed.sh",
                                        "ipv6_prefix|/etc/utopia/service.d/service_routed.sh",
                                        "ripd-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "zebra-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "staticroute-restart|/etc/utopia/service.d/service_routed.sh|NULL|"TUPLE_FLAG_EVENT,
                                        #ifdef WAN_FAILOVER_SUPPORTED
                                        "routeset-ula|/usr/bin/service_routed|NULL|"TUPLE_FLAG_EVENT,
                                        "routeunset-ula|/usr/bin//service_routed|NULL|"TUPLE_FLAG_EVENT,
                                        #endif 
                                        NULL
                                      };

#endif

void srv_register(void) {
   // not sure is the rm is necessary anymore
   // system("rm -Rf /etc/iproute2/rt_tables");
   DBG_PRINT("20_routing : %s Entry\n", __FUNCTION__);
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
   v_secure_system("sysevent set rip-status stopped");
   DBG_PRINT("20_routing : %s Exit\n", __FUNCTION__);
}

#ifdef RDKB_EXTENDER_ENABLED
void stop_service()
{
    v_secure_system(SERVICE_DEFAULT_HANDLER " " SERVICE_NAME "-stop");
}
#endif

void srv_unregister(void) {
   DBG_PRINT("20_routing : %s Entry\n", __FUNCTION__);
   #ifdef RDKB_EXTENDER_ENABLED
      stop_service();
   #endif
   sm_unregister(SERVICE_NAME);
   DBG_PRINT("20_routing : %s Exit\n", __FUNCTION__);
}

int main(int argc, char **argv)
{
   DBG_PRINT("20_routing : %s Entry\n", __FUNCTION__);
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
         DBG_PRINT("%s called with invalid parameter (%s)\n", argv[0], 1==argc ? "" : argv[1]);
   }   
   DBG_PRINT("20_routing : %s Exit\n", __FUNCTION__);
   return(0);
}

