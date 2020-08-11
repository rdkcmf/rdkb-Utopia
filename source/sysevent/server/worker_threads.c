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
======================================================================
              worker_threads.c

This file contains the code to run the worker threads.
Each worker thread waits for a semaphore to become active. When the
semaphore is granted the worker will wait for a message to arrive, and 
when one does, then it receives the message into a thread specific 
buffer, signals the semphore to allow another worker to become active,
and then executes the received message to completion. When the message
has been acted upon the thread will return to the semaphore to wait its 
turn to reactivate.

Messages will arrive on the inter-thread fd or any of the connected 
client fds. Each message has a common header describing the message,
and then some message specific fields.

Author: mark enright

======================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>    // for _NSIG
#include <sys/ioctl.h> // for TIOCNOTTY
#include <sys/wait.h>  // for WIFEXITED and WEXITSTATUS
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "libsysevent_internal.h"
#include "sysevent/sysevent.h"
#include "clientsMgr.h"
#include "syseventd.h"
#include "triggerMgr.h"
#include "dataMgr.h"

#ifdef USE_SYSCFG
#include <syscfg/syscfg.h>
#endif

#include "ulog/ulog.h"

static const char *emptystr = "";

/*
 * Procedure     : send_msg_to_fork_helper 
 * Purpose       : send a message to the fork helper process
 * Parmameters   :
 *                  fd      : File Descriptor to fork helper
 *                  buf     : Message to send
 *                  count   : Size of message in buf
 *                  erron   : On return, if error then this is errno
 * Return Code   :
 *   Number of bytes written (0 is 0 bytes)
 *  -1                      : error
 */
int send_msg_to_fork_helper(int fd, const void *buf, size_t count, int *error)
{
   int num_tries = 3;
   while (0 < num_tries) {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Attempting to get mutex: fork_helper_communication_mutex\n", id);
      )
      pthread_mutex_lock(&fork_helper_communication_mutex);
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Got mutex: fork_helper_communication_mutex\n", id);
      )
      int rc = write(fork_helper_pipe[1], buf, count);
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: fork_helper_communication_mutex\n", id);
      )
      pthread_mutex_unlock(&fork_helper_communication_mutex);
      if (-1 == rc) {
         if (EAGAIN == errno || EWOULDBLOCK == errno) {
            num_tries--;
            if (0 < num_tries) {
               struct timespec sleep_time;
               sleep_time.tv_sec = 0;
               sleep_time.tv_nsec  = 100000000;  // .1 secs
               nanosleep(&sleep_time, NULL);
            }
         } else {
            // write error
            *error = errno;
            SE_INC_LOG(ERROR,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Write to fork helper process failed Error: (%d) %s\n", id, errno, strerror(errno));
            )
            return(-1);
         }
      } else {
         if (rc != count) {
            *error = EWOULDBLOCK;
            SE_INC_LOG(ERROR,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Write to fork helper process incomplete  %d != %d\n", id, rc, count);
            )
            return(-1);
         } else {
            *error = 0;
            return(rc);
         }
      }
   }

   *error = EWOULDBLOCK;
   return(-1);
}

/*
 * Procedure     : _eval
 * Purpose       : activate a new program
 * Parmameters   :
 *    prog          : The program to execute
 *    name          : The name of the tuple that caused the action
 *    value         : The value of the tuple that caused the action
 *    argv          : The argument vector to pass to the program
 *    wait          : 0 if we should wait for the child to finish executing, 1 is we return immediately
 * Return Code   :
 *   0
 */
static int _eval(char *const prog, const char *const name, const char *const value, char *argv[], int wait)
{

   if (NULL == prog) {
      return(ERR_BAD_PARAMETER);
   }
   
   /* 
    * Some implementations of libc occasionally leave internal mutexes in locked state
    * for multi-threaded programs that use fork()
    * In order to get around this type of libc bug we pass on requests to activate a 
    * transient process to a fork helper process which listens on a pipe for messages
    * from worker threads, and responds to the workers using a thread specific pipe.
    */


   /*
    * create the message for the fork helper
    * See fork_helper.c for message format
    */
   char buf[2048];
   int bytes;
   int msg_len = 0;
   int num_args = 0;

   // start bufptr past the message header
   char *bufptr = buf+sizeof(int)+sizeof(int)+sizeof(int);
   // name of program to be activated
   bytes = sprintf(bufptr,"%s",prog);
   bufptr+=bytes;
   *bufptr = '\0';
   bufptr++;
   msg_len += (bytes+1);

   // implicit argument to the program (prog)
   char *prog_name = strrchr(prog, '/');
   if (NULL == prog_name) {
      prog_name = prog;
   } else {
      prog_name++;
   }

   bytes = sprintf(bufptr,"%s",prog_name);
   bufptr+=bytes;
   *bufptr = '\0';
   bufptr++;
   num_args++;
   msg_len += (bytes+1);

   // implicit arguments to the program (<name,value>)
   bytes = sprintf(bufptr, "%s", name);
   bufptr+=bytes;
   *bufptr = '\0';
   bufptr++;
   num_args++;
   msg_len += (bytes+1);

   bytes = sprintf(bufptr, "%s", value);
   bufptr+=bytes;
   *bufptr = '\0';
   bufptr++;
   num_args++;
   msg_len += (bytes+1);
   
   // explicit arguments to the program
   int arg_idx = 0;
   while (NULL != argv && NULL != argv[arg_idx] && (unsigned int)(bufptr-buf)<sizeof(buf)) {
      bytes = sprintf(bufptr, "%s", argv[arg_idx]);
      bufptr+=bytes;
      *bufptr = '\0';
      bufptr++;
      num_args++;
      arg_idx++;
      msg_len += (bytes+1);
   }

   /*
    * fill in the header of the message using:
    *    msg_len | num args | calling thread id (if reply required) or -1
    */
   while (0 != msg_len % 8) {
      *bufptr = '\0';
      bufptr++;
      msg_len++;
   }
   int *int_p = (int *)buf;
   *int_p = msg_len;
   int_p++;
   *int_p = num_args;
   int_p++;
   // if we are going to wait for a response then pass our thread id so that
   // the syseventd fork helper process can figure out which fifo we are reading on
   // ie. /tmp/syseventd_worker_(id)
   if (wait) {
      *int_p = thread_get_id(worker_data_key);
   } else {
      *int_p = -1;
   }

   int fd = -1;
   if (wait) {
      fd = thread_get_private_pipe_read(worker_data_key);
      if (-1 == fd) {
         SE_INC_LOG(ERROR,
              int id = thread_get_id(worker_data_key);
              printf("Thread %d Had bad fd for private pipe. Cannot execute %s\n", id, prog);
         )
         return(0);
      }
      // if there is any cruft in our read pipe then drain it
      int junk;
      while (0 < read(fd, &junk, sizeof(junk)));
   }

   // send the message to the fork helper process
   int error;
   SE_INC_LOG(ACTIVATION,
              int id = thread_get_id(worker_data_key);
              printf("Thread %d Calling fork_helper for %s\n", id, prog);
   )
   thread_set_state(worker_data_key, 2);
   int rc = send_msg_to_fork_helper(fork_helper_pipe[1], buf, msg_len+sizeof(int)+sizeof(int)+sizeof(int), &error);
   thread_set_state(worker_data_key, 1);
   if (0 > rc) {
      SE_INC_LOG(ERROR,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Not able to start %s\n", id, prog);
         )
      if (-1 == rc) {
         SE_INC_LOG(ERROR,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Write error (%d) %s\n", id, error, strerror(error));
         )
      }
      return(0);
   }

   if (!wait) {
      SE_INC_LOG(ACTIVATION,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Not waiting for %s\n", id, prog);
      )
      return(0);
   }

   /*
    * If we are waiting for the child to finish executing then we need to wait for
    * the helper to tell us its pid, and then wait on the child pid
    */
   int num_tries = 3;
   int log_success = 0; // this is used for logging when a retry succeeded

   thread_set_state(worker_data_key, 2);
            fd_set readfds;
   struct timeval tv;
   while (0 < num_tries) {
      tv.tv_sec = 2;
      tv.tv_usec = 0;
            FD_ZERO(&readfds);
      FD_SET(fd, &readfds);
      int rc = select(fd+1, &readfds, NULL, NULL, &tv);
      if (-1 == rc) {
         num_tries--;
      } else if (0 == rc) {
         num_tries--;
      } else {
         if (!FD_ISSET(fd, &readfds)) {
            // the expected fd was not set so try again
            num_tries--;
         } else {
            pid_t childpid;
            int read_bytes = 0;
            if (0 >= (read_bytes = read(fd, &childpid, sizeof(childpid))) ) {
               // the read failed or didnt read anything
               num_tries--;
               if (-1 == read_bytes) {
                  incr_stat_info(STAT_FORK_HELPER_PIPE_READ);
                  ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Pipe read for fork helper reply failed. %s <%s, %s> (%d) %s", 
                        prog, name, value, errno, strerror(errno)) ;
                  log_success = 1;
               }
               // if resource temporarily unavailable
               if (0 == read_bytes || (-1 == read_bytes && (EAGAIN == errno || EWOULDBLOCK == errno)) ) {
                  struct timespec sleep_time;
                  sleep_time.tv_sec = 0;
                  sleep_time.tv_nsec  = 200000000;  // .2 secs
                  nanosleep(&sleep_time, NULL);
               } else if (-1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno) {
                  ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Pipe read for fork helper reply system error. %s <%s, %s> (%d) %s", 
                        prog, name, value, errno, strerror(errno)) ;
                  num_tries = 0;  // we are done trying  
               }
            } else {
               // we are waiting for the activated process to complete before retuning
               if (-1 == childpid) {
                  ulogf(ULOG_SYSTEM, UL_SYSEVENT, "fork helper returned error for child %s <%s, %s>", prog, name, value) ;
               } else {
                  if (log_success) {
                     ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Retry of pipe read for fork helper succeeded. %s <%s, %s>", 
                           prog, name, value) ;
                     log_success = 0;
                  }
                  int pid_slot = thread_get_id(worker_data_key) - 1;
                  waiting_pid[pid_slot].pid  = childpid;
               waiting_pid[pid_slot].mark = 0;
                  snprintf(waiting_pid[pid_slot].name, sizeof(waiting_pid[pid_slot].name), "%s <%s,%s>", prog, name, value);
                  int    done = 0;
                  struct timespec sleep_time;
                  while (!done) {
                     done = kill(childpid, 0);
                     if (!done) {
                        sleep_time.tv_sec  = 0;
                        sleep_time.tv_nsec = 100000;
                        nanosleep(&sleep_time, NULL);
                     }
                  }
               waiting_pid[pid_slot].pid       = 0;
               waiting_pid[pid_slot].mark      = 0;
               (waiting_pid[pid_slot].name)[0] = '\0';
                  SE_INC_LOG(ACTIVATION,
                             int id = thread_get_id(worker_data_key);
                             printf("Thread %d Finished waiting for %s\n", id, prog);
                  )
               }
               num_tries = 0;
            }
         }
      }
   }
   thread_set_state(worker_data_key, 1);

   return(0);
}

