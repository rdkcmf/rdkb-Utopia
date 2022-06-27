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

#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU  // to pick up strsignal in string.h
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "ulog/ulog.h"

/*
 =============================================================================
 =                        sysevent daemon fork helper 
 = 
 = sysevent daemon activates processes upon events, and this activation is a 
 = fork followed by an exec. However fork copies the heap and since sysevent 
 = daemon can use a fair amount of data this copy is memory expensive. Therefore
 = sysevent daemon instead messages to the fork helper process to do the activation
 =
 = The format of a mesasge is:
 =    field 1 : 4 bytes : length of the program name and all arguments (value does not count first 3 fields)
 =    field 2 : 4 bytes : the number of arguments
 =    field 3 : 4 bytes : -1 if we dont message back to sysevent daemon, else it is id of the waiting thread
 =                        which we use for named pipe /tmp/syseventd_worker_(id)
 =    field 4 : null terminated program name
 =    field 5-?: null terminated arguments
 =
 =============================================================================
 */

/*
 * file where pid is kept
 */
#define PID_FILE "/var/run/syseventd_fork_helper.pid"

/*
 * read end of a pipe from sysevent daemon
 */
static int read_pipe;

/*
 * maximum number of arguments in a process we can start
 */
#define MAX_ARGS 15

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
 * Purpose       : UniInitialize the system
 * Parameters    : None
 * Return Code   :
 *    0             : All is uninitialized
 */
static int deinitialize_system(void)
{
   close(read_pipe);
   unlink(PID_FILE);
   ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper stopped");
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
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper received terminate signal %s (%d)\n", 
         strsignal (signum), signum);
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
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper received ignore signal %s (%d)\n", 
         strsignal (signum), signum);
}

/*
 * Procedure     : reinit_signal_handler
 * Purpose       : Handle a signal
 * Parameters    :
 *   signum         : The signal received
 * Return Code   : None
 */
