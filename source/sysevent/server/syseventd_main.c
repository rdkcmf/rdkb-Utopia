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
                       syseventd_main.c
  
  This file contains the initialization for syseventd, as well as
  the logic for the main thread.

  The main thread is responsible for accepting new client connections
  and then passing them off to the worker threads.

  Author : mark enright 
  ================================================================
 */
#include <stdio.h>
#define __USE_GNU  // to pick up strsignal in string.h
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/time.h>

#ifdef BACKTRACE
#include <execinfo.h>
#endif

#include "safec_lib_common.h"
#include "libsysevent_internal.h"
#include "syseventd.h"
#include "clientsMgr.h"
#include "triggerMgr.h"
#include "dataMgr.h"
#include "secure_wrapper.h"
#ifdef USE_SYSCFG
#include <syscfg/syscfg.h>
#endif
#include <ulog/ulog.h>

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif


int debug_num_sets;
int debug_num_gets;
int debug_num_accepts;

/*
 * name of syseventd fork helper process which does the actual activation of 
 * transient processes
 */
#define SYSEVENTD_FORK_HELPER_PROCESS "syseventd_fork_helper"
#define FORK_HELPER_PATH "/usr/bin"
/*
 * file where sysevent daemon pid is kept
 */
#define SE_SERVER_PID_FILE "/var/run/syseventd.pid"


#define LISTEN_IP_ADDR     INADDR_ANY

#define SYSEVENTD_LOGFILE "/var/log/syseventd.out"
#define SYSEVENTD_ERRFILE "/var/log/syseventd.err"
static FILE *fp_log;
static FILE *fp_err;

//#ifdef SE_SERVER_CODE_DEBUG
// the debug level for debug output
int debugLevel = 0;
//#endif

// id for this sysevent daemon (for if there are multiple syseventd communicating ie syseventd_proxy)
int daemon_node_id;
int daemon_node_msg_num;

// Port for server to listen on.
static short global_client_accept_port;

// worker threads semaphore
sem_t worker_sem;

// thread specific data key
pthread_key_t worker_data_key;

// per thread info
worker_thread_private_info_t thread_private_info[NUM_WORKER_THREAD];

// per thread stats
worker_thread_stat_info_t    thread_stat_info[NUM_WORKER_THREAD];

// number of threads to use
static int numThreads = NUM_WORKER_THREAD;
// Listener thread ids
static pthread_t global_thread_id[NUM_WORKER_THREAD];

// mutex to protect communication fd between main thread and workers
pthread_mutex_t  main_communication_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex to protect communication fd between trigger thread and workers
pthread_mutex_t  trigger_communication_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex to protect communication fd between worker threads to fork helper process
// return communication is via per process pipe
pthread_mutex_t  fork_helper_communication_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex used to serialize
//   activation (using _eval) of executables
pthread_mutex_t  serialization_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex used to serialize
//   serial messages
pthread_mutex_t  global_serial_msgs_mutex = PTHREAD_MUTEX_INITIALIZER;

// mutex used to capture error statistics
//   used to protect se_stat_info buffer
pthread_mutex_t  stat_info_mutex = PTHREAD_MUTEX_INITIALIZER;

// writer side of the communication fd between main thread and workers
int main_communication_fd_writer_end = 0;

// listener side of the communication fd between main thread and workers
int main_communication_fd_listener_end = 0;

// writer side of the communication fd between trigger thread and workers
int trigger_communication_fd_writer_end = 0;

// listener side of the communication fd between trigger thread and workers
int trigger_communication_fd_listener_end = 0;

// writer side of the communication fd between trigger thread and workers
int trigger_datacommunication_fd_writer_end = 0;

// listener side of the communication fd between trigger thread and workers
int trigger_datacommunication_fd_listener_end = 0;

// pipe for threads to communicate to fork helper process;
int fork_helper_pipe[2];

// tcp accept fd waiting for client connections
static int global_tcp_fd = -1;

// uds accept fd waiting for client connections
static int global_uds_connection_fd = -1;

// list of lists of commands triggered by a tuple's change in value
serial_msg_link_t *global_serial_msgs_head = NULL;

// list of executables blocked while waiting for a previous invokation of the executable to exit
blocked_exec_link_t *global_blocked_exec_head = NULL;

// list of pids of clients that have been started by workers but not yet finished
waiting_pid_t waiting_pid[NUM_WORKER_THREAD];

// signal to sanity thread to abort
int sanity_thread_abort = 0;

// structure to maintain general failure statistics
se_stat_info_t stat_info;

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
   chdir("/"); // change working directory

   umask(0);   // clear our file mode creation mask

   return(0);
}

/*
 * Procedure     : deinitialize_system
 * Purpose       : UniInitialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is uninitialized
 * Note: 
 * IMPORTANT: use only async-signal-safe functions
 *            printf, ulog, syslog are not safe
 */
