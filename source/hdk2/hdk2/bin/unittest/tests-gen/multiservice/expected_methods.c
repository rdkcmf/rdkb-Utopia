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

#include "actual.h"

#include "hdk_srv.h"


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result)                        prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)


/*
 * Method http://cisco.com/HNAPExt/CiscoAction
 */

#ifdef __ACTUAL_METHOD_CISCO_CISCOACTION__

void ACTUAL_Method_Cisco_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;
    (void)pOutput;
}

#endif /* __ACTUAL_METHOD_CISCO_CISCOACTION__ */


/*
 * Method http://cisco.com/HNAPExt/A/CiscoAction
 */

#ifdef __ACTUAL_METHOD_CISCO_A_CISCOACTION__

void ACTUAL_Method_Cisco_A_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;
    (void)pOutput;
}

#endif /* __ACTUAL_METHOD_CISCO_A_CISCOACTION__ */
