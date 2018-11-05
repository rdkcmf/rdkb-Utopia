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

#ifndef __HDK_XML_LOG_H__
#define __HDK_XML_LOG_H__

#include "hdk_log.h"

#define HDK_XML_LOG_PREFIX "[HDK_XML] "

#define HDK_XML_LOG(_level, _str) \
    HDK_LOG(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str)
#define HDK_XML_LOGFMT1(_level, _str, _arg1) \
    HDK_LOGFMT1(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1)
#define HDK_XML_LOGFMT2(_level, _str, _arg1, _arg2) \
    HDK_LOGFMT2(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2)
#define HDK_XML_LOGFMT3(_level, _str, _arg1, _arg2, _arg3) \
    HDK_LOGFMT3(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2, _arg3)
#define HDK_XML_LOGFMT4(_level, _str, _arg1, _arg2, _arg3, _arg4) \
    HDK_LOGFMT4(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4)
#define HDK_XML_LOGFMT5(_level, _str, _arg1, _arg2, _arg3, _arg4, _arg5) \
    HDK_LOGFMT5(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4, _arg5)
#define HDK_XML_LOGFMT6(_level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6) \
    HDK_LOGFMT6(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6)
#define HDK_XML_LOGFMT7(_level, _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7) \
    HDK_LOGFMT7(HDK_XML_GetLoggingCallbackData(), _level, HDK_XML_LOG_PREFIX _str, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7)

extern const HDK_LOG_CallbackData* HDK_XML_GetLoggingCallbackData(void);

#endif /* __HDK_XML_LOG_H__ */
