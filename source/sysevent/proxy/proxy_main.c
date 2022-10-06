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
 ***************************************************************************
                           sysevent proxy

   The sysevent proxy will proxy events occurring on a remote sysevent daemon
   to the local sysevent daemon.

   This allows processes interested in sysevents occurring on a remote host
   to receive them as though they were locally generated

   The configuration file has the format:
      # comment
      tuple_name
      tuple_name

   where tuple_name is the name of the sysevent tuple to be proxied from 
   the remote to the local sysevent daemon

 ***************************************************************************
 */

#include <stdio.h>
#define __USE_GNU  // to pick up strsignal in string.h
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sysevent.h"
#include "libsysevent_internal.h"
#include <ulog/ulog.h>

#define SERVICE_NAME "syseventd_proxy"
#define SYSEVENT_PROXY_PID_FILE "/var/run/syseventd_proxy.pid"
/*
 * The number of seconds that we sleep between retrying DNS lookup of 
 * sysevent daemon
 */
#define SYSEVENT_DISCOVER_LOOP_SLEEP_TIME 5

/*
 * The number of seconds that we sleep between pinging the remote 
 * sysevent daemon
 */
#define SYSEVENT_PING_TIME 5

/*
 * The number of retries of PING before we give up
 */
#define SYSEVENT_PING_RETRIES 3

typedef struct {
   int     fd;
   int     query_fd;
   token_t token;
   token_t query_token;
   char    name[256];
} remote_syseventd_t;

/*
 * information about the remote sysevent daemon
 */
static remote_syseventd_t remote_syseventd;

/*
 * information about the local sysevent daemon
 */
static remote_syseventd_t local_syseventd;

static char *event_name = NULL;

/*
 * The name of the remote sysevent daemon host
 */
static char* remote_syseventd_host; 

/*
 * daemon initialization code
 */
static int daemon_init(void)
{
   pid_t pid;

   // are we already a daemon
   if (1 == getppid()) {
      return(0);
   }

   if ( 0 > (pid = fork()) ) {
      return(-1);
   } else if (0 != pid) {
      exit(0);  // parent exits
   }

   // child
   setsid();   // become session leader
   if (chdir("/") < 0) { // change working directory
      return(-1);
   }

   umask(0);   // clear our file mode creation mask

   return(0);
}

/*
 * Procedure     : deinitialize_system
 * Purpose       : Deinitialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is deinitialized
 */
static int deinitialize_system(void)
{
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy exiting"); 
   if (0 <= remote_syseventd.fd) {
      sysevent_close(remote_syseventd.fd, remote_syseventd.token);
      remote_syseventd.fd          = -1;
   }
   if (0 <= remote_syseventd.query_fd) {
      sysevent_close(remote_syseventd.query_fd, remote_syseventd.query_token);
      remote_syseventd.fd          = -1;
   }
   remote_syseventd.token       = 0;
   remote_syseventd.query_token = 0;
   remote_syseventd.name[0]     = '\0';

   // end connection with local syseventd
   unlink(SYSEVENT_PROXY_PID_FILE);

   if (-1 != local_syseventd.fd) {
      if ( 0 != sysevent_set(local_syseventd.fd,
                              local_syseventd.token,
                              SERVICE_NAME, "stopped",0) ) {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot locally set %s %s", SERVICE_NAME, "stopped");
      }
   }
   // if we are proxying for another event, then set that status
   if (NULL != event_name) {
      if (-1 != local_syseventd.fd) {
         if ( 0 != sysevent_set(local_syseventd.fd,
                                 local_syseventd.token,
                                 event_name, "stopped",0) ) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot locally set %s %s", event_name, "stopped");
         } else {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy set %s %s", event_name, "stopped");
         }
      }
   }
   if (0 <= local_syseventd.fd) {
      sysevent_close(local_syseventd.fd, local_syseventd.token);
      local_syseventd.fd          = -1;
   }

   if (0 <= local_syseventd.query_fd) {
      sysevent_close(local_syseventd.query_fd, local_syseventd.query_token);
      local_syseventd.query_fd    = -1;
   }
   local_syseventd.token       = 0;
   local_syseventd.query_token = 0;
   local_syseventd.name[0]     = '\0';

   return(0);
}