static int deinitialize_system(void)
{

   // stop the fork helper process
   v_secure_system("killall -TERM "SYSEVENTD_FORK_HELPER_PROCESS);
   close(fork_helper_pipe[0]);
   close(fork_helper_pipe[1]);

   if (0 != main_communication_fd_listener_end) {
      close(main_communication_fd_listener_end);
      main_communication_fd_listener_end = 0;
   }

   if (0 != main_communication_fd_writer_end) {
      close(main_communication_fd_writer_end);
      main_communication_fd_writer_end = 0;
   }

   if (0 != trigger_communication_fd_listener_end) {
      close(trigger_communication_fd_listener_end);
      trigger_communication_fd_listener_end = 0;
   }
   if (0 != trigger_communication_fd_writer_end) {
      close(trigger_communication_fd_writer_end);
      trigger_communication_fd_writer_end = 0;
   }

   if (0 != trigger_datacommunication_fd_listener_end) {
      close(trigger_datacommunication_fd_listener_end);
      trigger_datacommunication_fd_listener_end = 0;
   }

   if (0 != trigger_datacommunication_fd_writer_end) {
      close(trigger_datacommunication_fd_writer_end);
      trigger_datacommunication_fd_writer_end = 0;
   }

   if (-1 != global_tcp_fd) {
      close(global_tcp_fd);
      global_tcp_fd = -1;
   }
   if (-1 != global_uds_connection_fd) {
      close(global_uds_connection_fd);
      global_uds_connection_fd = -1;
   }
  unlink(UDS_PATH);

   // uninitialize the clients manager
   CLI_MGR_deinit_clients_table();

   // uninitialize the trigger manager
   TRIGGER_MGR_deinit();

   // uninitialize the data model manager
   DATA_MGR_deinit();

   // close all of the fifos shared with the fork helper process
   int i;
   char fifo_name[256];
   for (i=0; i<numThreads; i++) {
      if (-1 != thread_private_info[i].fd) {
         close(thread_private_info[i].fd);
         thread_private_info[i].fd = -1;
         snprintf(fifo_name, sizeof(fifo_name), "/tmp/syseventd_worker_%d", thread_private_info[i].id);
         unlink(fifo_name);
      }
   }

   SE_INC_LOG(SEMAPHORE,
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "Destroying semaphore worker_sem\n");
   )
   sem_destroy(&worker_sem);

   // we are not freeing memory from global_serial_msgs_head and global_blocked_exec_head because we are exiting
   global_serial_msgs_head = NULL;
   global_blocked_exec_head = NULL;

   unlink(SE_SERVER_PID_FILE);

   daemon_node_id     = 0;
   daemon_node_msg_num = 1;
   ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd stopped\n");
   return(0);
}

/*
 * Procedure     : terminate_signal_handler
 * Purpose       : Handle a signal
 * Parameters    : 
 *   signum         : The signal received
 * Return Code   : None
 * Note: 
 * IMPORTANT: use only async-signal-safe functions
 *            printf, ulog, syslog are not safe
 */
static void terminate_signal_handler (int signum)
{
#ifdef BACKTRACE
   void *buffer[1000];
   char **strings;
   int nptrs;
   nptrs = backtrace(buffer, 1000);
   fprintf(stderr, "backtrace returned %d address\n", nptrs);
         char **backtrace_symbols(void *const *buffer, int size);

   backtrace_symbols_fd(buffer, 1000, stderr);
#endif

   ulog(ULOG_SYSTEM, UL_SYSEVENT, "Received terminate signal\n");
   fprintf(stdout, "Received terminate signal %d\n", signum);
   sanity_thread_abort=1;
   if (0 != main_communication_fd_writer_end) {
      char msg[100];
      se_die_msg *body;
      body = (se_die_msg *) SE_msg_prepare(msg, sizeof(msg), SE_MSG_DIE, TOKEN_NULL);
      if (NULL != body) {
         int i;
         void *result;
         SE_INC_LOG(MUTEX,
            ulog(ULOG_SYSTEM, UL_SYSEVENT, "Attempting to get mutex: main_communication\n");
         )
         pthread_mutex_lock(&main_communication_mutex);
         SE_INC_LOG(MUTEX,
            ulog(ULOG_SYSTEM, UL_SYSEVENT, "Got mutex: main_communication\n");
         )
         for (i=0; i<numThreads; i++) {
            if (0 != global_thread_id[i]) {
               SE_msg_send(main_communication_fd_writer_end, msg);
               global_thread_id[i] = 0;
            }
         }
         SE_INC_LOG(MUTEX,
            ulog(ULOG_SYSTEM, UL_SYSEVENT, "Releasing mutex: main_communication\n");
         )
         pthread_mutex_unlock(&main_communication_mutex);

         for (i=0; i<numThreads; i++) {
            if (0 != global_thread_id[i]) {
               pthread_join(global_thread_id[i], &result);
               global_thread_id[i] = 0;
            }
         }
      }
   }

   deinitialize_system();
   exit(0);
}

/*
 * Procedure     : ignore_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 * Note: 
 * IMPORTANT: use only async-signal-safe functions
 *            printf, ulog, syslog are not safe
 */
static void ignore_signal_handler (int signum)
{
    printf("Received ignore signal %s (%d)\n", strsignal (signum), signum);
   //ulog_safe(1, ULOG_SYSTEM, UL_SYSEVENT, "Received ignore signal\n");
}

/*
 * Procedure     : reinit_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 * Note: 
 * IMPORTANT: use only async-signal-safe functions
 *            printf, ulog, syslog are not safe
 */
