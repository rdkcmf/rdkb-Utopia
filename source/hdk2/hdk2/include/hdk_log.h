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

#ifndef __HDK_LOG_H__
#define __HDK_LOG_H__

#ifdef HDK_LOGGING
#  include <stdio.h>
#endif /* HDK_LOGGING */

typedef enum _HDK_LOG_Level
{
    HDK_LOG_Level_Error = 0,
    HDK_LOG_Level_Warning,
    HDK_LOG_Level_Info,
    HDK_LOG_Level_Verbose
} HDK_LOG_Level;

/* Logging callback function. */
typedef void (*HDK_LOG_CallbackFn)(HDK_LOG_Level level, void* pCtx, const char* pszMessage);

typedef struct _HDK_LOG_CallbackData
{
    HDK_LOG_CallbackFn pfn;
    void* pCtx;
} HDK_LOG_CallbackData;


#ifdef HDK_LOGGING
#  define HDK_LOG_FORMAT_BUFFER_SIZE 512
#  ifdef _MSC_VER
#    define snprintf _snprintf
#  endif
#  define HDK_LOG(_pCallbackData, _level, _str) \
    if (_pCallbackData && _pCallbackData->pfn) \
    { \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, _str); \
    }
#  define HDK_LOGFMT1(_pCallbackData, _level, _str, _arg1) \
    if (_pCallbackData && _pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT2(_pCallbackData, _level, _str, _arg1, _arg2) \
    if (_pCallbackData && _pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT3(_pCallbackData, _level, _str, _arg1, _arg2, _arg3) \
    if (_pCallbackData &&_pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2, _arg3); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT4(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4) \
    if (_pCallbackData &&_pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2, _arg3, _arg4); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT5(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5) \
    if (_pCallbackData &&_pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2, _arg3, _arg4, _arg5); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT6(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6) \
    if (_pCallbackData &&_pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#  define HDK_LOGFMT7(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7) \
    if (_pCallbackData &&_pCallbackData->pfn) \
    { \
        char sz[HDK_LOG_FORMAT_BUFFER_SIZE]; \
        snprintf(sz, sizeof(sz), _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7); \
        sz[HDK_LOG_FORMAT_BUFFER_SIZE - 1] = '\0'; \
        _pCallbackData->pfn(_level, _pCallbackData->pCtx, sz); \
    }
#else /* ndef HDK_LOGGING */
#  define HDK_LOG(_pCallbackData, _level, _str) (void)0;
#  define HDK_LOGFMT1(_pCallbackData, _level, _str, _arg1) (void)0;
#  define HDK_LOGFMT2(_pCallbackData, _level, _str, _arg1, _arg2) (void)0;
#  define HDK_LOGFMT3(_pCallbackData, _level, _str, _arg1, _arg2, _arg3) (void)0;
#  define HDK_LOGFMT4(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4) (void)0;
#  define HDK_LOGFMT5(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5) (void)0;
#  define HDK_LOGFMT6(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6) (void)0;
#  define HDK_LOGFMT7(_pCallbackData, _level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7) (void)0;
#endif /* def HDK_LOGGING */
#endif /* __HDK_LOG_H__ */
