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

/*
 *------------------------------------------------------------------------------
 *                   registration_template.c
 *
 * This is a template for registering a lego service to the default events, as well
 * as setting the service's default status to stopped.
 *
 * For services which only require the default events and default status, all that
 * needs to be done is specify:
 *   1) the SERVICE_NAME
 *   2) the SERVICE_DEFAULT_HANDLER 
 *   3) SERVICE_CUSTOM_EVENTS 
 *
 * The SERVICE_NAME is the name of the service and this is used to register for
 * service name specific events.
 * The SERVICE_DEFAULT_HANDLER is the path and name of an executable that will be activated
 * upon receipt of default events (start, stop, restart). The handler must not block.
 * SERVICE_CUSTOM_EVENTS is used if you want to register for other events.
 *
 * For services which require more event registration, or which need to perform
 * extra boot-time work, then the functions do_start() and do_stop() will need
 * to be enhanced.
 *------------------------------------------------------------------------------
 */

#include <stdio.h>
#include "srvmgr.h"

/*
 * 1) Name of this service
 *    You MUST set this to a globally unique string
 */
const char* SERVICE_NAME            = "byoi";

/*
 * 2) Name of the default event handler
 *    The path and name of an executable to be activated up default events [start|stop|restart]
 *    If the value is set to NULL, then no default events will be installed for this service
 *
 *    It is your responsibility to ensure that the default handler code exists in the runtime
 *    directory that you have specified
 */  
const char* SERVICE_DEFAULT_HANDLER = "/etc/utopia/service.d/service_byoi.sh";

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
const char* SERVICE_CUSTOM_EVENTS[] = { "desired_hsd_mode|/etc/utopia/service.d/service_byoi.sh",
					"retry_desired_hsd_mode|/etc/utopia/service.d/service_byoi.sh|NULL|"TUPLE_FLAG_EVENT,
				        "current_wan_ipaddr|/etc/utopia/service.d/service_byoi.sh|"ACTION_FLAG_NOT_THREADSAFE,
					"bridge_ipv4_ipaddr|/etc/utopia/service.d/service_byoi.sh", 
					"current_hsd_mode|/etc/utopia/service.d/service_byoi.sh|"ACTION_FLAG_NOT_THREADSAFE, 
					"primary_HSD_allowed|/etc/utopia/service.d/service_byoi.sh", 
					"system-start|/etc/utopia/service.d/service_byoi.sh|NULL|"TUPLE_FLAG_EVENT};

/*
 *******************************************************************************************
 *                       NOTHING MORE TO DO
 * In general there is no need to change anything below
 *******************************************************************************************
*/

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

