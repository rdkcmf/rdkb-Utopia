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
===================================================================
    This programs will monitor specified processes and restart
    is they are dead.
===================================================================
*/
#include <telemetry_busmessage_sender.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include "secure_wrapper.h"

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
//unused function
#if 0
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
#endif

static void str_trim (char **p)
{
    if ((p == NULL) || (*p == NULL))
        return;
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

/*
 * Returns 1 if process is found, 0 otherwise
 */
static int find_process (const char *proc_name,const char *pidfile)
{
    //struct dirent *pentry;
    char fname[64], buf[256], *name , pidFname[64] = {0}, pid[16] = {0};
    char *pos;

  //  DIR *pdir = opendir("/proc");
   // if (NULL == pdir) {
     //   return errno;
   // }

   #if 0
    while ((pentry = readdir(pdir))) {
        if (!strcmp(pentry->d_name, ".") ||
            !strcmp(pentry->d_name, "..")) {
            continue;
        }
        // ignore non PID entries in /proc
        if (!str_isdigit(pentry->d_name)) {
            continue;
        }

#endif

 snprintf(pidFname, sizeof(pidFname), "%s",pidfile);
        FILE *pidFp = fopen(pidFname, "r");
        if(pidFp) {
        	fgets(pid, sizeof(pid), pidFp);

		 if((pos = strrchr(pid, '\n')) != NULL)
        *pos = '\0';
        	fclose(pidFp);
        } else {
//              return errno;
//Restart will not happen if PID file is not present
                return 0;

        }

        // printf("%s", pentry->d_name);
        snprintf(fname, sizeof(fname), "/proc/%s/cmdline",pid);
       
	
        FILE *fp = fopen(fname, "r");
        if (fp) {
            fgets(buf, sizeof(buf), fp);
            fclose(fp);
            name = strrchr(buf, '/');
            name = (NULL == name) ? buf : (name+1);
            printf("name is -%s ", name);
            if (0 == strcmp(name, proc_name)) {
                //closedir(pdir);
                /* process found */
                return 1; 
            }
        }
  //  }

   // closedir(pdir);
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

    if (find_process(proc_name,pid_file)) {
        // process exists, nothing to do
        printf("pmon: %s process exists, nothing to do\n", proc_name);
        return 0;
    }
    v_secure_system("echo ' RDKB_PROCESS_CRASHED : %s is not running, need restart ' >> /rdklogs/logs/SelfHeal.txt.0 ",proc_name);
    //dnsmasq selfheal mechanism is in Aggresive Selfheal for DHCP Manager
    #if !defined (FEATURE_RDKB_DHCP_MANAGER)

    if(!strcmp(proc_name,"dnsmasq")) {
        t2_event_d("SYS_SH_dnsmasq_restart",1);
    }
    #endif
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
    char buf2[512];
    char *proc_name, *pid_file, *restart_cmd;

    if (1 == argc) {
        pmon_usage();
        return 1;
    }
    if (NULL == (fp = fopen(argv[1], "r"))) {
        return 1;
    }
    while((p_buf = fgets(buf, sizeof(buf), fp))) {
        size_t len = strlen(buf);
        memcpy(buf2, buf, len + 1);
        str_trim(&p_buf);
        proc_name = strsep(&p_buf, " \t\n");
        str_trim(&p_buf);
        pid_file = strsep(&p_buf, " \t\n");
        str_trim(&p_buf);
        restart_cmd = strsep(&p_buf, "\n");
        if (NULL == proc_name || NULL == pid_file || NULL == restart_cmd || 
            0 == strlen(proc_name) || 0 == strlen(pid_file) || 0 == strlen(restart_cmd)) {
            if ((len > 0) && (buf2[len - 1] == '\n'))
                buf2[len - 1] = 0;
            printf("pmon: skip malformed config line '%s'\n", buf2);
            continue;
        }
        printf("%s: proc_name '%s', pid_file '%s', restart_cmd '%s'\n", __FUNCTION__, proc_name, pid_file, restart_cmd);
        pid_file = (strcasecmp(pid_file,"none")) ? pid_file : NULL;
        proc_mon(proc_name, pid_file, restart_cmd);
    }
    fclose(fp);
    
    return 0;
}

