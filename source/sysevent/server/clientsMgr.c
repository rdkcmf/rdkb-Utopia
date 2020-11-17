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
  ================================================================
                       clientsMgr.c
  
  This file contains the logic for a database of currently connected
  clients. Clients can be uniquely identified across time using the
  client id (a monotonically increasing value), as well as uniquely
  identified among via the file descriptor through which we communicate
  to the client.

  Author : mark enright  
  ================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "syseventd.h"
#include "clientsMgr.h"

static int CLI_MGR_inited = 0;

clients_t global_clients = { PTHREAD_MUTEX_INITIALIZER, 0, 0, NULL };

#define TOKEN_START 1
// next id to give to a client
static token_t global_next_id = TOKEN_START;


#ifdef SE_SERVER_CODE_DEBUG
/*
 * Procedure     : print_a_client_t 
 * Purpose       : print an element of type a_client_t
 * Parameters    :
 *    c             : A pointer to the a_client_t structure to print
 * Return Code   :
 *    0             : OK
 */
static int print_a_client_t(a_client_t *c)
{
   printf("   +----------------------------------------+\n");
   printf("   | used          : %d\n", c->used);
   if (c->used) {
      printf("   | name          : %s\n", c->name ? c->name : "" );
      printf("   | id            : %x\n", c->id);
      printf("   | fd            : %d\n", c->fd);
      printf("   | notifications : %d\n", c->notifications);
      printf("   | errors        : %d\n", c->errors);
   }
   printf("   +----------------------------------------+\n");
   return(0);
}

/*
 * Procedure     : print_clients_t 
 * Purpose       : print all elements in the clients_t
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int print_clients_t(void)
{
   if (NULL == global_clients.clients) {
      return(0);
   }
   printf("+----------------------------------------+\n");
   printf("| num cur clients : %d\n", global_clients.num_cur_clients);
   printf("| max cur clients : %d\n", global_clients.max_cur_clients);
   unsigned int i;
   for (i=0; i<global_clients.max_cur_clients; i++) {
      print_a_client_t(&(global_clients.clients[i]));
   }
   printf("+----------------------------------------+\n");
   return(0);
}
#endif //SE_SERVER_CODE_DEBUG 

/*
 * Procedure     : init_a_client
 * Purpose       : init an element of type a_client_t
 * Parameters    :
 *    c             : A pointer to the a_client_t structure to init
 * Return Code   :
 */
static void init_a_client(a_client_t *c)
{
   c->name[0]       = '\0';
   c->id            = TOKEN_NULL;
   c->fd            = -1;
   c->notifications = 0;
   c->errors        =  0;
   c->used          =  0;
   c->isData        =  0;
}

/*
 * Procedure     : free_a_client
 * Purpose       : free an element of type a_client_t
 * Parameters    :
 *    c             : A pointer to the a_client_t structure to free
 * Return Code   :
 *    0             : OK
 */
static int free_a_client(a_client_t *c)
{
   init_a_client(c);
   return(0);
}

/*
 * Procedure     : expand_clients_table
 * Purpose       : This procedure dynamically expands the global_clients table
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *   <0             : some error. Table is not expanded
 * Notes        : This must be called while locked
 */
static int expand_clients_table(void)
{
   // expand the clients table if necessary and init new memory to NULL
   if (global_clients.max_cur_clients <= global_clients.num_cur_clients) {
      int orig_bytes = global_clients.max_cur_clients * sizeof(a_client_t);

      if (0 == NUM_CLIENTS_IN_ALLOCATED_BLOCK) {
         int size =  orig_bytes+sizeof(a_client_t);
         global_clients.clients = 
            (a_client_t *)sysevent_realloc(global_clients.clients, size, __FILE__, __LINE__);
         if (NULL == global_clients.clients) {
            return(ERR_ALLOC_MEM);
         } else {
            init_a_client(&(global_clients.clients[global_clients.num_cur_clients]));
            global_clients.max_cur_clients++;
         }
      } else {
         int size =  orig_bytes+((sizeof(a_client_t)*NUM_CLIENTS_IN_ALLOCATED_BLOCK));
         global_clients.clients = (a_client_t *)sysevent_realloc(global_clients.clients,size, __FILE__, __LINE__);
        if (NULL == global_clients.clients) {
           return(ERR_ALLOC_MEM);
        }

        int i;
        int previous_num = global_clients.max_cur_clients;
        for (i = 0; i<NUM_CLIENTS_IN_ALLOCATED_BLOCK; i++) {
           init_a_client(&(global_clients.clients[previous_num+i]));
        }
         global_clients.max_cur_clients += NUM_CLIENTS_IN_ALLOCATED_BLOCK;
      }
   }

   if (NULL == global_clients.clients) {
      return(-1);
   }
   return(0);
}

/*
 * Procedure     : get_new_client_structure
 * Purpose       : Find an open a_client_t within the table of clients, or 
 *                 grow the table of clients. Return a pointer to the 
 *                 data structure.
 * Parameters    :
 * Return Code   :
 *    non NULL      : OK
 *    NULL          : some error. 
 * Notes         : This must be called locked
 */
