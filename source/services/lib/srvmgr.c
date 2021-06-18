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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include "ulog/ulog.h"
#include "sysevent/sysevent.h"
#include "srvmgr.h"

#define SM_PREFIX  "xsm_"
#define SM_POSTFIX "_async_id"

static int            sysevent_fd = 0;
static char          *sysevent_name = "srvmgr";
static token_t        sysevent_token;
static unsigned short sysevent_port;
static char           sysevent_ip[19];


/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in situ
 */
static char *trim(char *in)
{
   // trim the front of the string
   if (NULL == in) {
      return(NULL);
   }
   char *start = in;
   while(isspace(*start)) {
      start++;
   }
   // trim the end of the string

   char *end = start+strlen(start);
   end--;
   while(isspace(*end)) {
      *end = '\0';
      end--;
   }
   return(start);
}


/*
 * Procedure     : token_get
 * Purpose       : given the start of a string
 *                 containing a delimiter,
 *                 put the token into a char array and
 *                 return the end of the string
 * Parameters    :
 *    in         : The start of the string containing a token
 *    delim      : A character used as delimiter
 *    token      : A character array to place the token
 *    size       : The number of bytes in token
 *
 * Return Value  : The start of the next possible token
 *                 NULL if end
 */
static char *token_get(const char *in, const char delim, char *token, int size)
{
   char *end = strchr(in, delim);
   if (NULL != end) {
      int len = end-in;
      if (len > (size-1)) {
         len = size-1;
      }
      memcpy(token, in, len);
      token[len] = '\0';
      end++;
   } else {
      strncpy(token, in, size);
      token[size-1] = '\0';
   }
   return(end);
}


/*
 * Procedure     : sm_cancel_one_event
 * Purpose       : Find the async id associated with a service & event name
 *                 and cancel future notifications on that event to the service
 * Parameters    :
 *   srv_name       : The name of the service that registered for the notification
 *   event          : The name of the event that is register 
 * Return Value  : 
 *   0              : Success. the notification was cancelled or never existed
 *   <0             : some error
 */
