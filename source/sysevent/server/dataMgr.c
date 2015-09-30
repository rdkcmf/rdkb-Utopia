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
                       dataMgr.c
  
 This is a database of tuples composed of <name  value flags>,
 as well as the utilities to utilize the tuples.
 DataMgr is used to keep runtime tuples and to monitor changes to the
 state of the tuples. If the value changes, then dataMgr will 
 notify triggerMgr.

 DataMgr is the repository of information about tuples and therefore
 is heavily involved in all tuple set/get. 

 DataMgr is also used to create an argument list based on current
 tuple values and/or current syscfg values. This argument list 
 is created on behalf of an event handler and will be used as the
 arguments to a call to the event handler in response to a trigger.

 In otherwords, a change in the value of a tuple can trigger an event.
 If there is a handler registered for the event which uses variable
 runtime arguments is its invokation, then when the worker thread
 is ready to invoke the handler it will request the dataMgr to prepare
 the argv.

  Author : mark enright  
  ================================================================
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctype.h>   // for isspace
#include "syseventd.h"
#include "dataMgr.h"
#ifdef USE_SYSCFG
#include <syscfg/syscfg.h>
#endif
#include "ulog/ulog.h"

// data manager inited 
static int DATA_MGR_inited = 0;

// file global data structure
static data_element_list_t global_data_elements = { PTHREAD_MUTEX_INITIALIZER, 0, 0, NULL };

// unique names alway have the following string immediately after the
// non-unique portion of the name
#define UNIQUE_DELIM "-_!"

// monotonoically increasing counter for unique namespaces
static unsigned int unique_counter = 0;

// symbol to indicate preserving order in unique namespaces.
// this wastes memory, but is useful if you want to iterate
// through a namespace in the same order that elements where added
// to it

#define UNIQUE_FORCE_ORDER '!'

/*
 ===================================================================
                 utilities for data_element_t
 ===================================================================
*/

/*
 * Procedure     : init_data_element_t
 * Purpose       : Initialize a data_element_t to an unused state
 * Parameters    :
 *    ta            : A pointer to the data_element_t to init
 * Return Code   :
 *    0
 */
static int init_data_element_t(data_element_t *de)
{
   de->used           = 0;
   de->source         = 0;
   de->tid            = 0;
   de->trigger_id     = 0;
   de->name           = NULL;
   de->value          = NULL;
   de->options        = TUPLE_FLAG_NONE;
   return(0);
}

/*
 * Procedure     : free_data_element_t
 * Purpose       : Free a data element
 * Parameters    :
 *    element       : The data_element_t to free
 * Return Value  :
 *   0
 */
static int free_data_element_t(data_element_t *element)
{
   if (NULL == element || 0 == element->used) {
      return(0);
   }
   if (NULL != element->name) {
      sysevent_free(&(element->name), __FILE__, __LINE__);
   }

   if (NULL != element->value) {
      sysevent_free(&(element->value), __FILE__, __LINE__);
   }
   init_data_element_t(element);
   return(0);
}

/*
 * Procedure     : free_data_elements
 * Purpose       : Free all data elements
 * Parameters    :
 * Return Value  :
 *   0
 */

static int free_data_elements(void)
{
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   unsigned int i;
   if (NULL == global_data_elements.elements) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(0);
   }
   for (i = 0; i<global_data_elements.max_elements; i++) {
      if (0 != ((global_data_elements.elements)[i]).used) {
         free_data_element_t(&((global_data_elements.elements)[i]));
      }
   }
   sysevent_free(&(global_data_elements.elements), __FILE__, __LINE__);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}

/*
 * Procedure     : show_all_data_elements
 * Purpose       : print the tuples to a file
 * Parameters    :
 *     fp          : An open file pointer
 * Return Value  :
 *     none
 */
 static int show_all_data_elements(FILE *fp)
{
   unsigned int i;
   for (i=0; i<global_data_elements.max_elements; i++) {
      data_element_t *cur_element;

      cur_element = &((global_data_elements.elements)[i]);

      if (0 != cur_element->used && NULL != cur_element->name) {
         fprintf(fp, "< %-55s %-50s 0x%x  0x%x %x >\n", 
                              cur_element->name,
                              cur_element->value,
                              cur_element->options,
                              cur_element->source,
                              cur_element->tid);
      }
   }
   return(0);
}

