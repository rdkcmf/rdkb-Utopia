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
===================================================================
    This programs will monitor specified processes and restart
    is they are dead.
===================================================================
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

/*
 * Config file format
 *   <process-name> <pid-file | none> <restart-cmd>
 * For eg:
 *   "mini_httpd /var/run/mini_httpd.pid /etc/init.d/service_httpd.sh httpd-restart"
 *   "samba      none                    /etc/init.d/samba restart
 */

static void pmon_usage ()
{
    printf("Usage: pmon config-file\n");
}

/*
 * Returns 1 - if given string just has digit chars [0-9]
 *         0 - otherwise
 */
static int str_isdigit (const char *str)
{
    int i = 0, sz = strlen(str);
    while (i < sz) {
        if (!isdigit(str[i++])) {
            return 0;
        }
    }
    return 1;
}

static void str_trim (char **p)
{
    if (NULL == p)
        return;
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

/*
 * Returns 1 if process is found, 0 otherwise
 */
static int find_process (const char *proc_name)
{
    struct dirent *pentry;
    char fname[64], buf[256], *name;

    DIR *pdir = opendir("/proc");
    if (NULL == pdir) {
        return errno;
    }

    while ((pentry = readdir(pdir))) {
        if (!strcmp(pentry->d_name, ".") ||
            !strcmp(pentry->d_name, "..")) {
            continue;
        }
        // ignore non PID entries in /proc
        if (!str_isdigit(pentry->d_name)) {
            continue;
        }
        // printf("%s", pentry->d_name);
        snprintf(fname, sizeof(fname), "/proc/%s/cmdline", pentry->d_name);
        FILE *fp = fopen(fname, "r");
        if (fp) {
            fgets(buf, sizeof(buf), fp);
            fclose(fp);
            name = strrchr(buf, '/');
            name = (NULL == name) ? buf : (name+1);
            // printf("-%s ", name);
            if (0 == strcmp(name, proc_name)) {
                closedir(pdir);
                /* process found */
                return 1; 
            }
        }
    }

    closedir(pdir);
    /* process not found */
    return 0; 
}

/*
 * Procedure     : proc_mon
 * Purpose       : check if given process is active, if not start it
 * Parmameters   :
 *    proc_name     : process name (case sensitive)
 *    pid_file      : any pid file to cleanup
 *    cmd           : command to restart the process
 * Return Code   :
 *   0           : on success
 *   1           : on exec error (caller may want to do _exit(127) if appropriate)
 *   2           : on fork error
 */
static int proc_mon (const char *proc_name, const char *pid_file, const char *cmd)
{
    int pid;

    if (find_process(proc_name)) {
        // process exists, nothing to do
        printf("pmon: %s process exists, nothing to do\n", proc_name);
        return 0;
    }

    printf("pmon: attempting to restart '%s' using '%s'\n", proc_name, cmd);
    if (pid_file) {
        printf("pmon: removing pid file %s\n", pid_file);
        unlink(pid_file);
    }

    pid = fork();
    if (0 == pid) {
        // child 
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        return 1; /* exec error */
    }

    // parent
    if (pid < 0) {
        printf("pmon: fork() error while processing '%s'\n", proc_name);
        return 2;  /* fork error */
    }

    return 0;
}

int main (int argc, char *argv[])
{
    FILE *fp;
    char buf[512], *p_buf;
    char *proc_name, *pid_file, *restart_cmd;

    if (1 == argc) {
        pmon_usage();
        return 1;
    }
    if (NULL == (fp = fopen(argv[1], "r"))) {
        return 1;
    }
    while((p_buf = fgets(buf, sizeof(buf), fp))) {
        str_trim(&p_buf);
        proc_name = strsep(&p_buf, " \t\n");
        str_trim(&p_buf);
        pid_file = strsep(&p_buf, " \t\n");
        str_trim(&p_buf);
        restart_cmd = strsep(&p_buf, "\n");
        if (NULL == proc_name || NULL == pid_file || NULL == restart_cmd || 
            0 == strlen(proc_name) || 0 == strlen(pid_file) || 0 == strlen(restart_cmd)) {
            printf("pmon: skip malformed config line '%s'\n", buf);
            continue;
        }
        printf("%s: proc_name '%s', pid_file '%s', restart_cmd '%s'\n", __FUNCTION__, proc_name, pid_file, restart_cmd);
        pid_file = (strcasecmp(pid_file,"none")) ? pid_file : NULL;
        proc_mon(proc_name, pid_file, restart_cmd);
    }
    fclose(fp);
    
    return 0;
}

