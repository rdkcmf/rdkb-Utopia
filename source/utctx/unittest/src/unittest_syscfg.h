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
 * unittest_syscfg.h - Unittest SysCfg interface
 */

#ifndef __UNITTEST_SYSCFG_H__
#define __UNITTEST_SYSCFG_H__

extern void SysCfg_Commit(void);
extern int  SysCfg_Init(void);
extern int  SysCfg_Get(char* pszNamespace, char* pszKey, char* pszValue, int cbBuf);
extern int  SysCfg_GetAll(char* pBuffer, int ccbBuf, int* pccbBuf);
extern int  SysCfg_Set(char* pszNamespace, char* pszKey, char* pszValue);
extern int  SysCfg_Unset(char* pszNamespace, char* pszKey);

#endif /* __UNITTEST_SYSCFG_H__ */