/*
 * Procedure     : handle_close_connection_request
 * Purpose       : Handle an close connection request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking to close a connection to the sysevent daemon
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_close_connection_request(const int fd, const token_t who)
{

    SE_INC_LOG(LISTENER,
       printf("Request to remove client %x on fd %d\n", (unsigned int)who, fd);
    )

   // remove the client from the table of clients
   int rc;
   rc = CLI_MGR_remove_client_by_fd(fd, who, 0);
      if (0 != rc) {
      SE_INC_LOG(LISTENER,
         printf("Unable to remove client %x on fd %d. Reason: %d\n", (unsigned int)who, fd, rc);
      )
   }
   return(0);
}

/*
 * Procedure     : handle_ping_request
 * Purpose       : Handle an ping request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the ping
 *    msg          : the ping request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_ping_request(const int fd, const token_t who, se_ping_msg *msg)
{

   // prepare the reply message
   se_buffer reply_msg_buffer;

   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_ping_reply_msg) ;

   if (reply_msg_size >= sizeof(reply_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   }

   se_ping_reply_msg *reply_msg_body;

   reply_msg_body = (se_ping_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_PING_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->reserved = 0;
      int rc                   = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_PING_REPLY), SE_MSG_PING_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
   return(0);
}

/*
 * Procedure     : handle_get_request
 * Purpose       : Handle an get request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the get
 *    msg          : the get request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_get_request(const int fd, const token_t who, se_get_msg *msg)
{
   int   subject_bytes;
   int   rc             = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   char *subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);


   // extract the subject string.
   if (NULL == inmsg_data_ptr || NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   se_buffer value_buf;
   // make sure not to get a larger value from data manager than could fit in the reply buffer
   int       value_buf_size = sizeof(value_buf) - (sizeof(se_msg_hdr)+sizeof(se_get_reply_msg)+4);
   char     *value;
   if (!rc) {
      value = DATA_MGR_get( subject_str, value_buf, &value_buf_size);
   } else {
      value = NULL;
   }

   int value_str_size = NULL == value ? SE_string2size("") :  SE_string2size(value);

   // prepare the reply message
   se_buffer reply_msg_buffer;

   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_get_reply_msg) +
                        SE_string2size(subject_str) + value_str_size  - sizeof(void *);

   if (reply_msg_size >= sizeof(reply_msg_buffer)) {
      rc = ERR_MSG_TOO_LONG;
   }

   se_get_reply_msg *reply_msg_body;

   reply_msg_body = (se_get_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_GET_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      if (!rc) {
         char *data_str    = (char *)&(reply_msg_body->data);
         int buf_size      = sizeof(reply_msg_buffer);
         buf_size         -= sizeof(se_msg_hdr);
         buf_size         -= sizeof(se_get_reply_msg);
         int strsize       = SE_msg_add_string(data_str, buf_size, subject_str);
         if (0 == strsize) {
            rc = (ERR_CANNOT_SET_STRING);
         } else {
            buf_size -= strsize;
            data_str += strsize;
            if (NULL == value) {
               strsize = SE_msg_add_string(data_str, buf_size, "");
            } else {
               strsize = SE_msg_add_string(data_str, buf_size, value);
            }
            if (0 == strsize) {
               rc = (ERR_CANNOT_SET_STRING);
            }
         }
      }

      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_GET_REPLY), SE_MSG_GET_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(reply_msg_buffer);
         )
         debug_num_gets++;
         return(0);
      }
   }
}

static int handle_get_request_data(const int fd, const token_t who, se_get_msg *msg)
{
   int   subject_bytes;
   int   rc             = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   char *subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);


   // extract the subject string.
   if (NULL == inmsg_data_ptr || NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   char *value_buf = sysevent_malloc(bin_size, __FILE__, __LINE__);
   if (!value_buf)
       return -1;
   // make sure not to get a larger value from data manager than could fit in the reply buffer
   int       value_buf_size = bin_size - (sizeof(se_msg_hdr)+sizeof(se_get_reply_msg)+4);
   char     *value;
   if (!rc) {
      value = DATA_MGR_get_bin( subject_str, value_buf, &value_buf_size);
   } else {
      value = NULL;
   }

   int value_str_size = NULL == value ? 0 :  value_buf_size;

   // prepare the reply message
   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_get_reply_msg) +
                        SE_string2size(subject_str) + value_str_size  - sizeof(void *);

   if (reply_msg_size >= bin_size) {
      sysevent_free(&value_buf, __FILE__, __LINE__);
      rc = ERR_MSG_TOO_LONG;
   }

   se_get_reply_msg *reply_msg_body;
   char  *reply_msg_buffer = sysevent_malloc(bin_size, __FILE__, __LINE__);
   if (!reply_msg_buffer)
   {
       sysevent_free(&value_buf, __FILE__, __LINE__);
       return -1;
   }

   reply_msg_body = (se_get_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       bin_size, 
                                                       SE_MSG_GET_DATA_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
        sysevent_free(&value_buf, __FILE__, __LINE__);
         sysevent_free(&reply_msg_buffer, __FILE__, __LINE__);
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      if (!rc) {
         char *data_str    = (char *)&(reply_msg_body->data);
         int buf_size      = bin_size;
         buf_size         -= sizeof(se_msg_hdr);
         buf_size         -= sizeof(se_get_reply_msg);
         int strsize       = SE_msg_add_string(data_str, buf_size, subject_str);
         if (0 == strsize) {
            rc = (ERR_CANNOT_SET_STRING);
         } else {
            buf_size -= strsize;
            data_str += strsize;
            if (NULL == value) {
               strsize = SE_msg_add_data(data_str, buf_size, NULL,0);
            } else {
               strsize = SE_msg_add_data(data_str, buf_size, value,value_buf_size);
            }
            if (0 == strsize) {
               rc = (ERR_CANNOT_SET_STRING);
            }
         }
      }

      reply_msg_body->status = htonl(rc);
      sysevent_free(&value_buf, __FILE__, __LINE__);

      int rc                 = SE_msg_send_data(fd, reply_msg_buffer,bin_size);
      if (0 != rc) {
           sysevent_free(&reply_msg_buffer, __FILE__, __LINE__);
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_GET_DATA_REPLY), SE_MSG_GET_DATA_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(reply_msg_buffer);
         )
         debug_num_gets++;
          sysevent_free(&reply_msg_buffer, __FILE__, __LINE__);
         return(0);
      }
   }
}


/*
 * Procedure     : handle_set_request
 * Purpose       : Handle an set request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the set request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_set_request(const int fd, const token_t who, se_set_msg *msg)
{
   int  subject_bytes;
   int  value_bytes;
   char *subject_str;
   char *value_str;
   int   rc           = 0;
   int   source;
   int   tid;

   // extract the subject and value strings.
   source               = ntohl(msg->source);
   tid                  = ntohl(msg->tid);
   char *inmsg_data_ptr = (char *) &(msg->data);
   subject_str          = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   inmsg_data_ptr      += subject_bytes;
   value_str            =  SE_msg_get_string(inmsg_data_ptr, &value_bytes);

   // value is allowed to be 0
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   if (!rc) {
      rc = DATA_MGR_set( subject_str, value_str, source, tid); 
      if (0 != rc) {
         SE_INC_LOG(LISTENER,
            printf("handle_set_request: Call to Data Mgr failed. Reason (%d), %s\n",
                                rc, SE_strerror(rc));
         )
      }
   } else {
      SE_INC_LOG(ERROR,
         printf("handle_set_request got bad subject parameter\n");
      )
   }

#ifdef SET_REPLY_REQUIRED
   se_buffer        reply_msg_buffer;
   se_set_reply_msg *reply_msg_body;

   reply_msg_body = (se_set_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_SET_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_SET_REPLY), SE_MSG_SET_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(reply_msg_buffer);
         )
         debug_num_sets++;
         return(0);
      }
   }
#endif
   return(0);
}

static int handle_set_request_data(const int fd, const token_t who, se_set_msg *msg)
{
   int  subject_bytes;
   int  value_bytes;
   char *subject_str;
   char *value_str;
   int   rc           = 0;
   int   source;
   int   tid;
   int fileread = access("/tmp/sysevent_debug", F_OK);
   // extract the subject and value strings.
   source               = ntohl(msg->source);
   tid                  = ntohl(msg->tid);
   char *inmsg_data_ptr = (char *) &(msg->data);
   subject_str          = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   inmsg_data_ptr      += subject_bytes;   
   value_str            =  SE_msg_get_data(inmsg_data_ptr, &value_bytes);

   // value is allowed to be 0
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   if (!rc) {
       if (fileread == 0)
       {
          char buf[256] = {0};
         snprintf(buf,sizeof(buf),"echo fname %s: %d >> /tmp/sys_d.txt",__FUNCTION__,value_bytes);
         system(buf);
       }
      rc = DATA_MGR_set_bin( subject_str, value_str, value_bytes, source, tid); 
      if (0 != rc) {
         SE_INC_LOG(LISTENER,
            printf("handle_set_request: Call to Data Mgr failed. Reason (%d), %s\n",
                                rc, SE_strerror(rc));
         )
      }
   } else {
      SE_INC_LOG(ERROR,
         printf("handle_set_request got bad subject parameter\n");
      )
   }

#ifdef SET_REPLY_REQUIRED
   se_buffer        reply_msg_buffer;
   se_set_reply_msg *reply_msg_body;

   reply_msg_body = (se_set_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_SET_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send_data(fd, reply_msg_buffer,sizeof(reply_msg_buffer));
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_SET_REPLY), SE_MSG_SET_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
             SE_print_message_hdr(reply_msg_buffer);
         )
         debug_num_sets++;
         return(0);
      }
   }
#endif
   return(0);
}



/*
 * Procedure     : handle_set_unique_request
 * Purpose       : Handle an set unique request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the set request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_set_unique_request(const int fd, const token_t who, se_set_unique_msg *msg)
{
   int  subject_bytes;
   int  value_bytes;
   char *subject_str;
   char *value_str;
   int   rc           = 0;

   // extract the subject and value strings.
   char *inmsg_data_ptr = (char *) &(msg->data);
   subject_str          = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   inmsg_data_ptr      += subject_bytes;
   value_str            =  SE_msg_get_string(inmsg_data_ptr, &value_bytes);

   if ( NULL == subject_str || 0 >= subject_bytes || NULL == value_str || 0 >= value_bytes ) {
      rc = ERR_BAD_PARAMETER;
   }

   se_buffer uname_buf;
   int       uname_buf_size = sizeof(uname_buf) - (sizeof(se_msg_hdr)+sizeof(se_set_unique_reply_msg)+4);
   char     *uname;

   if (!rc) {
      uname = DATA_MGR_set_unique(subject_str, value_str, uname_buf, &uname_buf_size); 
   } else {
      uname = NULL;
   }

   int uname_str_size = (NULL == uname ? SE_string2size("") :  SE_string2size(uname));

   se_buffer        reply_msg_buffer;

   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_set_unique_reply_msg) +
                        SE_string2size(subject_str) + uname_str_size - sizeof(void *);

   if (reply_msg_size >= sizeof(reply_msg_buffer)) {
      rc = ERR_MSG_TOO_LONG;
   }

   se_set_unique_reply_msg *reply_msg_body;

   reply_msg_body = (se_set_unique_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_SET_UNIQUE_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
         char *data_str    = (char *)&(reply_msg_body->data);
         int buf_size      = sizeof(reply_msg_buffer);
         buf_size         -= sizeof(se_msg_hdr);
         buf_size         -= sizeof(se_set_unique_reply_msg);
         int strsize = 0;
         if (NULL != uname) {
            strsize = SE_msg_add_string(data_str, buf_size, uname);
         } else {
            strsize = SE_msg_add_string(data_str, buf_size, "");
         }
        if (0 == strsize) {
           rc = (ERR_CANNOT_SET_STRING);
        }
      }
   reply_msg_body->status = htonl(rc);
   rc = SE_msg_send(fd, reply_msg_buffer);
   SE_INC_LOG(MESSAGES,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
             id, SE_print_mtype(SE_MSG_SET_UNIQUE_REPLY), SE_MSG_SET_UNIQUE_REPLY, (unsigned int)who, fd);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
      SE_print_message_hdr(reply_msg_buffer);
   )
   return(rc);
}

/*
 * Procedure     : handle_get_unique_request
 * Purpose       : Handle an unique get request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the get
 *    msg          : the get request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_get_unique_request(const int fd, const token_t who, se_iterate_get_msg *msg)
{
   int   subject_bytes;
   int   rc             = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   char *subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);

   // extract the subject string.
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   unsigned int iter = ntohl(msg->iterator);
   se_buffer    value_buf;
   unsigned int value_buf_size = sizeof(value_buf) - (sizeof(se_msg_hdr)+sizeof(se_iterate_get_reply_msg)+4);
   se_buffer    subject_buf;
   unsigned int subject_buf_size = sizeof(subject_buf) - (sizeof(se_msg_hdr)+sizeof(se_iterate_get_reply_msg)+4);

   char *value;
   if (!rc) {
      value = DATA_MGR_get_unique( subject_str, &iter, subject_buf, &subject_buf_size, value_buf, &value_buf_size);
   } else {
      value = NULL;
   }

   int value_str_size = NULL == value ? SE_string2size("") :  SE_string2size(value);
   char *subject = subject_buf;
   int subject_str_size = NULL == subject ? SE_string2size("") :  SE_string2size(subject);

   // prepare the reply message
   se_buffer reply_msg_buffer;

   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_iterate_get_reply_msg) +
                        subject_str_size + value_str_size  - sizeof(void *);

   if (reply_msg_size >= sizeof(reply_msg_buffer)) {
      rc = ERR_MSG_TOO_LONG;
   }

   se_iterate_get_reply_msg *reply_msg_body;

   reply_msg_body = (se_iterate_get_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_ITERATE_GET_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      if (!rc) {
         reply_msg_body->iterator = htonl(iter);
         char *data_str           = (char *)&(reply_msg_body->data);
         int buf_size             = sizeof(reply_msg_buffer);
         buf_size                -= sizeof(se_msg_hdr);
         buf_size                -= sizeof(se_iterate_get_reply_msg);
         int strsize              = SE_msg_add_string(data_str, buf_size, subject);
         if (0 == strsize) {
            rc = (ERR_CANNOT_SET_STRING);
         } else {
            buf_size -= strsize;
            data_str += strsize;
            if (NULL == value) {
               strsize = SE_msg_add_string(data_str, buf_size, "");
            } else {
               strsize = SE_msg_add_string(data_str, buf_size, value);
            }
            if (0 == strsize) {
               rc = (ERR_CANNOT_SET_STRING);
            }
         }
      }

      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_ITERATE_GET_REPLY), SE_MSG_ITERATE_GET_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
            SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
}

/*
 * Procedure     : handle_del_unique_request
 * Purpose       : Delete an element from a namespace
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_del_unique_request(const int fd, const token_t who, se_del_unique_msg *msg)
{
   int  subject_bytes;
   char *subject_str;
   int   rc           = 0;

   unsigned int iter = ntohl(msg->iterator);
   char *inmsg_data_ptr = (char *) &(msg->data);
   subject_str          = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   if (NULL == subject_str) {
      return(ERR_BAD_PARAMETER);
   } else {
   rc = DATA_MGR_del_unique(subject_str, iter);
   return(rc);
   }

}


/*
 * Procedure     : handle_get_next_unique_iterator
 * Purpose       : Handle an request for the next iterator in a namespace 
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the get
 *    msg          : the message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_get_next_unique_iterator(const int fd, const token_t who, se_iterate_get_iterator_msg *msg)
{
   int   subject_bytes;
   int   rc             = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   char *subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);


   // extract the subject string.
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   unsigned int iter = ntohl(msg->iterator);

   if (!rc) {
      rc = DATA_MGR_get_next_iterator(subject_str, &iter);
   }

   int subject_str_size = NULL == subject_str ? SE_string2size("") :  SE_string2size(subject_str);
   // prepare the reply message
   se_buffer reply_msg_buffer;

   unsigned int reply_msg_size = sizeof(se_msg_hdr) + sizeof(se_iterate_get_iterator_reply_msg) +
                        subject_str_size  - sizeof(void *);

   if (reply_msg_size >= sizeof(reply_msg_buffer)) {
      rc = ERR_MSG_TOO_LONG;
   }

   se_iterate_get_iterator_reply_msg *reply_msg_body;

   reply_msg_body = (se_iterate_get_iterator_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_NEXT_ITERATOR_GET_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      if (!rc) {
         reply_msg_body->iterator = htonl(iter);
         char *data_str           = (char *)&(reply_msg_body->data);
         int buf_size             = sizeof(reply_msg_buffer);
         buf_size                -= sizeof(se_msg_hdr);
         buf_size                -= sizeof(se_iterate_get_iterator_reply_msg);
         int strsize              = SE_msg_add_string(data_str, buf_size, subject_str);
         if (0 == strsize) {
            rc = (ERR_CANNOT_SET_STRING);
         }
      }

      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_ITERATE_GET_REPLY), SE_MSG_ITERATE_GET_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
            SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
}

/*
 * Procedure     : handle_set_options_request
 * Purpose       : Handle an set options request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the set request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_set_options_request(const int fd, const token_t who, se_set_options_msg *msg)
{
   int  subject_bytes;
   char *subject_str;
   int   rc           = 0;

   // extract the subject string.
   char *inmsg_data_ptr = (char *) &(msg->data);
   subject_str          = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);

   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   }

   if (!rc) {
      rc = DATA_MGR_set_options( subject_str, ntohl(msg->flags)); 
      if (0 != rc) {
         SE_INC_LOG(LISTENER,
            printf("handle_set_options_request: Call to Data Mgr failed. Reason (%d), %s\n",
                                rc, SE_strerror(rc));
         )
      }
   } else {
      SE_INC_LOG(ERROR,
         printf("handle_set_options_request got bad subject parameter\n");
      )
   }

#ifdef SET_REPLY_REQUIRED
   se_buffer        reply_msg_buffer;
   se_set_options_reply_msg *reply_msg_body;

   reply_msg_body = (se_set_options_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                       sizeof(reply_msg_buffer), 
                                                       SE_MSG_SET_OPTIONS_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->status = htonl(rc);
      int rc                 = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_SET_OPTIONS_REPLY), SE_MSG_SET_OPTIONS_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
            SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
#endif
   return(0);
}

/*
 * Procedure     : handle_set_async_action_request
 * Purpose       : Handle an set async action request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the set request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_set_async_action_request(const int fd, const token_t who, se_set_async_action_msg *msg)
{
   int  subject_bytes         = 0;
   int  function_bytes        = 0; /*RDKB-7131, CID-33267, initialize before use*/
   char *subject_str          = NULL;
   char *function_str         = NULL;
   int   rc                   = 0;
   char **args = NULL;
   char *inmsg_data_ptr = (char *)&(msg->data);

   // extract the subject, and function
   subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   } else {
      inmsg_data_ptr += subject_bytes;
      function_str = SE_msg_get_string(inmsg_data_ptr, &function_bytes);
      if (NULL == function_str || 0 >= function_bytes) {
         rc = ERR_BAD_PARAMETER;
      } else {
         // extract the parameters later
      }
   }

   if (rc) {
      SE_INC_LOG(ERROR,
         printf("handle_set_async_action_request got bad subject and/or function\n");
      )
   }

   // go through the parameters and add them to an argument list which will
   // be used later in the exev call
   int numparams = ntohl(msg->num_params);
   args = (char **) sysevent_malloc( sizeof(char *) * (numparams + 1), __FILE__, __LINE__);
   if (NULL == args) {
      rc = ERR_ALLOC_MEM;
   } else {
      int size;
      int cur_param;

      inmsg_data_ptr += function_bytes;
      for (cur_param=0; cur_param<numparams; cur_param++) {
         args[cur_param] = SE_msg_get_string(inmsg_data_ptr, &size); 

         inmsg_data_ptr += size;
      }
      args[cur_param] = (char *) NULL;
   }

   int trigger_id;
   int action_id;
   action_flag_t action_flags = ntohl(msg->flags);
   if (!rc) {
      rc = DATA_MGR_set_async_external_executable(subject_str, who, action_flags, function_str, args, &trigger_id, &action_id ); 
      if (0 != rc) {
         SE_INC_LOG(ERROR,
            printf("handle_set_async_action_request: Call to Data Mgr failed. Reason (%d), %s\n",
                                rc, SE_strerror(rc));
         )
      }
   }
   sysevent_free(&args, __FILE__, __LINE__);

   se_buffer               reply_msg_buffer;
   se_set_async_reply_msg *reply_msg_body;
   reply_msg_body = (se_set_async_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                             sizeof(reply_msg_buffer), 
                                                             SE_MSG_SET_ASYNC_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->status = htonl(rc);
      if (0 == rc) {
         (reply_msg_body->async_id).trigger_id = htonl(trigger_id);
         (reply_msg_body->async_id).action_id  = htonl(action_id);
      } else {
         (reply_msg_body->async_id).trigger_id = htonl(0);
         (reply_msg_body->async_id).action_id  = htonl(0);
      }
      int rc = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_SET_ASYNC_REPLY), SE_MSG_SET_ASYNC_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
            SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
   return(0);
}