static int sm_cancel_one_event(const char* srv_name, const char*event)
{
   char async_name[256];
   char async_next_name[256];
   char async_id_str[256];
   char async_next_id_str[256];
   char trigger_str[256];
   char action_str[256];

   int idx;

   /*
    * if this is a default event then it has a default format
    */
   int default_flags = strtoll("0x00000000", NULL, 16);
   char start_event_str[256];
   char stop_event_str[256];
   char restart_event_str[256];
   snprintf(start_event_str, sizeof(start_event_str), "%s-start", srv_name);
   snprintf(stop_event_str, sizeof(stop_event_str), "%s-stop", srv_name);
   snprintf(restart_event_str, sizeof(restart_event_str), "%s-restart", srv_name);
   if (0 == strcmp(start_event_str, event) || 0 == strcmp(stop_event_str, event) || 0 == strcmp(restart_event_str, event)) {
      snprintf(async_name, sizeof(async_name), "%s%s%s_%s", SM_PREFIX, srv_name, SM_POSTFIX, event);
      async_id_str[0] = '\0';
      if (0 == sysevent_get(sysevent_fd, sysevent_token, async_name, async_id_str, sizeof(async_id_str)) && '\0' != async_id_str[0] ) {
         char levent[256];
         char *asyncs;
         asyncs = token_get(async_id_str, ' ', levent, sizeof(levent));
         if (NULL != asyncs && '\0' != levent[0]) {
            /*
             * The tuple containing the async_id existed, so extract the async id for the
             * notification that we want to cancel
             */
            sscanf(asyncs, "%s %s", trigger_str, action_str);
            async_id_t async_id;
            async_id.trigger_id = strtoll(trigger_str, NULL, 16); // strtol truncates on some platforms
            async_id.action_id = strtoll(action_str, NULL, 16);   // strtol truncates on some platforms
            if (LONG_MIN != async_id.trigger_id && LONG_MAX != async_id.trigger_id &&
               LONG_MIN != async_id.action_id && LONG_MAX != async_id.action_id) {
               /*
                * cancel the notification
                */
               sysevent_rmcallback(sysevent_fd, sysevent_token, async_id);
               sysevent_set(sysevent_fd, sysevent_token, async_name, NULL, 0);
               sysevent_set_options(sysevent_fd, sysevent_token, async_name, default_flags);
               ulogf(ULOG_SERVICE, UL_INFO, "Unregistered %s from %s", srv_name, event);
            }
         }
      }
      return (0);
   }

   /*
    * otherwise a non default event has an indexable format
    */
   for(idx=1 ; ; idx++) {
      /*
       * create the tuple name according to a well known format.
       */
      snprintf(async_name, sizeof(async_name), "%s%s%s_%d", SM_PREFIX, srv_name, SM_POSTFIX, idx);
      async_id_str[0] = '\0';
      if (0 != sysevent_get(sysevent_fd, sysevent_token, async_name, async_id_str, sizeof(async_id_str)) ) {
         return(-1);
      }
      /*
       * if no events are found for this service then we are done
       */
      if ('\0' == async_id_str[0]) {
         return(0);
      }
     
      char levent[256];
      char *event_p;
      char *asyncs;
      asyncs = token_get(async_id_str, ' ', levent, sizeof(levent));
      if (NULL == asyncs || '\0' == levent[0]) {
         continue;
      } else {
         event_p = trim(levent);
      }

      /*
       * We found the tuple containing the event
       */
      if (0 == strcmp(event_p, event)) {
         /*
          * The tuple containing the async_id existed, so extract the async id for the 
          * notification that we want to cancel
          */
         sscanf(asyncs, "%s %s", trigger_str, action_str);
         async_id_t async_id;
         async_id.trigger_id = strtoll(trigger_str, NULL, 16);  // strtol truncates on some platforms
         async_id.action_id = strtoll(action_str, NULL, 16);    // strtol truncates on some platforms
         if (LONG_MIN != async_id.trigger_id && LONG_MAX != async_id.trigger_id &&
             LONG_MIN != async_id.action_id && LONG_MAX != async_id.action_id) {
            /*
             * cancel the notification
             */
            sysevent_rmcallback(sysevent_fd, sysevent_token, async_id);
            sysevent_set(sysevent_fd, sysevent_token, async_name, NULL, 0);
            ulogf(ULOG_SERVICE, UL_INFO, "Unregistered %s from %s", srv_name, event);
            break;
         }
      }
   }

   /*
    * idx is the current index in the tuple list where we found our entry
    * Now close up the whole in the array
    */
   do {
      snprintf(async_name, sizeof(async_name), "%s%s%s_%d", SM_PREFIX, srv_name, SM_POSTFIX, idx);
      snprintf(async_next_name, sizeof(async_next_name), "%s%s%s_%d", SM_PREFIX, srv_name, SM_POSTFIX, idx+1);
	  //printf("\n >>>>>> %s %d async_name = %s async_next_name = %s <<<<<< \n", __FUNCTION__, __LINE__,async_name, async_next_name);
      async_next_id_str[0] = '\0';
      sysevent_get(sysevent_fd, sysevent_token, async_next_name, async_next_id_str, sizeof(async_next_id_str));
      sysevent_set(sysevent_fd, sysevent_token, async_name, async_next_id_str, 0);
      idx++;
	  //printf("\n >>>>>> %s %d async_next_id_str = %s <<<<<< \n", __FUNCTION__, __LINE__,async_next_id_str);
   } while ('\0' != async_next_id_str[0]);
      
   return(0);
}

/*
 * Procedure     : sm_save_async_id
 * Purpose       : Save an async id for future cancellation
 * Parameters    : 
 *   srv_name       : The service owning the notification upon event
 *   event          : The name of the event that will trigger notifications
 *   async_id       : The async id that was returned when the registration for notification occured
 * Return Value  :
 *   The value returned when the async id was stored in sysevent
 */
