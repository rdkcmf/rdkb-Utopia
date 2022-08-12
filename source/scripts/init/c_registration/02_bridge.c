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
#include <stdlib.h>
#endif
const char* SERVICE_NAME            = "bridge";
#ifdef INTEL_PUMA7
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_bridge_puma7.sh";
const char* SERVICE_CUSTOM_EVENTS[] = {"wan-start|/etc/utopia/service.d/service_bridge_puma7.sh|NULL|" TUPLE_FLAG_EVENT, NULL};
#elif defined (_CBR_PRODUCT_REQ_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_bridge_tchcbr.sh";
const char* SERVICE_CUSTOM_EVENTS[] = { NULL };   //has to be commented due to NOT hit for Commscope platform then also added by patches 
#elif defined (_COSA_BCM_ARM_)  && ! defined (_PLATFORM_IPQ_) && ! defined (_PLATFORM_RASPBERRYPI_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_bridge_tchxb6.sh";
#if defined(DOCSIS_EXTENDED_MTU_SUPPORT)
const char* SERVICE_CUSTOM_EVENTS[] = {"wan-start|/etc/utopia/service.d/service_bridge_tchxb6.sh|NULL|" TUPLE_FLAG_EVENT, NULL};
#else
const char* SERVICE_CUSTOM_EVENTS[] = { NULL };
#endif
#elif defined (_PLATFORM_RASPBERRYPI_)
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_bridge_rpi.sh";
const char* SERVICE_CUSTOM_EVENTS[] = { NULL };
#else
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_bridge.sh";
const char* SERVICE_CUSTOM_EVENTS[] = { NULL };
#endif

void srv_register(void) {
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);
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