/*
 * Procedure     : terminate_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 */
static void terminate_signal_handler (int signum)
{
   ulogf(ULOG_SYSTEM, UL_SYSEVENT,"Proxy received terminate signal %s (%d)", strsignal (signum), signum);
   deinitialize_system();
   exit(0);
}

/*
 * Procedure     : ignore_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 */
static void ignore_signal_handler (int signum)
{
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Proxy received ignore signal %s (%d)", strsignal (signum), signum);
}


/*
 * Procedure     : initialize_system
 * Purpose       : Initialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is initialized
 *    else          : the error
 */
static int initialize_system(void)
{
   daemon_init();

   // save our pid in the pid file
   pid_t pid = getpid();
   FILE *fp = fopen(SYSEVENT_PROXY_PID_FILE, "w");
   if (NULL != fp) {
      fprintf(fp, "%d", pid);
      fclose(fp);
   }

   ulog_init();

   /* set up signal handling
    * we want:
    *   
    * SIGINT     Interrupt from keyboard - Terminate
    * SIGQUIT    Quit from keyboard - Ignore
    * SIGTERM   Termination signal  - Terminate
    * SIGKILL    Kill signal Cant be blocked or Caught
    * SIGPIPE    - Ignore
    * All others are default
    */
   struct sigaction new_action, old_action;
   new_action.sa_handler = terminate_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);

   if (-1 == sigaction (SIGINT, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
            "Proxy problem getting original signal handler for SIGINT. Reason (%d) %s\n",
            errno, strerror(errno));
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGINT, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                 "Proxy problem setting signal handler for SIGINT. Reason (%d) %s\n",
                 errno, strerror(errno));
        }
      }
   }

   if (-1 == sigaction (SIGQUIT, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
            "Proxy problem getting original signal handler for SIGQUIT. Reason (%d) %s\n",
            errno, strerror(errno));
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGQUIT, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                 "Proxy problem setting signal handler for SIGQUIT. Reason (%d) %s\n",
                 errno, strerror(errno));
        }
      }
   }
   if (-1 == sigaction (SIGTERM, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
            "Proxy problem getting original signal handler for SIGTERM. Reason (%d) %s\n",
            errno, strerror(errno));
   } else {
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGTERM, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                 "Proxy problem setting signal handler for SIGTERM. Reason (%d) %s\n",
                 errno, strerror(errno));
        }
      }
   }

   // sigpipe ignore
   new_action.sa_handler = ignore_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGPIPE, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
            "Proxy problem getting original signal handler for SIGPIPE. Reason (%d) %s\n",
            errno, strerror(errno));
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGPIPE, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                 "Proxy problem setting signal handler for SIGPIPE. Reason (%d) %s\n",
                 errno, strerror(errno));
        }
      }
   }

   remote_syseventd.fd          = -1;
   remote_syseventd.query_fd    = -1;
   remote_syseventd.token       = 0;
   remote_syseventd.query_token = 0;
   remote_syseventd.name[0]     = '\0';
   local_syseventd.fd           = -1;
   local_syseventd.query_fd     = -1;
   local_syseventd.token        = 0;
   local_syseventd.query_token  = 0;
   local_syseventd.name[0]      = '\0';
   return(0);
}

/*
 * Procedure     : connect_to_remote_syseventd
 * Purpose       : Connect to a remote sysevent daemon
 * Parameters    : 
 *    remote        : name of the remote sysevent
 *    token         : sysevent token to use
 * Return Code   :
 *    -1            : Unable to connect
 *    >= 0          : The file descriptor used to for this sysevent connection
 */
