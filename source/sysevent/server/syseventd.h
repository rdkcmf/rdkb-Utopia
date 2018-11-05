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

#ifndef _SYSEVENTD_H_
#define _SYSEVENTD_H_

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "clientsMgr.h"
#include "triggerMgr.h"
#include "dataMgr.h"

// an id for the sysevent daemon
extern int daemon_node_id;
// monotonically increasing message number
extern int daemon_node_msg_num;

// pipe between syseventd and fork helper process
extern int fork_helper_pipe[2];

// debug counters
extern int debug_num_sets;
extern int debug_num_gets;
extern int debug_num_accepts;

// the number of consecutive errors that we detect for a client before we
// automatically disconnect it
#define MAX_ERRORS_BEFORE_DISCONNECTION 3


/*
======================================================
    thread related stuff
======================================================
*/
// semaphore for the workers
extern sem_t worker_sem;

// number of worker threads created
// note that serialized data tuples are by their nature blocking a thread 
// which is serially executing externally defined programs. This is
// somewhat dangerous since those programs could be in turn using syseventd.
// The number of threads used should be reasonably high.
// Best performance is when the thread count is at the high water level of 
// parallel activation calls (determined during boot)
// On the other hand, each thread takes memory and too high a count makes syseventd
// an attractive candidate for the kernel to choose to kill when Out Of Memory.
// Since syseventd is integral to the infrastructure, protecting it from the kernel
// is prudent.
//#define NUM_WORKER_THREAD 7
#define NUM_WORKER_THREAD 10

// the number of client only threads (out of the NUM_WORKER_THREAD pool)
// This number must be at least 1 to ensure that client can never starve for get/set
// It must be less than NUM_WORKER_THREAD or else no events will be generated nor
// will any activations occur
#define NUM_CLIENT_ONLY_THREAD 2

// the maximum number of seconds that an activated process can run
// while blocking use of a thread. More than this and the process will 
// be killed
// This value is changed to 300 sec as some processes are taking time for completing.
// e.g Multinet process is taking more time to finish in some of the field units.
#define MAX_ACTIVATION_BLOCKING_SECS 300

#define WORKER_THREAD_STACK_SIZE  65536

#define SANITY_THREAD_STACK_SIZE  65536

// structure of argument to worker thread init function
typedef struct {
   int  id;   // id assigned to thread from main
   int  fd;   // private fd
} worker_thread_private_info_t;

// structure to hold per thread stat info
typedef struct {
   int  num_activation;  // how many times has thread received semaphore
   int  state;           // 0=waiting for semaphore, 1=executing, 2=waiting for fork manager
} worker_thread_stat_info_t;
extern worker_thread_stat_info_t    thread_stat_info[NUM_WORKER_THREAD];
   
// thread specific data key
extern pthread_key_t worker_data_key;

// fds for inter thread communication
// communication between main thread and workers
extern int main_communication_fd_writer_end;
extern int main_communication_fd_listener_end;
// communicaton between trigger thread and workers
extern int trigger_communication_fd_writer_end;
extern int trigger_communication_fd_listener_end;

// mutex to protect inter thread communication
extern pthread_mutex_t  main_communication_mutex;
extern pthread_mutex_t  trigger_communication_mutex;

// mutex to protect communication to fork helper process
// (only needed for communication to helper process since return
// communication is via thread specific pipe
extern pthread_mutex_t  fork_helper_communication_mutex;

// mutex used to serialize
//   a) serial messages
//   b) activation (using _eval) of executables
extern pthread_mutex_t  serialization_mutex;

// global data
extern clients_t  global_clients; //deined in clientMgr.c

// worker thread init routine
extern void *worker_thread_main(void *arg);

/*
 * blocked_exec_link_t
 *   A link in a chain of executables that are waiting for activation
 *   We are attaching the link as a doubly linked list to the chain
 *   For multiple links with the same name of executable, they are attached using the
 *   bucket link.
 */