static void reinit_signal_handler (int signum)
{
   // printf("Received reinit signal %s (%d)\n", strsignal (signum), signum);
   ulog(ULOG_SYSTEM, UL_SYSEVENT, "Received reinit signal");
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
   sanity_thread_abort      = 0;

   // initialize our name in libsysevent
   init_libsysevent("syseventd");

   // initialize debug counters
   debug_num_sets    = 0;
   debug_num_gets    = 0;
   debug_num_accepts = 0;

   // initialize the thread specific data key
   if (0 != pthread_key_create(&worker_data_key, NULL)) {
      printf("ERROR: syseventd unable to pthread_key_create\n");
      return(ERR_SYSTEM);
   }

   int i;
   for (i=0; i<NUM_WORKER_THREAD; i++) {
      thread_private_info[i].id             = 0;
      thread_private_info[i].fd             = -1;

      thread_stat_info[i].num_activation = 0;
      thread_stat_info[i].state          = 0;

      waiting_pid[i].pid = 0;
      waiting_pid[i].mark = 0;
   }

   for (i=0; i<numThreads; i++) {
      global_thread_id[i] = 0;
   }
   
   global_serial_msgs_head = NULL;
   global_blocked_exec_head = NULL;

   // initialize the listener semaphore
   sem_init (&worker_sem, 0, 1);
   SE_INC_LOG(SEMAPHORE,
      printf("Initialized semaphore worker_sem\n");
   )

   // initialize the logging subsysem
   ulog_init();


   daemon_init();

   /* set up signal handling
    * we want:
    *    
    * SIGINT	 Interrupt from keyboard - Terminate
    * SIGQUIT	 Quit from keyboard - Ignore
    * SIGTERM	 Termination signal  - Terminate
    * SIGSEGV    Segmentation Fault - Terminate
    * SIGKILL	 Kill signal Cant be blocked or Caught
    * SIGPIPE	 - Ignore
    * SIGHUP                        - Reinit
    * SIGUSR1	User-defined signal 1 - Reinit
    * SIGUSR2	User-defined signal 2 - Ignore
    * SIGCHLD	Child stopped or terminated - IGN
    * All others are default
    */
   struct sigaction new_action, old_action;

   new_action.sa_handler = terminate_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);

   if (-1 == sigaction (SIGINT, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGINT. Reason (%d) %s\n", 
                 errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGINT, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGINT. Reason (%d) %s\n", 
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

   if (-1 == sigaction (SIGQUIT, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGQUIT. Reason (%d) %s\n", 
                 errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGQUIT, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGQUIT. Reason (%d) %s\n", 
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

   if (-1 == sigaction (SIGTERM, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGTERM. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGTERM, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGTERM. Reason (%d) %s\n", 
                     errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

#ifndef INCLUDE_BREAKPAD
   if (-1 == sigaction (SIGSEGV, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGSEGV. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGSEGV, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGSEGV. Reason (%d) %s\n", 
                     errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }
#endif

   // we want sighup and sigusr1 to reinit - not supported yet
   new_action.sa_handler = reinit_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGUSR1, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGUSR1. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGUSR1, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGUSR1. Reason (%d) %s\n", 
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }
   if (-1 == sigaction (SIGHUP, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGHUP. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGHUP, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGHUP. Reason (%d) %s\n", 
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

   // we want to ignore SIGCHLD so that there wont be zombies
   new_action.sa_handler = SIG_IGN;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGCHLD, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGCHLD. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGCHLD, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGCHLD. Reason (%d) %s\n", 
                     errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

   // sigpipe ignore
   new_action.sa_handler = ignore_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGPIPE, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGPIPE. Reason (%d) %s\n", 
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else { 
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGPIPE, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGPIPE. Reason (%d) %s\n", 
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }
   if (-1 == sigaction (SIGUSR2, NULL, &old_action)) {
      SE_INC_LOG(ERROR,
         printf("Problem getting original signal handler for SIGUSR2. Reason (%d) %s\n",
                errno, strerror(errno));
      )
      return(ERR_SIGNAL_DEFINE);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGUSR2, &new_action, NULL)) {
           SE_INC_LOG(ERROR,
              printf("Problem setting signal handler for SIGUSR2. Reason (%d) %s\n",
                      errno, strerror(errno));
           )
           return(ERR_SIGNAL_DEFINE);
        }
      }
   }

   // save our pid in the pid file
   pid_t pid = getpid();
   FILE *fp = fopen(SE_SERVER_PID_FILE, "w");
   if (NULL != fp) {
      fprintf(fp, "%d", pid);
      fclose(fp);
   }
   
   // make syseventd immune from the oom-killer (Out of Memory)
   char oom_filename[256];
   snprintf(oom_filename, sizeof(oom_filename), "/proc/%d/oom_adj", pid);
   FILE* oom_fp = fopen(oom_filename, "w");
   if (NULL == oom_fp) {
      SE_INC_LOG(ERROR,
         printf("Unable to write to oom-killer file %s\n", oom_filename);
      )
   } else {
      // -17 means oom-killer must ignore this process when figuring out who to kill
      fprintf(oom_fp, "%d", -17);
      fclose(oom_fp);
   }

   // set up the intra-thread communication
   // se are using pipes because select() can be used with them
   // and they are usually supported in the os
   int pfd[2];

   if (-1 == pipe(pfd)) {
      SE_INC_LOG(ERROR,
         printf("Problem making a msg queue 1. Reason (%d) %s\n", errno, strerror(errno));
      )
      return(ERR_COMMUNICATION_FD);
    }
    main_communication_fd_writer_end      = pfd[1];
    main_communication_fd_listener_end    = pfd[0];
    SE_INC_LOG(MESSAGES,
               printf("main thread/workers communication fds: writer: %d reader: %d\n",
                       main_communication_fd_writer_end, main_communication_fd_listener_end);
    )

   if (-1 == pipe(pfd)) {
      SE_INC_LOG(ERROR,
         printf("Problem making msg queue 2. Reason (%d) %s\n", errno, strerror(errno));
      )
      return(ERR_COMMUNICATION_FD);
    }
    trigger_communication_fd_writer_end      = pfd[1];
    trigger_communication_fd_listener_end    = pfd[0];
    SE_INC_LOG(MESSAGES,
               printf("trigger thread/workers communication fds: writer: %d reader: %d\n",
                       trigger_communication_fd_writer_end, trigger_communication_fd_listener_end);
    )

   if (-1 == pipe(pfd)) {
      SE_INC_LOG(ERROR,
         printf("Problem making msg queue 2. Reason (%d) %s\n", errno, strerror(errno));
      )
      return(ERR_COMMUNICATION_FD);
    }
    trigger_datacommunication_fd_writer_end      = pfd[1];
    trigger_datacommunication_fd_listener_end    = pfd[0];
    SE_INC_LOG(MESSAGES,
               printf("trigger thread/workers data communication fds: writer: %d reader: %d\n",
                       trigger_datacommunication_fd_writer_end, trigger_datacommunication_fd_listener_end);
    )
   // Prepare the pipe for communication with the fork helper
   if (-1 == pipe(fork_helper_pipe)) {
      fork_helper_pipe[0] = -1;
      fork_helper_pipe[1] = -1;
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Unable to create fork helper pipe. (%d) %s", errno, strerror(errno)) ;
      return(ERR_COMMUNICATION_FD);
   }

   // initialize the client manager
   CLI_MGR_init_clients_table();

   // initialize the trigger manager
   TRIGGER_MGR_init();

   // initialize the data manager
   DATA_MGR_init();

   char serialNum[50];
   int  seed = 0;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   seed = tv.tv_sec + tv.tv_usec;

   serialNum[0] = '\0';
   syscfg_get(NULL, "device::serial_number", serialNum, sizeof(serialNum));
   if ('\0' == serialNum[0]) {
      snprintf(serialNum, sizeof(serialNum),"1234");
   }
   
   char *num = serialNum;
   while ('\0' != *num) {
      seed += *num;
      num++;
   }
   srand(seed);
   daemon_node_id      = rand();
   daemon_node_msg_num = 1;

#ifdef NO_IPV6
   /*
    * Initialize the ipv4 TCP socket used by clients to register
    */
   if ( 0 > (global_tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) ) {
      SE_INC_LOG(ERROR,
         printf("TCP Socket open error (errno %d) %s\n", errno, strerror(errno));
      )
      global_tcp_fd = -1;
      return(ERR_WELL_KNOWN_SOCKET);
   }

   // the fcntl allows sysevent daemon to give up its tcp connections
   //  quickly if it dies abnormally
   int oldflags = fcntl (global_tcp_fd, F_GETFD, 0);
   if (0 > oldflags) {
     fcntl (global_tcp_fd, F_SETFD, FD_CLOEXEC);
   } else {
     oldflags |= FD_CLOEXEC;
     fcntl (global_tcp_fd, F_SETFD, oldflags);
   }

   // Enable address reuse
   int on = 1;
   if ( 0 > (setsockopt( global_tcp_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on))) ) {
      SE_INC_LOG(ERROR,
         printf("Unable to set SO_REUSEADDR (error %d) %s\n", errno, strerror(errno));
      )
   }

  // bind our local address
   struct sockaddr_in se_server_addr;

   memset(&se_server_addr, 0, sizeof(se_server_addr));
   se_server_addr.sin_family      = AF_INET;
   se_server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   se_server_addr.sin_port        = htons(global_client_accept_port);

   if (0 > (bind(global_tcp_fd, (struct sockaddr *) &se_server_addr, sizeof(se_server_addr))) ) {
      close(global_tcp_fd);
      global_tcp_fd = -1;
      SE_INC_LOG(ERROR,
         printf("Unable to bind TCP Socket (error %d) %s\n", errno, strerror(errno));
      )
      return(ERR_WELL_KNOWN_SOCKET);
   }

   if (0 > listen(global_tcp_fd, 10)) {
      close(global_tcp_fd);
      global_tcp_fd = -1;
      SE_INC_LOG(ERROR,
         printf("Unable to listen to TCP Socket (errno %d) %s\n", errno, strerror(errno));
      )
      return(ERR_WELL_KNOWN_SOCKET);
   }
#else  
   /*
    * Initialize the ipv6/ipv4 TCP socket used by clients to register
    */
   int force_ipv4 = 0;

   if ( 0 > (global_tcp_fd = socket(AF_INET6, SOCK_STREAM, 0)) ) {
      SE_INC_LOG(ERROR,
         printf("IPv6 TCP Socket open error (errno %d) %s\n", errno, strerror(errno));
         printf("Trying IPv4\n");
      )
      global_tcp_fd = -1;
      // We couldnt initialize using ipv6 socket, so fall back to ipv4 only
      if ( 0 > (global_tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) ) {
         SE_INC_LOG(ERROR,
            printf("IPv4 TCP Socket open error (errno %d) %s\n", errno, strerror(errno));
            printf("Nothing left to try\n");
         )
         global_tcp_fd = -1;
         return(ERR_WELL_KNOWN_SOCKET);
      } else {
         force_ipv4 = 1;
      }
   }
      
   // the fcntl allows sysevent daemon to give up its tcp connections
   //  quickly if it dies abnormally
   int oldflags = fcntl (global_tcp_fd, F_GETFD, 0);
   if (0 > oldflags) {
     /* CID 73196: Unchecked return value from library */
       if (fcntl (global_tcp_fd, F_SETFD, FD_CLOEXEC))
       {
	   close(global_tcp_fd);
           return -1;
       }

   } else {
     oldflags |= FD_CLOEXEC;
     if (fcntl (global_tcp_fd, F_SETFD, oldflags))
     {
         close(global_tcp_fd);
         return -1;
     }
   }

   // Enable address reuse
   int on = 1;
   if ( 0 > (setsockopt( global_tcp_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on))) ) {
      SE_INC_LOG(ERROR,
         printf("ipv6 Unable to set SO_REUSEADDR (error %d) %s\n", errno, strerror(errno));
      )
   }

   if (!force_ipv4) {
      // bind our local address
      struct sockaddr_in6 server6;

      server6.sin6_family = AF_INET6;
      server6.sin6_addr = in6addr_loopback;
      server6.sin6_port = htons(global_client_accept_port);

      if (0 > (bind(global_tcp_fd, (struct sockaddr *) &server6, sizeof(server6))) ) {
         close(global_tcp_fd);
         global_tcp_fd = -1;
         SE_INC_LOG(ERROR,
            printf("Unable to bind ipv6 TCP Socket (error %d) %s\n", errno, strerror(errno));
         )
         return(ERR_WELL_KNOWN_SOCKET);
      }
   } else {
      // bind our local address
        struct sockaddr_in se_server_addr;

        memset(&se_server_addr, 0, sizeof(se_server_addr));
        se_server_addr.sin_family      = AF_INET;
        se_server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        se_server_addr.sin_port        = htons(global_client_accept_port);

        if (0 > (bind(global_tcp_fd, (struct sockaddr *) &se_server_addr, sizeof(se_server_addr))) ) {
           close(global_tcp_fd);
           global_tcp_fd = -1;
           SE_INC_LOG(ERROR,
              printf("Unable to bind TCP Socket (error %d) %s\n", errno, strerror(errno));
           )
           return(ERR_WELL_KNOWN_SOCKET);
        }
   }

   if (0 > listen(global_tcp_fd, 10)) {
      close(global_tcp_fd);
      global_tcp_fd = -1;
      SE_INC_LOG(ERROR,
         printf("Unable to listen to ipv6 TCP Socket (errno %d) %s\n", errno, strerror(errno));
      )
      return(ERR_WELL_KNOWN_SOCKET);
   }
#endif // NO_IPV6

   /*
    * Initialize the UDS connection oriented socket used by clients to register
    */
   if ( 0 > (global_uds_connection_fd = socket(AF_UNIX, SOCK_STREAM, 0)) ) {
      SE_INC_LOG(ERROR,
         printf("UDS Socket open error (errno %d) %s\n", errno, strerror(errno));
      )
      global_uds_connection_fd = -1;
      return(ERR_WELL_KNOWN_SOCKET);
   }

  unlink(UDS_PATH);

  // bind our local address
   struct sockaddr_un se_server_uds_addr;
   size_t             se_server_uds_addr_length;
   errno_t  safec_rc = -1;

   memset(&se_server_uds_addr, 0, sizeof(se_server_uds_addr));
   se_server_uds_addr.sun_family = AF_UNIX;
   safec_rc = sprintf_s(se_server_uds_addr.sun_path, sizeof(se_server_uds_addr.sun_path), "%s", UDS_PATH);
   if( safec_rc < EOK)
   {
       ERR_CHK(safec_rc);
   }
   se_server_uds_addr_length     = sizeof(se_server_uds_addr.sun_family) + strlen(se_server_uds_addr.sun_path);

   if (0 > (bind(global_uds_connection_fd, (struct sockaddr *) &se_server_uds_addr, se_server_uds_addr_length)) ) {
      close(global_uds_connection_fd);
      global_uds_connection_fd = -1;
      SE_INC_LOG(ERROR,
         printf("Unable to bind UDS Socket (error %d) %s\n", errno, strerror(errno));
      )
      return(ERR_WELL_KNOWN_SOCKET);
   }

   if (0 > listen(global_uds_connection_fd, 10)) {
      close(global_uds_connection_fd);
      global_uds_connection_fd = -1;
      SE_INC_LOG(ERROR,
         printf("Unable to listen to UDS Socket (errno %d) %s\n", errno, strerror(errno));
      )
      return(ERR_WELL_KNOWN_SOCKET);
   }

   return(0);
}

/*
 * Procedure     : get_options
 * Purpose       : read commandline parameters and set configurable
 *                 parameters
 * Parameters    :
 *    argc       : The number of input parameters
 *    argv       : The input parameter strings
 * Return Value  :
 *    0               : Success
 *   -1               : Some error
 */
static int get_options(int argc, char **argv)
{
   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"port", 1, 0, 'p'},
#ifdef SE_SERVER_CODE_DEBUG
         {"debug", 1, 0, 'd'},
#endif
         {"threads", 1, 0, 't'},
         {"help", 0, 0, 'h'},
         {0, 0, 0, 0}
      };

      // optstring has a leading : to stop debug output
      // p takes an argument
      // t takes an argument
      // d takes an argument
      // h takes no argument
      c = getopt_long (argc, argv, ":p:d:t:h", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'p':
            global_client_accept_port = (0x0000FFFF & (unsigned short) atoi(optarg));
            break;

#ifdef SE_SERVER_CODE_DEBUG
         case 'd':
            debugLevel = strtol(optarg, NULL, 16);
            break;
#endif
         case 't':
            numThreads = atoi(optarg);
            if (0 > numThreads || NUM_WORKER_THREAD < numThreads) {
               printf("You cannot request more than %d threads\n", NUM_WORKER_THREAD);
               exit(0);
            }
            break; 
         case 'h':
         case '?':
            printf ("Usage %s --port port ", argv[0]);
#ifdef SE_SERVER_CODE_DEBUG
            printf ("--debug debug_flags ");
#endif
            printf ("--threads num_threads --help\n");
            exit(0);
            break;

         default:
            printf ("Usage %s --port port ", argv[0]);
#ifdef SE_SERVER_CODE_DEBUG
            printf ("--debug debug_flags ");
#endif
            printf ("--threads num_threads --help\n");
            break;
      }
   }
   return(0);
}

