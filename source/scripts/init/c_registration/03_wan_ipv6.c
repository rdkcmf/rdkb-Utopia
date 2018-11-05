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
#include <unistd.h>
#include "srvmgr.h"

const char* SERVICE_NAME            = "ipv6";
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_wan_ipv6.sh";
const char* SERVICE_CUSTOM_EVENTS[] = { 
                                        "current_ipv4_link_state|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "wan-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "lan-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "6rd-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "6to4-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "aiccu-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "he-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "dhcpv6_client-status|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        "cron_every_minute|/etc/utopia/service.d/service_wan_ipv6.sh",
                                        NULL
                                      };

const char * SIXRD_CUSTOM_EVENTS[] = {
					"current_wan_ipaddr|/etc/utopia/service.d/service_wan_ipv6/6rd.sh",
                                        "lan-status|/etc/utopia/service.d/service_wan_ipv6/6rd.sh",
					NULL,
				     };

const char * SIXTO4_CUSTOM_EVENTS[] = {
					"current_wan_ipaddr|/etc/utopia/service.d/service_wan_ipv6/6to4.sh",
					NULL,
				     };

const char * AICCU_CUSTOM_EVENTS[] = {
					"current_wan_ipaddr|/etc/utopia/service.d/service_wan_ipv6/aiccu.sh",
					NULL,
				     };

const char * HE_CUSTOM_EVENTS[] = {
					"current_wan_ipaddr|/etc/utopia/service.d/service_wan_ipv6/he.sh",
					NULL,
				     };

void srv_register(void) {
   system("sysevent set ipv6_connection_state wan_down") ;
   sm_register("6rd","/etc/utopia/service.d/service_wan_ipv6/6rd.sh",SIXRD_CUSTOM_EVENTS) ;
   sm_register("6to4","/etc/utopia/service.d/service_wan_ipv6/6to4.sh",SIXTO4_CUSTOM_EVENTS) ;
   sm_register("aiccu","/etc/utopia/service.d/service_wan_ipv6/aiccu.sh",AICCU_CUSTOM_EVENTS) ;
   sm_register("he","/etc/utopia/service.d/service_wan_ipv6/he.sh",HE_CUSTOM_EVENTS) ;
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS); /* Should be the last one else it would be called by all *-status :-) */
}

void srv_unregister(void) {
   sm_unregister(SERVICE_NAME);
   sm_unregister("6rd");
   sm_unregister("6to4");
   sm_unregister("aiccu");
   sm_unregister("he");
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

