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

const char* SERVICE_NAME            = "dhcpv6_server";
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_dhcpv6_server.sh";

#if defined(_CBR_PRODUCT_REQ_) && !defined(_CBR2_PRODUCT_REQ_)
const char* SERVICE_CUSTOM_EVENTS[] = {
                                        "dhcpv6_option_changed|/usr/bin/service_ipv6",
                                        "dhcpv6_server-start|/usr/bin/service_ipv6",
                                        "dhcpv6_server-stop|/usr/bin/service_ipv6",
                                        "dhcpv6_server-restart|/usr/bin/service_ipv6",
                                        NULL
                                      };
#elif defined (CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION)
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "dhcpv6_option_changed|/etc/utopia/service.d/service_dhcpv6_server.sh|NULL|"TUPLE_FLAG_EVENT,
                                        NULL
                                      };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "lan-status|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        "ipv6_nameserver|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        "ipv6_domain|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        "ipv6_ntp_server|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        "dhcp_domain|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        "current_lan_ipv6address|/etc/utopia/service.d/service_dhcpv6_server.sh",
                                        NULL
                                      };
#endif                                      
                                   

void srv_register(void) {
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
}

void srv_unregister(void) {
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

