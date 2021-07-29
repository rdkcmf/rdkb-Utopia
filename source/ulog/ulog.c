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
    This library provide logging API and routines to filter logs
    based on defined component.subcomponent
===================================================================
*/


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include "ulog.h"
#include "unistd.h"
#include "time.h"

#define ULOG_FILE   "/var/log/messages"
#define ULOG_IDENT  "UTOPIA"

static _sys_Log_Info sys_Log_Info = {"", 0, LOG_INFO, 1, 0, NULL};

typedef struct ucomp {
    char *name;
    int   id;
} ucomp_t;

ucomp_t components[] =
  {
    { "system",   ULOG_SYSTEM },
    { "lan",      ULOG_LAN },
    { "wan",      ULOG_WAN },
    { "wlan",     ULOG_WLAN },
    { "fw",       ULOG_FIREWALL },
    { "igd",      ULOG_IGD },
    { "config",   ULOG_CONFIG },
    { "ipv6",     ULOG_IPV6 },
    { "service",  ULOG_SERVICE },
    { "ethswitch",ULOG_ETHSWITCH },
    { NULL, -1 },
  };

ucomp_t subcomponents[] =
  {
    { "info",     UL_INFO },
    { "status",   UL_STATUS },
    { "syscfg",   UL_SYSCFG },
    { "sysevent", UL_SYSEVENT },
    { "utctx",    UL_UTCTX },    
    { "dhcpserver",  UL_DHCPSERVER },
    { "wmon",     UL_WMON },
    { "pppoe",    UL_PPPOE },
    { "pptp",     UL_PPTP },
    { "l2tp",     UL_L2TP },
    { "wlancfg",  UL_WLANCFG },
    { "pktdrop",  UL_PKTDROP },
    { "trigger",  UL_TRIGGER },
    { "igd",      UL_IGD },
    { "webui",    UL_WEBUI },
    { "utapi",    UL_UTAPI },
    { "manager",  UL_MANAGER },
    { "tunnel",   UL_TUNNEL },
    { "dhcp",     UL_DHCP },
    { NULL, -1 },
  };

static const char *getsubcomp (int subcomp)
{
    int i = 0;
    while (subcomponents[i].name) {
        if (subcomponents[i].id == subcomp) {
            return subcomponents[i].name;
        }
        i++;
    }
    return "unknown";
}

static const char *getcomp (int comp)
{
    int i = 0;
    while (components[i].name) {
        if (components[i].id == comp) {
            return components[i].name;
        }
        i++;
    }
    return "unknown";
}

void ulog_init ()
{
    openlog(ULOG_IDENT, LOG_NDELAY, LOG_LOCAL7);
}

/*
 * Logging APIs
 */
void ulog (UCOMP comp, USUBCOMP sub, const char *mesg)
{
    syslog(LOG_NOTICE, "%s.%s %s", getcomp(comp), getsubcomp(sub), mesg);
}

void ulogf (UCOMP comp, USUBCOMP sub, const char *fmt, ...)
{
    va_list ap;
    char sfmt[256];

    snprintf(sfmt, sizeof(sfmt), "%s.%s %s", getcomp(comp), getsubcomp(sub), fmt);

    va_start(ap, fmt);
    vsyslog(LOG_NOTICE, sfmt, ap);
    va_end(ap);
}

void ulog_debug (UCOMP comp, USUBCOMP sub, const char *mesg)
{
    syslog(LOG_DEBUG, "%s.%s %s", getcomp(comp), getsubcomp(sub), mesg);
}
 
void ulog_debugf (UCOMP comp, USUBCOMP sub, const char *fmt, ...)
{
    va_list ap;
    char sfmt[256];

    snprintf(sfmt, sizeof(sfmt), "%s.%s %s", getcomp(comp), getsubcomp(sub), fmt);

    va_start(ap, fmt);
    vsyslog(LOG_DEBUG, sfmt, ap);
    va_end(ap);
}

void ulog_error (UCOMP comp, USUBCOMP sub, const char *mesg)
{
    syslog(LOG_ERR, "%s.%s %s", getcomp(comp), getsubcomp(sub), mesg);
}

void ulog_errorf (UCOMP comp, USUBCOMP sub, const char *fmt, ...)
{
    va_list ap;
    char sfmt[256];

    snprintf(sfmt, sizeof(sfmt), "%s.%s %s", getcomp(comp), getsubcomp(sub), fmt);

    va_start(ap, fmt);
    vsyslog(LOG_ERR, sfmt, ap);
    va_end(ap);
}

void
ulog_get_mesgs (UCOMP comp, USUBCOMP sub, char *mesgbuf, unsigned int size)
{
    FILE *fp;
    char match_string[128], linebuf[256];

    if (NULL == mesgbuf) {
        return;
    }
    mesgbuf[0] = '\0';

    snprintf(match_string, sizeof(match_string), "%s: %s.%s",
             ULOG_IDENT, getcomp(comp), getsubcomp(sub));
  
    if (NULL == (fp = fopen(ULOG_FILE, "r"))) {
        return;
    }

    int lsz, remaining_sz = size;

    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        if (strstr(linebuf, match_string)) {
            lsz = strlen(linebuf);
            if (lsz >= remaining_sz) {
                break;  // no more room
            }
            strcat(mesgbuf, linebuf);
            remaining_sz -= lsz;
        }
    }

    fclose(fp);
}

/*
 * Log and Command APIs
 */