/*
 * Procedure     : handle_set_async_message_request
 * Purpose       : Handle an set async message request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the set
 *    msg          : the set request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_set_async_message_request(const int fd, const token_t who, se_set_async_message_msg *msg)
{
   int  subject_bytes;
   char *subject_str   = NULL;
   int   rc            = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   // extract the subject
   subject_str   = SE_msg_get_string(inmsg_data_ptr, &subject_bytes);
   if (NULL == subject_str || 0 >= subject_bytes) {
      rc = ERR_BAD_PARAMETER;
   } 

   if (rc) {
      SE_INC_LOG(ERROR,
         printf("handle_set_async_message_request got bad subject and/or function\n");
      )
   }

   int trigger_id;
   int action_id;
   action_flag_t flags = ntohl(msg->flags);
   if (!rc) {
      rc = DATA_MGR_set_async_message_notification(subject_str, who, flags, &trigger_id, &action_id ); 
      if (0 != rc) {
         SE_INC_LOG(ERROR,
            printf("handle_set_async_message_request: Call to Data Mgr failed. Reason (%d), %s\n",
                                rc, SE_strerror(rc));
         )
      } else {
         // let client mgr know that the client has registered for notifications
         // so that they can be removed when the client disconnects
         CLI_MGR_add_notification_to_client_by_fd (fd);
      }
   }

   se_buffer               reply_msg_buffer;
   se_set_async_reply_msg *reply_msg_body;
   reply_msg_body = (se_set_async_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                             sizeof(reply_msg_buffer), 
                                                             SE_MSG_SET_ASYNC_REPLY, TOKEN_NULL);
   if (NULL == reply_msg_body) {
      return(ERR_UNABLE_TO_PREPARE_MSG);
   } else {
      reply_msg_body->status = htonl(rc);
      if (0 == rc) {
         (reply_msg_body->async_id).trigger_id = htonl(trigger_id);
         (reply_msg_body->async_id).action_id  = htonl(action_id);
      } else {
         (reply_msg_body->async_id).trigger_id = htonl(0);
         (reply_msg_body->async_id).action_id  = htonl(0);
      }
      int rc = SE_msg_send(fd, reply_msg_buffer);
      if (0 != rc) {
         return(ERR_UNABLE_TO_SEND);
      } else {
         SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Sent %s (%d) to client %x on fd %d\n",
                   id, SE_print_mtype(SE_MSG_SET_ASYNC_REPLY), SE_MSG_SET_ASYNC_REPLY, (unsigned int)who, fd);
         )
         SE_INC_LOG(MESSAGE_VERBOSE,
            SE_print_message_hdr(reply_msg_buffer);
         )
         return(0);
      }
   }
   return(0);
}

/*
 * Procedure     : handle_remove_async_request
 * Purpose       : Handle an remove async request message from a client
 * Parameters    :
 *    fd           : the fd to reply on
 *    who          : the client asking for the remove
 *    msg          : the remove request message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_remove_async_request(const int fd, const token_t who, se_remove_async_msg *msg)
{
   int   rc            = 0;
   int   trigger_id;
   int   action_id;

   // extract the trigger_id and action_id
   trigger_id = ntohl((msg->async_id).trigger_id);
   action_id  = ntohl((msg->async_id).action_id);
   if (0 == trigger_id || 0 == action_id) {
      rc = ERR_UNKNOWN_ASYNC_ID;
   }

   if (!rc) {
      rc = DATA_MGR_remove_async_notification(trigger_id, action_id, who);
   }


   return(0);
}

/*
 * Procedure     : handle_send_notification_msg
 * Purpose       : Handle an se_send_notification_msg command from an internal thread 
 * Parameters    :
 *    fd           : the fd the message came in on. Ignore it
 *    who          : the client asking for the remove
 *    msg          : the message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_send_notification_msg(const int local_fd, const token_t who, se_send_notification_msg *msg)
{
   int   rc            = 0;
   int   trigger_id;
   int   action_id;

   // extract the trigger_id and action_id
   trigger_id = ntohl((msg->async_id).trigger_id);
   action_id  = ntohl((msg->async_id).action_id);
   if (0 == trigger_id || 0 == action_id) {
      rc = ERR_UNKNOWN_ASYNC_ID;
   }

   // extract the subject and value from the message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *data_ptr;

   data_ptr      = (char *)&(msg->data);
   subject_str   = SE_msg_get_string(data_ptr, &subject_bytes);
   data_ptr += subject_bytes;
   value_str     =  SE_msg_get_string(data_ptr, &value_bytes);

   // it is possible for the value to be NULL. This would occur if the
   // tuple were unset.
   // However some clients might not be able to handle a NULL so we
   // will send an empty string
   if (NULL == value_str) {
      value_str = emptystr;
   }
   if (NULL == subject_str) {
      subject_str = emptystr;
   }

   token_t id = ntohl(msg->token_id);
   action_flag_t action_flags = ntohl(msg->flags); // not currently used

   int fd = CLI_MGR_id2fd(id);
   if (0 > fd) {
      SE_INC_LOG(INFO,
         printf("Dead Peer %x Detected while attempting notification.Cleaning up %d %d\n", (unsigned int)id, trigger_id, action_id);
      )
      int rc;
      rc = DATA_MGR_remove_async_notification(trigger_id, action_id, id);
      return(0);
   }

   se_buffer            send_msg_buffer;
   se_notification_msg *send_msg_body;

   // figure out how much space the se_msg_strings will take
   int subbytes  = SE_string2size(subject_str);
   int valbytes  = SE_string2size(value_str);

   // calculate the size of the se_notification_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_notification_msg) +
                       subbytes + valbytes - sizeof(void *);

   if (send_msg_size >= sizeof(send_msg_buffer)) {
      return(ERR_MSG_TOO_LONG);
   }

   if (NULL ==
      (send_msg_body = (se_notification_msg *)
             SE_msg_prepare(send_msg_buffer, sizeof(send_msg_buffer), SE_MSG_NOTIFICATION, TOKEN_NULL)) ) {
     return(ERR_MSG_TOO_LONG);
   }

   // prepare the message
   int  remaining_buf_bytes;
   send_msg_body->source  = msg->source;
   send_msg_body->tid     = msg->tid;
   char *send_data_ptr    = (char *)&(send_msg_body->data);
   remaining_buf_bytes    = sizeof(send_msg_buffer);
   remaining_buf_bytes    -= sizeof(se_msg_hdr);
   remaining_buf_bytes    -= sizeof(se_notification_msg);
   int strsize    = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      subject_str);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
   strsize              = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      value_str);
   if (0 == strsize) {
      return(ERR_CANNOT_SET_STRING);
   }

   (send_msg_body->async_id).trigger_id = htonl(trigger_id);
   (send_msg_body->async_id).action_id  = htonl(action_id);

   SE_INC_LOG(LISTENER,
       printf("Sending event <%s %s> to client: %x (fd:%d)\n", subject_str, value_str, (unsigned int)id, fd);
   )
   int rc2 = SE_msg_send(fd, send_msg_buffer);
   if (rc2) {
      SE_INC_LOG(ERROR,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: Failed to send %s (%d) to client on fd %d (%d) %s\n",
                id, SE_print_mtype(SE_MSG_NOTIFICATION), SE_MSG_NOTIFICATION, fd, rc2, SE_strerror(rc2));
      )
   } else {
      SE_INC_LOG(MESSAGES,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: Sent %s (%d) to client on fd %d\n",
                id, SE_print_mtype(SE_MSG_NOTIFICATION), SE_MSG_NOTIFICATION, fd);
      )
      SE_INC_LOG(MESSAGE_VERBOSE,
         SE_print_message_hdr(send_msg_buffer);
      )
   }
   return(0);
}

static int handle_send_notification_msg_data(const int local_fd, const token_t who, se_send_notification_msg *msg)
{
   int   rc            = 0;
   int   trigger_id;
   int   action_id;

   // extract the trigger_id and action_id
   trigger_id = ntohl((msg->async_id).trigger_id);
   action_id  = ntohl((msg->async_id).action_id);
   if (0 == trigger_id || 0 == action_id) {
      rc = ERR_UNKNOWN_ASYNC_ID;
   }

   // extract the subject and value from the message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *data_ptr;

   data_ptr      = (char *)&(msg->data);
   subject_str   = SE_msg_get_string(data_ptr, &subject_bytes);
   data_ptr += subject_bytes;
   value_str     =  SE_msg_get_data(data_ptr, &value_bytes);

   // it is possible for the value to be NULL. This would occur if the
   // tuple were unset.
   // However some clients might not be able to handle a NULL so we
   // will send an empty string
   if (NULL == value_str) {
      value_str = emptystr;
   }
   if (NULL == subject_str) {
      subject_str = emptystr;
   }

   token_t id = ntohl(msg->token_id);
   action_flag_t action_flags = ntohl(msg->flags); // not currently used

   int fd = CLI_MGR_id2fd(id);
   if (0 > fd) {
      SE_INC_LOG(INFO,
         printf("Dead Peer %x Detected while attempting notification.Cleaning up %d %d\n", (unsigned int)id, trigger_id, action_id);
      )
      int rc;
      rc = DATA_MGR_remove_async_notification(trigger_id, action_id, id);
      return(0);
   }

  se_notification_msg *send_msg_body;

   // figure out how much space the se_msg_strings will take
   int subbytes  = SE_string2size(subject_str);
   int valbytes  = value_bytes;

   // calculate the size of the se_notification_msg once it will be populated
   unsigned int send_msg_size = sizeof(se_msg_hdr) + sizeof(se_notification_msg) +
                       subbytes + valbytes - sizeof(void *);

   unsigned int bin_size = sysevent_get_binmsg_maxsize();
   if (send_msg_size >= bin_size) {
      return(ERR_MSG_TOO_LONG);
   }
   char *send_msg_buffer = sysevent_malloc(bin_size, __FILE__, __LINE__);
   if (!send_msg_buffer)
       return -1;
 
   if (NULL ==
      (send_msg_body = (se_notification_msg *)
             SE_msg_prepare(send_msg_buffer, bin_size, SE_MSG_NOTIFICATION_DATA, TOKEN_NULL)) ) {
       sysevent_free(&send_msg_buffer, __FILE__, __LINE__);
     return(ERR_MSG_TOO_LONG);
   }

   // prepare the message
   int  remaining_buf_bytes;
   send_msg_body->source  = msg->source;
   send_msg_body->tid     = msg->tid;
   char *send_data_ptr    = (char *)&(send_msg_body->data);
   remaining_buf_bytes    = bin_size;
   remaining_buf_bytes    -= sizeof(se_msg_hdr);
   remaining_buf_bytes    -= sizeof(se_notification_msg);
   int strsize    = SE_msg_add_string(send_data_ptr,
                                      remaining_buf_bytes,
                                      subject_str);
   if (0 == strsize) {
       sysevent_free(&send_msg_buffer, __FILE__, __LINE__);
      return(ERR_CANNOT_SET_STRING);
   }
   remaining_buf_bytes -= strsize;
   send_data_ptr       += strsize;
   strsize              = SE_msg_add_data(send_data_ptr,
                                      remaining_buf_bytes,
                                      value_str,
                                      valbytes);
   if (0 == strsize) {
      sysevent_free(&send_msg_buffer, __FILE__, __LINE__);
      return(ERR_CANNOT_SET_STRING);
   }

   (send_msg_body->async_id).trigger_id = htonl(trigger_id);
   (send_msg_body->async_id).action_id  = htonl(action_id);

   SE_INC_LOG(LISTENER,
       printf("Sending event <%s %d> to client: %x (fd:%d)\n", subject_str, valbytes, (unsigned int)id, fd);
   )
   int rc2 = SE_msg_send_data(fd, send_msg_buffer,bin_size);
   if (rc2) {
      SE_INC_LOG(ERROR,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: Failed to send %s (%d) to client on fd %d (%d) %s\n",
                id, SE_print_mtype(SE_MSG_NOTIFICATION_DATA), SE_MSG_NOTIFICATION_DATA, fd, rc2, SE_strerror(rc2));
      )
   } else {
      SE_INC_LOG(MESSAGES,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: Sent %s (%d) to client on fd %d\n",
                id, SE_print_mtype(SE_MSG_NOTIFICATION_DATA), SE_MSG_NOTIFICATION_DATA, fd);
      )
      SE_INC_LOG(MESSAGE_VERBOSE,
         SE_print_message_hdr(send_msg_buffer);
      )
   }

  sysevent_free(&send_msg_buffer, __FILE__, __LINE__);
   return(0);
}


/*
 * Procedure     : handle_show_data_elements
 * Purpose       : Handle an se_show_data_elements msg
 * Parameters    :
 *    who          : 
 *    msg          : the message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_show_data_elements(const token_t who, se_show_data_elements_msg *msg)
{
   int   filename_bytes;
   int   rc             = 0;
   char *inmsg_data_ptr = (char *)&(msg->data);

   char *filename_str   = SE_msg_get_string(inmsg_data_ptr, &filename_bytes);


   // extract the filename string.
   if (NULL == filename_str || 0 >= filename_bytes) {
      rc = ERR_BAD_PARAMETER;
   } else {

      DATA_MGR_show(filename_str);
   }

   // no reply is expected
   return(rc);
}

/*
 =======================================================================
                   EXECUTABLE ACTIONS
 =======================================================================
*/

