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
#include "secure_wrapper.h"
#ifdef RDKB_EXTENDER_ENABLED
#include <string.h>
#include <stdlib.h>
#endif

#define SERVICE_NAME "dhcpv6_client"
#define SERVICE_DEFAULT_HANDLER "/etc/utopia/service.d/service_dhcpv6_client.sh"

#if defined(CORE_NET_LIB)
const char* SERVICE_CUSTOM_EVENTS[] = {
                                        "dhcpv6_client-start|/usr/bin/service_dhcpv6_client",
                                        "dhcpv6_client-stop|/usr/bin/service_dhcpv6_client",
                                        "dhcpv6_client-restart|/usr/bin/service_dhcpv6_client",
                                        "erouter_mode-updated|/usr/bin/service_dhcpv6_client",
                                        "phylink_wan_state|/usr/bin/service_dhcpv6_client",
                                        "current_wan_ifname|/usr/bin/service_dhcpv6_client",
                                        "bridge_mode|/usr/bin/service_dhcpv6_client",
                                        "current_ipv4_link_state|/usr/bin/service_dhcpv6_client",
                                        "lan-status|/usr/bin/service_dhcpv6_client",
                                        NULL
                                      };
#elif defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION)
const char* SERVICE_CUSTOM_EVENTS[] = {
                                        "erouter_mode-updated|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        "phylink_wan_state|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        "current_wan_ifname|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        "bridge_mode|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        NULL
                                      };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "current_ipv4_link_state|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        "current_wan_ifname|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        "lan-status|/etc/utopia/service.d/service_dhcpv6_client.sh",
                                        NULL
                                      };
#endif

void srv_register(void) {
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
}

#ifdef RDKB_EXTENDER_ENABLED
void stop_service()
{
    v_secure_system(SERVICE_DEFAULT_HANDLER " " SERVICE_NAME "-stop");
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

