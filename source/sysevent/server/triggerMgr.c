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
 * Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 */

 /*
  ================================================================
                       triggerMgr.c
  
  TriggerMgr is responsible for coordinating the sending of events
  to registered handlers.

  There are two types of event handlers:
  1) Process based
  2) Executable based

  A process based event handler has a current connection established,
  and is known to the ClientsMgr. Events to process based handlers
  are se_notification_msg containing the <name value> of the tuple
  that initiated the event.

  An executable based event handler has a registered callback.
  It is not known to the ClientsMgr, but it is known to the TriggerMgr
  and the triggerMgr maintains information about the path/name to the
  callback executable, as well as the arguments with which to call
  the executable. Events to executable based event handlers will be 
  organized by the triggerMgr, although the resolution of some argument
  values will be left for the worker thread/dataMgr.

   TriggerMgr also keeps track of whether the event handlers need to be
   executed in serial order, or whether they can execute in parallel.
 

  Author : mark enright  
  ================================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace
#include <arpa/inet.h> // for htonl
#include "triggerMgr.h"

#include <fcntl.h>
#include <unistd.h>
#include "syseventd.h"
#include "libsysevent_internal.h"

static int TRIGGER_MGR_inited = 0;
static int  next_trigger_id   = 1;  // dont assign 0

static trigger_list_t  global_triggerlist = { PTHREAD_MUTEX_INITIALIZER, 0, 0, NULL };
/*
=======================================================================
Utilities to manipulate the list of actions that is associated with
a trigger.
=======================================================================
*/
/*
 * Procedure     : init_trigger_action_t
 * Purpose       : Initialize a trigger_action_t to an unused state
 * Parameters    :
 *    ta            : A pointer to the trigger_action_t to init
 * Return Code   :
 *    0
 */
static int init_trigger_action_t(trigger_action_t *ta)
{
   ta->used         = 0;
   ta->owner        = TOKEN_INVALID;
   ta->action_type  = ACTION_TYPE_UNKNOWN;
   ta->action_id    = 0;
   ta->action       = NULL;
   ta->action_flags = 0;
   ta->argc         = 0;
   ta->argv         = NULL;
   return(0);
}

/*
 * Procedure     : init_trigger_t
 * Purpose       : Initialize a trigger_t to an unused state
 * Parameters    :
 *    tr            : A pointer to the trigger_t to init
 * Return Code   :
 *    0
 */
static int init_trigger_t(trigger_t *tr)
{
   tr->used             = 0;
   tr->trigger_id       = 0;
   tr->max_actions      = 0;
   tr->num_actions      = 0;
   tr->next_action_id   = 1;   // dont give out 0 as an action_id
   tr->trigger_actions  = NULL;
   tr->trigger_flags    = TUPLE_FLAG_NONE;
   return(0);
}

/*
 * Procedure     : init_global_trigger_list
 * Purpose       : Initialize the trigger_list_t to an unused state
 * Parameters    :
 * Return Code   :
 *    0
 */
static int init_global_trigger_list(void)
{
   global_triggerlist.max_triggers     = 0;
   global_triggerlist.num_triggers     = 0;
   global_triggerlist.trigger_list     = NULL;
   return(0);
}

/*
 * Procedure     : free_trigger_action_t
 * Purpose       : Recover all allocated memory used by a trigger_action_t 
 * Parameters    :
 *    ta            : A pointer to the trigger_action_t to free
 * Return Code   :
 *    0
 */
static int free_trigger_action_t(trigger_action_t *ta)
{
   int i;

   if (NULL == ta || 0 == ta->used) {
      return(0);
   }

   ta->used = 0;

   if (ACTION_TYPE_EXT_FUNCTION == ta->action_type) {
      // Free the arguments in the argv
      int num_args = ta->argc;
      if (NULL != ta->argv) {
         for (i=0; i<num_args; i++) {
            if (NULL != ta->argv[i]) {
               sysevent_free(&(ta->argv[i]), __FILE__, __LINE__);
            }
         }
         // Free the argument vector
         sysevent_free(&(ta->argv), __FILE__, __LINE__);
      }

      // Free the action
      if (NULL != ta->action) {
         sysevent_free(&(ta->action), __FILE__, __LINE__);
      }
   }

   init_trigger_action_t(ta);
   return(0);
}

/*
 * Procedure     : free_trigger_t
 * Purpose       : Recover all allocated memory used by  a trigger_t
 * Parameters    :
 *    tr            : A pointer to the trigger_t to free
 * Return Code   :
 *    0
 */
static int free_trigger_t(trigger_t *tr)
{
   int i;

   if (NULL == tr || 0 == tr->used) {
      return(0);
   }

   // Free each trigger_action_t that the trigger has
   int num_actions = tr->max_actions;
   if (NULL != tr->trigger_actions) {
      for (i=0; i<num_actions; i++) {
         if (0 != (((tr->trigger_actions)[i]).used)) {
            free_trigger_action_t(&(tr->trigger_actions[i]));
         }
      }

      // Free the trigger actions array
      sysevent_free(&(tr->trigger_actions), __FILE__, __LINE__);
   }

   // Reinitialize
   init_trigger_t(tr);
   return(0);
}

/*
 * Procedure     : free_global_trigger_list
 * Purpose       : Recover all allocated memory used by a trigger_list_t
 * Parameters    :
 * Return Code   :
 *    0
 */
static int free_global_trigger_list(void)
{
   // Free each of the triggers 
   if (NULL != global_triggerlist.trigger_list) {
      unsigned int i;
      for (i=0; i<global_triggerlist.max_triggers; i++) {
         if (0 != ((global_triggerlist.trigger_list)[i]).used) {
            free_trigger_t(&((global_triggerlist.trigger_list)[i]));
         }
      }
 
      // Free the trigger array
      sysevent_free(&(global_triggerlist.trigger_list), __FILE__, __LINE__);
   }

   // Reinitialize
   init_global_trigger_list();
   return(0);
}