/*
 * Procedure     : make_blocked_exec_link
 * Purpose       : make a blocked_exec_link_t
 * Parameters    :
 *    trigger_id   : the trigger id
 *    action_id    : the trigger action id
 *    wait         : non zero if wait for action completion is desired
 *    name         : the name of the tuple which caused this trigger/action
 *    value        : the value of the tuple which caused this trigger/action
 * Return        :
 *    the blocked_exec_link_t containing the action corresponding to trigger_id, action_id
 */
static blocked_exec_link_t * make_blocked_exec_link(const int trigger_id, const int action_id, const int wait, const char * const name, const char * const value)
{
   blocked_exec_link_t *link = sysevent_malloc(sizeof(blocked_exec_link_t ), __FILE__, __LINE__);

   if (NULL == link) {
      return(NULL);
   } else {
      link->prev     = NULL;
      link->next     = NULL;
      link->bucket   = NULL;
      link->wait     = wait;
      link->name     = sysevent_strdup(name, __FILE__, __LINE__);
      if (NULL == value || '\0' == value[0]) {
         link->value = sysevent_strdup("NULL", __FILE__, __LINE__);
      } else {
         link->value = sysevent_strdup(value, __FILE__, __LINE__);
      }
   }

   // using trigger_id and action_id, ask Trigger Mgr to create the data structure for this action
   // TRIGGER_MGR_get_cloned_action will allocate memory within the trigger_action_t
   // It must be freed using TRIGGER_MGR_free_cloned_action
   if (0 != TRIGGER_MGR_get_cloned_action(trigger_id, action_id, &(link->action)) ) {
      sysevent_free(&(link->name), __FILE__, __LINE__);
      link->name = NULL;
      sysevent_free(&(link->value), __FILE__, __LINE__);
      link->value = NULL;
      sysevent_free(&link, __FILE__, __LINE__);
      return(NULL);
   }

   return(link);
}