static int sm_save_async_id(const char* srv_name, const char* event, async_id_t async_id)
{
   char async_name[256];
   char async_id_str[256];

   /*
    * a standard event has a standard format
    */
   char start_event_str[256];
   char stop_event_str[256];
   char restart_event_str[256];
   snprintf(start_event_str, sizeof(start_event_str), "%s-start", srv_name);
   snprintf(stop_event_str, sizeof(stop_event_str), "%s-stop", srv_name);
   snprintf(restart_event_str, sizeof(restart_event_str), "%s-restart", srv_name);
   if (0 == strcmp(start_event_str, event) || 0 == strcmp(stop_event_str, event) || 0 == strcmp(restart_event_str, event)) {
      snprintf(async_name, sizeof(async_name), "%s%s%s_%s", SM_PREFIX, srv_name, SM_POSTFIX, event);
      snprintf(async_id_str, sizeof(async_id_str), "%s 0x%x 0x%x", event, async_id.trigger_id, async_id.action_id);
      return(sysevent_set(sysevent_fd, sysevent_token, async_name, async_id_str, 0));
   }


   /*
    * a custom event has an indexable format
    */
   int idx;
   for (idx=1; ; idx++) {
      snprintf(async_name, sizeof(async_name), "%s%s%s_%d", SM_PREFIX, srv_name, SM_POSTFIX, idx);
      async_id_str[0] = '\0';
      sysevent_get(sysevent_fd, sysevent_token, async_name, async_id_str, sizeof(async_id_str));
      if ('\0' == async_id_str[0]) {
         snprintf(async_id_str, sizeof(async_id_str), "%s 0x%x 0x%x", event, async_id.trigger_id, async_id.action_id);
         /*
          * store the async id in the tuple
          */
         return(sysevent_set(sysevent_fd, sysevent_token, async_name, async_id_str, 0));
      }
   }

   return(0);
}

/*
 * Procedure     : sm_register_one_event
 * Purpose       : Register the service's handler for activation upon some event
 * Parameters    :
 *   srv_name        : The name of the service registering for the event
 *   custom          : Information about the event and handlers using the format:
 *                        event_name | path/file to handler to be activated | 
 *                        sysevent activation flags or NULL | sysevent tuple flags or NULL |
 *                        any extra parameters to be given to the handler upon activation
 *                     Note that extra parameters can be a syscfg runtime value (eg $wan_proto),
 *                     a sysevent runtime value (eg @current_wan_ipaddr) or a constant.
 * Return Value  : 
 *   0                : success
 *   <0               : some failure
 */