/*
 * Procedure     : find_trigger_by_trigger_id
 * Purpose       : find a trigger using the trigger_id
 * Parameters    :
 *   id              : the trigger_id to look for
 * Return Value  : 
 *   NULL            : trigger not found
 *  !NULL            : the trigger
 * Notes        :
 *   Should be called with trigger manager locked
 */
static trigger_t *find_trigger_by_trigger_id(int id)
{
   unsigned int i;
   for (i=0; i<global_triggerlist.max_triggers; i++) {
      if (((global_triggerlist.trigger_list)[i]).trigger_id == id) {
         return( &((global_triggerlist.trigger_list)[i]) );
      }
   }
   return(NULL);
}

/*
 * Procedure     : prepare_action_type_function_msg
 * Purpose       : prepare a msg of type ACTION_TYPE_EXT_FUNCTION
 * Parameters    :
 *    buffer        : buffer to put the msg in
 *    trigger_id    : The id of the trigger
 *    action        : The action 
 *    name          : The name of the tuple that caused the trigger
 *    value         : The value of the tuple that caused the trigger
 * Return Code   :
 *    0
 *   -1          : some error
 */
static int prepare_action_type_function_msg (se_buffer buffer, int trigger_id, trigger_action_t *action, 
                                             const char const *name, const char const *value)
{
   // figure out how much space the se_msg_strings will take
   int subbytes  = SE_string2size(name);
   int valbytes  = SE_string2size(value);

   // calculate the size of the se_send_notification_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_run_executable_msg) +
                       subbytes + valbytes - sizeof(void *);

   if (send_msg_size >= sizeof(se_buffer)) {
      return(ERR_MSG_TOO_LONG);
   }

   se_run_executable_msg   *send_msg_body;

   send_msg_body = (se_run_executable_msg *)
             SE_msg_prepare(buffer, sizeof(se_buffer), SE_MSG_RUN_EXTERNAL_EXECUTABLE, TOKEN_NULL);
   if (NULL != send_msg_body) {
      // prepare the message
      (send_msg_body->async_id).trigger_id = htonl(trigger_id);
      (send_msg_body->async_id).action_id  = htonl(action->action_id);
      send_msg_body->token_id = htonl(action->owner);
      send_msg_body->flags    = htonl(action->action_flags);
      int  remaining_buf_bytes;
      char *send_data_ptr = (char *)&(send_msg_body->data);
      remaining_buf_bytes = sizeof(se_buffer);
      remaining_buf_bytes -= sizeof(se_msg_hdr);
      remaining_buf_bytes -= sizeof(se_run_executable_msg);
      int strsize    = SE_msg_add_string(send_data_ptr,
                                         remaining_buf_bytes,
                                         name);
      if (0 == strsize) {
         return(ERR_CANNOT_SET_STRING);
      }
      remaining_buf_bytes -= strsize;
      send_data_ptr       += strsize;
      strsize              = SE_msg_add_string(send_data_ptr,
                                               remaining_buf_bytes,
                                               value);
      if (0 == strsize) {
         return(ERR_CANNOT_SET_STRING);
      }

   } else {
      return(-1);
   }
   return(0);
}

/*
 * Procedure     : prepare_action_type_message_msg
 * Purpose       : prepare a msg of type ACTION_TYPE_MESSAGE
 * Parameters    :
 *    buffer        : buffer to put the msg in
 *    trigger_id    : The id of the trigger
 *    action        : The action 
 *    name          : The name of the tuple that caused the trigger
 *    value         : The value of the tuple that caused the trigger
 *    source        : The source of the message
 *    tid           : Transaction ID for notification messages
 * Return Code   :
 *    0
 *   -1          : some error
 *   -2          : Dead peer detected and cleaned up
 * Note          : The buffer must be at least sizeof(se_buffer)
 */
static int prepare_action_type_message_msg (se_buffer buffer, const int trigger_id, trigger_action_t *action, const char const *name, const char const *value, const int source, const int tid)
{

   // if the action owner has disconnected, then there is no need to send this message
   // so check with the client manager
   int fd = CLI_MGR_id2fd(action->owner);
   
   if (0 == fd) {
      SE_INC_LOG(INFO,
         printf("Dead Peer %x Detected while preparing notification msg. Cleaning up stale notification request\n", 
                 action->owner);
      )
      // we are mutex locked so calling free_trigger_action is valid
      free_trigger_action_t(action);
      return(-2);
   }

   // figure out how much space the se_msg_strings will take
   int subbytes  = SE_string2size(name);
   int valbytes  = SE_string2size(value);

   // calculate the size of the se_send_notification_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_send_notification_msg) +
                       subbytes + valbytes - sizeof(void *);

   if (send_msg_size >= sizeof(se_buffer)) {
      return(ERR_MSG_TOO_LONG);
   }

   se_send_notification_msg *send_msg_body;
   send_msg_body = (se_send_notification_msg *)
             SE_msg_prepare(buffer, sizeof(se_buffer), SE_MSG_SEND_NOTIFICATION, TOKEN_NULL);
      if (NULL != send_msg_body) {
         // prepare the message
         send_msg_body->source = htonl(source);
         send_msg_body->tid = htonl(tid);
         int  remaining_buf_bytes;
         char *send_data_ptr = (char *)&(send_msg_body->data);
         remaining_buf_bytes = sizeof(se_buffer);
         remaining_buf_bytes -= sizeof(se_msg_hdr);
         remaining_buf_bytes -= sizeof(se_send_notification_msg);
         int strsize    = SE_msg_add_string(send_data_ptr,
                                            remaining_buf_bytes,
                                            name);
         if (0 == strsize) {
            return(ERR_CANNOT_SET_STRING);
         }
         remaining_buf_bytes -= strsize;
         send_data_ptr       += strsize;
         strsize              = SE_msg_add_string(send_data_ptr,
                                            remaining_buf_bytes,
                                            value);
         if (0 == strsize) {
            return(ERR_CANNOT_SET_STRING);
         }
         (send_msg_body->async_id).trigger_id = htonl(trigger_id);
         (send_msg_body->async_id).action_id  = htonl(action->action_id);
         send_msg_body->token_id = htonl(action->owner);
         send_msg_body->flags    = htonl(action->action_flags);
      } else {
         return(-1);
      }
   return(0);
}