/*
 * Procedure     : free_blocked_exec_link
 * Purpose       : free a blocked_exec_link_t
 * Parameters    :
 *    link   : the blocked_exec_link_t to free
 * Return Code   : none
 */
static void free_blocked_exec_link(blocked_exec_link_t * link)
{
   sysevent_free(&(link->name), __FILE__, __LINE__);
   link->name = NULL;
   sysevent_free(&(link->value), __FILE__, __LINE__);
   link->value = NULL;
   TRIGGER_MGR_free_cloned_action(&(link->action));
   sysevent_free(&link, __FILE__, __LINE__);
}

/*
 * Procedure     : add_executing_action_to_list
 * Purpose       : add an action to a list
 * Parameters    :
 *    link   : a link containing the action to execute
 *    list   : the head of the list to add the action to
 *    collapse : if non zero then a list of this action can only be 2 deep 
 *               (the one currently executing and 1 to execute next)
 * Return Code   :
 *    NOTE  : This must be called with serialization mutex locked
 */
static void add_executing_action_to_list(blocked_exec_link_t * link, blocked_exec_link_t * list, const int collapse)
{
   if (NULL == list->bucket) {
      list->bucket = link;
   } else {
      if (collapse) {
         /*
          * if collapse is set then only allow one entry in the bucket
          * we save the latest entry (the one in link)
          */
         blocked_exec_link_t* temp;
         temp = list->bucket;
         list->bucket = link;

         while (NULL != temp) {
            blocked_exec_link_t* temp2;
            temp2 = temp->bucket; 
            free_blocked_exec_link(temp);
            temp = temp2;
         }
      } else {
         while (NULL != (list->bucket)->bucket) {
            list = list->bucket;
         }
         (list->bucket)->bucket = link;
      }
   }
}

/*
 * Procedure     : find_executing_action
 * Purpose       : find an action in the list if it is already there
 * Parameters    :
 *    link   : a link containing the action to execute
 *    collapse : if non zero then a list of this action can only be 2 deep 
 *               (the one currently executing and 1 to execute next)
 * Return        : a block_exec_link to execute by this thread, 
 *                 or NULL if the thread doesnt have to execute anything
 *
 *    NOTE  : This must be called with serialization mutex locked
 */
static blocked_exec_link_t * find_executing_action(blocked_exec_link_t * link, int collapse)
{
   /*
    * global_blocked_exec_head points to the first link of a chain of blocked executables
    * each link (prev/next) describes an action with a unique target (executable) that
    * requires activation. The bucket pointer points to other instances of the same target
    * executable that are waiting their turn for activation.
    */
   if (NULL == global_blocked_exec_head) {
      global_blocked_exec_head = link;
      return(link);
   } else {
      blocked_exec_link_t *cur = global_blocked_exec_head;
      while (NULL != cur) {
         if ((cur->action).used) {
            if (0 == strcmp((cur->action).action, (link->action).action)) {
               /*
                * we have found an executing thread already working on this executable.
                * So add ourselves to its bucket
                */
               add_executing_action_to_list(link, cur, collapse);
               return(NULL);
            }
         } 
         if (NULL == cur->next) {
            // we are at the end of the chain without finding an executing instance for this tuple
            // so link it up and we will use the link
            cur->next = link;
            link->prev = cur;
            return(link);
         }

         cur = cur->next;
      }
      return(NULL);
   }
   return(NULL);
}

/*
 * Procedure     : get_next_to_execute
 * Purpose       : find the next action that needs to be executed.
 *                 If there is an already executing action of this type
 *                 then we will add the link to the queue and there wont
 *                 be any action that this thread needs to do
 * Parameters    :
 *    link     : a link containing the action to execute
 *    collapse : if non zero then a list of this action can only be 2 deep 
 *               (the one currently executing and 1 to execute next)
 * Return        :
 *    A cloned action or NULL if no action needs to be executed by this thread
 */
static blocked_exec_link_t * get_next_to_execute(blocked_exec_link_t * link, int collapse)
{
   // there is no need to wait, so just execute this action
   if (! link->wait) {
      return(link);
   } else {
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Attempting to get mutex: serialization\n", id);
      )
      int rc = pthread_mutex_lock(&serialization_mutex);
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Got mutex: serialization(%d)\n", id, rc);
      )

      link = find_executing_action(link, collapse);

      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: serialization\n", id);
      )
      pthread_mutex_unlock(&serialization_mutex);
      return(link);
   }
}

/*
 * Procedure     : handle_run_executable_msg
 * Purpose       : Handle an se_run_executable_msg command from an internal thread 
 * Parameters    :
 *    fd           : the fd the message came in on. Ignore it
 *    who          : the client asking for the remove
 *    msg          : the message
 *    wait         :
 *       0           : dont wait for child process to return
 *      !0           : wait for child process to terminate before returning
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 * Notes         :
 *   The default for running executables (activations) is that only one copy of 
 *   the executable is allowed to be running on the system at a time. This behavior
 *   is set on a per executable name basis, and may be overridden by setting the 
 *   flag ACTION_FLAG_MULTIPLE_ACTIVATION during the creation of the async request.
 *   The behavior is that a single thread will serially run each executable name - 
 *   one after the other. Multiple treads may be each serially running a different
 *   executable name.
 *   When one thread is handling the arrival of a new activation request, it first checks
 *   to see if another thread is currently handling that name. If so it adds the new request
 *   to the list of requests for that name, and then returns to handle some other message, leaving
 *   the original thread to continue handling the invokation of the executable.)
 *
 *   If ACTION_FLAG_MULTIPLE_ACTIVATION is NOT set, every pending activation is added to a queue.
 *   If ACTION_FLAG_COLLAPSE_PENDING_QUEUE is set, then the queue depth is maintained at maximum 2 -
 *      the currently running process, and the latest request.
 */