static void reinit_signal_handler (int signum)
{
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper received reinit signal %s (%d)\n", 
         strsignal (signum), signum);
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
   // initialize the logging subsysem
   ulog_init();

   daemon_init();

   /* set up signal handling
    * we want:
    *   
    * SIGINT     Interrupt from keyboard - Terminate
    * SIGQUIT    Quit from keyboard - Ignore
    * SIGTERM   Termination signal  - Terminate
    * SIGKILL    Kill signal Cant be blocked or Caught
    * SIGPIPE    - Ignore
    * SIGHUP                        - Reinit
    * SIGUSR1   User-defined signal 1 - Reinit
    * SIGUSR2   User-defined signal 2 - Ignore
    * SIGCHLD   Child stopped or terminated - IGN
    * All others are default
    */
   struct sigaction new_action, old_action;

   new_action.sa_handler = terminate_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);

   if (-1 == sigaction (SIGINT, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGINT. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGINT, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGINT. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   if (-1 == sigaction (SIGQUIT, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGQUIT. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGQUIT, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGQUIT. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   if (-1 == sigaction (SIGTERM, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGTERM. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGTERM, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGTERM. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   // we want sighup and sigusr1 to reinit - not supported yet
   new_action.sa_handler = reinit_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGUSR1, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGUSR1. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGUSR1, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGUSR1. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   if (-1 == sigaction (SIGHUP, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGHUP. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGHUP, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGHUP. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   // we want to ignore SIGCHLD so that there wont be zombies
   new_action.sa_handler = SIG_IGN;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGCHLD, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGCHLD. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
        if (-1 == sigaction (SIGCHLD, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGCHLD. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   // sigpipe ignore
   new_action.sa_handler = ignore_signal_handler;
   new_action.sa_flags   = 0;
   sigemptyset (&new_action.sa_mask);
   if (-1 == sigaction (SIGPIPE, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGPIPE. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGPIPE, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGPIPE. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }
   if (-1 == sigaction (SIGUSR2, NULL, &old_action)) {
      ulogf(ULOG_SYSTEM, UL_SYSEVENT,
           "Problem getting original signal handler for SIGUSR2. Reason (%d) %s", errno, strerror(errno));
      return(-1);
   } else {
      if (SIG_IGN != old_action.sa_handler) {
         if (-1 == sigaction (SIGUSR2, &new_action, NULL)) {
           ulogf(ULOG_SYSTEM, UL_SYSEVENT,
                "Problem setting signal handler for SIGUSR2. Reason (%d) %s", errno, strerror(errno));
           return(-2);
        }
      }
   }

   // save our pid in the pid file
   pid_t pid = getpid();
   FILE *fp = fopen(PID_FILE, "w");
   if (NULL != fp) {
      fprintf(fp, "%d", pid);
      fclose(fp);
   }

   // make process immune from the oom-killer (Out of Memory)
   char oom_filename[256];
   snprintf(oom_filename, sizeof(oom_filename), "/proc/%d/oom_adj", pid);
   FILE* oom_fp = fopen(oom_filename, "w");
   if (NULL != oom_fp) {
      // -17 means oom-killer must ignore this process when figuring out who to kill
      fprintf(oom_fp, "%d", -17);
      fclose(oom_fp);
   }

   return(0);
}

/*
 * Procedure     : _eval
 * Purpose       : fork a process and execute a new program
 * Parmameters   :
 *    prog          : The program to execute
 *    argv          : The argument vector to pass to the program
 *    pid           : The pid of the forked/execed process
 * Return Code   :
 *   0 for success
 *   !0 for failure
 */
static pid_t _eval(char *const prog, char *argv[], pid_t *pid)
{
   pid_t   local_pid = -1;
   /*RDKB-7133, CID-33339, CID-33010, initializing before use*/
   struct sigaction ignore , saveintr, savequit;
   sigset_t         chldmask= {{0}}, savemask= {{0}};
   memset( &ignore, 0, sizeof(ignore));
   memset( &saveintr, 0, sizeof(saveintr));
   memset( &savequit, 0, sizeof(savequit));
   int    rc = 0;;

   *pid = -1;
   if (NULL == prog) {
      return(-10);
   }

   /*
    * ignore SIGINT and SIGQUIT
    */
   ignore.sa_handler = SIG_IGN;
   sigemptyset(&ignore.sa_mask);
   ignore.sa_flags   = 0;
   if (0 > sigaction(SIGINT, &ignore, &saveintr)) {
      rc = -11;
      goto eval_done;
   }
   if (0 > sigaction(SIGQUIT, &ignore, &savequit)) {
      rc = -12;
      goto eval_done;
   }

   /* block SIGCHLD */
   sigemptyset(&chldmask);
   sigaddset(&chldmask, SIGCHLD);
   if (0 > sigprocmask(SIG_BLOCK, &chldmask, &savemask)) {
      rc = -13;
      goto eval_done;
   }

   /*
    * now fork and execute
    */
   local_pid = fork();
   switch (local_pid) {
      case(-1) :
      {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "sysevent for helper fork failed. Unable to exec %s %s", prog, argv[1]) ;
         rc = -14;
         break;
      }
      case(0):
      {
         /* child */
         //nice value of 20 is removed as syseventd should run with normal priority

         /*
          * redirect output from child process
          */
         int dev_null_fd;
#define REDIRECT_CHILD_OUTPUT_TO_CONSOLE 1
#ifdef REDIRECT_CHILD_OUTPUT_TO_CONSOLE
         dev_null_fd = open ("/dev/console", O_RDWR);
#else
         // have childs output go to /dev/null
         dev_null_fd = open ("/dev/null", O_RDWR);
#endif
         if (dev_null_fd >= 0) {
             dup2 (dev_null_fd, 0);
             dup2 (dev_null_fd, 1);
             dup2 (dev_null_fd, 2);
         }
         sigaction(SIGINT, &saveintr, NULL);
         sigaction(SIGQUIT, &savequit, NULL);
         sigprocmask(SIG_SETMASK, &savemask, NULL);
       /* syseventd_fork_helper currently have SIGCHLD configured to SIG_IGN,
        * On Linux system such configuration is inherited by child process (look at execve(2))
        * and as a result it breaks wait() calls and effectively breaks system() calls (look into the NOTES in wait(2))
        * So we need to restore default SIGCHLD handler for child process: */
         signal(SIGCHLD, SIG_DFL);

        /*
         * set up the environment so binaries and libraries can also be found in packages in the /opt subsystem
         */
        /*char *env_init[] = { "USER=root", 
                             "PATH=/bin:/sbin:/usr/sbin:/usr/bin:/opt/sbin:/opt/bin", 
                             "LD_LIBRARY_PATH=/lib:/usr/lib:/opt/lib", NULL 
                           };*/

          /*
           * make process not immune from the oom-killer (Out of Memory)
           * (by default it will inherit the oom_adj of this process)
           */
         pid_t mypid = getpid();
         char oom_filename[256];
         snprintf(oom_filename, sizeof(oom_filename), "/proc/%d/oom_adj", mypid);
         FILE* oom_fp = fopen(oom_filename, "w");
         if (NULL != oom_fp) {
            fprintf(oom_fp, "%d", 0);
            fclose(oom_fp);
         }

        //execve(prog, argv, env_init);
        execv(prog, argv);

        ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper execve failed. %s. Reason: (%d) %s", prog, errno, strerror(errno)) ;
        _exit(127);   /* exec error */
         break;
      }
      default:
      {
         /* parent */
         *pid = local_pid;

         break;
      }
   }  // end of switch

eval_done:
   /* restore previous signal actions and reset signal mask */
   sigaction(SIGINT, &saveintr, NULL);
   sigaction(SIGQUIT, &savequit, NULL);
   sigprocmask(SIG_SETMASK, &savemask, NULL);
   return(rc);
}

/*
 *  Procedure     : get_args_and_eval
 *  Purpose       : given a ready file, read a message from sysevent daemon 
 *                  create the argument vector, and activate the process
 *  Parameters    :
 *     fd            : A file which is ready to read from
 */
static int get_args_and_eval(int fd)
{
   // the format of a message is 
   // 4 bytes : the length of the program name and all arguments
   // 4 bytes : the number of arguments
   // 4 bytes : -1 if we dont wait for process to finish, !1 = id of thread to lookup named pipe
   // null terminated program name
   // null terminated arguments   

   int          rc = 0;
   unsigned int msg_len;
   char buffer[2048];
   int          read_bytes;

   /*
    * Get length of program name and all arguments
    */
   read_bytes = -1;
   while (-1 == read_bytes) {
      read_bytes = read(fd, &msg_len, sizeof(msg_len));
      if (-1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno) {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read message from syseventd error (%d) %s", errno, strerror(errno));
         return(-1);
      }
   }
   if (0 == read_bytes) {
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read empty message from syseventd");
      return(-1);
   }
   /*
    * Get the number of arguments for the process to activate
    */
   int  num_args;
   read_bytes = -1;
   while (-1 == read_bytes) {
      read_bytes = read(fd, &num_args, sizeof(num_args));
      if (-1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno) {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read arguments in message from syseventd error (%d) %s", errno, strerror(errno));
         return(-2);
      }
   }
   if (0 == read_bytes ) {
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read arguments in message from syseventd");
      return(-2);
   } 
   /*
    * Get the id of the calling sysevent daemon thread
    */
   int thread_id;
   char thread_fifo_name[256];
   int  thread_fifo_fd = -1;
   read_bytes = -1;
   while (-1 == read_bytes) {
      read_bytes = read(fd, &thread_id, sizeof(thread_id));
      if (-1 == read_bytes && EAGAIN != errno && EWOULDBLOCK != errno) {
         ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read thread_id in message from syseventd error (%d) %s", errno, strerror(errno));
         return(-3);
      }
   }
   if (0 == read_bytes) {
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read thread_id in message from syseventd");
      return(-3);
   } 

   if (-1 == thread_id) {
      thread_fifo_name[0] = '\0';
   } else {
      snprintf(thread_fifo_name, sizeof(thread_fifo_name), "/tmp/syseventd_worker_%d", thread_id);
      thread_fifo_fd = open(thread_fifo_name, O_WRONLY | O_NONBLOCK); 
      if (-1 == thread_fifo_fd) {
         ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not open fifo to syseventd");
      }
   }

   pid_t child_pid = 0;
   // if msg is too long to fit into our buffer then drain pipe and return
   if (msg_len >= sizeof(buffer)) {
      while (0 != read(fd, buffer, sizeof(buffer)));
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper received too long a message from syseventd");
      if (-1 != thread_fifo_fd) {
         child_pid = -1;
         int num_tries = 3;
         while (0 < num_tries) {
            int rc = write(thread_fifo_fd, &child_pid, sizeof(child_pid));
            if (0 < rc) {
               num_tries = 0;
            } else {
               num_tries--;
               // if resource temporarily unavailable
               if (0  > rc && num_tries && EAGAIN == errno ) {
                  struct timespec sleep_time;
                  sleep_time.tv_sec = 0;
                  sleep_time.tv_nsec  = 200000000;  // .2 secs
                  nanosleep(&sleep_time, NULL);
               }
            }
         }
         close(thread_fifo_fd);
      }
      return(-2);
   }

   /*
    * Read in the rest of the message
    */
   unsigned int bytes_read = 0;
   while (bytes_read < msg_len) {
      read_bytes = -1;
      while (-1 == read_bytes) {
         read_bytes = read(fd, buffer+bytes_read, msg_len-bytes_read);
         if (-1 == read_bytes && EAGAIN != read_bytes && EWOULDBLOCK != read_bytes) {
            ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper could not read message from syseventd error (%d) %s", errno, strerror(errno));
            return(-10);
         }
      }
      bytes_read += read_bytes;
   }

   // if there is no program name and arguments then return
   if (0 >= msg_len) {
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper received malformed message from syseventd");
      if (-1 != thread_fifo_fd) {
         child_pid = -1;

         int num_tries = 3;
         while (0 < num_tries) {
            int rc = write(thread_fifo_fd, &child_pid, sizeof(child_pid));
            if (0 < rc) {
               num_tries = 0;
            } else {
               num_tries--;
               // if resource temporarily unavailable
               if (0  > rc && num_tries && EAGAIN == errno ) {
                  struct timespec sleep_time;
                  sleep_time.tv_sec = 0;
                  sleep_time.tv_nsec  = 200000000;  // .2 secs
                  nanosleep(&sleep_time, NULL);
               }
            }
         }
         close(thread_fifo_fd);
      }
      return(-3);
   }

   char *argv[MAX_ARGS+1];
   if (MAX_ARGS < num_args) {
      ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper found Too many arguments. Ignoring activation.");
      if (-1 != thread_fifo_fd) {
         child_pid = -1;
         int num_tries = 3;
         while (0 < num_tries) {
            int rc = write(thread_fifo_fd, &child_pid, sizeof(child_pid));
            if (0 < rc) {
               num_tries = 0;
            } else {
               num_tries--;
               // if resource temporarily unavailable
               if (0  > rc && num_tries && EAGAIN == errno ) {
                  struct timespec sleep_time;
                  sleep_time.tv_sec = 0;
                  sleep_time.tv_nsec  = 200000000;  // .2 secs
                  nanosleep(&sleep_time, NULL);
               }
            }
         }
         close(thread_fifo_fd);
      }
      return(-4);
   }


   char *prog;
   prog = buffer;
   /* CID 135555 : String not null terminated */
   prog[bytes_read] = '\0';
   
   char *buffer_ptr = buffer+strlen(prog)+1;

   int i;
   for (i = 0; i < num_args; i++) {
      argv[i] = buffer_ptr;
      buffer_ptr += strlen(argv[i])+1;
      if ( (unsigned int)(buffer_ptr-buffer) > sizeof(buffer)) {
         /* free(argv);	# non-heap object */
         ulog(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper found arguments overflow");
         if (-1 != thread_fifo_fd) {
            child_pid = -1;
            int num_tries = 3;
            while (0 < num_tries) {
               int rc = write(thread_fifo_fd, &child_pid, sizeof(child_pid));
               if (0 < rc) {
                  num_tries = 0;
               } else {
                  num_tries--;
                  // if resource temporarily unavailable
                  if (0  > rc && num_tries && EAGAIN == errno ) {
                     struct timespec sleep_time;
                     sleep_time.tv_sec = 0;
                     sleep_time.tv_nsec  = 200000000;  // .2 secs
                     nanosleep(&sleep_time, NULL);
                  }
               }
            }
            close(thread_fifo_fd);
         }
         return(-5);
      }
   }
   argv[i] = NULL;

   pid_t return_child_pid;
   rc =  _eval(prog, argv, &return_child_pid);

   if (-1 != thread_fifo_fd) {
      if (0 != rc) {
         child_pid = -1;
      } else {
         child_pid = return_child_pid;
      }

      int num_tries = 3;
      while (0 < num_tries) {
         int rc = write(thread_fifo_fd, &child_pid, sizeof(child_pid));
         if (0 < rc) {
            num_tries = 0;
         } else {
            num_tries--;
            // if resource temporarily unavailable
            if ( 0 > rc && num_tries && EAGAIN == errno ) {
               struct timespec sleep_time;
               sleep_time.tv_sec = 0;
               sleep_time.tv_nsec  = 200000000;  // .2 secs
               nanosleep(&sleep_time, NULL);
            }
         }
      }
     close(thread_fifo_fd);
   }
   return(rc);
}

/*
 * Parameters    :
 *   argv[1]         : a pipe to read from
 *   argv[2]         : a pipe to write to
 */
int main(int argc, char **argv)
{
   initialize_system();

   //nice value of -20 is removed as syseventd should run with normal priority
   if (2 > argc) {
     ulog(ULOG_SYSTEM, UL_SYSEVENT, "Error: syseventd fork helper called without pipe. Aborting");
     return(-1);
   }
   read_pipe  = atoi(argv[1]);
   ulogf(ULOG_SYSTEM, UL_SYSEVENT, "syseventd fork helper started using pipe %d", read_pipe);

   fd_set readfds;
   int    rc;
   for (;;) {
      FD_ZERO(&readfds);
      FD_SET(read_pipe, &readfds);
      rc = select(read_pipe+1, &readfds, NULL, NULL, NULL);
      if (-1 != rc && FD_ISSET(read_pipe, &readfds)) {
         get_args_and_eval(read_pipe);
      }
   }

   deinitialize_system();
   return(0);
}
