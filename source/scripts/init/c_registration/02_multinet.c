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
#include "syscfg/syscfg.h"
#include <string.h>
#include <stdlib.h>

const char* SERVICE_NAME            = "multinet";

#ifdef MULTILAN_FEATURE
#if defined (_CBR_PRODUCT_REQ_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_tchcbr.sh";
#elif (_COSA_BCM_ARM_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_tchxb6.sh";
#else
#if defined (_VLAN_PUMA7_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_xb7.sh";
#else
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_multinet_exec";
#endif
#endif
#else
#ifdef INTEL_PUMA7
#if defined (_XB7_PROD_REQ_) && defined (_VLAN_PUMA7_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_xb7.sh";
#else
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_xb6.sh";
#endif
#elif defined (_CBR_PRODUCT_REQ_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_tchcbr.sh";
#elif defined (_SR300_PRODUCT_REQ_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/bridgeutil_sr300.sh";
#elif defined (_HUB4_PRODUCT_REQ_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_hub4.sh";
#elif defined (_COSA_BCM_ARM_) && ! defined (_PLATFORM_RASPBERRYPI_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/vlan_util_tchxb6.sh";
#else
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_multinet_exec";
#endif
#endif
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
#ifdef MULTILAN_FEATURE
#if defined (_CBR_PRODUCT_REQ_) 
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_tchcbr.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_tchcbr.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_tchcbr.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_tchcbr.sh|NULL|"TUPLE_FLAG_EVENT,
     NULL };
#elif (_COSA_BCM_ARM_)
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshethbhaul-bridge-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/vlan_util_tchxb6.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };

#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
#if defined (_VLAN_PUMA7_)
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-bridge-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
#else
    "multinet-syncNets|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
#if defined(MESH_ETH_BHAUL)
    "meshethbhaul-bridge-setup|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/service_multinet_exec|NULL|"TUPLE_FLAG_EVENT,
#endif
#endif
    "sw_ext_restore|/etc/utopia/service.d/service_multinet/handle_sw.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    NULL };
#endif
#else
#ifdef INTEL_PUMA7
#if defined (_XB7_PROD_REQ_) && defined (_VLAN_PUMA7_)
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-bridge-setup|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/vlan_util_xb7.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    NULL };
#else
//Intel Proposed RDKB Generic Bug Fix from XB6 SDK - ACTION_FLAG_NOT_THREADSAFE
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/vlan_util_xb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    NULL };
#endif
#elif defined (_CBR_PRODUCT_REQ_) 
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_tchcbr.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_tchcbr.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_tchcbr.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_tchcbr.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
     NULL };
#elif defined (_SR300_PRODUCT_REQ_)
const char* SERVICE_CUSTOM_EVENTS[] = {
    "multinet-syncNets|/etc/utopia/service.d/bridgeutil_sr300.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/bridgeutil_sr300.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/bridgeutil_sr300.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/bridgeutil_sr300.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/bridgeutil_sr300.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#elif defined (_HUB4_PRODUCT_REQ_)
const char* SERVICE_CUSTOM_EVENTS[] = {
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_hub4.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_hub4.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_hub4.sh|NULL|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_hub4.sh|NULL|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_hub4.sh|NULL|"TUPLE_FLAG_EVENT,
    NULL };
#elif defined (_COSA_BCM_ARM_) && ! defined (_PLATFORM_RASPBERRYPI_)
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "lnf-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshbhaul-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-bridge-setup|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-up|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "meshethbhaul-down|/etc/utopia/service.d/vlan_util_tchxb6.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    NULL };

#else
const char* SERVICE_CUSTOM_EVENTS[] = { 
    "multinet-syncNets|/etc/utopia/service.d/service_multinet_exec|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-syncMembers|/etc/utopia/service.d/service_multinet_exec|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-down|/etc/utopia/service.d/service_multinet_exec|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "multinet-up|/etc/utopia/service.d/service_multinet_exec|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    "sw_ext_restore|/etc/utopia/service.d/service_multinet/handle_sw.sh|"ACTION_FLAG_NOT_THREADSAFE"|"TUPLE_FLAG_EVENT,
    NULL };
#endif
#endif
void srv_register(void) {

  #if defined (_BRIDGE_UTILS_BIN_)

      const char* SERVICE_DEFAULT_HANDLER_OVS = "/usr/bin/bridgeUtils";
      
      const char* SERVICE_CUSTOM_EVENTS_OVS[] = { 
      "multinet-syncNets|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "multinet-syncMembers|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "multinet-down|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "multinet-up|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "lnf-setup|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "meshbhaul-setup|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      "meshethbhaul-bridge-setup|/usr/bin/bridgeUtils|NULL|"TUPLE_FLAG_EVENT,
      NULL };

      syscfg_init();
      int ovsEnable = 0 , bridgeUtilEnable=0 ;
      char buf[ 8 ] = { 0 };
      if( 0 == syscfg_get( NULL, "mesh_ovs_enable", buf, sizeof( buf ) ) )
      {
          if ( strcmp (buf,"true") == 0 )
            ovsEnable = 1;
          else 
            ovsEnable = 0;

      }
      else
      {
          printf("syscfg_get failed to retrieve ovs_enable\n");

      }

      memset(buf,0,sizeof(buf));

      if( 0 == syscfg_get( NULL, "bridge_util_enable", buf, sizeof( buf ) ) )
      {
              if ( strcmp (buf,"true") == 0 )
                  bridgeUtilEnable = 1;
              else 
                  bridgeUtilEnable = 0;   
      }
      else
      {
          printf("syscfg_get failed to retrieve bridge_util_enable\n");

      }

      if ( ovsEnable == 1  || bridgeUtilEnable == 1 )
      {
            sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER_OVS, SERVICE_CUSTOM_EVENTS_OVS);
      }
      else
      {
            sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
      }

  #else
    sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);

#if defined (_VLAN_PUMA7_)
    char* str[2];
    str[0] = malloc(110);
    str[1] = NULL;
    for (int i = 0; i <= 15; i++)
      for (int j = 0; j <= 10; j++)
        {
            snprintf(str[0], 110, "if_wlan%d.%d-status|/etc/utopia/service.d/vlan_util_xb7.sh|%s|%s", i, j, ACTION_FLAG_NORMAL, TUPLE_FLAG_EVENT);
            sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, (const char**) str);
        }
    free(str[0]);
#endif
  #endif
#ifdef MULTILAN_FEATURE
#if !defined(_COSA_BCM_ARM_) || defined(INTEL_PUMA7) || !defined(_CBR_PRODUCT_REQ_)
   system("/etc/utopia/service.d/service_multinet/handle_sw.sh initialize");
#endif
#else
#if !defined(_COSA_BCM_ARM_) || !defined(INTEL_PUMA7) || !defined(_CBR_PRODUCT_REQ_) || defined(_PLATFORM_RASPBERRYPI_)
   system("/etc/utopia/service.d/service_multinet/handle_sw.sh initialize");
#endif
#endif
#ifdef MULTILAN_FEATURE
#if defined (INTEL_PUMA7)
   system("touch /var/multinet_exec_enabled");
#endif
#endif
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