/*
 * Procedure     : execute_trigger_actions
 * Purpose       : Execute all trigger actions for a trigger
 * Parameters    :
 *    tr            : A pointer to the trigger for whose actions to execute
 *    name          : The name of the data for the trigger
 *    value         : The value of the data when triggered
 *    source        : The source of the message
 *    tid           : Transaction id for notification messages
 * Return Code   :
 *    0
 *   -1    
 * Notes         :
 *   For safety this should be called while TRIGGER_MGR is locked, but it is
 *   going to take a while
 */
static int execute_trigger_actions(const trigger_t *tr, const char const *name, const char const *value, const int source, const int tid)
{
   if (NULL == tr->trigger_actions) {
      return(0);
   }
   if (0 == tr->num_actions) {
      return(0);
   }
   

   /*
    * Depending on the trigger_flags we will either
    * have each trigger action be sent to the worker thread immediately
    * or we will collect all of the actions in an array so that the 
    * worker will execute them serially
    */
   if (TUPLE_FLAG_SERIAL & tr->trigger_flags) {
      /*
       * prepare a list of messages, each of which describes
       * one work item. The only work items currently supported
       * is send_notification, and run_external msgs
       */
      se_buffer *list;
      /*
       * tr->num_actions indicates the number of actions registered for the trigger
       * num_actions indicates the number of valid actions encounted during processing
       * for example a Dead Peer detected will drop those actions
       */
      int        num_actions = 0;
      size_t     size        = (sizeof(se_buffer)* tr->num_actions);
      list  = (se_buffer *)sysevent_malloc(size, __FILE__, __LINE__);
      if (NULL == list) {
        return(-1);
      } else {
         unsigned int i;
         unsigned int idx = 0;
         for (i=0; i<tr->max_actions; i++) {
            trigger_action_t *action;
            action = &((tr->trigger_actions)[i]);
            if (0 != action->used) {
               se_buffer  *bufptr = (se_buffer *) (&(list[idx]));
               se_msg_hdr *msghdr = (se_msg_hdr *) (&(list[idx]));

               if (ACTION_TYPE_EXT_FUNCTION == action->action_type) {
                  int rc = prepare_action_type_function_msg(*bufptr, tr->trigger_id, action, name, value);
                  if (0 != rc) {
                     continue;
                  }
               } else if (ACTION_TYPE_MESSAGE == action->action_type) {
                  int rc = prepare_action_type_message_msg(*bufptr, tr->trigger_id, action, name, value, source, tid);
                  if (0 != rc) {
                     continue;
                  }
               } else {
                  SE_INC_LOG(TRIGGER_MGR,
                     printf("Unhandled action type %d\n", action->action_type);
                  )
                  continue;
            	}
                // we need to manually fixup the msg_hdr since we are not calling SE_msg_send
                SE_msg_hdr_mbytes_fixup(msghdr);
                num_actions++;
                idx++;
            }
         }
      }   

      // the ordered list has been prepared, now send it
      se_buffer            send_msg_buffer;
      se_run_serially_msg *send_msg_body;

      send_msg_body = (se_run_serially_msg *)
             SE_msg_prepare(send_msg_buffer, sizeof(send_msg_buffer), SE_MSG_EXECUTE_SERIALLY, TOKEN_NULL);
      if (NULL != send_msg_body) {
         // prepare the message
         send_msg_body->num_msgs = num_actions;
         send_msg_body->async_id.trigger_id = tr->trigger_id;
         // we can send a pointer because we KNOW that this is a message within this process
         send_msg_body->data     = list;
      } else {
         sysevent_free(&list, __FILE__, __LINE__);
         return(-1);
      }
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Attempting to get mutex: trigger_communication\n", id);
      )
      pthread_mutex_lock(&trigger_communication_mutex);
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Got mutex: trigger_communication\n", id);
      )
      int rc = SE_msg_send(trigger_communication_fd_writer_end, send_msg_buffer);
      if (0 != rc) {
         SE_INC_LOG(ERROR,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: TriggerMgr unable to send SE_MSG_EXECUTE_SERIALLY using fd %d\n",
               id, trigger_communication_fd_writer_end);
         )
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to threads using fd %d\n",
                   id, SE_print_mtype(SE_MSG_EXECUTE_SERIALLY), SE_MSG_EXECUTE_SERIALLY, trigger_communication_fd_writer_end);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(send_msg_buffer);
         )
      }

      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: trigger_communication\n", id);
      )
      pthread_mutex_unlock(&trigger_communication_mutex);
   } else {
      unsigned int i;
      for (i=0; i<tr->max_actions; i++) {
         trigger_action_t *action;
         action = &((tr->trigger_actions)[i]);
         if (0 != action->used) {
            if (ACTION_TYPE_EXT_FUNCTION == action->action_type) {
               if (0 != trigger_communication_fd_writer_end) {
                  se_buffer send_msg_buffer;
                  if (0 == prepare_action_type_function_msg(send_msg_buffer, tr->trigger_id, action, name, value)) {
                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Attempting to get mutex: trigger_communication\n", id);
                     )
                     pthread_mutex_lock(&trigger_communication_mutex);
                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Got mutex: trigger_communication\n", id);
                     )
                     int rc = SE_msg_send(trigger_communication_fd_writer_end, send_msg_buffer);
                     if (0 != rc) {
                        SE_INC_LOG(ERROR,
                           int id = thread_get_id(worker_data_key);
                           printf("Thread %d: TriggerMgr unable to send SE_MSG_SEND_NOTIFICATION, using fd %d\n",
                              id, trigger_communication_fd_writer_end);
                        )
                     } else {
                       SE_INC_LOG(MESSAGES,
                          int id = thread_get_id(worker_data_key);
                          printf("Thread %d: Sent %s (%d) to threads using fd %d\n",
                                 id, SE_print_mtype(SE_MSG_RUN_EXTERNAL_EXECUTABLE), SE_MSG_RUN_EXTERNAL_EXECUTABLE, trigger_communication_fd_writer_end);
                       )
                       SE_INC_LOG(MESSAGE_VERBOSE,
                           SE_print_message_hdr(send_msg_buffer);
                       )
                     }
                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Releasing mutex: trigger_communication\n", id);
                     )
                     pthread_mutex_unlock(&trigger_communication_mutex);
                  }
               }
            } else if (ACTION_TYPE_MESSAGE == action->action_type) {
               if (0 != trigger_communication_fd_writer_end) {
                  se_buffer send_msg_buffer;

                  if (0 == prepare_action_type_message_msg(send_msg_buffer, tr->trigger_id, action, name, value, source, tid)) {
                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Attempting to get mutex: trigger_communication\n", id);
                     )
                     pthread_mutex_lock(&trigger_communication_mutex);
                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Got mutex: trigger_communication\n", id);
                     )

                     int rc = SE_msg_send(trigger_communication_fd_writer_end, send_msg_buffer);
                     if (0 != rc) {
                        SE_INC_LOG(ERROR,
                           int id = thread_get_id(worker_data_key);
                           printf("Thread %d: TriggerMgr unable to send SE_MSG_SEND_NOTIFICATION, using fd %d\n",
                              id, trigger_communication_fd_writer_end);
                        )
                     } else {
                       SE_INC_LOG(MESSAGES,
                          int id = thread_get_id(worker_data_key);
                          printf("Thread %d: Sent %s (%d) to threads using fd %d\n",
                                 id, SE_print_mtype(SE_MSG_SEND_NOTIFICATION), SE_MSG_SEND_NOTIFICATION, trigger_communication_fd_writer_end);
                       )
                       SE_INC_LOG(MESSAGE_VERBOSE,
                           SE_print_message_hdr(send_msg_buffer);
                       )
                     }

                     SE_INC_LOG(MUTEX,
                        int id = thread_get_id(worker_data_key);
                        printf("Thread %d Releasing mutex: trigger_communication\n", id);
                     )
                     pthread_mutex_unlock(&trigger_communication_mutex);
                  }
               } else {
                  SE_INC_LOG(ERROR,
                     printf("prepare_action_type_message failed for <%s %s>\n", 
                               NULL == name ? "Unknown" : name, 
                               NULL == value ? "(null)" : value);
                  )
               }
            } else {
               SE_INC_LOG(TRIGGER_MGR,
                  printf("Unhandled action type %d\n", action->action_type);
               )
            }
         }
      }
   }
   return(0);
}