static a_client_t *get_new_client_structure(void)
{
   if (global_clients.num_cur_clients < global_clients.max_cur_clients) {
      unsigned int i;
      for (i = 0; i < global_clients.max_cur_clients; i++) {
         if (0 == (global_clients.clients[i]).used) {
            global_clients.clients[i].used = 1;
            global_clients.num_cur_clients++;
            return(&(global_clients.clients[i]));
         } 
      }
      // if we go here then something is wrong
      SE_INC_LOG(CLIENT_MGR,
                 printf("in get_new_client_structure: Our data structure seems corrupt\n");
      )
   } else {
      if (0 != expand_clients_table()) {
         return(NULL);
      } else {
         return(get_new_client_structure());
      }
   }
   // some error
   return(NULL);
}

/*
 * Procedure     : CLI_MGR_remove_client_by_fd
 * Purpose       : Remove a client from the table of clients
 *                 The client is identified by the file descriptor
 *                 that we receive messages from.
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 *    id            : The client id, if known
 *    force         : 1 if client data needs to be removed even if id doesnt match CLI_MGR data
 * Return Code   :
 *    0             : Success
 *    <0            : some error
 */
int CLI_MGR_remove_client_by_fd (const int fd, const token_t id, const int force)
{
   if (!CLI_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (fd == (global_clients.clients[i]).fd  && 
             0 != force ? 1 : id == global_clients.clients[i].id ) {
            SE_INC_LOG(CLIENT_MGR,
               a_client_t *client = &(global_clients.clients[i]);
               printf("Removing Client %s id:%x (fd: %d)\n", client->name, client->id, fd);
            )
            // if the client has registered for notifications, then remove the registration
            if (global_clients.clients[i].notifications) {
               TRIGGER_MGR_remove_notification_message_actions(global_clients.clients[i].id);
            }
            free_a_client(&(global_clients.clients[i]));
            global_clients.num_cur_clients--; 
            close(fd);

            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
            )
            pthread_mutex_unlock(&global_clients.mutex);
            return(0);
         }
      }
   }

   // client not found
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex);
   return(ERR_UNKNOWN_CLIENT);
}

/*
 * Procedure     : CLI_MGR_id2fd
 * Purpose       : Given a client id return the fd of that client
 * Parameters    :
 *    id            : The client it of the client
 * Return Code   :
 *    >0            : The fd that the client listens on
 *     0            : client id not found
 *    <0            : Some error
 */
int CLI_MGR_id2fd (token_t id)
{
   if (!CLI_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (id == (global_clients.clients[i]).id) {
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
            )
            pthread_mutex_unlock(&global_clients.mutex); 
            return((global_clients.clients[i]).fd);
         }
      }
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex); 
   return(0);
}

/*
 * Procedure     : CLI_MGR_fd2id
 * Purpose       : Given a file descriptor return the corresponding client id
 * Parameters    :
 *    fd            : A file descriptor
 * Return Code   :
 *    TOKEN_INVALID : Not a valid client
 *    token_t       : Valid client
 * NOTE          :
 *    Due to the multi threaded nature of syseventd, you cannot guarantee anything about
 *    the client or its validity after this call ends. Don't use the return token
 */
token_t CLI_MGR_fd2id (const int fd)
{
   if (!CLI_MGR_inited) {
      return(TOKEN_INVALID);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (fd == (global_clients.clients[i]).fd) {
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
            )
            pthread_mutex_unlock(&global_clients.mutex); 
            return((global_clients.clients[i]).id);
         }
      }
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex); 
   return(0);
}

/*
 * Procedure     : CLI_MGR_handle_client_error_by_fd
 * Purpose       : Increment the number of errors for a client
 *                 The client is identified by the file descriptor
 *                 that we receive messages from.
 *                 If the number of errors surpasses a threshold, then force close 
 *                 the client
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 * Return Code   :
 *    0    : errors incremented 
 *    1    : client forcibly disconnected
 *    <0   : error
 */
int CLI_MGR_handle_client_error_by_fd (int fd)
{
   if (!CLI_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   // fd 0 can't be used
   if (0 == fd) {
      SE_INC_LOG(CLIENT_MGR,
         printf("Notice: Received request to mark fd 0 as error. Ignored.\n");
      )
      return(0);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   int rc = 0;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (fd == (global_clients.clients[i]).fd) {
            (global_clients.clients[i]).errors++;
            if (MAX_ERRORS_BEFORE_DISCONNECTION <= (global_clients.clients[i]).errors) {
               SE_INC_LOG(LISTENER,
                  printf("Client %s on fd %d is acting strangely. Disconnecting it\n", 
                          NULL==(global_clients.clients[i]).name ? "Unknown" : (global_clients.clients[i]).name, fd);
               )
               free_a_client(&(global_clients.clients[i]));
               global_clients.num_cur_clients--;
               close(fd);
               rc = 1;
            }
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
            )
            pthread_mutex_unlock(&global_clients.mutex); 
            return(rc);
         }
      }
   }

   // client not found
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex); 
   return(0);
}