static int handle_run_executable_msg(const int local_fd, const token_t who, se_run_executable_msg *msg, int wait)
{
   int                  rc               = 0;
   int                  trigger_id;
   int                  action_id;

   // extract the trigger_id and action_id
   trigger_id = ntohl((msg->async_id).trigger_id);
   action_id  = ntohl((msg->async_id).action_id);
   // extract the subject and value from the message data
   int   subject_bytes;
   int   value_bytes;
   char *subject_str;
   char *value_str;
   char *data_ptr;

   data_ptr      = (char *)&(msg->data);
   subject_str   = SE_msg_get_string(data_ptr, &subject_bytes);
   data_ptr += subject_bytes;
   value_str     =  SE_msg_get_string(data_ptr, &value_bytes);

   // it is possible for the value to be NULL. This would occur if the
   // tuple were unset.
   if (NULL == subject_str) {
      subject_str = emptystr;
   }
   if (NULL == value_str) {
      value_str = emptystr;
   }

  blocked_exec_link_t * link = make_blocked_exec_link(trigger_id, action_id, wait, subject_str, value_str);
  if (NULL == link) {
     return(ERR_SERVER_ERROR);
  }

   if (0 == (link->action).used                               ||
       ACTION_TYPE_EXT_FUNCTION != (link->action).action_type ||
        NULL == (link->action).action) {
      SE_INC_LOG(ERROR,
         printf("Dropping executable msg request for %d %d\n", trigger_id, action_id);
      )
      free_blocked_exec_link(link);
      return(ERR_UNKNOWN_ASYNC_ID);
   }

   action_flag_t flags = ntohl(msg->flags);

   /*
    * if ACTION_FLAG_MULTIPLE_ACTIVATION is set, then multiple activations are allowed.
    * Otherwise we need to make sure only one target process is active at a time.
    */
   int serialize = (flags & ACTION_FLAG_MULTIPLE_ACTIVATION) ? 0 : 1;
   // if we are serializing, then we need to wait for execution to finish
   link->wait |= serialize;

   int collapse = (flags & ACTION_FLAG_COLLAPSE_PENDING_QUEUE);

   /*
    * Go through the list of currently executing actions
    * and find the next one to execute.
    * If there is a thread already working on the current action (in link)
    * then there may be no action to execute (but link will have been added 
    * to the queue)
    */

   link =   get_next_to_execute(link, collapse);
   if (NULL == link) {
               return(0);
   }

   /*
    * At this point  we should be the only thread ready to activate action->action,
    * or this action->action has been set (via flag) to not require
    * serialized activation. In either case we are ready to activate the action
    */
   int sub_needed;
   while (NULL != link) {

      // check the arguments list to see if any argument substitutions are needed
      int i;
      sub_needed = 0;
      for (i=0; i<(link->action).argc; i++) {
         if (NULL != (link->action).argv[i] &&
             (SYSCFG_NAMESPACE_SYMBOL == (link->action).argv[i][0] ||
              SYSEVENT_NAMESPACE_SYMBOL == (link->action).argv[i][0]) ) {
             sub_needed = 1;
             break;
         }
      }

      char **argv;
      if (sub_needed) {
         DATA_MGR_get_runtime_values((link->action).argv);
      }
      argv = (link->action).argv;
      SE_INC_LOG(LISTENER,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d Executing %s  ", id, (link->action).action);
                 for(i=0 ; i<(link->action).argc; i++) {
                    printf("%s ", argv[i]);
                 }
                 printf("\n");
      )
      _eval((link->action).action, link->name, link->value, argv, link->wait);
      SE_INC_LOG(LISTENER,
                 int id = thread_get_id(worker_data_key);
                 printf("Thread %d: Returned from executing %s\n", id, (link->action).action);
      )

      // check if there are any more activations in our bucket
         /*
          * get the next instance of this activation if any
          */
         SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Attempting to get mutex: serialization\n", id);
      )
      int rc = pthread_mutex_lock(&serialization_mutex);

         SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Got mutex: serialization(%d)\n", id, rc);
      )
         blocked_exec_link_t *child;

      if (NULL != link->bucket) {
            // if so, then we will work on it next
         child=link->bucket;
            child->next=link->next;
            child->prev=link->prev;
            if (NULL != link->next) {
               (link->next)->prev = child;
               link->next = NULL;
            }
            if (NULL != link->prev) {
               (link->prev)->next = child;
                link->prev = NULL;
            } else {
               global_blocked_exec_head = child;
            }
            link->bucket= NULL;
         free_blocked_exec_link(link);
         // ready to execute the new link
            link = child;
         } else {
            // unlink from the global list
         if (NULL == link->prev) {
            global_blocked_exec_head = link->next;
         } else {
            (link->prev)->next = link->next;
         }
         if (NULL != link->next) {
            (link->next)->prev = link->prev;
         }
         free_blocked_exec_link(link);
            // make link NULL so that we exit the loop
            link           = NULL;
         }

         SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Releasing mutex: serialization\n", id);
         )
         pthread_mutex_unlock(&serialization_mutex);
      }
   return(rc);
}
/*
 =======================================================================
                   SERIAL EXECUTION
 =======================================================================
*/

/*
 * Procedure     : get_next_to_execute_serially
 * Purpose       : find the next serial actions
 *                 If there is an already executing serial actions of this type
 *                 then we will add the link to the queue and there wont
 *                 be any action that this thread needs to do
 * Parameters    :
 *    link     : a link containing the action to execute
 * Return        :
 *    The serial executions to execute or NULL if none needed to be done by this thread 
 */
static serial_msg_link_t * get_next_to_execute_serially(serial_msg_link_t * link)
{
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: global_serial_msgs_mutex\n", id);
   )
      if (0 != pthread_mutex_lock(&global_serial_msgs_mutex)) {
         SE_INC_LOG(ERROR,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Got pthread_mutex_lock failed (%d) %s\n", id, errno, strerror(errno));
         )
      }
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: global_serial_msgs_mutex\n", id);
   )

   if (NULL == global_serial_msgs_head) {
      global_serial_msgs_head = link;

      SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
      )
      pthread_mutex_unlock(&global_serial_msgs_mutex);
      return(link);
   } else {

      serial_msg_link_t *cur = global_serial_msgs_head;

      do {
         if ((cur->async_id).trigger_id == (link->async_id).trigger_id) {
            /*
             * we have found an executing thread.
             * each trigger_id represents a tuple. We want to treat each tuple as a serial bottleneck.
             * So we will add this serial_msg to the list of serial msgs for this tuple.
             * A further optimization is that we dont need to do all of the pending serial msgs for 
             * the tuple, only the currently executing one, and this - the latest - one.
             * So we clean up any older serial msgs
             */
            if (NULL != cur->bucket) {
               SE_INC_LOG(ALLOC_FREE, 
                  printf("handle_execute_serially frees unrequired serial_msg_list_t at %p\n", cur->bucket->list);
               )
               // free the data because it had been allocated by Trigger Mgr
               sysevent_free(&(cur->bucket->list), __FILE__, __LINE__);
               sysevent_free(&(cur->bucket), __FILE__, __LINE__);
            }
            // link to ourself and we are done
            // the thread currently running this asyncid serially will run this serial msg next
            cur->bucket = link;
            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
            )
            pthread_mutex_unlock(&global_serial_msgs_mutex);
            return(NULL);
         }  

         if (NULL == cur->next) {
            // we are at the end of the chain without finding an executing instance for this tuple
            // so link it up and we will use the current serial msg
            cur->next = link;
            link->prev = cur;

            SE_INC_LOG(MUTEX,
               int id = thread_get_id(worker_data_key);
               printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
            )
            pthread_mutex_unlock(&global_serial_msgs_mutex);
            return(link);
         }
         cur = cur->next;
      } while (NULL != cur); 
   }

   // we should never get here
   SE_INC_LOG(ERROR,
       printf("Unexpected execution of get_next_to_execute_serially past normal return point\n");
   )
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
   )
   pthread_mutex_unlock(&global_serial_msgs_mutex);
   return(NULL);
}

/*
 * Procedure     : handle_execute_serially
 * Purpose       : Handle an se_handle_execute_serially command from an internal thread 
 * Parameters    :
 *    fd           : the fd the message came in on. Ignore it
 *    who          : 
 *    msg          : the message
 * Return Code   :
 *     0            : Message handled.
 *    !0            : some error
 */
static int handle_execute_serially(const int local_fd, const token_t who, se_run_serially_msg *msg)
{
   serial_msg_link_t *link = sysevent_malloc(sizeof(serial_msg_link_t), __FILE__, __LINE__);

   /*
    * global_serial_msgs_head points to the first link of a chain of serial_msgs 
    * each link represents serial msgs from one tuple
    */
   if (NULL == link) {
      // free the data because it had been allocated by Trigger Mgr
      sysevent_free (&(msg->data), __FILE__, __LINE__);
      return(-1011);
   } else {
      link->list     = (se_buffer *)msg->data;
      link->num_msgs = msg->num_msgs;
      link->async_id = msg->async_id;
      link->prev     = NULL;
      link->next     = NULL;
      link->bucket   = NULL;
   }

   // either add link to an existing serialized execution (returned link is NULL) or 
   // start on the list of executions (returned link is not NULL)

   link = get_next_to_execute_serially(link);

   while (NULL != link) {
      unsigned int i;
      se_buffer *list = link->list;

      for(i=0 ; i< link->num_msgs; i++) {
         se_msg_hdr *hdr      = (se_msg_hdr *)(&(list[i]));
         char       *body     = SE_MSG_HDR_2_BODY(hdr);
         int         msg_type =  ntohl(hdr->mtype);
         switch (msg_type) {
            case (SE_MSG_SEND_NOTIFICATION):
            {
               se_send_notification_msg *new = (se_send_notification_msg *)body;
               handle_send_notification_msg(local_fd, who, new);
               break;
            }
            case (SE_MSG_RUN_EXTERNAL_EXECUTABLE):
            {
               se_run_executable_msg *new = (se_run_executable_msg *)body;
               handle_run_executable_msg(local_fd, who, new, 1);
               break;
            }
            default:
            {
               SE_INC_LOG(ERROR,
                          printf("Unhandled case in handle_execute_serially 0x%x (%d)\n", msg_type,msg_type);
                                )
            }
         }
      }

      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Attempting to get mutex: global_serial_msgs_mutex\n", id);
      )
      if (0 != pthread_mutex_lock(&global_serial_msgs_mutex)) {
         SE_INC_LOG(ERROR,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Got pthread_mutex_lock failed (%d) %s\n", id, errno, strerror(errno));
         )
      }
      SE_INC_LOG(MUTEX,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Got mutex: global_serial_msgs_mutex\n", id);
      )

      serial_msg_link_t *child    = link->bucket;

      if (NULL != child) {
         serial_msg_link_t *tlink = NULL;
         /*
          * if there is a task to service in our bucket then do it
          */
         // if so, then we will work on it next
         child->next=link->next;
         child->prev=link->prev;
         if (NULL != link->next) {
            (link->next)->prev = child;
            link->next = NULL;
         }
         if (NULL != link->prev) {
            (link->prev)->next = child;
             link->prev = NULL;
         } else {
            global_serial_msgs_head = child;
         }
         link->bucket= NULL;
      
         tlink = link;

         // assign child to link to service it in the next loop
         link = child;
      SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
      )
      pthread_mutex_unlock(&global_serial_msgs_mutex);
      
         if (NULL != tlink) {
      // free the data because it had been allocated by Trigger Mgr
            sysevent_free(&(tlink->list), __FILE__, __LINE__);
            sysevent_free(&tlink, __FILE__, __LINE__);
         }
      } else {
         /*
          * The thread is finished with all serialized tasks
          */

         // unlink from the global list 
         if (NULL == link->prev) {
            global_serial_msgs_head = link->next;
         } else { 
            (link->prev)->next = link->next;
         }
         if (NULL != link->next) {
            (link->next)->prev = link->prev;
         }
         link->next = NULL;
         link->prev = NULL;

         SE_INC_LOG(MUTEX,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Releasing mutex: global_serial_msgs_mutex\n", id);
         )
         pthread_mutex_unlock(&global_serial_msgs_mutex);

         // free the data because it had been allocated by Trigger Mgr
         sysevent_free(&(link->list), __FILE__, __LINE__);
         sysevent_free(&link, __FILE__, __LINE__);

         // make link NULL so we exit the loop
         link = NULL;
      }         
      // mutex is off already
   }          // while (NULL != link)

   return(0);
}

/*
 * Procedure     : handle_messagedata_from_client
 * Purpose       : Handle a message from a client
 * Parameters    :
 *    fd           : File Descriptor to receive on
 * Return Code   :
 *     0            : Message handled.
 *    !0            : Error code 
 */