/*
 * Procedure     : provision_executable_call_action
 * Purpose       : Create a new trigger_action_t of type ACTION_TYPE_EXT_FUNCTION
 * Parameters    :
 *     new_action    : The action to provision
 *     owner         : The owner of the action
 *     flags         : Flags associated with this action
 *     action_id     : The id of the action
 *     action        : The path and filename of the action to call when the trigger changes value
 *     args          : The arguments of the command to add to the action list
 *                     The arguments are expected to be in the form
 *                     arg[0]   = path and filename of executable
 *                     arg[1-x] = arguments to send to executable
 *                     last argument is NULL
 * Return Value  :
 *   0               : Success
 *  !0               : Failure
 *
 * Notes         :
 *   It is the responsibility of the caller to free all of the action memory when done
 */
static int provision_executable_call_action(trigger_action_t *new_action, const token_t owner, action_flag_t flags, int action_id, char *action, char **args)
{
   int num_args = 0;

   // figure out how many arguments there are.
   // the last argument must be NULL
   if (NULL != args) {
      while (NULL != args[num_args]) {
         num_args++;
      }
   }
   if (TOO_MANY_ARGS <= num_args) {
      return(ERR_TOO_MANY_ARGUMENTS);
   }

   new_action->used         = 1;
   new_action->owner        = owner;
   // action_flags are additive. They can't be reset to 0
   new_action->action_flags = new_action->action_flags | flags;
   new_action->action_type  = ACTION_TYPE_EXT_FUNCTION;
   new_action->action_id    = action_id;
   new_action->argc         = 0;
   new_action->argv         = NULL;
   new_action->action       = NULL;

   // now provision the new action
   new_action->action   = sysevent_strdup(trim(action), __FILE__, __LINE__);
   if (NULL == new_action->action) {
      free_trigger_action_t(new_action);
      return(ERR_ALLOC_MEM);
   }

   // duplicate and save the argument list
   if (0 == num_args) {
      new_action->argc = 0;
      new_action->argv = NULL;
   } else {
      new_action->argv = (char **)sysevent_malloc((num_args + 1) * sizeof (char *), __FILE__, __LINE__);
      if (NULL == new_action->argv) {
         free_trigger_action_t(new_action);
         return(ERR_ALLOC_MEM);
      } else {
         new_action->argc = 0;
      }
      // now copy in the arguments
      int i;
      for (i=0; i<=num_args; i++) {
         new_action->argv[i] = NULL;
      }
      for (i=0; i<num_args; i++) {
         new_action->argv[i] = sysevent_strdup(trim(args[i]), __FILE__, __LINE__);
         if (NULL == new_action->argv[i]) {
            free_trigger_action_t(new_action);
            return(ERR_ALLOC_MEM);
         } else {
            (new_action->argc)++;
         }
      }
   }
   // The action is fully provisioned, so mark it used
   new_action->used = 1;
   return(0);
}

/*
 * Procedure     : provision_notification_action
 * Purpose       : Create a new trigger_action_t of type ACTION_TYPE_MESSAGE
 * Parameters    :
 *     new_action    : The action to provision
 *     owner         : The owner of the action
 *     flags         : Flags associated with the action
 *     action_id     : The id of the action
 * Return Value  :
 *   0               : Success
 *  !0               : Failure
 *
 */