static int connect_to_remote_syseventd(char* remote, remote_syseventd_t* info)
{
   if (NULL == remote) {
      return(-1);
   }

   /*
    * Open two connections to the remote sysevent daemon. One for notications, and
    * a short lived one for gets
    */
   int fd = sysevent_open(remote, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "syseventd_proxy", &(info->token));
   if (0 > fd) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot sysevent_open to remote %s", remote );
      return(-1);
   } else {
      info->fd = fd;
   }

   fd = sysevent_open(remote, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "syseventd_proxy2", &(info->query_token));
   if (0 > fd) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot sysevent_open 2 to remote %s", remote );
      sysevent_close(info->fd, info->token);
      info->fd    = -1;
      info->token = 0;
      return(-1);
   } else {
      info->query_fd = fd;
   }
   return(0);
}
/*
 * Procedure     : connect_to_local_syseventd
 * Purpose       : Connect to a local sysevent daemon
 * Parameters    :
 *    token         : sysevent token to use
 * Return Code   :
 *    -1            : Unable to connect
 *    >= 0          : The file descriptor used to for this sysevent connection
 */
static int connect_to_local_syseventd(remote_syseventd_t* info)
{
   int fd = sysevent_local_open(UDS_PATH, SE_VERSION, "syseventd_proxy", &(info->token));
   if (0 > fd) {
      return(-1);
   } else {
      info->fd = fd;
   }
   return(0);
}


/*
 * Procedure     : parse_line
 * Purpose       : Parse a line and extract the tuple name
 * Parameters    :
 *    line          : line to parse
 *    tuple         : tuple
 * Return Code   :
 *   -1             : error
 *    0             : no error
 *    1             : comment line or blank line
 */
int parse_line(char* line, char** tuple)
{
   int len = strlen(line);
   if (0 == len || '\n' == line[0]) {
      *tuple = NULL;
      return(1);
   }

   char* line_p = line;
   while (isspace(*line_p)) {
      line_p++;
   }

   if ('#' == *line_p) {
      *tuple = NULL;
      return(1);
   }
 
   *tuple = line_p;
   if ( (line_p - line) > len ) {
      *tuple = NULL;
      return(-1);
   } else {
      char *end = line+len;
      end--;
      while(isspace(*end)) {
         *end = '\0';
          end--;
      }
   }

   return(0);
}


/*
 * Procedure     : handle_notifications
 * Purpose       : Handle notifications from remote sysevent daemons
 * Parameters    :
 * Return Code   :
 *    -1         : Failure
 * Notes         :
 *   This procedure runs until it has detected a failure to communicate with
 *   the remote sysevent daemon.
 */