static int handle_messagedata_from_client(const int fd)
{
   unsigned  int msglen = sysevent_get_binmsg_maxsize();
   char *msg = sysevent_malloc(msglen,__FILE__, __LINE__);
   token_t   who;
   int       msgtype;
   int rc = 0;

   if (NULL == msg)
       return -1;
   /*
    * The multi threaded nature of syseventd requires us to ensure that the fd hasn't
    * been reused already by the main thread
    */ 
   if (TOKEN_INVALID == CLI_MGR_fd2id) {
      int id = thread_get_id(worker_data_key);
      SE_INC_LOG(ERROR,
        printf("Thread %d: fd %d represents a stale client. Handled correctly\n", id, fd);
      )
      sysevent_free(&msg, __FILE__, __LINE__);
      return(ERR_UNKNOWN_CLIENT);
   }

   msgtype =  SE_minimal_blocking_msg_receive(fd, msg, &msglen, &who);
      
   SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: %s (%d) received from client %x on fd %d\n", id, SE_print_mtype(msgtype), msgtype, (unsigned int)who, fd);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
       SE_print_message(msg, msgtype);
   )
   SE_INC_LOG(SEMAPHORE,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Posting to worker_sem (cli)\n", id);
   )

   thread_set_state(worker_data_key, 0);
   sem_post(&worker_sem);
   switch(msgtype) {
      case (SE_MSG_CLOSE_CONNECTION):
      {
         rc = handle_close_connection_request(fd, who);
         break;
      }
      case (SE_MSG_PING):
      {
         se_ping_msg *new = (se_ping_msg *)msg;
         rc = handle_ping_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_GET):
      {
         se_get_msg *new = (se_get_msg *)msg;
         rc = handle_get_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
     case (SE_MSG_GET_DATA):
      {
         se_get_msg *new = (se_get_msg *)msg;
         rc = handle_get_request_data(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }

      case (SE_MSG_SET):
      {
         se_set_msg *new = (se_set_msg *)msg;
         rc = handle_set_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SET_DATA):
      {
         se_set_msg *new = (se_set_msg *)msg;
         rc = handle_set_request_data(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SET_UNIQUE):
      {
         se_set_unique_msg *new = (se_set_unique_msg *)msg;
         rc = handle_set_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_DEL_UNIQUE):
      {
         se_del_unique_msg *new = (se_del_unique_msg *)msg;
         rc = handle_del_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_ITERATE_GET):
      {
         se_iterate_get_msg *new = (se_iterate_get_msg *)msg;
         rc = handle_get_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_NEXT_ITERATOR_GET):
      {
         se_iterate_get_iterator_msg *new = (se_iterate_get_iterator_msg *)msg;
         rc = handle_get_next_unique_iterator(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SET_OPTIONS):
      {
         se_set_options_msg *new = (se_set_options_msg *)msg;
         rc = handle_set_options_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SET_ASYNC_ACTION):
      {
         se_set_async_action_msg *new = (se_set_async_action_msg *)msg;
         rc = handle_set_async_action_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SET_ASYNC_MESSAGE):
      {
         se_set_async_message_msg *new = (se_set_async_message_msg *)msg;
         rc = handle_set_async_message_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_REMOVE_ASYNC):
      {
         se_remove_async_msg *new = (se_remove_async_msg *)msg;
         rc = handle_remove_async_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_SHOW_DATA_ELEMENTS):
      {
         se_show_data_elements_msg *new = (se_show_data_elements_msg *)msg;
         rc = handle_show_data_elements(who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         break;
      }
      case (SE_MSG_NONE): 
      {
         CLI_MGR_handle_client_error_by_fd(fd);
         break;
      }
      case (SE_MSG_ERRORED): {
         break;
      }
      default:
         SE_INC_LOG(INFO,
            printf("Unhandled message from client type 0x%x\n", msgtype);
         )
         rc = ERR_UNHANDLED_CASE_STATEMENT;
         break;
   }
   sysevent_free(&msg, __FILE__, __LINE__);
   return rc;
}


/*
 * Procedure     : handle_message_from_client
 * Purpose       : Handle a message from a client
 * Parameters    :
 *    fd           : File Descriptor to receive on
 * Return Code   :
 *     0            : Message handled.
 *    !0            : Error code 
 */
static int handle_message_from_client(const int fd)
{
   se_buffer msg;
   unsigned  int msglen = sizeof(msg);
   token_t   who;
   int       msgtype;
   /*
    * The multi threaded nature of syseventd requires us to ensure that the fd hasn't
    * been reused already by the main thread
    */ 
   if (TOKEN_INVALID == CLI_MGR_fd2id) {
      int id = thread_get_id(worker_data_key);
      SE_INC_LOG(ERROR,
        printf("Thread %d: fd %d represents a stale client. Handled correctly\n", id, fd);
      )
      return(ERR_UNKNOWN_CLIENT);
   }

   msgtype =  SE_minimal_blocking_msg_receive(fd, msg, &msglen, &who);
      
   SE_INC_LOG(MESSAGES,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: %s (%d) received from client %x on fd %d\n", id, SE_print_mtype(msgtype), msgtype, (unsigned int)who, fd);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
       SE_print_message(msg, msgtype);
   )
   SE_INC_LOG(SEMAPHORE,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Posting to worker_sem (cli)\n", id);
   )

   thread_set_state(worker_data_key, 0);
   sem_post(&worker_sem);
   switch(msgtype) {
      int rc;
      case (SE_MSG_CLOSE_CONNECTION):
      {
         rc = handle_close_connection_request(fd, who);
         return(rc);
         break;
      }
      case (SE_MSG_PING):
      {
         se_ping_msg *new = (se_ping_msg *)&msg;
         rc = handle_ping_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_GET):
      {
         se_get_msg *new = (se_get_msg *)&msg;
         rc = handle_get_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SET):
      {
         se_set_msg *new = (se_set_msg *)&msg;
         rc = handle_set_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SET_UNIQUE):
      {
         se_set_unique_msg *new = (se_set_unique_msg *)&msg;
         rc = handle_set_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_DEL_UNIQUE):
      {
         se_del_unique_msg *new = (se_del_unique_msg *)&msg;
         rc = handle_del_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_ITERATE_GET):
      {
         se_iterate_get_msg *new = (se_iterate_get_msg *)&msg;
         rc = handle_get_unique_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_NEXT_ITERATOR_GET):
      {
         se_iterate_get_iterator_msg *new = (se_iterate_get_iterator_msg *)&msg;
         rc = handle_get_next_unique_iterator(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SET_OPTIONS):
      {
         se_set_options_msg *new = (se_set_options_msg *)&msg;
         rc = handle_set_options_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SET_ASYNC_ACTION):
      {
         se_set_async_action_msg *new = (se_set_async_action_msg *)&msg;
         rc = handle_set_async_action_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SET_ASYNC_MESSAGE):
      {
         se_set_async_message_msg *new = (se_set_async_message_msg *)&msg;
         rc = handle_set_async_message_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_REMOVE_ASYNC):
      {
         se_remove_async_msg *new = (se_remove_async_msg *)&msg;
         rc = handle_remove_async_request(fd, who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_SHOW_DATA_ELEMENTS):
      {
         se_show_data_elements_msg *new = (se_show_data_elements_msg *)&msg;
         rc = handle_show_data_elements(who, new);
         CLI_MGR_clear_client_error_by_fd (fd);
         return(rc);
         break;
      }
      case (SE_MSG_NONE): 
      {
         CLI_MGR_handle_client_error_by_fd(fd);
         return(0);
         break;
      }
      case (SE_MSG_ERRORED): {
         return(0);
         break;
      }
      default:
         SE_INC_LOG(INFO,
            printf("Unhandled message from client type 0x%x\n", msgtype);
         )
         return(ERR_UNHANDLED_CASE_STATEMENT);
         break;
   }
   return(0);
}

/*
 * Procedure     : handle_message_from_main_thread
 * Purpose       : Handle a message from the main thread
 * Parameters    :
 *    fd           : File Descriptor to receive on
 * Return Code   :
 *     0            : Message handled.
 *    -1            : No message received
 */
static int handle_message_from_main_thread(int fd)
{
   se_buffer msg;
   unsigned  int msglen = sizeof(msg);
   token_t   who;

   // Note: Mutex protection is probably not required on the receive because
   // we are already under a semaphore. But just in case ...
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: main_communication\n", id);
   )
   pthread_mutex_lock(&main_communication_mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: main_communication\n", id);
   )

   int msgtype = SE_minimal_blocking_msg_receive(fd, msg, &msglen, &who);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: main_communication\n", id);
   )
   pthread_mutex_unlock(&main_communication_mutex);

   SE_INC_LOG(MESSAGES,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d: %s (%d) received from main thread\n", id, SE_print_mtype(msgtype), msgtype);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
         SE_print_message(msg, msgtype);
   )

   SE_INC_LOG(SEMAPHORE,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Posting to worker_sem (main)\n", id);
   )
   thread_set_state(worker_data_key, 0);
   sem_post(&worker_sem);
   switch(msgtype) {
      case (SE_MSG_NEW_CLIENT):
      {
         // just wake up and at least recalculate the read fdset
         break;
      }
      case (SE_MSG_DIE):
      {
         pthread_exit(NULL);
         break;
      }
      case (SE_MSG_NONE):
      {
         SE_INC_LOG(INFO,
            printf("SE_MSG_NONE message received from main thread/n");
         )
         break;
      }
      case (SE_MSG_ERRORED):
      {
         break;
      }
      default:
         SE_INC_LOG(INFO,
            printf("Unhandled message from main thread type 0x%x\n", msgtype);
         )
         return(ERR_UNHANDLED_CASE_STATEMENT);
         break;
   }
   return(0);
}

static int handle_messagedata_from_trigger_thread(int fd)
{
   unsigned  int msglen = sysevent_get_binmsg_maxsize();
   char *msg = sysevent_malloc(msglen,__FILE__, __LINE__);
   token_t   who;
   int       rc         = 0;

    if (NULL == msg)
            return -1;
   // Note: Mutex protection is probably not required on the receive because
   // we are already under a semaphore. But just in case ...
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: trigger_communication\n", id);
   )
   pthread_mutex_lock(&trigger_communication_mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: trigger_communication\n", id);
   )

   int msgtype = SE_minimal_blocking_msg_receive(fd, msg, &msglen, &who);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: trigger_communication\n", id);
   )
   pthread_mutex_unlock(&trigger_communication_mutex);

   SE_INC_LOG(MESSAGES,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: %s (%d) received from trigger thread\n", id, SE_print_mtype(msgtype), msgtype);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
         SE_print_message(msg, msgtype);
   )

   SE_INC_LOG(SEMAPHORE,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Posting to worker_sem (trig)\n", id);
   )
   thread_set_state(worker_data_key, 0);
   sem_post(&worker_sem);
   switch(msgtype) {
      case (SE_MSG_SEND_NOTIFICATION):
      {
         se_send_notification_msg *new = (se_send_notification_msg *)msg;
         rc = handle_send_notification_msg(fd, who, new);
         break;
      }
      case (SE_MSG_RUN_EXTERNAL_EXECUTABLE):
      {
         se_run_executable_msg *new = (se_run_executable_msg *)msg;
         rc = handle_run_executable_msg(fd, who, new, 0);
         break;
      }
      case (SE_MSG_SEND_NOTIFICATION_DATA):
      {
         se_send_notification_msg *new = (se_send_notification_msg *)msg;
         rc = handle_send_notification_msg_data(fd, who, new);
         break;
      }
      case (SE_MSG_EXECUTE_SERIALLY):
      {
         se_run_serially_msg *new = (se_run_serially_msg *)msg;
         rc = handle_execute_serially(fd, who, new);
         break;
      }
      default:
         SE_INC_LOG(INFO,
            printf("Unhandled message from trigger thread type 0x%x\n", msgtype);
         )
         rc = ERR_UNHANDLED_CASE_STATEMENT;
         break;
   }
    sysevent_free(&msg, __FILE__, __LINE__);
   return(rc);
}

/*
 * Procedure     : handle_message_from_trigger_thread
 * Purpose       : Handle a message from the trigger thread
 * Parameters    :
 *    fd           : File Descriptor to receive on
 * Return Code   :
 *     0            : Message handled.
 *    -1            : No message received
 */
static int handle_message_from_trigger_thread(int fd)
{
   se_buffer msg;
   unsigned  int msglen = sizeof(msg);
   token_t   who;
   int       rc         = 0;

   // Note: Mutex protection is probably not required on the receive because
   // we are already under a semaphore. But just in case ...
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Attempting to get mutex: trigger_communication\n", id);
   )
   pthread_mutex_lock(&trigger_communication_mutex);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Got mutex: trigger_communication\n", id);
   )

   int msgtype = SE_minimal_blocking_msg_receive(fd, msg, &msglen, &who);
   SE_INC_LOG(MUTEX,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Releasing mutex: trigger_communication\n", id);
   )
   pthread_mutex_unlock(&trigger_communication_mutex);

   SE_INC_LOG(MESSAGES,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d: %s (%d) received from trigger thread\n", id, SE_print_mtype(msgtype), msgtype);
   )
   SE_INC_LOG(MESSAGE_VERBOSE,
         SE_print_message(msg, msgtype);
   )

   SE_INC_LOG(SEMAPHORE,
      int id = thread_get_id(worker_data_key);
      printf("Thread %d Posting to worker_sem (trig)\n", id);
   )
   thread_set_state(worker_data_key, 0);
   sem_post(&worker_sem);
   switch(msgtype) {
      case (SE_MSG_SEND_NOTIFICATION):
      {
         se_send_notification_msg *new = (se_send_notification_msg *)&msg;
         rc = handle_send_notification_msg(fd, who, new);
         break;
      }
      case (SE_MSG_RUN_EXTERNAL_EXECUTABLE):
      {
         se_run_executable_msg *new = (se_run_executable_msg *)&msg;
         rc = handle_run_executable_msg(fd, who, new, 0);
         break;
      }
      case (SE_MSG_EXECUTE_SERIALLY):
      {
         se_run_serially_msg *new = (se_run_serially_msg *)&msg;
         rc = handle_execute_serially(fd, who, new);
         break;
      }
      default:
         SE_INC_LOG(INFO,
            printf("Unhandled message from trigger thread type 0x%x\n", msgtype);
         )
         return(ERR_UNHANDLED_CASE_STATEMENT);
         break;
   }
   return(rc);
}

