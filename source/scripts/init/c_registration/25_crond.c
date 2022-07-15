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
#include "secure_wrapper.h"
#ifdef RDKB_EXTENDER_ENABLED
#include <string.h>
#endif
#include <stdlib.h>
#include "srvmgr.h"

#define SERVICE_NAME "crond"
#define SERVICE_DEFAULT_HANDLER "/etc/utopia/service.d/service_crond.sh"
const char* SERVICE_CUSTOM_EVENTS[] = { "ntpclient-status|/etc/utopia/service.d/service_crond.sh", 
                                        NULL 
                                      };


void srv_register(void) {
   FILE *fp = fopen("/etc/cron/cron.allow", "w");
   if (NULL != fp) {
      // prepare the crond.allow file
      fprintf(fp, "root\n");
      fprintf(fp, "firewall\n");
      fclose(fp);
   }
   sm_register(SERVICE_NAME, SERVICE_DEFAULT_HANDLER, SERVICE_CUSTOM_EVENTS);

   // we also want to start crond here so do so
   v_secure_system("/etc/utopia/service.d/service_crond.sh crond-start");

   // we want cron_every_minute to be an event
   // this will generate a sysevent event every minute
   v_secure_system("sysevent setoptions cron_every_minute " TUPLE_FLAG_EVENT);
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