static int handle_notifications(void)
{
   int sane = SYSEVENT_PING_RETRIES;
   for ( ; ; ) {
      fd_set rd_set;
      int    maxfd = 0;
      FD_ZERO(&rd_set);
      if ( -1 != remote_syseventd.fd) {
         FD_SET(remote_syseventd.fd, &rd_set);
         maxfd = (remote_syseventd.fd + 1);
      } else {
         return(-1);
      }
      struct timeval tv;
      tv.tv_sec  = SYSEVENT_PING_TIME;
      tv.tv_usec = 0;
      // we need to poll so we can detect broken connections
      int rc = select(maxfd, &rd_set, NULL, NULL, &tv);
      if (-1 == rc) {
      } else {
         if (0 >= sane) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Remote Sysevent Proxy %s is non responsive. Shutting down", remote_syseventd_host);
            return(-1);
         }
         if (FD_ISSET(remote_syseventd.fd, &rd_set)) {
            se_buffer            msg_buffer;
            se_notification_msg *msg_body = (se_notification_msg *)msg_buffer;
            unsigned int         msg_size;
            int                  msg_type;
            token_t              from;

            msg_size  = sizeof(msg_buffer);
            msg_type = SE_msg_receive(remote_syseventd.fd, msg_buffer, &msg_size, &from);
            // if not a notification message then ignore it
            if (SE_MSG_NOTIFICATION == msg_type) {
               // extract the name and value from the return message data
               int   name_bytes;
               int   value_bytes;
               char *name_str;
               char *value_str;
               char *data_ptr;
               int   source;
               int   tid;

               source     = ntohl(msg_body->source);
               tid        = ntohl(msg_body->tid);
               data_ptr   = (char *)&(msg_body->data);
               name_str   = (char *)SE_msg_get_string(data_ptr, &name_bytes);
               data_ptr  += name_bytes;
               value_str =  (char *)SE_msg_get_string(data_ptr, &value_bytes);
                 
               sane = SYSEVENT_PING_RETRIES;

               if (-1 != local_syseventd.fd) {
                  if ( 0 != sysevent_set_with_tid(local_syseventd.fd,
                                         local_syseventd.token,
                                          name_str, value_str, source, tid) ) {
                     ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot locally set %s %s", name_str, value_str);
                  }
               }
            } else if (SE_MSG_PING_REPLY == msg_type) {
               sane=SYSEVENT_PING_RETRIES;
            } else {
               sane--;
            }
         } else {
            /*
             * probably a timeout. So ping
             */
            sane--;
            sysevent_ping (remote_syseventd.fd,
                           remote_syseventd.token);
         }
      }
   } 
   return (0);
}

/*
 * Procedure     : main
 * Purpose       : 
 * Parameters    :
 *    argv[1]       : The DNS name of the remote sysevent daemon to attach to
 *    argv[2]       : The path/name of the provisioning file
 *    argv[3]       : optional local name of event to issue upon connection
 *                    of remote sysevent
 */
int main (int argc, char** argv) 
{

   if (3 != argc && 4 != argc) {
      printf("Usage: %s syseventd_name provisioning_filename [local_event]\n", argv[0]);
      printf("   syseventd_name is the FQDN or ip address of the remote sysevent daemon to connect to\n");
      printf("   provisioning_filename is the path/name of the configuration file\n");
      printf("   local_event is the name of a sysevent tuple to set to\n"); 
      printf( "      started when connection is made to the remote sysevent daemon\n");
      printf( "      stopped when connection is lost with the remote sysevent daemon\n");
      return(-1);
   }
   if (4 <= argc) {
      event_name=argv[3];
   }

   // save up the name of the remote sysevent daemon for logging purposes
   remote_syseventd_host=strdup(argv[1]);
      
   initialize_system();
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy started for %s", remote_syseventd_host);
   if (NULL != event_name) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy eventing locally for %s", event_name);
   }

   struct in_addr   syseventd_in_addr;
#ifndef NO_IPV6
   struct in6_addr  syseventd_in6_addr;
