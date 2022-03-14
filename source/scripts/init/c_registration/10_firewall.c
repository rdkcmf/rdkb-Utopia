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

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include "srvmgr.h"
#include <stdlib.h>

#define SERVICE_NAME              "firewall"
#define SERVICE_DEFAULT_HANDLER   "/etc/utopia/service.d/firewall_log_handle.sh"

/*
 * Override firewall-restart to set the maximum waiting activation depth to 1
 * This prunes out extraneous invokations
 *
 * Also make newhost_trigger_monitor an event but not serial because the handler is blocking
 */
const char* SERVICE_CUSTOM_EVENTS[] = {
                                        "firewall-restart|"SERVICE_DEFAULT_HANDLER"|"ACTION_FLAG_COLLAPSE_PENDING_QUEUE"|"TUPLE_FLAG_EVENT,
                                        "syslog-status|"SERVICE_DEFAULT_HANDLER,
                                        "firewall_trigger_monitor-start|/etc/utopia/service.d/service_firewall/trigger_monitor.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "firewall_newhost_monitor-start|/etc/utopia/service.d/service_firewall/newhost_monitor.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
                                        "syslog_rotated|/etc/utopia/service.d/service_firewall/newhost_monitor.sh|NULL|"TUPLE_FLAG_EVENT,
                                        "ipv6_prefix|"SERVICE_DEFAULT_HANDLER,
                                        "current_wan_ipv6_interface|"SERVICE_DEFAULT_HANDLER,
                                        "ipv6_wan0_dhcp_solicNodeAddr|"SERVICE_DEFAULT_HANDLER,
                                        "ipv6_erouter0_dhcp_solicNodeAddr|"SERVICE_DEFAULT_HANDLER,
                                        NULL
                                      };

void srv_unregister(void) {
   #ifdef RDKB_EXTENDER_ENABLED
   system("rm -rf /tmp/lanhosts");
   #endif
   sm_unregister(SERVICE_NAME);
}

void srv_register(void) {
   /*
    * Set up a directory for known lan hosts discovered
    * using triggers. This will be used by firewall.c
    */
   /* CID 68276: Unchecked return value from library */
   if (0 != mkdir("/tmp/lanhosts", S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))
   {
       printf("Failed to Create lanhosts directory.\n");
       return;
   }
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
#if !defined(NO_TRIGGER)
#ifndef CONFIG_KERNEL_NF_TRIGGER_SUPPORT
   system("killall trigger");
   system("trigger");
#endif
#endif
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