static int provision_notification_action(trigger_action_t *new_action, const token_t owner, action_flag_t flags, int action_id)
{
   new_action->used         = 1;
   new_action->owner        = owner;
   // action flags are additive. They cant be reset to 0
   new_action->action_flags = new_action->action_flags | flags;
   new_action->action_type  = ACTION_TYPE_MESSAGE;
   new_action->action_id    = action_id;
   new_action->argc         = 0;
   new_action->argv         = NULL;
   new_action->action       = NULL;

   // there is no data to provision
   return(0);
}

/*
 * Procedure     : add_executable_call_action_to_trigger
 * Purpose       : Add an action to call an external executable to 
 *                 a trigger
 * Parameters    : 
 *     trigger       : The trigger to which to add the action
 *     owner         : The owner of the action
 *     action_flags  : Flags associated with this action
 *     action_id     : The action_id of the action
 *     action        : The path and filename of the action to call when the trigger changes value
 *     args          : The arguments of the command to add to the action list
 *                     last argument is NULL
 * Return Value  :
 *   0               : Success
 *  !0               : Error
 */
static int add_executable_call_action_to_trigger(trigger_t *trigger, token_t owner, action_flag_t action_flags, int action_id, char *action, char **args)
{
   int       cur_action_idx = -1;
   int       rc;

   /*
    * The list of trigger_actions is a dynamically allocated array which can grow but
    * not shrink. If the order of actions being executed is not order significant
    * then we can reuse an unused array element if it is available.
    * If the order of execution is serialized then order is significant
    * and we must grow the array and add the new action to the end.
    * Therefore there are 2 cases where we need to grow the array:
    *    trigger is set to TUPLE_FLAG_SERIAL and the last element is used, or
    *    trigger is not set to TUPLE_FLAG_SERIAL but there are no empty array elements
    */
   if ( (0 == trigger->max_actions)                   ||
       (trigger->max_actions == trigger->num_actions) ||
       ((TUPLE_FLAG_SERIAL & trigger->trigger_flags) && 0 != trigger->trigger_actions[trigger->max_actions-1].used) ) {
      int size = ((trigger->max_actions) + 1) * sizeof(trigger_action_t);
      if (NULL == (trigger->trigger_actions =
            (trigger_action_t *)sysevent_realloc(trigger->trigger_actions, size, __FILE__, __LINE__)) ) {
         return(ERR_ALLOC_MEM);
      } else {
         init_trigger_action_t(&(trigger->trigger_actions)[trigger->max_actions]);
         (trigger->max_actions)++;
         return(add_executable_call_action_to_trigger(trigger, owner, action_flags, action_id, action, args));
      }
   }

   /* 
    * There is room for another action. Add the action from the back. 
    * It will be added to execution order in the order of registration (for TUPLE_FLAG_SERIAL)
    * or will tend to be executed last (for ! TUPLE_FLAG_SERIAL)
    */
   int i;
   for (i=(trigger->max_actions-1); i>=0; i--) {
      if (0 == (trigger->trigger_actions)[i].used) {
         cur_action_idx = i;
         break;
      } 
   }
   if (-1 == cur_action_idx) {
      return(ERR_SYSTEM);
   }

   // cur_action_idx is the index to where to add the trigger_action_t
   rc = provision_executable_call_action(&(trigger->trigger_actions[cur_action_idx]), owner, action_flags, action_id, action, args);
   if (0 != rc) {
      return(ERR_ALLOC_MEM);
   } else {
      trigger->num_actions++;
   }
   return(0);
}

/*
 * Procedure     : add_notification_action_to_trigger
 * Purpose       : Add an action to send a message to a client when
 *                 a trigger activates
 * Parameters    : 
 *     trigger       : The trigger to which to add the action
 *     owner         : The owner of the action
 *     action_flags  : Flags associated with the action
 *     action_id     : The action_id of the action
 * Return Value  :
 *   0               : Success
 *  !0               : Error
 */
static int add_notification_action_to_trigger(trigger_t *trigger, token_t owner, action_flag_t action_flags, int action_id)
{
   int       cur_action_idx = -1;
   int       rc;

   /*
    * The list of trigger_actions is a dynamically allocated array which can grow but
    * not shrink. If the order of actions being executed is not order significant
    * then we can reuse an unused array element if it is available.
    * If the order of execution is serialized then order is significant
    * and we must grow the array and add the new action to the end.
    * Therefore there are 2 cases where we need to grow the array:
    *    trigger is set to TUPLE_FLAG_SERIAL and the last element is used, or
    *    trigger is not set to TUPLE_FLAG_SERIAL but there are no empty array elements
    */

   if ( (0 == trigger->max_actions)                   ||
       (trigger->max_actions == trigger->num_actions) ||
       ((TUPLE_FLAG_SERIAL & trigger->trigger_flags) && 0 != trigger->trigger_actions[trigger->max_actions-1].used) ) {
      int size = ((trigger->max_actions) + 1) * sizeof(trigger_action_t);
      if (NULL ==
         (trigger->trigger_actions =
            (trigger_action_t *)sysevent_realloc(trigger->trigger_actions, size, __FILE__, __LINE__)) ) {
         return(ERR_ALLOC_MEM);
      } else {
         init_trigger_action_t(&(trigger->trigger_actions)[trigger->max_actions]);
         (trigger->max_actions)++;
         return(add_notification_action_to_trigger(trigger, owner, action_flags, action_id));
      }
   }

   // There is room for another action, so add the action from the back so it will tend to be executed last
   int i;
   for (i=(trigger->max_actions-1); i>=0; i--) {
      if (0 == (trigger->trigger_actions)[i].used) {
         cur_action_idx = i;
         break;
      } 
   }
   if (-1 == cur_action_idx) {
      return(ERR_SYSTEM);
   }

   // cur_action_idx is the index to where to add the trigger_action_t
   rc = provision_notification_action(&(trigger->trigger_actions[cur_action_idx]), owner, action_flags, action_id);
   if (0 != rc) {
      return(ERR_ALLOC_MEM);
   } else {
      trigger->num_actions++;
   }
   return(0);
}

