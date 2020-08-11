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

#ifndef __DATA_MGR_H_
#define __DATA_MGR_H_

#define MIMIC_BROADCOM_RC 1
#include "triggerMgr.h"

#ifdef MIMIC_BROADCOM_RC
int  DATA_MGR_commit();
#endif

/*
 * data_element_t
 *
 * A data elements (name/value) and the trigger id of this data element
 *
 * Fields:
 *    used              : An indication of whether the data_element is
 *                        used or empty
 *    source            : The original source of this set
 *    tid               : The transaction id associated with this element
 *    trigger_id        : The id of the trigger associated with this element, 
 *                         0 is no trigger
 *    options           : A bitfield describing options. Note that
 *                        options are also kept in triggerMgr
 *    name              : The name of the data element
 *    value             : The current value of the data element
 */
typedef struct {
   int                   used;
   int                   source;
   int                   tid;
   int                   trigger_id;
   tuple_flag_t          options;
   char                  *name;
   char                  *value;
   int                  value_length;
} data_element_t;

/*
 * data_element_list_t
 *
 * A list of data_element_t
 *
 * Fields:
 *   mutex             : The mutex protecting this data structure
 *   max_elements      : The maximum number of data elements that can be in the list
 *   num_elements      : The number of data elements currently in the list
 *   elements          : The list of data elements
 */
typedef struct {
   pthread_mutex_t  mutex;
   unsigned int     max_elements;
   unsigned int     num_elements;
   data_element_t   *elements;
} data_element_list_t;


/*
 * Procedure     : DATA_MGR_get
 * Purpose       : Get the value of a particular data item
 * Parameters    :
 *     name          : The data item to retrieve
 *     value_buf     : The buffer to copy the value in
 *     buf_size      : On input the number of bytes in value_buf
 *                     On output the number of bytes copied into value_buf
 *                     subject to the Notes below
 * Return Value  :
 *     NULL         : No such data item found, or NULL is the data item value
 *    !NULL         : Pointer to the buffer containing the value
 * Notes         :
 *   If the value_buf is too small then the value WILL be truncated.
 *   There will always be a '\0' at the end of the value_buf string
 *   If the return value of buf_size is >= the input size of buf_size, then
 *   the value_buf contains a truncated string. The string will be untruncated
 *   only if outgoing buf_size < incoming buf_size
 */
char *DATA_MGR_get(char *name, char *value_buf, int *buf_size);
char *DATA_MGR_get_bin(char *name, char *value_buf, int *buf_size);

/*
 * Procedure     : DATA_MGR_set
 * Purpose       : Set the value of a particular data item and if the value has changed
 *                 then execute all actions for that trigger
 * Parameters    :
 *     name          : The name of the data item to set value
 *     value         : The value to set the data item to
 *     source        : The original source of this set
 *     tid           : Transaction id for this set
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_set(char *name, char *value, int source, int tid);
int DATA_MGR_set_bin(char *name, char *value, int value_length, int source, int tid);

/*
 * Procedure     : DATA_MGR_set_unique
 * Purpose       : Create a unique tuple using name as a seed and set its value
 * Parameters    :
 *     name          : The preamble name of the data item to create and set value
 *     value         : The value to set the data item to
 *     uname_buf     : The buffer to copy the unique name in
 *     buf_size      : On input the number of bytes in name_buf
 *                     On output the number of bytes copied into name_buf
 *                     subject to the Notes below
 * Return Value  : Pointer to the buffer containing the assigned unique name
 * Notes         :
 *   If the value_buf is too small then the value WILL be truncated.
 *   There will always be a '\0' at the end of the name_buf string
 *   If the return value of buf_size is >= the input size of buf_size, then
 *   the name_buf contains a truncated string. The string will be untruncated
 *   only if outgoing buf_size < incoming buf_size
 */
char *DATA_MGR_set_unique(char *name, char *value, char *uname_buf, int *buf_size);

