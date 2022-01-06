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
 * utctx_internal.h - Utopia system macros
 */

#ifndef __UTCTX_INTERNAL_H__
#define __UTCTX_INTERNAL_H__

#ifdef UTCTX_UNITTEST
#include "unittest_syscfg.h"
#else
#include <sysevent/sysevent.h>
#include <sysevent/lib/libsysevent_internal.h>
#include <syscfg/syscfg.h>
#include <sys/reboot.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <ulog/ulog.h>
#endif
#include <stdio.h>

//RDKB-18005 supressing logs
#undef UTCTX_LOG

/* Logging Macros */
#ifdef UTCTX_LOG
#ifdef UTCTX_UNITTEST
#define UTCTX_LOG_INIT()
#define UTCTX_LOG_CFG1(y,z)        printf("utctx.c: SysCfg: "); printf(y, z)
#define UTCTX_LOG_CFG2(x,y,z)      printf("utctx.c: SysCfg: "); printf(x, y, z)
#define UTCTX_LOG_CFG3(w,x,y,z)    printf("utctx.c: SysCfg: "); printf(w, x, y, z)
#define UTCTX_LOG_CFG4(v,w,x,y,z)  printf("utctx.c: SysCfg: "); printf(v, w, x, y, z)
#define UTCTX_LOG_DBG1(y,z)        printf("utctx.c: Debug: "); printf(y, z)
#define UTCTX_LOG_DBG2(x,y,z)      printf("utctx.c: Debug: "); printf(x, y, z)
#define UTCTX_LOG_DBG3(w,x,y,z)    printf("utctx.c: Debug: "); printf(w, x, y, z)
#define UTCTX_LOG_DBG4(v,w,x,y,z)  printf("utctx.c: Debug: "); printf(v, w, x, y, z)
#define UTCTX_LOG_ERR1(y,z)        printf("utctx.c: Error: "); printf(y, z)
#define UTCTX_LOG_ERR2(x,y,z)      printf("utctx.c: Error: "); printf(x, y, z)
#define UTCTX_LOG_ERR3(w,x,y,z)    printf("utctx.c: Error: "); printf(w, x, y, z)
#define UTCTX_LOG_ERR4(v,w,x,y,z)  printf("utctx.c: Error: "); printf(v, w, x, y, z)
#else
#define UTCTX_LOG_INIT()          ulog_init()
#define UTCTX_LOG_DBG(x)          ulog_debug(ULOG_SYSTEM, UL_UTCTX, x)
#define UTCTX_LOG_ERR(x)          ulog_error(ULOG_SYSTEM, UL_UTCTX, x)
#define UTCTX_LOG_CFG1(y,z)       ulogf(ULOG_CONFIG, UL_UTCTX, y, z)
#define UTCTX_LOG_CFG2(x,y,z)     ulogf(ULOG_CONFIG, UL_UTCTX, x, y, z)
#define UTCTX_LOG_CFG3(w,x,y,z)   ulogf(ULOG_CONFIG, UL_UTCTX, w, x, y, z)
#define UTCTX_LOG_CFG4(v,w,x,y,z) ulogf(ULOG_CONFIG, UL_UTCTX, v, w, x, y, z)
#define UTCTX_LOG_DBG1(y,z)       ulog_debugf(ULOG_SYSTEM, UL_UTCTX, y, z)
#define UTCTX_LOG_DBG2(x,y,z)     ulog_debugf(ULOG_SYSTEM, UL_UTCTX, x, y, z)
#define UTCTX_LOG_DBG3(w,x,y,z)   ulog_debugf(ULOG_SYSTEM, UL_UTCTX, w, x, y, z)
#define UTCTX_LOG_DBG4(v,w,x,y,z) ulog_debugf(ULOG_SYSTEM, UL_UTCTX, v, w, x, y, z)
#define UTCTX_LOG_ERR1(y,z)       ulog_errorf(ULOG_SYSTEM, UL_UTCTX, y, z)
#define UTCTX_LOG_ERR2(x,y,z)     ulog_errorf(ULOG_SYSTEM, UL_UTCTX, x, y, z)
#define UTCTX_LOG_ERR3(w,x,y,z)   ulog_errorf(ULOG_SYSTEM, UL_UTCTX, w, x, y, z)
#define UTCTX_LOG_ERR4(v,w,x,y,z) ulog_errorf(ULOG_SYSTEM, UL_UTCTX, v, w, x, y, z)
#endif
#else
#define UTCTX_LOG_INIT()
#define UTCTX_LOG_CFG1(y,z)
#define UTCTX_LOG_CFG2(x,y,z)
#define UTCTX_LOG_CFG3(w,x,y,z)
#define UTCTX_LOG_CFG4(v,w,x,y,z)
#define UTCTX_LOG_DBG1(y,z)
#define UTCTX_LOG_DBG2(x,y,z)
#define UTCTX_LOG_DBG3(w,x,y,z)
#define UTCTX_LOG_DBG4(v,w,x,y,z)
#define UTCTX_LOG_ERR1(y,z)
#define UTCTX_LOG_ERR2(x,y,z)
#define UTCTX_LOG_ERR3(w,x,y,z)
#define UTCTX_LOG_ERR4(v,w,x,y,z)
#endif

/* SysEvent Macros */
#ifdef UTCTX_UNITTEST
#define SysEvent_Close(y,z)       printf("Closing sysevent...\n")
#define SysEvent_Open(v,w,x,y,z)  printf("Initializing sysevent...\n")
#define SysEvent_Get(v,w,x,y,z)   SysCfg_Get(0,x,y,z)
#define SysEvent_Set(w,x,y,z)     SysCfg_Set(0,y,z)
#define SysEvent_Trigger(x,y,z,c)   printf("Triggering %s...\n", z)
#else
#define SysEvent_Close(y,z)       sysevent_close(y,z)
#define SysEvent_Open(v,w,x,y,z)  sysevent_open(v,w,x,y,z)
#define SysEvent_Get(v,w,x,y,z)   (sysevent_get(v,w,x,y,z) == 0 ? 1 : 0)
#define SysEvent_Set(w,x,y,z,c)     sysevent_set(w,x,y,z,c)
#define SysEvent_Trigger(x,y,z,c)   sysevent_set(x,y,z,"1",c)

/* Utopia event defines */
#define UTCTX_EVENT_ADDRESS      "127.0.0.1"
#define UTCTX_EVENT_NAME         "sect1"
#define UTCTX_EVENT_VERSION      SE_VERSION
#define UTCTX_EVENT_PORT         SE_SERVER_WELL_KNOWN_PORT & 0x0000FFFF
#endif

/* SysCfg Macros */
#ifndef UTCTX_UNITTEST
#define SysCfg_Commit()      syscfg_commit()
#define SysCfg_Init()        (1)
#define SysCfg_Free()
#define SysCfg_Get(w,x,y,z)  (syscfg_get(w,x,y,z) == 0 ? 1 : 0)
#define SysCfg_GetAll(x,y,z) (syscfg_getall(x,y,z) == 0 ? 1 : 0)
#define SysCfg_Set(x,y,z)    syscfg_set(x,y,z)
#define SysCfg_Unset(y,z)    syscfg_unset(y,z)
#else
#define SysCfg_Free()        SysCfg_Commit()
#endif

#endif /* __UTCTX_INTERNAL_H__ */