/*
 * Procedure     : get_trigger
 * Purpose       : Add an new trigger if none exists, and get it if it does
 * Parameters    : 
 *     trigger_id    : The trigger_id of a trigger to get, or 0 if a new trigger should be added
 * Return Value  :
 *     The pointer to the trigger if successfull
 *     NULL if some error
 * Notes         :
 *   This routine should be called with the TRIGGER_MGR mutex locked
 */
static trigger_t *get_trigger(int trigger_id)
{
   trigger_t *trigger;

   if (0 != trigger_id) {
      trigger = find_trigger_by_trigger_id(trigger_id);
      if (NULL != trigger) {
         return(trigger);
      }
   }

   if (global_triggerlist.max_triggers == global_triggerlist.num_triggers) {
      int size = (global_triggerlist.max_triggers + 1) * sizeof(trigger_t );
      global_triggerlist.trigger_list = 
           (trigger_t *) sysevent_realloc (global_triggerlist.trigger_list, size, __FILE__, __LINE__);
      if (NULL == global_triggerlist.trigger_list) {
         return(NULL);
      } else {
         init_trigger_t(&(global_triggerlist.trigger_list)[global_triggerlist.max_triggers]);
         (global_triggerlist.max_triggers)++;

      }
   }
   
   // we will take the first empty slot for our trigger

   unsigned int i;
   for (i=0; i<global_triggerlist.max_triggers; i++) {
      trigger_t *cur_trigger;

      cur_trigger = &((global_triggerlist.trigger_list)[i]);
         
      if (0 == cur_trigger->used) {
         // take possession of the new trigger
         cur_trigger->trigger_id = next_trigger_id;
         next_trigger_id++;
         if (0 == next_trigger_id) {
            next_trigger_id = 1;
         }
         cur_trigger->used = 1;
         (global_triggerlist.num_triggers)++;
         return(cur_trigger);
      }
   }
   return(NULL);
} 

#ifdef SE_SERVER_CODE_DEBUG
/*
 * Procedure     : print_trigger_action_t
 * Purpose       : Print a trigger_action_t
 * Parameters    : 
 *     tr            : The trigger_action_t to print
 * Return Value  :
 *     0            : Success
 */
static int print_trigger_action_t(trigger_action_t *tr)
{
   printf("   |  |  +------------ trigger_action_t -----------%p--+\n", tr);
   printf("   |  |  | used                : %d\n", tr->used);
   printf("   |  |  | owner               : %x\n", tr->owner);
   printf("   |  |  | action_type         : %d\n", tr->action_type);
   printf("   |  |  | action_id           : %d\n", tr->action_id);
   printf("   |  |  | action              : %p   %s\n", tr->action, tr->action);
   printf("   |  |  | num args            : %d\n", tr->argc);
   printf("   |  |  | args                : %p\n", tr->argv);
   int i;
   for (i=0; i<tr->argc; i++) {
   printf("   |  |  |    arg %d           : %p   %s\n", i, tr->argv[i], tr->argv[i]);
   }
   printf("   |  |  +----------------------------------------------------+\n");
   return(0);
}

/*
 * Procedure     : print_trigger_t
 * Purpose       : Print a trigger_t
 * Parameters    : 
 *     tr            : The trigger_t to print
 * Return Value  :
 *     0            : Success
 */
static int print_trigger_t(trigger_t *tr)
{
   printf("   |  +-------------------- trigger_t -------------%p--+\n", tr);
   printf("   |  | used                : %d\n", tr->used);
   printf("   |  | trigger_id          : %d\n", tr->trigger_id);
   printf("   |  | max_actions         : %d\n", tr->max_actions);
   printf("   |  | num_actions         : %d\n", tr->num_actions);
   printf("   |  | next_action_id      : %d\n", tr->next_action_id);
   printf("   |  | trigger_flags       : 0x%x\n", tr->trigger_flags);
   printf("   |  | trigger_actions     : %p\n", tr->trigger_actions);
   unsigned int i;
   for (i=0; i<tr->max_actions; i++) {
      print_trigger_action_t(&(tr->trigger_actions[i]));
   }
   printf("   |  +-------------------------------------------------------+\n");
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_print_trigger_list_t
 * Purpose       : Print the global trigger_list_t
 * Parameters    : 
 * Return Value  :
 *     0            : Success
 */
int TRIGGER_MGR_print_trigger_list_t(void)
{
   printf("   +----------------- trigger_list_t -------------------------+\n");
   printf("   | mutex               : Not Printed\n");
   printf("   | max_triggers        : %d\n", global_triggerlist.max_triggers);
   printf("   | num_triggers        : %d\n", global_triggerlist.num_triggers);
   printf("   | trigger_list        : %p\n", global_triggerlist.trigger_list);
   unsigned int i;
   for (i=0; i<global_triggerlist.max_triggers; i++) {
         print_trigger_t(&((global_triggerlist.trigger_list)[i]));
   }
   printf("   +----------------------------------------------------------+\n");
   return(0);
}
#endif // SE_SERVER_CODE_DEBUG

/*
 * Procedure     : TRIGGER_MGR_set_flags
 * Purpose       : Set the flags on a trigger
 * Parameters    :
 *     target_id     : A trigger id if assigned or 0
 *     flags         : The flags to set to
 *     trigger_id    : on return, The trigger id to set the flag
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Note          : This function is called by data manager while it
 *   has a mutex lock. DO NOT CALL ANY DATA_MGR functions
 */
int TRIGGER_MGR_set_flags(int target_id, tuple_flag_t flags, int *trigger_id)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   *trigger_id = 0;

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )


   trigger_t *trigger_ptr = get_trigger(target_id);

   if (NULL == trigger_ptr) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&(global_triggerlist.mutex));
      return(ERR_SYSTEM);
   } else {
      /*
       * if the flags are being set to TUPLE_FLAG_NONE then 
       * we can consider removing the trigger if it is not being used
       */
      if (TUPLE_FLAG_NONE == flags && 0 == trigger_ptr->num_actions) {
         trigger_ptr->trigger_flags = TUPLE_FLAG_NONE;
         free_trigger_t(trigger_ptr);
         *trigger_id                = 0;
      } else {
         trigger_ptr->trigger_flags = flags;
        *trigger_id                 = trigger_ptr->trigger_id;
      }
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&(global_triggerlist.mutex));
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_remove_action
 * Purpose       : Remove an action from a trigger
 * Parameters    :
 *     trigger_id    : The trigger id given when the action was added
 *     action_id     : The action id given when the action was added
 *     owner         : owner of the trigger action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Note        :
 *    In order to find the appropriate trigger, the trigger_id must match the trigger_id 
 *    that was returned  when the action was added, AND 
 *    the action_id must match the action_id that was given when the action was added
 */
