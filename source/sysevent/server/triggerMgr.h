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

#ifndef __TRIGGER_MGR_H_
#define __TRIGGER_MGR_H_

#include <pthread.h>
#include "sysevent/sysevent.h"


/*
 ====================================================================
                           Typedefs

The structures trigger_list_t, trigger_t, and trigger_action_t are
all related:
A triggerlist_t contains pointers to an arbitrary number of trigger_t
and a trigger_t contains pointers to an arbitrary number of trigger_action_t.

Each of the above structures are designed to hold arbitrary numbers of
other structures. This allows them to grow as required, and in theory
to shrink as well (although shrinking is not currently supported since
it does not appear that there will be a need for this). 

 ====================================================================
 */
typedef enum {
   ACTION_TYPE_UNKNOWN,
   ACTION_TYPE_EXT_FUNCTION,
   ACTION_TYPE_MESSAGE
}  action_type_t;

/*
 * trigger_action_t
 *
 * An action to execute when a trigger hits
 *
 * Fields:
 *    used              : An indication of whether the action is 
 *                        used or empty
 *    owner             : The owner of the action
 *    action_flags      : The flags of the action 
 *    action_type       : The type of action
 *    action_id         : The id of the action
 *    action            : The path and name of the action
 *    argc              : The number of arguments
 *    argv              : The arguments
 */
typedef struct {
   int                   used;
   token_t               owner;
   action_flag_t         action_flags;
   action_type_t         action_type;
   int                   action_id;
   char                  *action;
   int                    argc;
   char                   **argv;
} trigger_action_t;

/*
 * trigger_t
 *
 * The list of triggers along with the actions associated with the trigger
 * A trigger is a trigger id along with a set of actions.
 *
 * Fields:
 *    used                : An indication of whether the action is 
 *                          used or empty
 *    trigger_id          : The id of the trigger
 *    max_actions         : The maximum number of actions that can be in the list
 *    num_actions         : The number of actions currently in the list
 *    next_action_id      : The next action_id to allocate for a new action
 *    trigger_actions     : Actions to execute when the trigger hits
 *    trigger_flags       : Flags to control trigger action execution
 */
typedef struct {
   int                   used;
   int                   trigger_id;
   unsigned int          max_actions;
   unsigned int          num_actions;
   int                   next_action_id;
   tuple_flag_t          trigger_flags;
   trigger_action_t      *trigger_actions;
} trigger_t;

/*
 * trigger_list_t
 *
 * A list of trigger and the notifications associated with them
 *
 * Fields:
 *   mutex             : The mutex protecting this data structure
 *   max_triggers      : The maximum number of triggers that can be in the list 
 *   num_triggers      : The number of triggers curerntly in the list
 *   triggers          : A list of triggers and notifications
 */
typedef struct {
   pthread_mutex_t  mutex;
   unsigned int     max_triggers;
   unsigned int     num_triggers;
   trigger_t        *trigger_list;
} trigger_list_t;

/*
 ====================================================================
                       FUNCTIONS

The TRIGGER_MGR is responsible for maintaining triggers and actions
to be applied when those triggers change state.

An action is a call that is executed when the trigger changes state.
An action is a function call along with the parameters to be passed
in that call. In general parameters are passed with exactly the same
value as was given when creating the action. However, if the parameter
begins with a $ then the value sent will be the current value of the
trigger for that value. If no such trigger exists yet, then "NULL" will
be sent as that parameter.


 ====================================================================
 */

#ifdef SE_SERVER_CODE_DEBUG
/*
 * Procedure     : TRIGGER_MGR_print_trigger_list_t
 * Purpose       : Print the global list of triggers
 * Parameters    :
 * Return Value  :
 *     0            : Success
 */
int TRIGGER_MGR_print_trigger_list_t(void);
#endif

