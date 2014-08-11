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
 * Copyright (c) 2008-2010 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#ifndef __HDK_CLI_LOG_H__
#define __HDK_CLI_LOG_H__

#include "hdk_log.h"

#define HDK_CLI_LOG_PREFIX "[HDK_CLI] "

#define HDK_CLI_LOG(_level, _str) \
    HDK_LOG(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str)
#define HDK_CLI_LOGFMT1(_level, _str, _arg1) \
    HDK_LOGFMT1(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str, _arg1)
#define HDK_CLI_LOGFMT2(_level, _str, _arg1, _arg2) \
    HDK_LOGFMT2(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str, _arg1, _arg2)
#define HDK_CLI_LOGFMT3(_level, _str, _arg1, _arg2, _arg3) \
    HDK_LOGFMT3(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str, _arg1, _arg2, _arg3)
#define HDK_CLI_LOGFMT4(_level, _str, _arg1, _arg2, _arg3, _arg4) \
    HDK_LOGFMT4(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4)
#define HDK_CLI_LOGFMT5(_level, _str, _arg1, _arg2, _arg3, _arg4, _arg5) \
    HDK_LOGFMT5(HDK_CLI_GetLoggingCallbackData(), _level, HDK_CLI_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4, _arg5)

extern const HDK_LOG_CallbackData* HDK_CLI_GetLoggingCallbackData(void);

#endif /* __HDK_CLI_LOG_H__ */