/*
 * Procedure     : sanity_thread_main
 * Purpose       : Thread start routine for sanity thread
 * Parameters    :
 * Return Code   :
 * Notes         :
 *    Sanity Tests
 *         For serial events syseventd waits for a forked process to complete
 *         This forked process is blocking all other processes waiting for the event
 *         as well as tying up a thread.
 *         If an errant process refuses to give up the cpu then sysevents are blocked
 *         The sanity thread will not let any process run longer than MAX_ACTIVATION_BLOCKING_SECS
 *
 *     SANITY debugging will print the sets/gets and blocked pids to syslog every 5 mins
 */
static void *sanity_thread_main(void *arg)
{
   int sleep_time = 10;
   int numLoops   = MAX_ACTIVATION_BLOCKING_SECS/sleep_time;

   unsigned int counter = 0;  // for SANITY debugging
   int modulo           = 30; // 5 mins    // for SANITY debugging
   FILE *l_FsyseventFp = NULL;
   errno_t  rc = -1;

   for ( ; ; ) {
      /*
       * This code is used at system shutdown
       */
      if (0 != sanity_thread_abort) {
         int i;
         for (i=0; i<NUM_WORKER_THREAD; i++) {
            if (0 != waiting_pid[i].pid) {
//               printf("Sanity thread aborting. Kill pid %d (%s)\n", waiting_pid[i].pid, waiting_pid[i].name);
               kill(waiting_pid[i].pid, 9);
            }
         }
         return(0);
      }

      /*
       * This code is the regular sanity thread code
       */
      char outstr[1024];             // for SANITY debugging
      char outstr2[1024];            // for SANITY debugging
      char tstr[1024];              // for SANITY debugging
      SE_INC_LOG(SANITY,
                 if (1 == counter % modulo) {
                    rc = sprintf_s(outstr, sizeof(outstr), "gets:%d,sets:%d||active q:", debug_num_gets, debug_num_sets);
                    if(rc < EOK)
                    {
                        ERR_CHK(rc);
                    }
                    rc = sprintf_s(outstr2, sizeof(outstr2),"Threads: [main]=%d|", debug_num_accepts);
                    if(rc < EOK)
                    {
                        ERR_CHK(rc);
                    }
                    int i;

                    for (i=0 ; i<numThreads; i++) {
                       rc = sprintf_s(tstr, sizeof(tstr), "[%d]=%d,%d|",
                            thread_private_info[i].id,
                            thread_stat_info[i].num_activation,
                            thread_stat_info[i].state);
                       if(rc < EOK)
                       {
                           ERR_CHK(rc);
                       }
                       rc = strcat_s(outstr2, sizeof(outstr2), tstr);
                       ERR_CHK(rc);
                    }
                 }
      )
      int i;
      for (i=0; i<numThreads; i++) {
         SE_INC_LOG(SANITY,
                    if (1 == counter % modulo) {
                       rc = sprintf_s(tstr, sizeof(tstr), "[%d]=%d:%d|", i+1, waiting_pid[i].pid, waiting_pid[i].mark);
                       if(rc < EOK)
                       {
                           ERR_CHK(rc);
                       }
                       rc = strcat_s(outstr, sizeof(outstr), tstr);
                       ERR_CHK(rc);
                    }
          )
         if (0 != waiting_pid[i].pid) {
            waiting_pid[i].mark++;
            if (numLoops < waiting_pid[i].mark) {
               ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Process (%d) (%s) runs too long. Killing it.", 
                     waiting_pid[i].pid, waiting_pid[i].name);
               
               //The below code is to log the processes (started by sysevent) 
               //that are running for more than 300 sec.  
               l_FsyseventFp = fopen("/rdklogs/logs/syseventd_kill.log", "a+");
		if (l_FsyseventFp != NULL) { /*RDKB-12965 & CID:-34067*/
               fprintf(l_FsyseventFp, "Process (%d) (%s) runs for more than %d secs sending SIGKILL !!!", waiting_pid[i].pid, 
                                      waiting_pid[i].name, MAX_ACTIVATION_BLOCKING_SECS);
               fclose(l_FsyseventFp);
		}
               kill(waiting_pid[i].pid, 9);
            }
         }
      }
      SE_INC_LOG(SANITY,
                 if (1 == counter % modulo) {
                    ulogf(ULOG_SYSTEM, UL_SYSEVENT, "STATS (run %d): %s", counter, outstr); 
                    ulogf(ULOG_SYSTEM, UL_SYSEVENT, "%s", outstr2); 
                 }
      )
      counter++;  // for SANITY debugging

      sleep(sleep_time);
   }
   return(0); 
}

