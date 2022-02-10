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

const char* SERVICE_NAME            = "ipv4";
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_ipv4.sh";
/*
 * 3) Custom Events
 *    If the service should receive events other than start stop restart, then
 *    declare them. If there are no custom events then set to NULL
 *   
 *    The format of each line of a custom event string is:
 *    name_of_event | path/filename_of_handler | activation_flags or NULL | tuple_flags or NULL | extra parameters
 *   
 *    Each custom event string is a null terminated string.
 *    The array of strings must be terminated with NULL
 *   
 *    Example of a custom event string arrray containing several events
 *       const char* SERVICE_CUSTOM_EVENTS[] = {
 *                            "event1 | /etc/utopia/service.d/event1_handler.sh",
 *                            "event2 |/etc/utopia/service.d/event1_handler.sh|NULL|NULL|aconstant $wan_proto @current_wan_ipaddr",
 *                            "event3-status | /etc/utopia/service.d/event1_handler.sh",
 *                            NULL
 *                                             };
 *    Each custom event string must have at least "event_name | path/filename of handler"
 *    The other fields are for specifing events which can pass extra parameters or which have special
 *    sysevent handling requirements. This is documented in 
 *    lego_overlay/proprietary/init/service.d/service_registration_functions.sh
 *
 * Also note that if you are using string values for events (as defined in srvmgr.h), then you need to 
 * keep the define outside of the string quotation symbols
 * eg. "event3|/etc/code|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_SERIAL
 */
#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7) && !defined(_COSA_BCM_ARM_) && !defined(_PLATFORM_IPQ_)
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "ipv4-resyncAll|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-down|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-up|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-set_dyn_config|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-sync_tsip_all|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-stop_tsip_all|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip_asn|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#elif defined(CORE_NET_LIB) && \
        ((defined(_XB6_PRODUCT_REQ_) && !defined (_XB8_PRODUCT_REQ_)) || \
         (defined(_CBR_PRODUCT_REQ_) && !defined(_CBR2_PRODUCT_REQ_)))
const char* SERVICE_CUSTOM_EVENTS[] = {
    "ipv4-resyncAll|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-down|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-up|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-set_dyn_config|/usr/bin/service_dhcp",
    "ipv4-sync_tsip_all|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-stop_tsip_all|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip_asn|/usr/bin/service_dhcp|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "ipv4-resyncAll|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-down|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-up|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-set_dyn_config|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-sync_tsip_all|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-stop_tsip_all|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    "ipv4-resync_tsip_asn|/etc/utopia/service.d/service_ipv4.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
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