void ulog_runcmd (UCOMP comp, USUBCOMP sub, const char *cmd)
{
    syslog(LOG_NOTICE, "%s.%s %s", getcomp(comp), getsubcomp(sub), cmd);
    if (cmd) {
        system(cmd);
    }
}

int ulog_runcmdf (UCOMP comp, USUBCOMP sub, const char *fmt, ...)
{
    va_list ap, ap_copy;
    char sfmt[256];

    snprintf(sfmt, sizeof(sfmt), "%s.%s %s", getcomp(comp), getsubcomp(sub), fmt);

    va_copy(ap_copy, ap);

    va_start(ap, fmt);
    vsyslog(LOG_NOTICE, sfmt, ap);
    va_end(ap);

    va_start(ap_copy, fmt);
    vsnprintf(sfmt, sizeof(sfmt), fmt, ap_copy);
    va_end(ap_copy);
    return system(sfmt);
}

void ulog_sys_Init(int prior, unsigned int enable)
{
    char    name[80];
    int     ret;

    //ulog_GetGlobalPrior();
    ulog_GetPrior();
    ulog_SetPrior(prior);
    ulog_SetEnable(enable);

    ret = ulog_GetProcId(sizeof(sys_Log_Info.name), sys_Log_Info.name, &sys_Log_Info.pid);
    printf("After ulog_GetProcId \n");
    if(ret != 0)
        return;

    sprintf(name, "/var/log/%s.log", sys_Log_Info.name);

    sys_Log_Info.stream = fopen(name, "w");
    printf("After fopen pid  file  \n");
    if (sys_Log_Info.stream == NULL)
    {
        ulog_LOG_Err("%s() failed fopen() %s\n", __FUNCTION__, sys_Log_Info.stream);
    }
    if(sys_Log_Info.stream != NULL)
         fclose(sys_Log_Info.stream);
}

int ulog_GetProcId(size_t size, char *name, pid_t *pid)
{
    char  *buf[ULOG_STR_SIZE];
    char  *file_name[ULOG_STR_SIZE];
    int   len;  
    char  *retStr; 
    char  *str[ULOG_STR_SIZE];
    FILE* stream;        

    if(size == 0 || !name || !pid)
        return -1;
    name[0] = '\0';
    pid = (pid_t *)getpid();
    printf("After getpid,  pid = %d  \n", *pid);

    sprintf((char *)file_name, "/proc/%d/stat", *pid);
    stream = fopen((const char *)file_name, "r");
    printf("After fopen /proc/pid/stat  file  \n");
    if (stream == NULL)
    {
        return -1;
    }
    retStr = fgets((char *)buf, sizeof(buf), stream);
    if (retStr == NULL)
    {
	fclose(stream);
        return -1;
    }
    sscanf((const char *)buf, "%*d (%s", (char *)str);
    len = strlen((const char *)str);
    str[len - 1] = '\0';
    strncpy(name, (const char *)str, size - 1);
    name[size - 1] = '\0';

    fclose(stream);
    return 0;
}

int ulog_GetGlobalPrior(void)
{
     int             mask;                                                           // current priority mask
     int             prior;                                                          // priority level

     mask = setlogmask(0);

     for (prior = LOG_DEBUG; prior >= LOG_EMERG; prior--)
     {
         if (mask & LOG_MASK(prior))
	 {
             sys_Log_Info.gPrior = prior;
	     return prior;
	 }
     }

     return -1;
}

void ulog_SetGlobalPrior(int prior)
{
    if (prior < LOG_EMERG || prior > LOG_DEBUG)
    {
        return;
    }

    setlogmask(LOG_UPTO(prior));
    sys_Log_Info.gPrior = prior;
}


int ulog_GetPrior(void)
{
    return sys_Log_Info.prior;
}

void ulog_SetPrior(int prior)
{
    if (prior < LOG_EMERG || prior > LOG_DEBUG)
    {
         return;
    }

    sys_Log_Info.prior = prior;
}

unsigned int ulog_GetEnable(void)
{
    return sys_Log_Info.enable;
}

void ulog_SetEnable(unsigned int enable)
{
    sys_Log_Info.enable = enable;
}

void ulog_sys(int prior, const char* fileName, int line, const char* fmt, ...)
{
    char buf[ULOG_STR_SIZE] = {'\0'};
    struct timeval tv;
    time_t curtime;
    va_list ap;
    char sfmt[512];

    /*sprintf(buf, "Process Name	%s\nProcess ID	%d\nsyslog() Level   %s\n", sys_Log_Info.name, sys_Log_Info.pid, sys_Log_Info.prior);
    if(sys_Log_Info.stream != NULL && sys_Log_Info.enable == 1)
        fprintf(sys_Log_Info.stream, "%s", buf);*/

    gettimeofday(&tv, NULL);
    curtime=tv.tv_sec;

    strftime(buf, sizeof(buf), "%Y-%m-%d  %T.", (const void *)localtime(&curtime));

    snprintf(sfmt, sizeof(sfmt), "%s, %s:%d, %s", buf, fileName, line, fmt);

    va_start(ap, fmt);
    vsyslog(prior, sfmt, ap);
    va_end(ap);
}


#ifdef SAMPLE_MAIN
main()
{
    ulog_init();
    ulog(ULOG_SYSTEM, UL_SYSCFG, "Hello world");
    closelog();

    char buf[1024];
    buf[0]='\0';
    ulog_get_mesgs(ULOG_SYSTEM, UL_SYSCFG, buf, sizeof(buf));
    printf("ulog_get_mesgs buf - \n%s\n", buf);
}
#endif