static int sm_register_one_event(const char* srv_name, const char* custom)
{
   char event[256];
   char handler[256];
   char async_flags[256];
   char tuple_flags[256];
   char params[256];
   char* next;
   char* event_p    = NULL;
   char* handler_p  = NULL;
   char* aflags_p   = NULL;
   char* tflags_p   = NULL;
   char* params_p   = NULL;
   int   rc         = 0;

   if (NULL == srv_name || NULL == custom) {
      return(-1);
   }

   /*
    * parse the custom string 
    */
   event[0] = handler[0] = async_flags[0] = tuple_flags[0] = params[0] = '\0';
   next = token_get(custom, '|', event, sizeof(event));
   if (NULL == next || '\0' == event[0]) {
      return (-1);
   } else {
      event_p = trim(event);
   }
   custom = next;

   next = token_get(custom, '|', handler, sizeof(handler));
   if ('\0' == handler[0]) {
      return (-1);
   } else {
      handler_p = trim(handler);
   }
   custom = next;

   if (NULL != next) {
      next = token_get(custom, '|', async_flags, sizeof(async_flags));
      aflags_p = trim(async_flags);
      custom = next;
   }

   if (NULL != next) {
      next = token_get(custom, '|', tuple_flags, sizeof(tuple_flags));
      tflags_p = trim(tuple_flags);
      custom = next;
   }

   if (NULL != next) {
      token_get(custom, '|', params, sizeof(params));
      params_p = trim(params);
   }

   /*
    * in case the service has already registered for this event, cancel that notification
    */
   sm_cancel_one_event(srv_name, event_p);

   /*
    * figure out how many parameters will be passed to the handler upon activation.
    * Note that the event name is implicitly registered as the first parameter
    */
   int num_params = 1;  // the implicit event name parameter
   if (NULL != params_p) {
      char token[256];
      char* start=params_p;
      char* next;
      do {
         next=token_get(start, ' ', token, sizeof(token));
         start=trim(next);
         num_params++;
      } while (NULL != next);
   }
   /*
    * we know how many parameters there are, so now prepare the argument string
    */
   char **args;
   args = (char **) malloc( sizeof(char *) * (num_params));
   if (NULL == args) {
      return(-2);
   }
   args[0]=strdup(event_p);
   int i;
   char token[256];
   char* start=params_p;
   for (i=1; i<num_params; i++) {
      next=token_get(start, ' ', token, sizeof(token));
      args[i]=strdup(trim(token));
      start=next;
   }

   /*
    * register the handler for activation upon event
    */
   async_id_t    async_id;
   action_flag_t flags = ACTION_FLAG_NONE;
   if (NULL != aflags_p && strcmp("NULL", aflags_p)) {
      flags = strtoll(aflags_p, NULL, 16);
      if (LONG_MIN == flags || LONG_MAX == flags) {
         rc = -3;
         goto sm_register_done;
      }
   }
   if (0 == sysevent_setcallback(sysevent_fd, sysevent_token, flags, 
                                 event_p, handler_p, num_params, args, &async_id)) {
      /*
       * save the async id in case the service want to cancel notifications for the event
       */
      sm_save_async_id(srv_name, event_p, async_id);
      ulogf(ULOG_SERVICE, UL_INFO, "Registered %s for %s using %s", srv_name, event_p, handler_p);
   } else {
      ulogf(ULOG_SERVICE, UL_INFO, "Failed to registered %s for %s", srv_name, event_p);
   }

   /*
    * if there are any tuple flags then we set the sysevent tuple options
    */
   if (NULL != tflags_p && strcmp("NULL", tflags_p)) {
      int flags = strtoll(tflags_p, NULL, 16);
      if (LONG_MIN == flags || LONG_MAX == flags) {
         rc = -4;
         goto sm_register_done;
      }
      sysevent_set_options(sysevent_fd, sysevent_token, event_p, flags);
   }
   
sm_register_done:
   for (i=0; i<num_params; i++) {
      free(args[i]);
      args[i] = NULL;
   }
   free(args);
   args=NULL;
   return(rc);
}


/*
 ************************************************************************************
 ************************************************************************************
 */

/*
 * Procedure     : parse_cmd_line
 * Purpose       : Figure out what the command line parameter is
 * Parameters    :
 *   argc            : The number of command line parameters
 *   argv            : The command line string array
 * Return Value  : the cmd_type_t
 */
cmd_type_t parse_cmd_line (int argc, char **argv)
{
   if (1 >= argc) {
      return(nochoice);
   }
   if (0 == strcmp("start", argv[1])) {
      return(start);
   }
   if (0 == strcmp("restart", argv[1])) {
      return(restart);
   }
   if (0 == strcmp("stop", argv[1])) {
      return(stop);
   }
   return(nochoice);
}

/*
 * Procedure     : sm_unregister
 * Purpose       : Unregister all notifications for a service
 * Parameters    :
 *   srv_name        : The name of the service unregistering
 * Return Value  :
 *   0                : success
 *   <0               : some failure
 */