/*
 * Procedure     : worker_thread_main
 * Purpose       : Thread start routine for worker
 * Parameters    :
 *   arg             : a pointer to private data for this thread. 
 * Return Code   :
 */
void *worker_thread_main(void *arg)
{
//   worker_thread_private_info_t *priv_data_p;
   if (NULL == arg) {
      return((void *)-1);
   } else {
//      priv_data_p = (worker_thread_private_info_t *) arg;
//      if (0 != pthread_setspecific(worker_data_key, priv_data_p)) {
      if (0 != pthread_setspecific(worker_data_key, arg)) {
         SE_INC_LOG(ERROR,
            printf("Thread  Unable to pthread setspecific\n");
         )
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, 
               "pthread setspecific failed (%d) %s", errno, strerror(errno));
      }
   }

   /*
    * A worker thread spends its life waiting on a semaphore,
    * then waiting for a message containing work,
    * and then toiling on the work item until it has
    * been completed.
    */
   int failures = 0;
   for ( ; ; ) {
      SE_INC_LOG(SEMAPHORE,
         int id = thread_get_id(worker_data_key);
         printf("Thread %d Waiting on worker_sem\n", id);
      )

      // wait on the semaphore
      while (-1 == sem_wait(&worker_sem));

      thread_set_state(worker_data_key, 1); 
      thread_activated(worker_data_key);
      int id = thread_get_id(worker_data_key);
      SE_INC_LOG(SEMAPHORE,
         printf("Thread %d Got worker_sem\n", id);
      )

      fd_set rd_set;
      int maxfd;
      FD_ZERO(&rd_set);
      FD_SET(main_communication_fd_listener_end, &rd_set);
      maxfd = main_communication_fd_listener_end;
      FD_SET(trigger_communication_fd_listener_end, &rd_set);
      if (trigger_communication_fd_listener_end > maxfd) {
         maxfd = trigger_communication_fd_listener_end;
      }

      FD_SET(trigger_datacommunication_fd_listener_end, &rd_set);
      if (trigger_datacommunication_fd_listener_end > maxfd) {
         maxfd = trigger_datacommunication_fd_listener_end;
      }


      // go through the table of clients and figure out what file descriptors
      // we should be listening to
      unsigned int i;
      unsigned int j;
      for (i=0,j=0; j<global_clients.num_cur_clients && i<global_clients.max_cur_clients; i++) {
         if ((global_clients.clients)[i].used) {
            int cur_fd;
            cur_fd = (global_clients.clients)[i].fd;
            if (-1 == cur_fd) {
               SE_INC_LOG(ERROR,
                  printf("main select got used client with a bad fd. Ignoring\n");
               )
              incr_stat_info(STAT_WORKER_MAIN_SELECT_BAD_FD);
            } else {
               FD_SET(cur_fd, &rd_set);
               if (cur_fd > maxfd) {
                  maxfd = cur_fd;
               }
            }
            j++;
         }
      }

      // now wait for a message
SE_INC_LOG(SEMAPHORE,
  int id = thread_get_id(worker_data_key);
  printf("Thread %d selecting ...", id);
  fflush(NULL);
)

      int rc  = select(maxfd+1, &rd_set, NULL, NULL, NULL);

SE_INC_LOG(SEMAPHORE,
  int id = thread_get_id(worker_data_key);
  printf("Thread %d Awake rc = %d\n", id, rc);
  fflush(NULL);
)

      // if the select had an error then there is nothing to do
      // just allow the next thread to get the semaphore and go
      // back to waiting on the sema
      if (0 > rc) {
         failures++;
         SE_INC_LOG(ERROR,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d: Select error. (%d) %s\n", id, errno, strerror(errno));
         )
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, 
               "Select on main fds failed (%d) %s", errno, strerror(errno));
            
         if (5 < failures) {
            SE_INC_LOG(ERROR,
               printf("There have been %d consecutive select failure. Backing off\n", failures);
            )
            ulog(ULOG_SYSTEM, UL_SYSEVENT, "Multiple select failures. Sysevent will slow servicing") ;
            sleep(failures);
         }
         SE_INC_LOG(SEMAPHORE,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Posting to worker_sem (err)\n", id);
         )
         thread_set_state(worker_data_key, 0);
         sem_post(&worker_sem);
         continue;
      } else if (0 == rc) {
         SE_INC_LOG(SEMAPHORE,
            int id = thread_get_id(worker_data_key);
            printf("Thread %d Posting to worker_sem (err)\n", id);
         )
         thread_set_state(worker_data_key, 0);
         sem_post(&worker_sem);
         continue;
      } else {
         failures = 0;
      }

      /*
       * Every thread first accepts messages from the main thread
       */
      if (FD_ISSET(main_communication_fd_listener_end, &rd_set)) {
         handle_message_from_main_thread(main_communication_fd_listener_end);
         continue;
      }

      int cur_read_fd;
      /* first look for the first ready fd from a client */
      for (cur_read_fd=0; cur_read_fd<=maxfd; cur_read_fd++) {
         if (cur_read_fd != trigger_communication_fd_listener_end && FD_ISSET(cur_read_fd, &rd_set)) {
             if (cur_read_fd != trigger_datacommunication_fd_listener_end)
             {
                break;
             }
         }
      }
      /* if found then handle */
      if (maxfd >= cur_read_fd) {
          int dataClient = 0;
          int index = 0;
          for (index=0; index < global_clients.max_cur_clients; ++index) {
              if ((global_clients.clients)[index].used) {
                  int cur_fd;
                  cur_fd = (global_clients.clients)[index].fd;
                  if (-1 == cur_fd) {
                      SE_INC_LOG(ERROR,
                              printf("main select got used client with a bad fd. Ignoring\n");
                              )
                  }
                  else if (cur_fd == cur_read_fd)
                  {
                      if ((global_clients.clients)[index].isData)
                      {
                          dataClient = 1;
                          break;
                      }
                  }
              }
          }
           int fileread = access("/tmp/sysevent_debug", F_OK);

          if ((fileread == 0) && dataClient)
          {
              char buf[256] = {0};
              snprintf(buf,sizeof(buf),"echo fname %s: fd %d dataclient %d >> /tmp/sys_d.txt",__FUNCTION__,cur_read_fd,dataClient);
              system(buf);

          }

          if (dataClient == 1)
          {
              handle_messagedata_from_client(cur_read_fd);
          }
          else
          {

              handle_message_from_client(cur_read_fd);
          }
         continue;
      }

      /*
       * If there are no main thread messages and no client messages then
       * all threads except those reserved for non blocking messages will accept messages
       * from the trigger mgr
       */
      int myid = thread_get_id(worker_data_key);
      if (NUM_CLIENT_ONLY_THREAD < myid && FD_ISSET(trigger_communication_fd_listener_end, &rd_set)) {
          handle_message_from_trigger_thread(trigger_communication_fd_listener_end);
          continue;
      }

      if (NUM_CLIENT_ONLY_THREAD < myid && FD_ISSET(trigger_datacommunication_fd_listener_end, &rd_set)) {
          handle_messagedata_from_trigger_thread(trigger_datacommunication_fd_listener_end);
          continue;
      }

      /*
       * If we are here, then no message was handled, probably because this is a cleint only thread and the only ready fd
       * is for trigger, so just set the semaphore and go back to waiting for a message. But take a little nap so give some
       * general thread a chance to run. 
       */
      SE_INC_LOG(SEMAPHORE,
         printf("Thread %d Posting to worker_sem (nowork)\n", id);
      )
      thread_set_state(worker_data_key, 0);
      sem_post(&worker_sem);
      struct timespec req;
      req.tv_sec  = 0;
      req.tv_nsec = 150000000 * thread_get_id(worker_data_key); // 150 msec x thread id
      nanosleep(&req, NULL);
   } // for
}
