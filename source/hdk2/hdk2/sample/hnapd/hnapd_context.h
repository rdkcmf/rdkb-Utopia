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

#ifndef __HNAPD_CONTEXT_H__
#define __HNAPD_CONTEXT_H__

#include "hdk_srv.h"

/* Initialize the server device context.  Returns non-zero for success, zero otherwise. */
extern void* HNAPD_ServerContext_Init(void);

/* Free the server device context */
extern void HNAPD_ServerContext_Free(void* pServerCtx, int fCommit);

/* Initialize the server module list */
extern HDK_SRV_ModuleContext** HNAPD_ServerModules_Init(void* pServerCtx);

/* Free the server module list */
extern void HNAPD_ServerModules_Free(HDK_SRV_ModuleContext** ppModuleContexts);

/* Server authentication callback function */
extern int HNAPD_Authenticate(void* pServerCtx, const char* pszUser, const char* pszPassword);

/* Server HNAP result callback function */
extern void HNAPD_HNAPResult(void* pServerCtx, HDK_XML_Struct* pOutput,
                               HDK_XML_Element resultElement, HDK_XML_Type resultType,
                               int resultOK, int resultReboot);

#endif /* __HNAPD_CONTEXT_H__ */