int sm_unregister (const char* srv_name)
{
   ulog_init();

   /*
    * These are well known values for reaching syseventd
    */
   snprintf(sysevent_ip, sizeof(sysevent_ip), "127.0.0.1");
   sysevent_port = SE_SERVER_WELL_KNOWN_PORT;
   sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
   /*CID 57568: Improper use of negative value*/
   if (sysevent_fd < 0)
   {
       ulogf(ULOG_SERVICE, UL_INFO, "sysevent_fd can't be negative\n");
       return (-1);
   }

   char async_name[270];
   char async_id_str[256];
   char trigger_str[256];
   char action_str[256];

   /*
    * Cancel default events
    */
   int i;
   char start_event_str[256];
   char stop_event_str[256];
   char restart_event_str[256];
   snprintf(start_event_str, sizeof(start_event_str), "%s-start", srv_name);
   snprintf(stop_event_str, sizeof(stop_event_str), "%s-stop", srv_name);
   snprintf(restart_event_str, sizeof(restart_event_str), "%s-restart", srv_name);

   for (i=0 ; i<3 ; i++) {
      snprintf(async_name, sizeof(async_name), "%s%s%s_%s", SM_PREFIX, srv_name, SM_POSTFIX, 0==i ? start_event_str : 1==i ? stop_event_str : restart_event_str);
      async_id_str[0] = '\0';
      if (0 == sysevent_get(sysevent_fd, sysevent_token, async_name, async_id_str, sizeof(async_id_str)) && '\0' != async_id_str[0] ) {
         char levent[256];
         char *asyncs;
         asyncs = token_get(async_id_str, ' ', levent, sizeof(levent));
         if (NULL != asyncs) {
            /*
             * The tuple containing the async_id existed, so extract the async id for the
             * notification that we want to cancel
             */
            sscanf(asyncs, "%s %s", trigger_str, action_str);
            async_id_t async_id;
            async_id.trigger_id = strtoll(trigger_str, NULL, 16);  // strtol truncates on some platforms
            async_id.action_id = strtoll(action_str, NULL, 16);    // strtol truncates on some platforms
            if (LONG_MIN != async_id.trigger_id && LONG_MAX != async_id.trigger_id &&
               LONG_MIN != async_id.action_id && LONG_MAX != async_id.action_id) {
               /*
                * cancel the notification
                */
               sysevent_rmcallback(sysevent_fd, sysevent_token, async_id);
               sysevent_set(sysevent_fd, sysevent_token, async_name, NULL, 0);
               ulogf(ULOG_SERVICE, UL_INFO, "Unregistered %s from %s", srv_name, 0==i ? start_event_str : 1==i ? stop_event_str : restart_event_str);
            }
         }
      }
   }

   /*
    * go through the sysevent namespace for saved indexable asyncs and cancel them
    */
   int idx;

   for(idx=1 ; ; idx++) {
      /*
       * create the tuple name according to a well known format.
       */
      snprintf(async_name, sizeof(async_name), "%s%s%s_%d", SM_PREFIX, srv_name, SM_POSTFIX, idx);
      async_id_str[0] = '\0';
      if (0 != sysevent_get(sysevent_fd, sysevent_token, async_name, async_id_str, sizeof(async_id_str)) ) {
         break;
      }
      /*
       * if no more events are found for this service then we are done
       */
      if ('\0' == async_id_str[0]) {
         break;
      }

      char event[256];
      char *asyncs;
      asyncs = token_get(async_id_str, ' ', event, sizeof(event));
      if (NULL == asyncs || '\0' == event[0]) {
         continue;
      }

      /*
       * Extract the async id for the notification that we want to cancel
       */
      sscanf(asyncs, "%s %s", trigger_str, action_str);
      async_id_t async_id;
      async_id.trigger_id = strtoll(trigger_str, NULL, 16);  // strtol truncates on some platforms
      async_id.action_id = strtoll(action_str, NULL, 16);    // strtol truncates on some platforms
      if (LONG_MIN != async_id.trigger_id && LONG_MAX != async_id.trigger_id &&
          LONG_MIN != async_id.action_id && LONG_MAX != async_id.action_id) {
         /*
          * cancel the notification
          */
         sysevent_rmcallback(sysevent_fd, sysevent_token, async_id);
         sysevent_set(sysevent_fd, sysevent_token, async_name, NULL, 0);
         ulogf(ULOG_SERVICE, UL_INFO, "Unregistered %s from %s", srv_name, trim(event));
      }
   }

   /*
    * unset the tuple option on the default events
    * setting the tuple flags to 0 will let the tuple be recovered if no one is using it
    * Leave these here to give syseventd time to write the output file which we read
    * later.
    *
    * due to the way sysevent dataMgr and triggerMgr share trigger id
    * the cancellation of the options should take place after the event notification
    * has been cancelled. This doesnt have any system implications because syseventd
    * will do the right thing either way, but it does allow the data tuple to be
    * garbage collected.
    */
   char tstr[256];
   int flags = strtoll("0x00000000", NULL, 16);
   snprintf(tstr, sizeof(tstr), "%s-%s", srv_name, "start"); 
   sysevent_set_options(sysevent_fd, sysevent_token, tstr, flags);

   snprintf(tstr, sizeof(tstr), "%s-%s", srv_name, "stop"); 
   sysevent_set_options(sysevent_fd, sysevent_token, tstr, flags);

   snprintf(tstr, sizeof(tstr), "%s-%s", srv_name, "restart"); 
   sysevent_set_options(sysevent_fd, sysevent_token, tstr, flags);

   char default_str[256];
   snprintf(default_str, sizeof(default_str), "%s-status", srv_name);
   sysevent_set(sysevent_fd, sysevent_token, default_str, "stopped", 0);
   sysevent_close(sysevent_fd, sysevent_token);
   return(0);
}