/*
 * Procedure     : find_existing_data_element
 * Purpose       : Returns a data_element with a certain name if it already exists
 * Parameters    :
 *     name          : The name  of the data_element
 *     empty_slot    : The first empty slot in the data element array
 *                     which can be used by the caller to add a new element if desired
 *                     The empty slot is guaranteed ONLY IF UNDER MUTEX
 * Return Value  :
 *     The pointer to the data element if successfull
 *     else NULL
 * Notes         :
 *   This routine should be called with the DATA_MGR mutex locked
 *   This routine does a strcasecmp on name for finding existence
 */
static data_element_t *find_existing_data_element(const char *name, int *empty_slot)
{
   int             first_empty_entry = -1;
   data_element_t *de_ptr            = NULL;

   *empty_slot = -1;

   if (! DATA_MGR_inited) {
      return(NULL);
   }
   if (NULL == name) {
      return(NULL);
   }

   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   snprintf(local_name_buf, sizeof(local_name_buf), "%s", name);
   char *local_name = trim(local_name_buf);
   if (NULL == local_name) {
      return(NULL);
   }

   /*
    * Look for the name in already defined data elements.
    */
   unsigned int i;
   for (i=0; i<global_data_elements.max_elements; i++) {
      data_element_t *cur_element;

      cur_element = &((global_data_elements.elements)[i]);

      // keep track of an empty entry while we are looping
      // we may need this is no matching element is found
      if (-1 == first_empty_entry && 0 == cur_element->used) {
         first_empty_entry = i;
      }
      if (0 != cur_element->used && NULL != cur_element->name &&  0 == strcasecmp(local_name, cur_element->name)) {
         return(cur_element);
      }
   }

   *empty_slot = first_empty_entry;
   return(NULL);
}


/*
 * Procedure     : get_data_element
 * Purpose       : Returns a data_element with a certain name
 * Parameters    :
 *     name          : The name  of the data_element
 * Return Value  :
 *     The pointer to the data element if successfull
 * Notes         :
 *   This routine should be called with the DATA_MGR mutex locked
 *   This routine will MAKE a data element and assign the value NULL
 *   if no element of that name existed
 *   This routine does a strcasecmp on name for finding existence
 */
static data_element_t *get_data_element(const char *name) 
{
   int             first_empty_entry = -1;
   data_element_t *de_ptr            = NULL;

   if (! DATA_MGR_inited) {
      return(NULL);
   }
   if (NULL == name) {
      return(NULL);
   }

   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   snprintf(local_name_buf, sizeof(local_name_buf), "%s", name);
   char *local_name = trim(local_name_buf);
   if (NULL == local_name) {
      return(NULL);
   }

   data_element_t* cur_element = find_existing_data_element(local_name, &first_empty_entry);
   if (NULL != cur_element) {
      return(cur_element);
   }

   // The data element wasn't already created so we need to create one
   if (-1 != first_empty_entry) {
      SE_INC_LOG(DATA_MGR,
         printf("Reusing empty element %d\n", first_empty_entry);
      )
      de_ptr =  &((global_data_elements.elements)[first_empty_entry]);
   } else {
      int size = (global_data_elements.max_elements + 1) * sizeof(data_element_t );
      global_data_elements.elements =
                 (data_element_t *) sysevent_realloc (global_data_elements.elements, size, __FILE__, __LINE__);
      if (NULL == global_data_elements.elements) {
         return(NULL);
      } else {
         init_data_element_t(&(global_data_elements.elements)[global_data_elements.max_elements]);
         de_ptr =  &((global_data_elements.elements)[global_data_elements.max_elements]);
         (global_data_elements.max_elements)++;
      }
   }
   // take possession of the new data element
   if (NULL == de_ptr) {
      return(NULL);
   }
   de_ptr->used = 1;
   de_ptr->name = sysevent_strdup(local_name, __FILE__, __LINE__);
   if (NULL == de_ptr->name) {
      free_data_element_t(de_ptr);
      return(NULL);
   }
   de_ptr->value = NULL;
   (global_data_elements.num_elements)++;

   return(de_ptr);
}

/*
 * Procedure     : rm_data_element
 * Purpose       : Removes a data_element and if triggers
 *                 are set on the element removes them as well
 * Parameters    :
 *     element      :  A pointer to the  data_element
 * Return Value  :
 *     0
 *   !0
 */
