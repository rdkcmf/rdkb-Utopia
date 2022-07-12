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

#ifndef _ULOG_H_
#define _ULOG_H_

#include <stdio.h>              // FILE
#include <sys/types.h>          // pid_t
#include <sys/syslog.h>

#define ULOG_STR_SIZE  64

typedef struct _sys_log_info{
    char         name[ULOG_STR_SIZE]; // process name
    pid_t        pid;                 // process ID
    int          gPrior;              // global log priority
    int          prior;               // log priority
    unsigned int enable;              // logging enabled 
    FILE*        stream;              // stream of log file
}_sys_Log_Info;


#define ulog_LOG_Emerg(format, ...)   ulog_sys(LOG_EMERG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Alert(format, ...)   ulog_sys(LOG_ALERT, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Crit(format, ...)    ulog_sys(LOG_CRIT, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Err(format, ...)     ulog_sys(LOG_ERR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Warn(format, ...)    ulog_sys(LOG_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Note(format, ...)    ulog_sys(LOG_NOTICE, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Info(format, ...)    ulog_sys(LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ulog_LOG_Dbg(format, ...)     ulog_sys(LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)

int ulog_GetGlobalPrior(void);
void ulog_SetGlobalPrior(int prior);
int ulog_GetPrior(void);
void ulog_SetPrior(int prior);
int ulog_GetProcId(size_t size, char *name, pid_t *pid);
unsigned int ulog_GetEnable(void);
void ulog_SetEnable(unsigned int enable);
void ulog_sys(int prior, const char* fileName, int line, const char* format, ...);

typedef enum {
    ULOG_SYSTEM,
    ULOG_LAN,
    ULOG_WAN,
    ULOG_WLAN,
    ULOG_FIREWALL,
    ULOG_IGD,
    ULOG_CONFIG,
    ULOG_IPV6,
    ULOG_SERVICE,
    ULOG_ETHSWITCH,
} UCOMP;

typedef enum {
    /* GENERAL */
                UL_INFO,
                UL_STATUS,
    /* SYSTEM */
                UL_SYSEVENT,
                UL_SYSCFG,
                UL_UTCTX,                
    /* LAN */
                UL_DHCPSERVER,
    /* WAN */
                UL_WMON,
                UL_PPPOE,
                UL_PPTP,
                UL_L2TP,
    /* WLAN */
                UL_WLANCFG,
    /* FIREWALL */
                UL_PKTDROP,
                UL_TRIGGER,
    /* CONFIG */
                UL_IGD,
                UL_WEBUI,
                UL_UTAPI,
    /* IPv6 */
                UL_MANAGER,
                UL_TUNNEL,
                UL_DHCP
} USUBCOMP;


/*
 * Procedure     : ulog_init
 * Purpose       : Per process initialization of logging infrastruture
 * Parameters    : None
 * Return Values : None
 * Notes         :
 *    Opens connect to system logget and sets up a prefix string
 *    Current prefix string is "UTOPIA: "
 */
void ulog_init();

/*
 * Procedure     : ulog
 * Purpose       : Log a general message to system logger
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesg     - message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.NOTICE facility
 */
void ulog (UCOMP comp, USUBCOMP sub, const char *mesg);

/*
 * Procedure     : ulogf
 * Purpose       : Log a message to system logger with variable arg 
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     fmt     - format of message string
 *     ...     - variable args format for message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.NOTICE facility
 */
void ulogf (UCOMP comp, USUBCOMP sub, const char *fmt, ...);

/*
 * Procedure     : ulog_debug
 * Purpose       : Log a debug message to system logger
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesg     - message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.DEBUG facility
 */
void ulog_debug (UCOMP comp, USUBCOMP sub, const char *mesg);

/*
 * Procedure     : ulog_debugf
 * Purpose       : Log debug message to system logger with variable arg 
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     fmt     - format of message string
 *     ...     - variable args format for message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.DEBUG facility
 */
void ulog_debugf (UCOMP comp, USUBCOMP sub, const char *fmt, ...);

/*
 * Procedure     : ulog_error
 * Purpose       : Log an error message to system logger
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesg     - message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.ERROR facility
 */
void ulog_error (UCOMP comp, USUBCOMP sub, const char *mesg);

/*
 * Procedure     : ulog_errorf
 * Purpose       : Log error message to system logger with variable arg 
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesg     - message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.ERR facility
 */
void ulog_errorf (UCOMP comp, USUBCOMP sub, const char *fmt, ...);

/*
 * Procedure     : ulog_get_mesgs
 * Purpose       : Retrieve mesgs for given component.subcomponent
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesgbuf  - message strings output buffer
 *     size     - size of above buffer
 * Return Values : None
 * Notes         :
 *     mesgbuf will be truncated before mesgs are stored, 
 *     and upto allowed size
 */
void ulog_get_mesgs (UCOMP comp, USUBCOMP sub, char *mesgbuf, unsigned int size);

#if 0
/*
 * Procedure     : ulog_runcmd
 * Purpose       : Log and run command string
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     cmd     - command string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.NOTICE facility
 */
void ulog_runcmd (UCOMP comp, USUBCOMP sub, const char *cmd);

/*
 * Procedure     : ulog_runcmdf
 * Purpose       : Log and run command string with variable arg 
 * Parameters    : 
 *     UCOMP - component id
 *     USUBCOMP - subcomponent id
 *     mesg     - message string
 * Return Values : None
 * Notes         :
 *     uses syslog LOCAL7.NOTICE facility
 */
int ulog_runcmdf (UCOMP comp, USUBCOMP sub, const char *fmt, ...);
#endif

#endif /* _ULOG_H_ */