int TRIGGER_MGR_remove_action(int trigger_id, int action_id, const token_t owner)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   if (NULL == global_triggerlist.trigger_list) {
      return(0);
   }

   trigger_t *trigger = NULL;
   
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )


   unsigned int i;
   for (i=0; i<global_triggerlist.max_triggers; i++) {
      if (((global_triggerlist.trigger_list)[i]).trigger_id == trigger_id) {
         trigger = &((global_triggerlist.trigger_list)[i]);
         break;
      }
   }
   if (NULL != trigger) {
      if (NULL != trigger->trigger_actions) {
         unsigned int j;
         for (j=0; j<trigger->max_actions; j++) {
            trigger_action_t *action;
            action = &((trigger->trigger_actions)[j]);
            if (0 != action->used && action_id == action->action_id) {
               free_trigger_action_t(action);
               (trigger->num_actions)--;
               break;
            } 
         }
      }
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&(global_triggerlist.mutex));
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_remove_trigger
 * Purpose       : Remove all actions owned by a trigger and remove the trigger
 * Parameters    :
 *     trigger_id    : The trigger id of the trigger to remove
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int TRIGGER_MGR_remove_trigger(int trigger_id)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   if (NULL == global_triggerlist.trigger_list) {
      return(0);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )
   trigger_t *trigger = find_trigger_by_trigger_id(trigger_id);

   if (NULL != trigger && 0 != trigger->used) {
      free_trigger_t(trigger);
      (global_triggerlist.num_triggers)--;
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&(global_triggerlist.mutex));
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_add_executable_call_action
 * Purpose       : Add an action to call an external executable 
 *                 which will be executed when a trigger value changes
 * Parameters    : 
 *     target_id     : The trigger_id to which to add this action.
 *                     0 means a new trigger should be assigned
 *     owner         : owner of the trigger action
 *     action_flags  : Flags to apply to this action
 *     action        : The path and filename of the action to call when the trigger changes value
 *     args          : The arguments of the command to add to the action list
 *                     last argument is NULL
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error    
 * Note          : This function is called by data manager while it
 *   has a mutex lock. DO NOT CALL ANY DATA_MGR functions
 */
int TRIGGER_MGR_add_executable_call_action(int target_id, const token_t owner, action_flag_t action_flags, char *action, char **args, int *trigger_id, int *action_id)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   if (NULL == action) {
      return(ERR_BAD_PARAMETER);
   }

   *trigger_id = *action_id = 0;

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )

   trigger_t *trigger_ptr = get_trigger(target_id);

   if (NULL == trigger_ptr) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&(global_triggerlist.mutex));
      return(ERR_SYSTEM);
   } else {
      int rc = add_executable_call_action_to_trigger(trigger_ptr, owner, action_flags, trigger_ptr->next_action_id, action, args);
      if(0 == rc) {
         *trigger_id = trigger_ptr->trigger_id;
         *action_id  = trigger_ptr->next_action_id;
      } 
      (trigger_ptr->next_action_id)++;
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&(global_triggerlist.mutex));
      return(rc);
   }
} 

/*
 * Procedure     : TRIGGER_MGR_add_notification_message_action
 * Purpose       : Add an action to call an external executable 
 *                 which will be executed when a trigger value changes
 * Parameters    : 
 *     target_id     : The trigger_id to which to add this action.
 *                     0 means a new trigger should be assigned
 *     owner         : owner of the trigger action
 *     action_flags  : Flags to apply to this action
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error    
 * Note          : This function is called by data manager while it
 *   has a mutex lock. DO NOT CALL ANY DATA_MGR functions
 */
int TRIGGER_MGR_add_notification_message_action(int target_id, const token_t owner, action_flag_t action_flags, int *trigger_id, int *action_id)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   *trigger_id = *action_id = 0;

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )

   trigger_t *trigger_ptr = get_trigger(target_id);

   if (NULL == trigger_ptr) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&(global_triggerlist.mutex));
      return(ERR_SYSTEM);
   } else {
      int rc = add_notification_action_to_trigger(trigger_ptr, owner, action_flags, trigger_ptr->next_action_id);
      if(0 == rc) {
         *trigger_id = trigger_ptr->trigger_id;
         *action_id  = trigger_ptr->next_action_id;
      } 
      (trigger_ptr->next_action_id)++;
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&(global_triggerlist.mutex));
      return(rc);
   }
} 

/*
 * Procedure     : TRIGGER_MGR_remove_notification_message_actions
 * Purpose       : Remove all actions to send a message owned by a particular owner
 * Parameters    :
 *     owner         : owner of the trigger action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Note : this only affect IPC notifcations (SE_MSG_NOTIFICATION)
 */