typedef struct blocked_exec_link_s {
   struct blocked_exec_link_s *prev;
   struct blocked_exec_link_s *next;
   struct blocked_exec_link_s *bucket;
   int                         wait;
   trigger_action_t           action;
   char                       *name;
   char                       *value;
} blocked_exec_link_t;

// list of executables for activation which are blocked
// this is used by the worker threads
extern blocked_exec_link_t *global_blocked_exec_head;


/*
 * serial_msg_link_t
 *   A link in a chain of se_run_serially_msg
 *   We are attaching the link as a doubly linked list to the chain
 *   For multiple links with the same async_id, they are attached using the
 *   bucket link.
 */
typedef struct serial_msg_link_s {
   struct serial_msg_link_s *prev;
   struct serial_msg_link_s *next;
   struct serial_msg_link_s *bucket;
   se_buffer                *list;
   async_id_t                async_id;
   unsigned int              num_msgs;
} serial_msg_link_t;

// list of serial msgs
// this is used by the worker threads
extern serial_msg_link_t *global_serial_msgs_head;
extern pthread_mutex_t global_serial_msgs_mutex;

/*
 * waiting_pid_t
 *    When a serial tuple is being handled we keep track of the 
 *    currently handled activated process. If the process takes
 *    too long to complete then it will be killed
 * pid  : pid of the process being waited for
 * mark : a monotonically increasing value indicating how many time slices the process used
 * name : the name of the process (used for debugging only)
 */
typedef struct {
   pid_t   pid;
   int     mark;
   char    name[256];
} waiting_pid_t;

extern waiting_pid_t waiting_pid[NUM_WORKER_THREAD];

typedef enum {
    STAT_WORKER_FORKS,
    STAT_WORKER_FORK_FAILURES,
    STAT_WORKER_PIPE_FD_SELECT_FAILURES,
    STAT_WORKER_PIPE_CREAT_FAILURES,
    STAT_WORKER_PIPE_WRITE_FD_INVALID,
    STAT_WORKER_EXECVE_FAILURES,
    STAT_WORKER_SIGPIPE_COUNT,
    STAT_WORKER_MAIN_SELECT_BAD_FD,
    STAT_FORK_HELPER_PIPE_READ
} stat_id_t;

typedef struct {
    unsigned long worker_forks;
    unsigned long worker_fork_failures;
    unsigned long worker_pipe_fd_select_failures;
    unsigned long worker_pipe_creat_failures;
    unsigned long worker_pipe_write_fd_invalid;
    unsigned long worker_execve_failures;
    unsigned long worker_sigpipe_count;
    unsigned long worker_main_select_bad_fd;
    unsigned long fork_helper_pipe_read_failures;
} se_stat_info_t;

// mutex used to capture error statistics
//   used to protect se_stat_info buffer
extern pthread_mutex_t  stat_info_mutex;

/*
======================================================
    triggerMgr stuff 
======================================================
*/
// maximum number of arguments we are willing to hold for a client
#define TOO_MANY_ARGS  10

/*
=====================================================
   parsing symbols
=====================================================
*/
// symbols used to dictate whether a callback parameter is 
// looked up in syscfg or sysevent namespace
#define SYSCFG_NAMESPACE_SYMBOL   '$'
#define SYSEVENT_NAMESPACE_SYMBOL '@'

/*
======================================================
    general utilities
======================================================
*/

/*
 * get id assigned to a thread
 */
int thread_get_id(pthread_key_t key);

/*
 * get read side of pipe assigned to a thread
 */
int thread_get_private_pipe_read(pthread_key_t key);

/*
 * statistics
 */
void thread_set_state(pthread_key_t key, int state);
void thread_activated(pthread_key_t key);

/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in situ
 */
char *trim(char *in);

/*
 * Procedure     : sysevent_strdup
 * Purpose       : strdup
 * Parameters    :
 *    s          : The sting to duplicate 
 *    file       : The file of the procedure that called sysevent_realloc
 *    line       : the line number of the call to sysevent_realloc
 * Return Value  : A pointer to the allocated store or NULL
 */