/*
 * Procedure     : CLI_MGR_add_notification_to_client_by_fd
 * Purpose       : Increment the number of notifications that client has registered for
 *                 The client is identified by the file descriptor
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 * Return Code   :
 *    0    : 
 */
int CLI_MGR_add_notification_to_client_by_fd (int fd)
{
   if (!CLI_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   // fd 0 can't be used
   if (0 == fd) {
      SE_INC_LOG(CLIENT_MGR,
         printf("Notice: Received request to add notification to fd 0. Ignored.\n");
      )
      return(0);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (fd == (global_clients.clients[i]).fd) {
            (global_clients.clients[i]).notifications++;
            break;
         }
      }
   }

   SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex);
   return(0);
}

/*
 * Procedure     : CLI_MGR_clear_client_error_by_fd
 * Purpose       : Clear the number of errors for a client
 *                 The client is identified by the file descriptor
 *                 that we receive messages from.
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 * Return Code   :
 *    0
 */
int CLI_MGR_clear_client_error_by_fd (int fd)
{
   if (!CLI_MGR_inited) {
      return(ERR_NOT_INITED);
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: clients\n", id);
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: clients\n", id);
   )

   unsigned int i;
   for (i = 0; i < global_clients.max_cur_clients; i++) {
      if (0 != (global_clients.clients[i]).used) {
         if (fd == (global_clients.clients[i]).fd) {
           (global_clients.clients[i]).errors = 0;;
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: clients\n", id);
            )
            pthread_mutex_unlock(&global_clients.mutex); 
            return(0);
         }
      }
   }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: clients\n", id);
   )
   pthread_mutex_unlock(&global_clients.mutex); 
   return(0);
}
/*
 * Procedure     : CLI_MGR_new_client
 * Purpose       : Add a new client to the database of clients 
 * Parameters    :
 *    name          : A printable name assigned by the client
 *    fp            : The connection id for communication with this client
 * Return Code   :
 *    non NULL      : OK
 *    NULL          : some error.
 */
a_client_t *CLI_MGR_new_client(const char *name, const int fd)
{
   if (!CLI_MGR_inited) {
      return(NULL);
   }
   SE_INC_LOG(MUTEX,
      printf("Attempting to get mutex: clients\n");
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      printf("Got mutex: clients\n");
   )

   a_client_t *new_client = get_new_client_structure();
   if (NULL == new_client) {
      SE_INC_LOG(MUTEX,
         printf("Releasing mutex: clients\n");
      )
      pthread_mutex_unlock(&global_clients.mutex);
      return(NULL);
   } else {
      if (NULL != name) {
         snprintf(new_client->name, CLIENT_NAME_SIZE,"%s", name);
         new_client->name[CLIENT_NAME_SIZE-1] = '\0';
      } else {
         snprintf(new_client->name, CLIENT_NAME_SIZE,"%s", "anonymous");
         new_client->name[CLIENT_NAME_SIZE-1] = '\0';
      }
      new_client->fd   = fd;
      token_t client_id = global_next_id;
      global_next_id++;
      if ((token_t)TOKEN_INVALID == global_next_id || TOKEN_NULL == global_next_id) {
         global_next_id = TOKEN_START;
      }
      new_client->id   = client_id;
      SE_INC_LOG(CLIENT_MGR,
         printf("Adding Client %s id:%x (fd: %d)\n", new_client->name, new_client->id, new_client->fd);
      )

      SE_INC_LOG(MUTEX,
         printf("Releasing mutex: clients\n");
      )
      pthread_mutex_unlock(&global_clients.mutex);
      return(new_client);
   }
}

/*
 * Procedure     : CLI_MGR_init_clients_table
 * Purpose       : Initialize a table of clients
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int CLI_MGR_init_clients_table(void)
{   
   global_clients.clients         = NULL;
   global_clients.num_cur_clients = 0;
   global_clients.max_cur_clients = 0;
   // initialize next id to give to client
   global_next_id = TOKEN_START;
   CLI_MGR_inited = 1;
   return(0);
}

/*
 * Procedure     : CLI_MGR_deinit_clients_table
 * Purpose       : Uninitialize a table of clients
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int CLI_MGR_deinit_clients_table()
{   

   unsigned int i;
   SE_INC_LOG(MUTEX,
      printf("Attempting to get mutex: clients\n");
   )
   pthread_mutex_lock(&global_clients.mutex);
   SE_INC_LOG(MUTEX,
      printf("Got mutex: clients\n");
   )

   CLI_MGR_inited = 0;
   for (i=0; i<global_clients.max_cur_clients; i++) {
      if ((global_clients.clients)[i].used) {
         free_a_client(&(global_clients.clients[i]));
      }
   }

   global_clients.num_cur_clients = 0;
   global_clients.max_cur_clients = 0;
   sysevent_free((void **)&(global_clients.clients), __FILE__, __LINE__);
   global_clients.clients         = NULL;
   // reset next id to give to client
   global_next_id = TOKEN_START;


   SE_INC_LOG(MUTEX,
      printf("Releasing mutex: clients\n");
   )
   pthread_mutex_unlock(&global_clients.mutex);
   return(0);
}
