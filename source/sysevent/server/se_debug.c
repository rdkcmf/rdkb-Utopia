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

#include <sys/time.h>
#include <time.h>
#include "syseventd.h"

#ifdef SE_SERVER_CODE_DEBUG
#ifdef NO_PRINTTIME
int printTime(void)
{
   return(1);
}
#else
#include <stdio.h>
// this always returns 1
int printTime(void)
{
   struct tm      result;
   struct timeval tv;

   if (SHOW_TIMESTAMP & debugLevel)
   {
      gettimeofday(&tv, NULL);
      gmtime_r(&(tv.tv_sec), &result);
      printf("%d/%d %02d:%02d:%02d\n",
               result.tm_mon,
               result.tm_mday,
               result.tm_hour,
               result.tm_min,
               result.tm_sec);
   }
   return(1);
}
#endif //NO_PRINTTIME
#endif //SE_SERVER_CODE_DEBUG

extern se_stat_info_t stat_info;

void incr_stat_info(stat_id_t id)
{
    pthread_mutex_lock(&stat_info_mutex);
    switch (id) {
    case STAT_WORKER_FORKS:
        stat_info.worker_forks++;
        break;
    case STAT_WORKER_FORK_FAILURES:
        stat_info.worker_fork_failures++;
        break;
    case STAT_WORKER_PIPE_FD_SELECT_FAILURES:
        stat_info.worker_pipe_fd_select_failures++;
        break;
    case STAT_WORKER_PIPE_CREAT_FAILURES:
        stat_info.worker_pipe_creat_failures++;
        break;
    case STAT_WORKER_PIPE_WRITE_FD_INVALID:
        stat_info.worker_pipe_write_fd_invalid++;
        break;
    case STAT_WORKER_EXECVE_FAILURES:
        stat_info.worker_execve_failures++;
        break;
    case STAT_WORKER_SIGPIPE_COUNT:
        stat_info.worker_sigpipe_count++;
        break;
    case STAT_WORKER_MAIN_SELECT_BAD_FD:
        stat_info.worker_main_select_bad_fd++;
        break;
    case STAT_FORK_HELPER_PIPE_READ:
        stat_info.fork_helper_pipe_read_failures++;
        break;
    }
    pthread_mutex_unlock(&stat_info_mutex);
}

void printStat()
{
    pthread_mutex_lock(&stat_info_mutex);
    printf("stat_info.worker_forks = %ld\n", stat_info.worker_forks);
    printf("stat_info.worker_fork_failures = %ld\n", stat_info.worker_fork_failures);
    printf("stat_info.worker_pipe_fd_select_failures = %ld\n", stat_info.worker_pipe_fd_select_failures);
    printf("stat_info.worker_pipe_creat_failures = %ld\n", stat_info.worker_pipe_creat_failures);
    printf("stat_info.worker_pipe_write_fd_invalid = %ld\n", stat_info.worker_pipe_write_fd_invalid);
    printf("stat_info.worker_execve_failures = %ld\n", stat_info.worker_execve_failures);
    printf("stat_info.worker_sigpipe_count = %ld\n", stat_info.worker_sigpipe_count);
    printf("stat_info.worker_main_select_bad_fd = %ld\n", stat_info.worker_main_select_bad_fd);
    printf("stat_info.fork_helper_pipe_read_failure = %ld\n", stat_info.fork_helper_pipe_read_failures);
    pthread_mutex_unlock(&stat_info_mutex);
}