static int rm_data_element(data_element_t *element)
{
   if (! DATA_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   if (0 == element->used) {
      return(0);
   }
   if (0 != element->trigger_id ) {
      TRIGGER_MGR_remove_trigger(element->trigger_id);
   }
   free_data_element_t(element);
   (global_data_elements.num_elements)--;
   return(0);
}


/*
 * Procedure     : set_unique_data_element
 * Purpose       : Returns a new data_element with a unique name
 *                 derived from the input name. We also call this 
 *                 a namespace
 * Parameters    :
 *     name          : The name  of the data_element to derive from
 * Return Value  :
 *     The pointer to the data element if successfull
 * Notes         :
 *   This routine should be called with the DATA_MGR mutex locked
 *   This routine will MAKE a data element and assign the value NULL
 *   if no element of that name existed
 *   This routine does a strcasecmp on name for finding existence
 *   Also note that if the first character of the name is ! then
 *   order of addition of elements to the namespace is preserved
 *   when iterating through them. This wastes memory of empty spaces
 *   in underlying arrays. If is nicer to just slot new elements 
 *   in whereever they fit
 */
static data_element_t *set_unique_data_element(const char *name) 
{
   int             first_empty_entry = -1;
   data_element_t *de_ptr            = NULL;
   
   if (! DATA_MGR_inited) {
      return(NULL);
   }
   if (NULL == name) {
      return(NULL);
   }
   
   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   snprintf(local_name_buf, sizeof(local_name_buf), "%s%s", name, UNIQUE_DELIM);
   char *local_name = trim(local_name_buf);
   if (NULL == local_name) {
      return(NULL);
   } 



   int preserve_order = (UNIQUE_FORCE_ORDER == local_name[0]) ? 1 : 0;
   int ordered_tuple_found = 0;
   int  local_name_len;
   local_name_len = strlen(local_name);       
   
   unsigned int i;
   for (i=0; i<global_data_elements.max_elements; i++) {
      data_element_t *cur_element;

      cur_element = &((global_data_elements.elements)[i]);
 
     // keep track of an empty entry while we are looping
      // we may need this is no matching element is found
      if (-1 == first_empty_entry && 0 == cur_element->used) {
         first_empty_entry = i;
      }

      if (preserve_order  &&
          0 != cur_element->used && NULL != cur_element->name && 
          0 == strncasecmp(local_name, cur_element->name, local_name_len)) {
         ordered_tuple_found = 1;
      }
   }

   // if a entry for the pool exists and the pool is maintaining order, then we cannot insert this
   // new tuple into the middle of the list. It must be appended.
   if (ordered_tuple_found) {
      first_empty_entry = 1;
   }

   if (-1 != first_empty_entry) {
      SE_INC_LOG(DATA_MGR,
         printf("Reusing empty element %d\n", first_empty_entry);
      )
      de_ptr =  &((global_data_elements.elements)[first_empty_entry]);
   } else {
      int size = (global_data_elements.max_elements + 1) * sizeof(data_element_t );
      global_data_elements.elements = 
                 (data_element_t *) sysevent_realloc (global_data_elements.elements, size, __FILE__, __LINE__);
      if (NULL == global_data_elements.elements) {
         return(NULL);
      } else {
         init_data_element_t(&(global_data_elements.elements)[global_data_elements.max_elements]);
         de_ptr =  &((global_data_elements.elements)[global_data_elements.max_elements]);
         (global_data_elements.max_elements)++;
      }
   }

   // take possession of the new data element
   if (NULL == de_ptr) {
      return(NULL);
   }
   de_ptr->used = 1;
   char unique_name_buf[512];
   unique_counter++;
   unsigned int obfuscator = (unique_counter + ((unique_counter*strlen(local_name))%0xffff) );
   snprintf(unique_name_buf, sizeof(unique_name_buf), "%s%x", local_name, obfuscator);
   
   de_ptr->name = sysevent_strdup(unique_name_buf, __FILE__, __LINE__);
   de_ptr->value = NULL;
   (global_data_elements.num_elements)++; 


   return(de_ptr);
}

/*
 * Procedure     : get_unique_data_element
 * Purpose       : Returns a data_element from a namespace using an iterator
 * Parameters    :
 *     name          : The name space
 *     interator     : The iterator
 * Return Value  :
 *     The pointer to the data element if successfull
 * Notes         :
 *   This routine should be called with the DATA_MGR mutex locked
 *   This routine will MAKE a data element and assign the value NULL
 *   if no element of that name existed
 *   This routine does a strcasecmp on name for finding existence
 *   This routine updates the iterator so it is pointing after the current element
 */
static data_element_t *get_unique_data_element(const char *name, unsigned int *iterator) 
{
   if (! DATA_MGR_inited) {
      return(NULL);
   }
   if (NULL == name) {
      return(NULL);
   }

   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   // The unique names are made up of an input_string followed by UNIQUE_DELIM followed by an integer
   snprintf(local_name_buf, sizeof(local_name_buf), "%s%s", name, UNIQUE_DELIM);
   char *local_name = trim(local_name_buf);
   if (NULL == local_name) {
      return(NULL);
   } 

   if (*iterator > global_data_elements.max_elements) {
      *iterator = 0;
   }
   
   unsigned int i;
   for (i=*iterator; i<global_data_elements.max_elements; i++) {
      data_element_t *cur_element = NULL;

      cur_element = &((global_data_elements.elements)[i]);
 
      if (0 != cur_element->used && NULL != cur_element->name &&  
          0 == strncasecmp(local_name, cur_element->name, strlen(local_name)) &&
          NULL != cur_element->value && '\0' != cur_element->value[0]) {
         *iterator = i+1;
         return(cur_element);
      }
   }

   *iterator=SYSEVENT_NULL_ITERATOR;
   return(NULL);
}

/*
 * Procedure     : get_next_unique_data_element_iterator
 * Purpose       : Fetches the next iterator from a namespace using a seed iterator
 * Parameters    :
 *     name          : The name space
 *     interator     : The seed iterator (initialize it to SYSEVENT_NULL_ITERATOR)
 * Return Value  :
 *     the iterator
 */
static int get_next_unique_data_element_iterator(const char *name, unsigned int iterator)
{
   if (! DATA_MGR_inited) {
      return(SYSEVENT_NULL_ITERATOR);
   }
   if (NULL == name) {
      return(SYSEVENT_NULL_ITERATOR);
   }

   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   snprintf(local_name_buf, sizeof(local_name_buf), "%s%s", name, UNIQUE_DELIM);
   char *local_name = trim(local_name_buf);

   data_element_t *cur_element = NULL;

   if (NULL == local_name) {
      return(SYSEVENT_NULL_ITERATOR);
   }

   if (global_data_elements.max_elements < iterator || SYSEVENT_NULL_ITERATOR == iterator) {
      cur_element = &((global_data_elements.elements)[0]);
      if (0 != cur_element->used && NULL != cur_element->name &&  
           0 == strncasecmp(local_name, cur_element->name, strlen(local_name))) {
         return(0);
      }
   }

   unsigned int i;
   for (i=iterator+1; i<global_data_elements.max_elements; i++) {
      cur_element = NULL;

      cur_element = &((global_data_elements.elements)[i]);

      if (0 != cur_element->used && NULL != cur_element->name &&  0 == strncasecmp(local_name, cur_element->name, strlen(local_name))) {
         return(i);
      }
   }

   // not found
   return(SYSEVENT_NULL_ITERATOR);
}

/*
 * Procedure     : del_unique_data_element_iterator
 * Purpose       : Deletes a unique data element from a namespace
 * Parameters    :
 *     name          : The name space
 *     interator     : The iterator pointing at the element to delete
 * Return Value  :
 *     0
 */
static int del_unique_data_element_iterator(const char *name, unsigned int iterator) 
{
   if (! DATA_MGR_inited) {
      return(0);
   }
   if (NULL == name) {
      return(0);
   }
   
   // just in case the parameter name is a static string
   // we will copy it to an automatic string. Otherwise
   // trim will cause a memory exception
   char local_name_buf[512];
   snprintf(local_name_buf, sizeof(local_name_buf), "%s%s", name, UNIQUE_DELIM);
   char *local_name = trim(local_name_buf);
   if (NULL == local_name) {
      return(0);
   } 

   if (SYSEVENT_NULL_ITERATOR == iterator) {
      iterator=0;
   }

   if (global_data_elements.max_elements < iterator) {
      return(0);
   }

   unsigned int i;
   for (i=iterator; i<global_data_elements.max_elements; i++) {
      data_element_t *cur_element = NULL;

      cur_element = &((global_data_elements.elements)[i]);

      /*
       * TODO :
       * do we need to consider the possiblity of a trigger set on this unique element????
       */
      if (0 != cur_element->used && NULL != cur_element->name &&  0 == strncasecmp(local_name, cur_element->name, strlen(local_name)) ) {
         rm_data_element(cur_element);
         return(0);
      }
   }

   return(0);
}

/*
==============================================================================
==============================================================================
*/
/*
 * Procedure     : substitute_runtime_arguments
 * Purpose       : Make string substitutions wherever original
 *                 argument starts with SYSCFG_NAMESPACE_SYMBOL or SYSEVENT_NAMESPACE_SYMBOL
 * Parameters   :
 *   in_argv       : The original argument vector
 * Return Code  :
 *   NULL          : An error
 *   !NULL         : the argument vector replacement failed
 * Note         :
 *   The argument vector entries MUST be alloced, because they WILL be freed here and re allocated
 */
static char **substitute_runtime_arguments(char **in_argv)
{
   if (NULL == in_argv) {
      return(NULL);
   }

   // Go through the argument list making value substitutions if necessary
   int i;
   for (i=0; i < TOO_MANY_ARGS && in_argv[i] != NULL; i++) {
      if (SYSEVENT_NAMESPACE_SYMBOL == in_argv[i][0]) {
         char value_buf[500];
         int  value_buf_size = sizeof(value_buf);
         char *value;
         char *substitute = &(in_argv[i][1]);

         // use find_existing_data_element instead of get_data_element so that
         // in the case of an element that doesnt exist, we dont allocate and immediately
         // deallocate it.
         int ignore;
         data_element_t *element = find_existing_data_element(substitute, &ignore);
         if (NULL != element && NULL != element->value) {
            snprintf(value_buf, value_buf_size, "%s", element->value);
            value = value_buf;
         } else {
            value = NULL;
         }

        /*
          * We know that the current in_argv has been malloced, so free it
          * before reassigning the value
          */
        if (NULL != in_argv[i]) {
            sysevent_free (&(in_argv[i]), __FILE__, __LINE__);
         }

         if (NULL == value) {
            in_argv[i] = sysevent_strdup("NULL", __FILE__, __LINE__);
         } else {
            in_argv[i] = sysevent_strdup(trim(value), __FILE__, __LINE__);
         }
      }

#ifdef USE_SYSCFG
       else if (SYSCFG_NAMESPACE_SYMBOL == in_argv[i][0]) {
         char *pre_substitute_str = in_argv[i];
         char value_buf[500];
         int  value_buf_size = sizeof(value_buf);
         char *value;
         char *substitute = &(in_argv[i][1]);

         int rc = syscfg_get(NULL, substitute, value_buf, value_buf_size);
         value = (0 == rc) ? value_buf : NULL;
         if (NULL == value) {
            in_argv[i] = sysevent_strdup("NULL", __FILE__, __LINE__);
         } else {
            in_argv[i] = sysevent_strdup(trim(value), __FILE__, __LINE__);
         }
         /*
          * We know that the current in_argv had been malloced, so free it
          */
         sysevent_free (&pre_substitute_str, __FILE__, __LINE__);
      }
#endif        // USE_SYSCFG

   }

   return(in_argv);
}

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
char *DATA_MGR_get(char *name, char *value_buf, int *buf_size)
{
   if (!DATA_MGR_inited) {
      return(NULL);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   // use find_existing_data_element instead of get_data_element so that
   // in the case of an element that doesnt exist, we dont allocate and immediately
   // deallocate it.
   int ignore;
   data_element_t *element = find_existing_data_element(name, &ignore);
   
   int value_buf_size = *buf_size;
   *buf_size          = 0;

   // it is possible that the tuple has no value.
   // in this case element->value is NULL
   // it is also possible that the tuple does not exist
   if (NULL == element || NULL == element->value) {
      value_buf[0] = '\0';
      *buf_size = 0;
   } else {
      // There is some legitimate data, so copy it to the provided buffer
      *buf_size = snprintf(value_buf, value_buf_size, "%s", element->value);
   } 

   if (NULL != element) {
      if (NULL == element->value && 0 == element->trigger_id && TUPLE_FLAG_NONE == element->options) {
         // the element is quite useless and memory for it can be
         // recovered. 
         rm_data_element(element);
      }
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(value_buf);
}

/*
 * Procedure     : DATA_MGR_show
 * Purpose       : Print the value of a all data item
 * Parameters    :
 *    file           : name of the file to dump the values to
 * Return Value  :
 *     0
 */
int DATA_MGR_show(char *file)
{
   if (!DATA_MGR_inited) {
      return(0);
   }

   FILE *fp = fopen(file, "w");
   if (NULL == fp) {
      return(0);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   show_all_data_elements(fp);
   fclose(fp);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}

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
 * Note that if the data item has a source/tid associated with it, and 
 * if that tid == tid, then no set is performed and 0 is returned.
 * if that tid = 0, then a tid is assigned
 * if that tid != tid, then the tid is overwritten by tid
 */
int DATA_MGR_set(char *name, char *value, int source, int tid)
{

   if (!DATA_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   char *local_value;
   char local_value_buf[512];

   if (NULL == value) {
      local_value = NULL;
   } else {
      // just in case the parameter value is a static string
      // we will copy it to an automatic string. Otherwise
      // trim will cause a memory exception
      snprintf(local_value_buf, sizeof(local_value_buf), "%s", value);
      local_value = trim(local_value_buf);

      if (NULL == local_value) {
         return(ERR_ALLOC_MEM);
      } 
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = get_data_element(name);

   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(ERR_SYSTEM);
   }

   /*
    * If the tuple is set write once read many, and is already set then
    * dont reset it
    */
   if (TUPLE_FLAG_WORM & element->options) {
      if (NULL != element->value && '\0' != (element->value)[0]) {
         SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: data_elements\n", id);
         )
         pthread_mutex_unlock(&global_data_elements.mutex);
         return(0);
      }
   }

   /*
    * If the tuple has a tid then it was previously seem on this node.
    * We need to determine if it is a looped message.
    * If the message came from this node (source is our source) then
    * if the tid is lower or equal to the current tid, then it must be old
    * If the message came from another source, then if the tid is equal to our saved tid
    * then we have already seen it.
    */
   if (0 != element->tid && 0 != tid) {
      if ( (source == daemon_node_id && element->tid >= tid) ||
           (element->source == source && element->tid == tid) ) { 
         SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: data_elements\n", id);
         )
         pthread_mutex_unlock(&global_data_elements.mutex);
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Loop detected by DATA_MGR. < %s , %s, 0x%x %x>, Ignoring set.\n",
                    element->name, element->value, element->source, element->tid);
         SE_INC_LOG(ERROR,
            printf("Sysevent Loop detected by DATA_MGR. < %s , %s, 0x%x %x>, Ignoring set.\n",
                    element->name, element->value, element->source, element->tid);
            )
         return(ERR_DUPLICATE_MSG);
      }
   }
 
   int changed = 0;

   if (NULL == local_value && NULL == element->value) {
      if (TUPLE_FLAG_NONE == element->options) {
         SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: data_elements\n", id);
         )
         pthread_mutex_unlock(&global_data_elements.mutex);
         return(0);
      }
   } else if (NULL == local_value && NULL != element->value) {
      changed = 1; 
       sysevent_free(&(element->value), __FILE__, __LINE__);
   } else if (NULL != local_value && NULL == element->value) {
      changed = 1;
      element->value = sysevent_strdup(local_value, __FILE__, __LINE__);
      if (NULL == element->value) {
         SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: data_elements\n", id);
         )
         pthread_mutex_unlock(&global_data_elements.mutex);
         return(ERR_ALLOC_MEM);
      }
   } else {
      changed = strcasecmp(local_value, element->value);
      if (changed) {
          if (strlen(local_value) > strlen(element->value)) {
             sysevent_free(&(element->value), __FILE__,__LINE__);
             element->value = sysevent_strdup(local_value, __FILE__, __LINE__);
         } else {
            sprintf(element->value, "%s", local_value);
         }
         if (NULL == element->value) {
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: data_elements\n", id);
            )
            pthread_mutex_unlock(&global_data_elements.mutex);
            return(ERR_ALLOC_MEM);
         }
      }
   }   
   // if the tuple options are set for event then we execute notifications
   // even if there is no tuple value change
   if (TUPLE_FLAG_EVENT & element->options) {
      changed = 1;
   }

   // set or reset the tid if necessary
   if (0 == tid) {
      // this is a new set request. So create a new tid
      element->source = daemon_node_id; 
      element->tid=daemon_node_msg_num;
      daemon_node_msg_num++;
   } else {
      /* reset the tid to the specified value */
      element->source = source;
      element->tid    = tid;
   }

   if (changed && 0 != element->trigger_id) {
      // if there is  a trigger id then there is a trigger for this data element
      // we will start the trigger manager 
      TRIGGER_MGR_execute_trigger_actions(element->trigger_id, element->name, element->value, element->source, element->tid);
   }

   if (NULL == element->value && 0 == element->trigger_id && TUPLE_FLAG_NONE == element->options) {
      // the element is quite useless and memory for it can be
      // recovered. 
      rm_data_element(element);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}

