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

#ifndef __CLIENTS_MGR_H_
#define __CLIENTS_MGR_H_

#include <pthread.h>
#include "sysevent/sysevent.h"

// define the number of a_client_t elements to increase our dynamic
// clients array by each time
#define NUM_CLIENTS_IN_ALLOCATED_BLOCK  4

// define the maximum length of a client name
#define CLIENT_NAME_SIZE 15

/*
 * Typedef      : a_client_t
 * Purpose      : Hold information about one connected client
 * Fields       :
 *   used          : does this structure contain information
 *                   0 is unused, non-zeof means it contains 
 *                   information about some connected client
 *   id            : id assigned by the server for this client.
 *                   This is an opaque value to the client, but
 *                   the client must send this value when 
 *                   communicating with the server
 *   fd            : The file descriptor that the server uses
 *                   to communicate with this client
 *   notifications : The number of notifications registered by this client
 *   errors        : The number of errors detected on this client
 *   name          : A string which the client self assigned as
 *                   identification. Its value is only used
 *                   as human viewable info about the client.
 */
typedef struct {
   int       used;
   token_t   id;
   int       fd;
   int       notifications;
   int       errors;
   char      name[CLIENT_NAME_SIZE];
   int       isData;
} a_client_t;

/*
 * Typedef      : clients_t
 * Purpose      : Hold information about all connected clients
 * Fields       :
 *   mutex           :  The mutex protecting this data structure
 *   num_cur_clients :  The number of clients in array of clients
 *   max_cur_clients :  The maximum number of clients that can fit
 *                      in the array of clients as it is currently
 *                      sized.
 *   clients         :  A dynamically growable array of clients
 */
typedef struct {
    pthread_mutex_t mutex;
    unsigned int    num_cur_clients;
    unsigned int    max_cur_clients;
    a_client_t     *clients;
} clients_t;

/*
 * Procedure     : print_clients_t 
 * Purpose       : print all elements in clientx_t
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int print_clients_t(void);

/*
 * Procedure     : CLI_MGR_id2fd
 * Purpose       : Given a client id return the fd of that client
 * Parameters    :
 *    id            : The client id of the client
 * Return Code   :
 *    >0            : The fd that the client listens on
 *     0            : client id not found
 *    <0            : Some error
 */
int CLI_MGR_id2fd (token_t id);

/*
 * Procedure     : CLI_MGR_fd2id
 * Purpose       : Given a file descriptor return the corresponding client id
 * Parameters    :
 *    fd            : A file descriptor
 * Return Code   :
 *    NULL          : Not a valid client
 *    token_t       : Valid client
 * NOTE          :
 *    Due to the multi threaded nature of syseventd, you cannot guarantee anything about
 *    the client or its validity after this call ends. Don't use the return token
 */
token_t CLI_MGR_fd2id (const int fd);

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
a_client_t *CLI_MGR_new_client(const char *name, const int fd);

/*
 * Procedure     : CLI_MGR_remove_client_by_fd
 * Purpose       : Remove a client from the table of clients
 *                 The client is identified by the file descriptor
 *                 that we receive messages from.
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 *    id            : The client id, if known
 *    force         : 1 if client data needs to be removed even if id doesnt match CLI_MGR data
 *                            0 is not force, 1 is force
 * Return Code   :
 *    0             : OK
 *    < 0           : some error
 */
int CLI_MGR_remove_client_by_fd (const int fd, const token_t id, const int force);

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
int CLI_MGR_clear_client_error_by_fd (int fd);

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
 *    <0   : an error
 */
int CLI_MGR_handle_client_error_by_fd (int fd);

/*
 * Procedure     : CLI_MGR_add_notification_to_client_by_fd
 * Purpose       : Increment the number of notifications that client has registered for
 *                 The client is identified by the file descriptor
 * Parameters    :
 *    fd            : The file descriptor that we receive messages from that client
 * Return Code   : 
 *    0    : 
 */
int CLI_MGR_add_notification_to_client_by_fd (int fd);


/*
 * Procedure     : CLI_MGR_init_clients_table
 * Purpose       : Initialize a table of clients
 * Parameters    :
 * Return Code   :
 *    0             : OK
 *    <0            : some error.
 */
int CLI_MGR_init_clients_table(void);

/*
 * Procedure     : CLI_MGR_deinit_clients_table
 * Purpose       : Uninitialize a table of clients
 * Parameters    :
 * Return Code   :
 *    0             : OK
 */
int CLI_MGR_deinit_clients_table(void);


#endif   // __CLIENTS_MGR_H_