/*
 * Procedure     : DATA_MGR_get_unique
 * Purpose       : Get the value of the next tuple in a namespace 
 * Parameters    :
 *     name          : The namespace
 *     iterator      : An iterator for within the namespace
 *     sub_buf       : The buffer to copy the unique name of the subject
 *     sub_size      : On input the number of bytes in sub_buf. On output the number
 *                     of copied bytes
 *     value_buf     : The buffer to copy the value in
 *     value_size      : On input the number of bytes in value_buf
 *                     On output the number of bytes copied into value_buf
 *                     subject to the Notes below
 * Return Value  :
 *     NULL         : No such data item found, or NULL is the data item value
 *    !NULL         : Pointer to the buffer containing the value
 * Return Value  : Pointer to the buffer containing the assigned unique name
 * Notes         :
 *   If the value_buf is too small then the value WILL be truncated.
 *   There will always be a '\0' at the end of the name_buf string
 *   If the return value of buf_size is >= the input size of buf_size, then
 *   the name_buf contains a truncated string. The string will be untruncated
 *   only if outgoing buf_size < incoming buf_size
 */
char *DATA_MGR_get_unique(char *name, unsigned int *iterator, char *sub_buf, unsigned int *sub_size, char *value_buf, unsigned int *value_size);

/*
 * Procedure     : DATA_MGR_del_unique
 * Purpose       : Delete one element from a unique namespace
 * Parameters    :
 *     name          : The namespace
 *     iterator      : An iterator for within the namespace of the element to delete

 * Return Value  :
 *     0
 */
int DATA_MGR_del_unique(char *name, unsigned int iterator);

/*
 * Procedure     : DATA_MGR_get_next_iterator
 * Purpose       : Given a namespace and an iterator, return the next iterator
 * Parameters    :
 *     name          : The namespace
 *     iterator      : An iterator for within the namespace

 * Return Value  :
 *     0
 */
int DATA_MGR_get_next_iterator(char *name, unsigned int *iterator);

/*
 * Procedure     : DATA_MGR_set_options
 * Purpose       : Set the option flags of a particular data item
 * Parameters    :
 *     name          : The name of the data item to set value
 *     flags         : The flag values of the data item
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_set_options(char *name, tuple_flag_t flags);

/*
 * Procedure     : DATA_MGR_get_runtime_values
 * Purpose       : assign an argument vector to a global argv
 *                 making string substitutions wherever original
 *                 argument starts with $
 * Parameters   :
 *   in_argv       : The original argument vector
 * Return Code  :
 *   NULL          : An error
 *   !NULL         : the argument vector replacement
 * Notes        :
 *   The caller must NOT free any of the memory associated with the returned argument
 *   It will be freed as required by the data manager
 *   This procedure should be called with the data manager locked
 */
char **DATA_MGR_get_runtime_values (char **in_argv);

/*
 * Procedure     : DATA_MGR_set_async_external_executable
 * Purpose       : Set an async notification on a data element which calls an 
 *                 external executable when a data variable changes value
 * Parameters    :
 *     name          : The name of the data item to add a async notification to
 *     owner         : owner of the async notification
 *     action_flags  : Flags to apply to the action
 *     action        : The path and filename of the action to call when the data element changes value
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
int DATA_MGR_set_async_external_executable(char *name, token_t owner, action_flag_t action_flags, char *action, char **args, int *trigger_id, int *action_id);

/*
 * Procedure     : DATA_MGR_set_async_message_notification
 * Purpose       : Set an async notification on a data element which sends a message
 *                 to the connected client when a data variable changes value
 * Parameters    :
 *     name          : The name of the data item to add a async notification to
 *     owner         : owner of the async notification
 *     action_flags  : Flags to apply to the action
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_set_async_message_notification(char *name, token_t owner, action_flag_t action_flags, int *trigger_id, int *action_id);

/*
 * Procedure     : DATA_MGR_remove_async_notification
 * Purpose       : remove an async notification on a data element
 * Parameters    :
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 *     owner         : owner of the async notification
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_remove_async_notification(int trigger_id, int action_id, const token_t owner);

/*
 * Procedure     : DATA_MGR_init
 * Purpose       : Initialize the DATA MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int DATA_MGR_init(void);

/*
 * Procedure     : DATA_MGR_deinit
 * Purpose       : Uninitialize the DATA MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int DATA_MGR_deinit(void);

/*
 * Procedure     : DATA_MGR_show
 * Purpose       : Print the value of all data items known to syseventd
 * Parameters    :
 *    file           : name of the file to dump the values to
 * Return Value  :
 *     0
 */
int DATA_MGR_show(char *file);

#endif   // __DATA_MGR_H_
