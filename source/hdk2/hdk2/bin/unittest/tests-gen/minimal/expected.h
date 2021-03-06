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

#ifndef __ACTUAL_H__
#define __ACTUAL_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_EXPORT ACTUAL_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_BUILD
#      define ACTUAL_EXPORT ACTUAL_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_EXPORT ACTUAL_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_BUILD
#      define ACTUAL_EXPORT ACTUAL_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_EXPORT ACTUAL_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_Element
{
    ACTUAL_Element_Cisco_CiscoAction = 0,
    ACTUAL_Element_Cisco_CiscoActionResponse = 1,
    ACTUAL_Element_Cisco_CiscoActionResult = 2,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body = 3,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 4,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header = 5
} ACTUAL_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_EnumType
{
    ACTUAL_EnumType_Cisco_CiscoActionResult = -1
} ACTUAL_EnumType;


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoActionResult
 */

typedef enum _ACTUAL_Enum_Cisco_CiscoActionResult
{
    ACTUAL_Enum_Cisco_CiscoActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_CiscoActionResult_OK = 0,
    ACTUAL_Enum_Cisco_CiscoActionResult_ERROR = 1
} ACTUAL_Enum_Cisco_CiscoActionResult;

#define ACTUAL_Set_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_Append_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_Get_Cisco_CiscoActionResult(pStruct, element) (ACTUAL_Enum_Cisco_CiscoActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoActionResult)
#define ACTUAL_GetEx_Cisco_CiscoActionResult(pStruct, element, value) (ACTUAL_Enum_Cisco_CiscoActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_GetMember_Cisco_CiscoActionResult(pMember) (ACTUAL_Enum_Cisco_CiscoActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_CiscoActionResult)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_MethodEnum
{
    ACTUAL_MethodEnum_Cisco_CiscoAction = 0
} ACTUAL_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_METHOD_CISCO_CISCOACTION__


/*
 * Methods
 */

extern void ACTUAL_Method_Cisco_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * Module
 */

ACTUAL_EXPORT const HDK_MOD_Module* ACTUAL_Module(void);

/* Dynamic server module export */
ACTUAL_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_H__ */