/*
 * Procedure     : TRIGGER_MGR_set_flags
 * Purpose       : Set the flags on a trigger
 * Parameters    :
 *     target_id     : A trigger id if assigned or 0
 *     flags         : The flags to set to
 *     trigger_id    : On return, The trigger id to set the flag
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int TRIGGER_MGR_set_flags(int target_id, tuple_flag_t flags, int *trigger_id);

/*
 * Procedure     : TRIGGER_MGR_remove_actions
 * Purpose       : Remove all actions owned by an owner for a given trigger
 * Parameters    :
 *     trigger_id    : The trigger id given when the action was added
 *     action_id     : The action id given when the action was added
 *     owner         : owner of the trigger action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Note        :
 *    In order to find the appropriate trigger, the trigger_id must match the trigger_id that was returned
 *    when the action was added, AND the action_id must match the action_id that was given when the
 *    action was added.
 *    owner is not currently used, but it is included in the call for future use if needed
 */
int TRIGGER_MGR_remove_action(int trigger_id, int action_id, const token_t owner);

/*
 * Procedure     : TRIGGER_MGR_remove_trigger
 * Purpose       : Remove all actions owned by a trigger and remove the trigger
 * Parameters    : 
 *     trigger_id    : The trigger id of the trigger to remove
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */    
int TRIGGER_MGR_remove_trigger(int trigger_id);

/*
 * Procedure     : TRIGGER_MGR_add_executable_call_action
 * Purpose       : Add an action to call an external executable to be executed when a trigger value changes
 * Parameters    :
 *     target_id     : The trigger_id to which to add this action.
 *                     0 means a new trigger should be assigned
 *     owner         : owner of the trigger action
 *     action_flags  : Flags to apply to this action
 *     action        : The path and filename of the action to call when the trigger changes value
 *     args          : The arguments of the command to add to the action list
 *                     The arguments are expected to be in the form
 *                     arg[0]   = path and filename of executable
 *                     arg[1-x] = arguments to send to executable
 *                     last argument is NULL
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int TRIGGER_MGR_add_executable_call_action(int target_id, const token_t owner, action_flag_t action_flags, char *action, char **args, int *trigger_id, int *action_id);

/*
 * Procedure     : TRIGGER_MGR_add_notification_message_action
 * Purpose       : Add an action to send a message to be sent when a trigger value changes
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
 */
int TRIGGER_MGR_add_notification_message_action(int target_id, const token_t owner, action_flag_t action_flags, int *trigger_id, int *action_id);

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
int TRIGGER_MGR_remove_notification_message_actions(const token_t owner);

/*
 * Procedure     : TRIGGER_MGR_execute_trigger_actions
 * Purpose       : Execute all actions set for a trigger
 * Parameters    :
 *     trigger_id    : The trigger id of the trigger upon which to execute actions
 *     name          : The name of the data tuple that the trigger is on
 *     value         : The value of the data tuple that the trigger is on
 *     source        : The original source of this set
 *     tid           : A Transaction id for notification messages

 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int TRIGGER_MGR_execute_trigger_actions(const int trigger_id, const char* const name, const char* const value, const int source, const int tid);
int TRIGGER_MGR_execute_trigger_actions_data(const int trigger_id, const char* const name, const char* const value, const int value_length, const int source, const int tid);

/*
 * Procedure     : TRIGGER_MGR_get_cloned_action
 * Purpose       : Find the action given a trigger_id action_id
 * Parameters    :
 *     trigger_id    : The trigger id of the trigger
 *     action_id     : The action_id of the action
 *     in_action     : Space to put the cloned action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 * Notes : Caller must free the action using TRIGGER_MGR_free_cloned_action
 */
int TRIGGER_MGR_get_cloned_action(int trigger_id, int action_id, trigger_action_t *in_action);

/*
 * Procedure     : TRIGGER_MGR_free_cloned_action
 * Purpose       : Given an action free it
 * Parameters    :
 *     action        : A pointer to a trigger_action_t clone to free
 * Return Value  :
 *    0                  : success
 */
int TRIGGER_MGR_free_cloned_action(trigger_action_t *action);

/*
 * Procedure     : TRIGGER_MGR_init
 * Purpose       : Initialize the Trigger Manager
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int TRIGGER_MGR_init(void);

/*
 * Procedure     : TRIGGER_MGR_deinit
 * Purpose       : Uninitialize the Trigger Manager
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int TRIGGER_MGR_deinit(void);


#endif   // __TRIGGER_MGR_H_
