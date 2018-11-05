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

#include "hdk_srv.h"

#include "hdk_srv_log.h"

#ifdef HDK_LOGGING
static HDK_LOG_CallbackData s_HDK_SRV_LoggingCallbackData =
{
    0 /* pfn */,
    0 /* pCtx */
};
#endif /* def HDK_LOGGING */

/* extern */ const HDK_LOG_CallbackData* HDK_SRV_GetLoggingCallbackData(void)
{
#ifdef HDK_LOGGING
    return &s_HDK_SRV_LoggingCallbackData;
#else /* ndef HDK_LOGGING */
    return 0;
#endif /* def HDK_LOGGING */
}

/* HDK_SRV_EXPORT */ void HDK_SRV_RegisterLoggingCallback(HDK_LOG_CallbackFn pfnCallback, void* pCtx)
{
#ifdef HDK_LOGGING
    s_HDK_SRV_LoggingCallbackData.pfn = pfnCallback;
    s_HDK_SRV_LoggingCallbackData.pCtx = pCtx;
#else /* ndef HDK_LOGGING */
    (void)pfnCallback;
    (void)pCtx;
#endif /* def HDK_LOGGING */
}
