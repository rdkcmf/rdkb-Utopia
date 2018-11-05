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


#ifndef PAL_LOG_H
#define PAL_LOG_H

#include "pal_def.h"

typedef enum log_level_s
{
    PAL_LOG_LEVEL_FAILURE = 1,
    PAL_LOG_LEVEL_WARNING,
    PAL_LOG_LEVEL_INFO,
    PAL_LOG_LEVEL_DEBUG
}log_level_t;

typedef VOID (*pal_log_show_module_debug_info_callback_t) (CHAR* input_string);

extern INT32 PAL_log_register(CHAR * module_name, pal_log_show_module_debug_info_callback_t log_show_module_debug_info_func);
extern VOID PAL_log (CHAR* module_name, UINT32 level_number, CHAR *file, UINT32 line, const CHAR *fmt, ... );

#ifdef PAL_LOG_ENABLE
#define PAL_LOG_REGISTER(module_name, log_show_module_debug_info_func)  PAL_log_register(module_name, log_show_module_debug_info_func)
#define PAL_LOG(module_name, level_number, fmt... )                     PAL_log(module_name, level_number, __FILE__, __LINE__, ##fmt )

#elif defined (PAL_ULOG)

#include <ulog/ulog.h>

#define PAL_LOG_REGISTER(module_name, log_show_module_debug_info_func) ulog_init()
#define PAL_LOG(module_name, level_number, fmt...) ulogf(ULOG_IGD, UL_INFO, ##fmt)

#else
#define PAL_LOG_REGISTER(module_name, log_show_module_debug_info_func)
#define PAL_LOG(module_name, level_number, fmt... ) 
#endif

#endif/*#ifndef PAL_LOG_H*/