int TRIGGER_MGR_remove_notification_message_actions(const token_t owner)
{
   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   if (NULL == global_triggerlist.trigger_list) {
      return(0);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )

   unsigned int i;
   for (i=0; i<global_triggerlist.max_triggers; i++) {
      if ( ((global_triggerlist.trigger_list)[i]).used) {
         unsigned int      max_actions = ((global_triggerlist.trigger_list)[i]).max_actions;
         trigger_action_t *actions     = ((global_triggerlist.trigger_list)[i]).trigger_actions;
         unsigned int j;
         for (j=0 ; j<max_actions ; j++) {
            if (actions[j].used && owner == actions[j].owner && ACTION_TYPE_MESSAGE == actions[j].action_type) {
               trigger_action_t *cur_action = &(actions[j]);
               free_trigger_action_t(cur_action);
               ((global_triggerlist.trigger_list)[i]).num_actions--;
            }
         }
      }
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&(global_triggerlist.mutex));
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_execute_trigger_actions
 * Purpose       : Execute all actions set for a trigger
 * Parameters    :
 *     trigger_id    : The trigger id of the trigger upon which to execute actions
 *     name          : The name of the data tuple that the trigger is on
 *     value         : The value of the data tuple that the trigger is on
 *     source        : The source of the message
 *     tid           : A Transaction id for notification messages
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Note          : This function is called by data manager while it
 *   has a mutex lock. DO NOT CALL ANY DATA_MGR functions
 */
 int TRIGGER_MGR_execute_trigger_actions(const int trigger_id, const char const *name, const char const *value, const int source, int tid)
{

   if (!TRIGGER_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&global_triggerlist.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )

   trigger_t *trigger = find_trigger_by_trigger_id(trigger_id);
   if (NULL == trigger) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&global_triggerlist.mutex);
      return(0);
   }

   execute_trigger_actions(trigger, name, value, source, tid);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&global_triggerlist.mutex);
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_free_cloned_action
 * Purpose       : Given an action free it
 * Parameters    :
 *     action        : A pointer to a trigger_action_t clone to free
 * Return Value  :
 *    0                  : success
 */
int TRIGGER_MGR_free_cloned_action(trigger_action_t *action)
{
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&(global_triggerlist.mutex));
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: triggerlist\n", id);
   )

   free_trigger_action_t(action);

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&(global_triggerlist.mutex));
   return(0);
}

/*
 * Procedure     : clone_action
 * Purpose       : Given an action and a destination, clone the action into the dest
 * Parameters    :
 *     dest_action       : A pointer to the trigger_action_t dest
 *     src_action        : A pointer to a trigger_action_t to clone
 * Return Value  :
 *    0                  : success
 *   !0                  : failure
 */
static int clone_action(trigger_action_t *dest_action, const trigger_action_t const *src_action)
{
   dest_action->used         = src_action->used;
   dest_action->owner        = src_action->owner;
   dest_action->action_flags = src_action->action_flags;
   dest_action->action_type  = src_action->action_type;
   dest_action->action_id    = src_action->action_id;
   dest_action->argc         = src_action->argc;
   if (NULL == src_action->action) {
      dest_action->action = NULL;
   } else {
      dest_action->action = sysevent_strdup(src_action->action, __FILE__, __LINE__);
      if (NULL == dest_action->action) {
         return (-1);
      }
   }
   int num_args              = dest_action->argc;
   if (0 == num_args) {
      dest_action->argv = NULL;
   } else {
      dest_action->argv = (char **)sysevent_malloc((num_args + 1) * sizeof (char *), __FILE__, __LINE__);
      if (NULL == dest_action->argv) {
         return(-1);
      } else {
         // now copy in the arguments
         int i;
         for (i=0; i<num_args; i++) {
            if (NULL == src_action->argv[i]) {
               dest_action->argv[i] = NULL;
            } else {
               dest_action->argv[i] = sysevent_strdup(src_action->argv[i], __FILE__, __LINE__);
               if (NULL == dest_action->argv[i]) {
                  return(-1);
               }
            }
         }
         dest_action->argv[i] = NULL;
      }
   }
   return(0);
}


/*
 * Procedure     : TRIGGER_MGR_get_cloned_action
 * Purpose       : Find the action given a trigger_id action_id and clone it in a new data structure
 * Parameters    : 
 *     trigger_id    : The trigger id of the trigger
 *     action_id     : The action_id of the action
 *     in_action     : A pointer to a trigger_action_t to put the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int TRIGGER_MGR_get_cloned_action(int trigger_id, int action_id, trigger_action_t *in_action)
{

   if (!TRIGGER_MGR_inited) {
      return(-1);
   }
   if (NULL == in_action) {
      return(-1);
   }

   init_trigger_action_t(in_action);

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: triggerlist\n", id);
   )
   pthread_mutex_lock(&global_triggerlist.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d (gca) Got mutex: triggerlist\n", id);
   )

   trigger_t *trigger = find_trigger_by_trigger_id(trigger_id);
   if (NULL == trigger) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: triggerlist\n", id);
      )
      pthread_mutex_unlock(&global_triggerlist.mutex);
      return(-1);
   }
   unsigned int i;
   for (i=0; i<trigger->max_actions; i++) {
      trigger_action_t *action;
      action = &((trigger->trigger_actions)[i]);
      if (0 != action->used && action_id == action->action_id) {
         if (0 != clone_action(in_action, action)) {
            free_trigger_action_t(action);
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: triggerlist\n", id);
            )
            pthread_mutex_unlock(&global_triggerlist.mutex);
            return(-1);
         } else {
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d (gca) Releasing mutex: triggerlist\n", id);
            )
            pthread_mutex_unlock(&global_triggerlist.mutex);
            return(0);
         }
      }
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: triggerlist\n", id);
   )
   pthread_mutex_unlock(&global_triggerlist.mutex);
   return(-1);
}

/*
 * Procedure     : TRIGGER_MGR_init
 * Purpose       : Initialize the TRIGGER MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int TRIGGER_MGR_init(void)
{
   init_global_trigger_list();

   global_triggerlist.max_triggers     = 0;

   next_trigger_id    = 1;
   TRIGGER_MGR_inited = 1;
   return(0);
}

/*
 * Procedure     : TRIGGER_MGR_deinit
 * Purpose       : Uninitialize the TRIGGER MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int TRIGGER_MGR_deinit(void)
{
   TRIGGER_MGR_inited = 0;
   free_global_trigger_list();
   next_trigger_id     = 1;

   return(0);
}