void *sysevent_strdup(const char* s, char* file, int line);

/*
 * Procedure     : sysevent_malloc
 * Purpose       : malloc
 * Parameters    :
 *    size       : The number of bytes to malloc
 *    file       : The file of the procedure that called sysevent_realloc
 *    line       : the line number of the call to sysevent_realloc
 * Return Value  : A pointer to the allocated store or NULL
 */
void *sysevent_malloc(size_t size, char* file, int line);

/*
 * Procedure     : sysevent_realloc
 * Purpose       : realloc
 * Parameters    :
 *    ptr        : A pointer to the store to realloc
 *    size       : The number of bytes to realloc
 *    file       : The file of the procedure that called sysevent_realloc
 *    line       : the line number of the call to sysevent_realloc
 * Return Value  : A pointer to the allocated store or NULL
 */
void *sysevent_realloc(void* ptr, size_t size, char* file, int line);

/*
 * Procedure     : sysevent_free
 * Purpose       : free
 * Parameters    :
 *    addr       : The address of the pointer to the store to free
 *    file       : The file of the procedure that called sysevent_realloc
 *    line       : the line number of the call to sysevent_realloc
 * Return Value  : A pointer to the allocated store or NULL
 */
void sysevent_free(void **addr, char* file, int line);

/*
======================================================
    error messages and utilities
======================================================
*/
// current debug level is defined in syseventd_main.c
extern int debugLevel;

// error codes
#define ERR_COMMUNICATION_FD           -500
#define ERR_WELL_KNOWN_SOCKET          -501
#define ERR_UNABLE_TO_SEND             -502
#define ERR_UNABLE_TO_PREPARE_MSG      -503
#define ERR_UNHANDLED_CASE_STATEMENT   -504
#define ERR_THREAD_CREATE              -505
#define ERR_SIGNAL_DEFINE              -507
#define ERR_BAD_PARAMETER              -508
#define ERR_UNABLE_TO_CALL_ASYNC       -509
#define ERR_ALLOC_MEM                  -510
#define ERR_SYSTEM                     -511
#define ERR_NOT_INITED                 -512
#define ERR_TOO_MANY_ARGUMENTS         -513
#define ERR_UNKNOWN_ASYNC_ID           -515
#define ERR_SYSCFG_FAILURE             -516
#define ERR_PIPE_CREATE                -517
#define ERR_FIFO_CREATE                -518
#define ERR_UNKNOWN_CLIENT             -519


// debug flags
// define bit map for various debugging levels
#define SHOW_ERROR             0x00000001
#define SHOW_INFO              0x00000002
#define SHOW_TRIGGER_MGR       0x00000004
#define SHOW_ALLOC_FREE        0x00000008
#define SHOW_CLIENT_MGR        0x00000010
#define SHOW_MESSAGES          0x00000020
#define SHOW_MESSAGE_VERBOSE   0x00000040
#define SHOW_DATA_MGR          0x00000080
#define SHOW_LISTENER          0x00000100
#define SHOW_SYSCFG            0x00000200
#define SHOW_MUTEX             0x00000400
#define SHOW_SEMAPHORE         0x00000800
#define SHOW_ACTIVATION        0x00001000
#define SHOW_SANITY            0x00002000

// put new debug levels above this
#define SHOW_TIMESTAMP         0x40000000
#define SHOW_ALL               0x4FFFFFFF
#define SHOW_STAT              0xFEEDFEED
#define SHOW_CLIENTS           0xFEECFEEC


#ifdef SE_SERVER_CODE_DEBUG
    #define SE_INC_LOG(level, code)  if (((SHOW_ ## level) & debugLevel) && printTime()) { code }
#else
    #define SE_INC_LOG(module, code)
#endif

int printTime(void);
void incr_stat_info(stat_id_t id);
void printStat();


#endif   // _SYSEVENTD_H_