/*
 * Procedure     : sm_register
 * Purpose       : Register a service's handler for activation upon default events
 *                 and upon custom events
 * Parameters    :
 *   srv_name        : The name of the service registering
 *   default_handler : The path/file to the handler for default events
 *   custom          : Information about the event and handlers using the format:
 *                        event_name | path/file to handler to be activated | 
 *                        sysevent activation flags or NULL | sysevent tuple flags or NULL |
 *                        any extra parameters to be given to the handler upon activation
 *                     Note that extra parameters can be a syscfg runtime value (eg $wan_proto),
 *                     a sysevent runtime value (eg @current_wan_ipaddr) or a constant.
 * Return Value  : 
 *   0                : success
 *   <0               : some failure
 */
int sm_register (const char* srv_name, const char* default_handler, const char** custom)
{
   ulog_init();
   /*
    * These are well known values for reaching syseventd
    */
   snprintf(sysevent_ip, sizeof(sysevent_ip), "127.0.0.1");
   sysevent_port = SE_SERVER_WELL_KNOWN_PORT;
   sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
   /*CID 56644: Improper use of negative value*/
   if (sysevent_fd < 0)
   {
       ulogf(ULOG_SERVICE, UL_INFO, "sysevent_fd can't be negative\n");
       return (-1);
   }

   char default_str[1024];
   if (NULL != default_handler) {
      /*
       * register for the default events service-start, service-stop, service-restart
       * default events have the tuple flag event set (0x00000002), so that the notification
       * occurs even if there is no change to the tuple value (resetting to the same value)
       */
      snprintf(default_str, sizeof(default_str), 
                "%s-%s|%s|NULL|0x00000002", srv_name, "start", default_handler); 
      sm_register_one_event(srv_name, default_str);

      snprintf(default_str, sizeof(default_str), 
                "%s-%s|%s|NULL|0x00000002", srv_name, "stop", default_handler); 
      sm_register_one_event(srv_name, default_str);

      snprintf(default_str, sizeof(default_str), 
                "%s-%s|%s|NULL|0x00000002", srv_name, "restart", default_handler); 
      sm_register_one_event(srv_name, default_str);
   }

   /*
    * register custom events
    */
   if (NULL != custom) {
      int i = 0;
      while (NULL != custom[i]) {
         sm_register_one_event(srv_name, custom[i]);
         i++;
      }
   }

   /*
    * set the service status as stopped
    */
   snprintf(default_str, sizeof(default_str), "%s-status", srv_name);
   sysevent_set(sysevent_fd, sysevent_token, default_str, "stopped", 0);

   sysevent_close(sysevent_fd, sysevent_token);
   return(0);
}