#endif
   if (0 == inet_aton(argv[1], &syseventd_in_addr)) {
#ifndef NO_IPV6
   if (0 == inet_pton(AF_INET6, argv[1], &syseventd_in6_addr)) {
#endif
      /*
       * sit in a loop waiting for the remote sysevent daemon name to become resolvable
       */
      int done = 0;
      while (!done) {

         struct addrinfo myaddr , *result, *rp;
         memset ( &myaddr , 0 , sizeof ( myaddr ));
         myaddr.ai_family   = AF_UNSPEC;
         myaddr.ai_socktype = SOCK_STREAM;
         myaddr.ai_flags    = AI_PASSIVE;
         int rc;
         if (0 != (rc = getaddrinfo(argv[1],NULL,&myaddr,&result)) ){
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy unable to getaddinfo for %s (%d) %s\n", 
                   argv[1], rc, gai_strerror(rc));
         } else {
           /* 
            * getaddrinfo() returns a list of address structures.
            */

           for (rp = result; NULL != rp; rp = rp->ai_next) {
#ifndef NO_IPV6
              if (AF_INET6 == rp->ai_family) {
                 syseventd_in6_addr = ((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
                 done=1;
                 break;
              }
#endif
             if (AF_INET == rp->ai_family) {
                syseventd_in_addr = ((struct sockaddr_in *)rp->ai_addr)->sin_addr;
                done=1;
                break;
              }
           }
            freeaddrinfo(result);
         }

          if (!done) {
             sleep(SYSEVENT_DISCOVER_LOOP_SLEEP_TIME);
          }
       }
#ifndef NO_IPV6
   }
#endif
   }


   FILE *fp = fopen(argv[2], "r");
   if (NULL == fp) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy unable to open %s. Exiting.", argv[2]);
      return(-1);
   }

   /*
    * Connect to the remote sysevent daemon
    */
   int done = 0;
   while (!done) {
      done = !(connect_to_remote_syseventd(argv[1], &remote_syseventd));
      if (!done) {
         sleep(SYSEVENT_DISCOVER_LOOP_SLEEP_TIME);
      }
   }
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy Connected to %s", argv[1]);

   /*
    * open a connection to our local sysevent daemon
    */
   connect_to_local_syseventd(&local_syseventd);
   if (NULL != event_name) {
      if (-1 != local_syseventd.fd) {
         if ( 0 != sysevent_set(local_syseventd.fd,
                                local_syseventd.token,
                                event_name, "started",0) ) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot locally set %s %s", event_name, "started");
         }
      }
   }

   /*
    * Read in each line from the provisioning file.
    * Register for each tuple, and get each tuple
    */
   char buffer[1024];
   char *tuple_name;
   int   num_registrations = 0;
   int   rc                = 0;
   while (NULL != fgets(buffer, sizeof(buffer), fp) ) {
      if ('\0' != buffer[0] && 0 == (rc = parse_line(buffer, &tuple_name)) ) {
         num_registrations++;
         se_buffer response;
         /*
          * get the current value of the tuple
          */
         if (0 != sysevent_get(remote_syseventd.query_fd, 
                               remote_syseventd.query_token, 
                               tuple_name, 
                               response, sizeof(response)) ) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot get %s::%s", remote_syseventd_host, tuple_name);
         } else {
            /*
             * Set the tuple locally iff it has a value. 
             * Otherwise we could set off an event
             */
            if (NULL != response && 
               '\0' != response[0] && 
                -1 != local_syseventd.fd) {
               if ( 0 != sysevent_set(local_syseventd.fd,
                                   local_syseventd.token,
                                    tuple_name, response,0) ) {
                  ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot locally set %s %s", tuple_name, response);
               }
            }
         }

         async_id_t async_id; 
         /*
          * request notification of any changes to the tuple
          */
         if (0 != sysevent_setnotification(remote_syseventd.fd, 
                                           remote_syseventd.token, 
                                           tuple_name, 
                                           &async_id)) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, 
                  "Sysevent Proxy cannot set notification on %s::%s", remote_syseventd_host, tuple_name);
         } else {
            sysevent_set_options(remote_syseventd.fd, remote_syseventd.token, tuple_name, TUPLE_FLAG_EVENT);
         }
      } else if (-1 == rc) {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Sysevent Proxy cannot parse line ->%s<-", buffer);
      }
   }

   /*
    * we dont need to get from the set/get connection because all other info comes 
    * from notifications on the primary connection
    */
   if (-1 != remote_syseventd.query_fd) {
      sysevent_close(remote_syseventd.query_fd, remote_syseventd.query_token);
      remote_syseventd.query_fd = -1;
   }

   /*
    * Handle all notification messages from the remote syseventd.
    * This procedure does not return unless remote syseventd becomes
    * unresponsive.
    */
   handle_notifications();

   /*
    * We are done
    */
   deinitialize_system();
   fclose(fp);
   return(0);
}
