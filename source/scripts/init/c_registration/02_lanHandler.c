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
#define SERVICE_NAME "lan"
#define SERVICE_DEFAULT_HANDLER "/etc/utopia/service.d/lan_handler.sh"

#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_) && !defined(_PLATFORM_IPQ_)
const char* SERVICE_CUSTOM_EVENTS[] = {
    "pnm-status|/etc/utopia/service.d/lan_handler.sh",
    "bring-lan|/usr/bin/service_dhcp",
    "lan-start|/usr/bin/service_dhcp",
    "lan-restart|/usr/bin/service_dhcp",
    "iot_status|/etc/utopia/service.d/lan_handler.sh",
    "ipv4-resync|/etc/utopia/service.d/lan_handler.sh|NULL|"TUPLE_FLAG_EVENT,
    "erouter_mode-updated|/etc/utopia/service.d/lan_handler.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#elif defined(CORE_NET_LIB) && \
        ((defined(_XB6_PRODUCT_REQ_) && !defined (_XB8_PRODUCT_REQ_)) || \
         (defined(_CBR_PRODUCT_REQ_) && !defined(_CBR2_PRODUCT_REQ_)))
const char* SERVICE_CUSTOM_EVENTS[] = {
    "pnm-status|/usr/bin/service_dhcp",
    "bring-lan|/usr/bin/service_dhcp",
    "lan-start|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "lan-stop|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "lan-restart|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "iot_status|/usr/bin/service_dhcp",
    //"multinet-resync|/usr/bin/service_dhcp",
    "ipv4-resync|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "erouter_mode-updated|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "pnm-status|/etc/utopia/service.d/lan_handler.sh",
    "bring-lan|/etc/utopia/service.d/lan_handler.sh",
    "iot_status|/etc/utopia/service.d/lan_handler.sh",
    "ipv4-resync|/etc/utopia/service.d/lan_handler.sh|NULL|"TUPLE_FLAG_EVENT,
    "erouter_mode-updated|/etc/utopia/service.d/lan_handler.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
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