/*
 * Procedure     : DATA_MGR_set_unique
 * Purpose       : Create a unique tuple based on the name of a data item and assign 
 *                 the value to it.
 *                 Return the unique name of the tuple
 * Parameters    :
 *     name          : The name of the data item to set value
 *     value         : The value to set the data item to
 *     uname_buf     : The buffer to copy the unique name in
 *     buf_size      : On input the number of bytes in name_buf
 *                     On output the number of bytes copied into name_buf
 * Return Value  :
 *     A pointer to the unique name
 */
char *DATA_MGR_set_unique(char *name, char *value, char *uname_buf, int *buf_size)
{

   if (!DATA_MGR_inited) {
      return(NULL);
   }

   char *local_value;
   char local_value_buf[512];
   char *local_name;
   char local_name_buf[512];

   if (NULL == name) {
      return(NULL);
   } else {
      // just in case the parameter value is a static string
      // we will copy it to an automatic string. Otherwise
      // trim will cause a memory exception
      snprintf(local_name_buf, sizeof(local_name_buf), "%s", name);
      local_name = trim(local_name_buf);

      if (NULL == local_name) {
         return(NULL);
      } 
   }

   if (NULL == value) {
      local_value = NULL;
   } else {
      // just in case the parameter value is a static string
      // we will copy it to an automatic string. Otherwise
      // trim will cause a memory exception
      snprintf(local_value_buf, sizeof(local_value_buf), "%s", value);
      local_value = trim(local_value_buf);

      if (NULL == local_value) {
         return(NULL);
      } 
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = set_unique_data_element(local_name);
   
   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(NULL);
   }

   element->value = sysevent_strdup(local_value, __FILE__, __LINE__);
   if (NULL == element->value) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(NULL);
   }

   *buf_size = snprintf(uname_buf, *buf_size, "%s", element->name);

   // we dont need to check triggers because this MUST be a newly created tuple

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(uname_buf);
}

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
char *DATA_MGR_get_unique(char *name, unsigned int *iterator, char *sub_buf, unsigned int *sub_size, char *value_buf, unsigned int *value_size)
{

   if (!DATA_MGR_inited) {
      return(NULL);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = get_unique_data_element(name, iterator);

  
   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(NULL);
   }
  
   int sub_buf_size = *sub_size;
   *sub_buf             = 0;
   int value_buf_size = *value_size;
   *value_size          = 0;

   if (NULL == element->name) {
      sub_buf[0] = '\0';
      *sub_size = 0;
   } else {
      *sub_size = snprintf(sub_buf, sub_buf_size, "%s", element->name);
   }
   // it is possible that the tuple has no value.
   // in this case element->value is NULL
   if (NULL == element->value) {
      value_buf[0] = '\0';
      *value_size = 0;
   } else {
      // There is some legitimate data, so copy it to the provided buffer
      *value_size = snprintf(value_buf, value_buf_size, "%s", element->value);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(value_buf);
}

/*
 * Procedure     : DATA_MGR_del_unique
 * Purpose       : Delete one element from a unique namespace
 * Parameters    :
 *     name          : The namespace
 *     iterator      : An iterator for within the namespace of the element to delete

 * Return Value  :
 *     0
 */
int DATA_MGR_del_unique(char *name, unsigned int iterator)
{
   if (!DATA_MGR_inited) {
      return(0);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
 
   del_unique_data_element_iterator(name, iterator);
  
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}

/*
 * Procedure     : DATA_MGR_get_next_iterator
 * Purpose       : Given a namespace and an iterator, return the next iterator
 * Parameters    :
 *     name          : The namespace
 *     iterator      : An iterator for within the namespace 

 * Return Value  :
 *     0
 */
int DATA_MGR_get_next_iterator(char *name, unsigned int *iterator)
{

   if (!DATA_MGR_inited) {
      return(0);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   *iterator = get_next_unique_data_element_iterator(name, *iterator);
  
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}
  
/*
 * Procedure     : DATA_MGR_set_options
 * Purpose       : Set the options flag of a particular data item
 * Parameters    :
 *     name          : The name of the data item to set value
 *     flags         : The flags to set the tuple flags to
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_set_options(char *name, tuple_flag_t flags)
{
   if (!DATA_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = get_data_element(name);
   
   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(ERR_SYSTEM);
   }

   int trigger_id;
   TRIGGER_MGR_set_flags(element->trigger_id, flags, &trigger_id);
   element->options = flags;
   element->trigger_id = trigger_id;

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(0);
}

/*
 * Procedure     : DATA_MGR_get_runtime_values
 * Purpose       : make inline substitutions of arguments
 *                 making string substitutions wherever original
 *                 argument starts with SYSCFG_NAMESPACE_SYMBOL
 *                 or SYSEVENT_NAMESPACE_SYMBOL
 * Parameters   :
 *   in_argv       : The original argument vector
 * Return Code  :
 *   NULL          : An error
 *   !NULL         : the argument vector replacement
 * Notes        :
 *   The caller must free all of the memory associated with the changed argument vector
 */
char **DATA_MGR_get_runtime_values (char **in_argv)
{
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   char **ret = substitute_runtime_arguments(in_argv);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(ret);
}

/*
 * Procedure     : DATA_MGR_set_async_external_executable
 * Purpose       : Set an async notification on a data element which will call an
 *                 external executable when the value of a data variable changes
 * Parameters    :
 *     name          : The name of the data item to add a async notification to 
 *     owner         : owner of the async notification
 *     action_flags  : Flags to apply to the action
 *     action        : The path and filename of the action to call when the data element changes value
 *     args          : The arguments of the command to add to the action list
 *                     last argument is NULL
 *     trigger_id    : On return the id of the trigger
 *     action_id     : On return the id of the action
 * Return Value  :
 *     0            : Success
 *     !0           : Some error
 */
int DATA_MGR_set_async_external_executable(char *name, token_t owner, action_flag_t action_flags, char *action, char **args, int *trigger_id, int *action_id)
{
   *trigger_id  = *action_id = htonl(0);
   if (!DATA_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = get_data_element(name);
   
   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(ERR_SYSTEM);
   }

   int rc;
   rc = TRIGGER_MGR_add_executable_call_action(element->trigger_id, owner, action_flags, action, args, trigger_id, action_id );

   /*
    * Trigger is done, so add it to DATA MGR's data element
    */
   if (0 == rc) {
      element->trigger_id = *trigger_id;
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(rc);
}

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
int DATA_MGR_set_async_message_notification(char *name, token_t owner, action_flag_t action_flags, int *trigger_id, int *action_id)
{
   *trigger_id  = *action_id = htonl(0);
   if (!DATA_MGR_inited) {
      return(ERR_NOT_INITED);
   }

   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: data_elements\n", id);
   )
   pthread_mutex_lock(&global_data_elements.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: data_elements\n", id);
   )
   data_element_t *element = get_data_element(name);
   
   if (NULL == element) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: data_elements\n", id);
      )
      pthread_mutex_unlock(&global_data_elements.mutex);
      return(ERR_SYSTEM);
   }

   int rc;
   rc = TRIGGER_MGR_add_notification_message_action(element->trigger_id, owner, action_flags, trigger_id, action_id );

   /*
    * Trigger is done, so add it to DATA MGR's data element
    */
   if (0 == rc) {
      element->trigger_id = *trigger_id;
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: data_elements\n", id);
   )
   pthread_mutex_unlock(&global_data_elements.mutex);
   return(rc);
}

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
int DATA_MGR_remove_async_notification(int trigger_id, int action_id, const token_t owner)
{
   int rc = TRIGGER_MGR_remove_action(trigger_id, action_id, owner);
   return(rc);
}

/*
 * Procedure     : DATA_MGR_init
 * Purpose       : Initialize the DATA MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int DATA_MGR_init(void)
{
   global_data_elements.max_elements = 0;
   global_data_elements.num_elements = 0;
   global_data_elements.elements     = NULL;

   DATA_MGR_inited = 1;
   return(0);
}

/*
 * Procedure     : DATA_MGR_deinit
 * Purpose       : Uninitialize the DATA MGR
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int DATA_MGR_deinit(void)
{ 
   DATA_MGR_inited = 0;
   free_data_elements();
   return(0);
}