int main (int argc, char **argv)
{
   int rc, ret;

   if (1 > NUM_CLIENT_ONLY_THREAD) {
      printf("NUM_CLIENT_ONLY_THREAD must be at least 1. Currently it is %d", NUM_CLIENT_ONLY_THREAD);
      exit(-1);
   }
   if (NUM_WORKER_THREAD <= NUM_CLIENT_ONLY_THREAD) {
      printf("NUM_CLIENT_ONLY_THREAD must be less than NUM_WORKER_THREAD. Currently it is %d vs %d", NUM_CLIENT_ONLY_THREAD, NUM_WORKER_THREAD);
      exit(-1);
   }
   // set our global defaults
  global_client_accept_port     = SE_SERVER_WELL_KNOWN_PORT; 
  // how many worker threads do we use
  numThreads = NUM_WORKER_THREAD;

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#endif

#ifdef SE_SERVER_CODE_DEBUG
//       debugLevel = SHOW_ALL;
//       debugLevel = SHOW_ALL ^ SHOW_MESSAGES;
//       debugLevel = SHOW_ALL ^ (SHOW_MESSAGES | SHOW_MUTEX | SHOW_SEMAPHORE);
//       debugLevel = SHOW_ERROR | SHOW_INFO | SHOW_TRIGGER_MGR | SHOW_DATA_MGR | SHOW_LISTENER | SHOW_CLIENT_MGR | SHOW_MESSAGES;
//       debugLevel = SHOW_ERROR | SHOW_MUTEX ;
//      debugLevel = SHOW_ERROR | SHOW_LISTENER;
//   debugLevel = SHOW_ERROR | SHOW_ALLOC_FREE;
//  debugLevel = SHOW_ERROR | SHOW_SEMAPHORE;
//  debugLevel = SHOW_ERROR|SHOW_SANITY;
  debugLevel = SHOW_ERROR;
#endif

   memset(&stat_info, 0, sizeof(stat_info));

   // get any command-line options which alter the global defaults
   get_options(argc, argv);


  /*
   * check if we are already alive
   */
   FILE *fp = fopen(SE_SERVER_PID_FILE, "r");
   if (NULL != fp) {
      int old_pid;
      /* CID 60917:Unchecked return value from library */
      if((ret = fscanf(fp, "%d", &old_pid)) <= 0 )
      {
	  printf("read error of %s\n",SE_SERVER_PID_FILE);
      }
      fclose(fp);

      // see if the process is still alive
      char filename[500];
      snprintf(filename, sizeof(filename), "/proc/%d/cmdline", old_pid);
      fp = fopen(filename, "r");
      if (NULL == fp) {
         printf("We are dead but have an old pid file. Cleaning up\n");
         unlink(SE_SERVER_PID_FILE);
      } else {
         char cmdline[500];
         if ((ret = fscanf(fp, "%s", cmdline)) <= 0)
	 {
	    printf("read error of %s\n",filename);
	 }
         fclose(fp);
         if (NULL == strstr(cmdline, argv[0])) {
            printf("Our pid has been taken over. We are dead. Cleaning up\n");
            unlink(SE_SERVER_PID_FILE);
         } else {
            printf("We are alive and well. Ignoring start command\n");
            return(0);
         }
      }
   }



#ifdef REDIRECT_CODE_DEBUG
   // redirect our standard files
   freopen("/dev/null", "r", stdin);
   fp_log = freopen(SYSEVENTD_LOGFILE, "a", stdout);
   setvbuf(fp_log, NULL, _IONBF, 0);
   fprintf(fp_log, "sysevent daemon logging starting\n");
   fp_err = freopen(SYSEVENTD_ERRFILE, "a", stderr);
   // if setvbuf isnt called on the error fp then we see 3 instances
   // of the line "sysevent daemon error logging started\n"
   setvbuf(fp_err, NULL, _IONBF, 0);
   fprintf(fp_err, "sysevent daemon error logging started\n");
   fflush(NULL);
#endif

#ifdef SE_SERVER_CODE_DEBUG
   SE_INC_LOG(INFO,
      printf("syseventd started with debug level 0x%x\n", debugLevel);
   fflush(NULL);
   )
#endif

   if (0  != (rc = initialize_system()) ) {
      SE_INC_LOG(ERROR,
         printf("Unable to start syseventd. Reason %d. Aborting\n", rc);
      )
      deinitialize_system();
      return(rc);
   } else {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd started.");
   }

   // just in case syseventd had been killed uncleanly remove old fork_helper process
   v_secure_system("killall -TERM "SYSEVENTD_FORK_HELPER_PROCESS);

   pthread_attr_t thread_attr;
   pthread_attr_init(&thread_attr);
   pthread_attr_setstacksize(&thread_attr, WORKER_THREAD_STACK_SIZE);
   // start fork helper process
   pid_t  pid;
   pid = fork();
   if (0 == pid) {   /* child */
      char pipe_str[20];
      snprintf(pipe_str, sizeof(pipe_str), "%d", fork_helper_pipe[0]);
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Starting fork helper process %s", SYSEVENTD_FORK_HELPER_PROCESS) ;
      execl(FORK_HELPER_PATH"/"SYSEVENTD_FORK_HELPER_PROCESS, FORK_HELPER_PATH"/"SYSEVENTD_FORK_HELPER_PROCESS, pipe_str, NULL);
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Unable to exec fork helper process %s. (%d) %s", 
            FORK_HELPER_PATH"/"SYSEVENTD_FORK_HELPER_PROCESS, errno, strerror(errno)) ;
      _exit(127);   /* exec error */
   }

   //nice value of -20 is removed as syseventd should run with normal priority
   if (-1 == pid) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Unable to create fork helper process %s. (%d) %s", 
       FORK_HELPER_PATH"/"SYSEVENTD_FORK_HELPER_PROCESS, errno, strerror(errno)) ;
      return(ERR_THREAD_CREATE);
   }

   // make our worker threads
   int i;
   for (i=0; i<numThreads; i++) {
      thread_private_info[i].id = i+1;

      // if the worker thread can handle TriggerMgr messages then it will communicate with the
      // syseventd fork helper process. So open a named pipe for that communicatons
      char fifo_name[256];
      if (NUM_CLIENT_ONLY_THREAD < thread_private_info[i].id) {
         snprintf(fifo_name, sizeof(fifo_name), "/tmp/syseventd_worker_%d", thread_private_info[i].id);
         // in case there is cruft remove the fifo
         unlink(fifo_name);
         if (0 != mkfifo(fifo_name, 0777)) {
            SE_INC_LOG(ERROR,
               printf("Unable to create fifo %s. (%d) %s. ", fifo_name, errno, strerror(errno));
            )
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Unable to mkfifo %s. (%d) %s", fifo_name, errno, strerror(errno)) ;
            deinitialize_system();
            return(ERR_FIFO_CREATE);
         } else {
             thread_private_info[i].fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
             if (-1 == thread_private_info[i].fd) {
                SE_INC_LOG(ERROR,
                   printf("Unable to open fifo %s. (%d) %s. ", fifo_name, errno, strerror(errno));
                )
                ulogf(ULOG_SYSTEM, UL_SYSEVENT, "Unable open fifo %s. fifo_name, (%d) %s", errno, strerror(errno)) ;
                deinitialize_system();
                return(ERR_FIFO_CREATE);
             }
         }
      } else {
         /* worker which does not process trigger manager messages */
         thread_private_info[i].fd = -1;
      }

      // pass each thread a pointer to its private data
      if (0 != pthread_create(&(global_thread_id[i]), &thread_attr, 
                              worker_thread_main, (void *)&(thread_private_info[i])) ) {
         SE_INC_LOG(ERROR,
            printf("Unable to create listener thread. (%d) %s. ", errno, strerror(errno));
         )
         deinitialize_system();
         return(ERR_THREAD_CREATE);
      }
   }
   // start the sanity thread
   pthread_t sanity_thread_id;
   pthread_attr_setstacksize(&thread_attr, SANITY_THREAD_STACK_SIZE);
   pthread_create(&sanity_thread_id, &thread_attr, sanity_thread_main, (void *)NULL);

   // all that this main thread does is listen on a well known port for 
   // clients to register. And when they do, set them up in the clients
   // data structure. From then on, the will communicate with workers
   for ( ; ; ) {
      struct sockaddr_in cli_addr;
      socklen_t          clilen;
      int                newsockfd;
      char              *inmsg_data_ptr;

      fd_set rd_set;
      int    maxfd = 0;
      FD_ZERO(&rd_set);
      if ( 0 <= global_tcp_fd) {
         FD_SET(global_tcp_fd, &rd_set);
         maxfd = (global_tcp_fd + 1);
      } 
      if ( 0 <= global_uds_connection_fd) {
         FD_SET(global_uds_connection_fd, &rd_set);
         if (maxfd <= global_uds_connection_fd) {
            maxfd = (global_uds_connection_fd + 1);
         }
      } 

      clilen = sizeof(cli_addr);
      int rc = select(maxfd, &rd_set, NULL, NULL, NULL);
      if (-1 == rc) {
         continue;
      }

      /*
       * One or more of the client sockets are ready for accept.
       * Handle them in the order of UDS first, ipv6 tcp next, ipv4 tcp next
       */
      if (FD_ISSET(global_uds_connection_fd, &rd_set)) {
         newsockfd = accept(global_uds_connection_fd, (struct sockaddr *) &cli_addr, &clilen);
      }
      else if (FD_ISSET(global_tcp_fd, &rd_set)) {
         newsockfd = accept(global_tcp_fd, (struct sockaddr *) &cli_addr, &clilen);
      } else {
         continue;
      }

      if (0 < newsockfd) {
         token_t      sender;
         se_buffer    inmsg_buffer;
         unsigned int inmsg_len = sizeof(inmsg_buffer);
         int          msgtype   = SE_minimal_blocking_msg_receive(newsockfd, 
                                                  inmsg_buffer, 
                                                  &inmsg_len, 
                                                  &sender);

         if (SE_MSG_DEBUG == msgtype) {
#ifdef SE_SERVER_CODE_DEBUG
            se_debug_msg *new = (se_debug_msg *)&inmsg_buffer;
            if (SHOW_STAT == ntohl(new->level)) {
                printStat();
            } else if (SHOW_CLIENTS == ntohl(new->level)) {
                print_clients_t();
            } else {
                debugLevel = ntohl(new->level);
                printf("Changing debug level to 0x%x as requested\n", debugLevel);
            }
#endif
            close(newsockfd);
            continue;
         }
         if ((SE_MSG_OPEN_CONNECTION != msgtype) && (SE_MSG_OPEN_CONNECTION_DATA != msgtype)) {
            SE_INC_LOG(ERROR,
               printf("Main thread: Received a unexpected msgtype (%d) instead of OPEN_CONNECTION\n", msgtype);
            )
            close(newsockfd);
         } else {
            SE_INC_LOG(MESSAGES,
               printf("Main thread: Received a new OPEN_CONNECTION request\n");
            )
            SE_INC_LOG(MESSAGE_VERBOSE,
               SE_print_message(inmsg_buffer, SE_MSG_OPEN_CONNECTION);
            )
            debug_num_accepts++;
            a_client_t                   *new_client;
            se_buffer                     reply_msg_buffer;
            se_open_connection_reply_msg *reply_msg_body;
            reply_msg_body  = (se_open_connection_reply_msg *)SE_msg_prepare(reply_msg_buffer, 
                                                                  sizeof(reply_msg_buffer), 
                                                                  SE_MSG_OPEN_CONNECTION_REPLY, TOKEN_NULL);
            if (NULL == reply_msg_body) {
               close(newsockfd);
               continue;
            } else {
               se_open_connection_msg *new = (se_open_connection_msg *)&inmsg_buffer;
               inmsg_data_ptr              = (char *) &(new->data);
               int   new_client_bytes;
               char *new_client_name_str   = SE_msg_get_string(inmsg_data_ptr, &new_client_bytes);

               new_client = 
                  CLI_MGR_new_client(NULL == new_client_name_str ? "Unknown" : new_client_name_str,
                                     newsockfd);
               if (NULL == new_client) {
                  reply_msg_body->status    = htonl(-1);
                  reply_msg_body->token_id  = htonl(TOKEN_NULL);
                  SE_msg_send(newsockfd, reply_msg_buffer); 
                  SE_INC_LOG(MESSAGES,
                     printf("Sent %s (%d) to client on fd %d\n",
                            SE_print_mtype(SE_MSG_OPEN_CONNECTION_REPLY), SE_MSG_OPEN_CONNECTION_REPLY, newsockfd);
                  )
                  SE_INC_LOG(MESSAGE_VERBOSE,
                     SE_print_message_hdr(reply_msg_buffer);
                  )

                  close(newsockfd);
                  continue;
               } else {
                   if (SE_MSG_OPEN_CONNECTION_DATA == msgtype)
                   {
                       new_client->isData = 1;
                       v_secure_system("echo fname %s: new fd %d dataclient %d msgtype %d >> /tmp/sys_d.txt",__FUNCTION__,newsockfd, new_client->isData, msgtype);
                   }                  

                  // first inform the workers about the new client. then ack the new client
                  SE_INC_LOG(LISTENER,
                     printf("New Client: %s id: %x (fd: %d)\n", new_client->name, new_client->id, new_client->fd);
                  )

                  se_new_client_msg *thread_msg_body;
                  se_buffer thread_msg_buffer;
                  thread_msg_body = (se_new_client_msg *) SE_msg_prepare(thread_msg_buffer, 
                                                                      sizeof(thread_msg_buffer), 
                                                                      SE_MSG_NEW_CLIENT, TOKEN_NULL);
                  if (NULL != thread_msg_body) {
                     thread_msg_body->token_id = (htonl)(new_client->id);
                     se_msg_hdr *hdr           = (se_msg_hdr *)thread_msg_buffer;
                     SE_msg_hdr_mbytes_fixup(hdr);

                     // send a new client msg to the workers so it will add it to its listen list
                     SE_INC_LOG(MUTEX,
                        printf("Attempting to get mutex: main_communication\n");
                     )
                     pthread_mutex_lock(&main_communication_mutex);
                     SE_INC_LOG(MUTEX,
                        printf("Got mutex: main_communication\n");
                     )
                     int rc = SE_msg_send(main_communication_fd_writer_end, thread_msg_buffer);
                     SE_INC_LOG(MUTEX,
                        printf("Releasing mutex: main_communication\n");
                     )
                     pthread_mutex_unlock(&main_communication_mutex);
                 
                     if (0 != rc) {
                        SE_INC_LOG(MESSAGES,
                           printf("Failed to send new client message to workers (%d) %s\n",
                                  rc, SE_strerror(rc));
                        )
                        reply_msg_body->status    = htonl(rc);
                     } else {
                        // finally we ack the client and provide it with its client id
                        reply_msg_body->token_id  = htonl(new_client->id);
                        reply_msg_body->status    = htonl(0);
                     }
                     SE_msg_send(newsockfd, reply_msg_buffer); 
                     SE_INC_LOG(MESSAGES,
                        printf("Sent %s (%d) to client on fd %d\n",
                            SE_print_mtype(SE_MSG_OPEN_CONNECTION_REPLY), SE_MSG_OPEN_CONNECTION_REPLY, newsockfd);
                     )
                     SE_INC_LOG(MESSAGE_VERBOSE,
                        SE_print_message_hdr(reply_msg_buffer);
                     )
                     // if we couldn't send new client msg to workers then close this socket
                     if (rc) {
                        close(newsockfd);
                     }
                  } else {
                     reply_msg_body->status     = htonl(-1);
                     reply_msg_body->token_id   = htonl(TOKEN_NULL);
                     SE_msg_send(newsockfd, reply_msg_buffer); 
                     SE_INC_LOG(MESSAGES,
                        printf("Sent %s (%d) to client on fd %d\n",
                            SE_print_mtype(SE_MSG_OPEN_CONNECTION_REPLY), SE_MSG_OPEN_CONNECTION_REPLY, newsockfd);
                     )
                     SE_INC_LOG(MESSAGE_VERBOSE,
                        SE_print_message_hdr(reply_msg_buffer);
                     )
                     close(newsockfd);
                  }
               }
            }
         }
      }
   }
}
