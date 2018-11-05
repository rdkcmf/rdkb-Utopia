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
    ACTUAL_Element_Cisco_CiscoCSVableStruct = 3,
    ACTUAL_Element_Cisco_CiscoStruct = 4,
    ACTUAL_Element_Cisco_a = 5,
    ACTUAL_Element_Cisco_as = 6,
    ACTUAL_Element_Cisco_b = 7,
    ACTUAL_Element_Cisco_bs = 8,
    ACTUAL_Element_Cisco_c = 9,
    ACTUAL_Element_Cisco_cs = 10,
    ACTUAL_Element_Cisco_csvInts = 11,
    ACTUAL_Element_Cisco_csvStruct = 12,
    ACTUAL_Element_Cisco_csvStructs = 13,
    ACTUAL_Element_Cisco_int = 14,
    ACTUAL_Element_Cisco_string = 15,
    ACTUAL_Element_Cisco_A_CiscoAction = 16,
    ACTUAL_Element_Cisco_A_CiscoActionResponse = 17,
    ACTUAL_Element_Cisco_A_CiscoActionResult = 18,
    ACTUAL_Element_Cisco_A_CiscoStruct = 19,
    ACTUAL_Element_Cisco_A_a = 20,
    ACTUAL_Element_Cisco_A_as = 21,
    ACTUAL_Element_Cisco_A_b = 22,
    ACTUAL_Element_Cisco_A_bs = 23,
    ACTUAL_Element_Cisco_A_c = 24,
    ACTUAL_Element_Cisco_A_cs = 25,
    ACTUAL_Element_Cisco_A_int = 26,
    ACTUAL_Element_Cisco_A_string = 27,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body = 28,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 29,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header = 30
} ACTUAL_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_EnumType
{
    ACTUAL_EnumType_Cisco_CiscoActionResult = -1,
    ACTUAL_EnumType_Cisco_CiscoEnum = -2,
    ACTUAL_EnumType_Cisco_A_CiscoActionResult = -3,
    ACTUAL_EnumType_Cisco_A_CiscoEnum = -4
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
 * Enumeration http://cisco.com/HNAPExt/CiscoEnum
 */

typedef enum _ACTUAL_Enum_Cisco_CiscoEnum
{
    ACTUAL_Enum_Cisco_CiscoEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_CiscoEnum_Value1 = 0,
    ACTUAL_Enum_Cisco_CiscoEnum_Value2 = 1,
    ACTUAL_Enum_Cisco_CiscoEnum_Value3 = 2
} ACTUAL_Enum_Cisco_CiscoEnum;

#define ACTUAL_Set_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_Append_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_Get_Cisco_CiscoEnum(pStruct, element) (ACTUAL_Enum_Cisco_CiscoEnum*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoEnum)
#define ACTUAL_GetEx_Cisco_CiscoEnum(pStruct, element, value) (ACTUAL_Enum_Cisco_CiscoEnum)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_GetMember_Cisco_CiscoEnum(pMember) (ACTUAL_Enum_Cisco_CiscoEnum*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_CiscoEnum)


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoActionResult
 */

typedef enum _ACTUAL_Enum_Cisco_A_CiscoActionResult
{
    ACTUAL_Enum_Cisco_A_CiscoActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_A_CiscoActionResult_OK = 0,
    ACTUAL_Enum_Cisco_A_CiscoActionResult_ERROR = 1
} ACTUAL_Enum_Cisco_A_CiscoActionResult;

#define ACTUAL_Set_Cisco_A_CiscoActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_Append_Cisco_A_CiscoActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_Get_Cisco_A_CiscoActionResult(pStruct, element) (ACTUAL_Enum_Cisco_A_CiscoActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoActionResult)
#define ACTUAL_GetEx_Cisco_A_CiscoActionResult(pStruct, element, value) (ACTUAL_Enum_Cisco_A_CiscoActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_GetMember_Cisco_A_CiscoActionResult(pMember) (ACTUAL_Enum_Cisco_A_CiscoActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_A_CiscoActionResult)


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoEnum
 */

typedef enum _ACTUAL_Enum_Cisco_A_CiscoEnum
{
    ACTUAL_Enum_Cisco_A_CiscoEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_A_CiscoEnum_Value1 = 0,
    ACTUAL_Enum_Cisco_A_CiscoEnum_Value2 = 1,
    ACTUAL_Enum_Cisco_A_CiscoEnum_Value3 = 2
} ACTUAL_Enum_Cisco_A_CiscoEnum;

#define ACTUAL_Set_Cisco_A_CiscoEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_Append_Cisco_A_CiscoEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_Get_Cisco_A_CiscoEnum(pStruct, element) (ACTUAL_Enum_Cisco_A_CiscoEnum*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoEnum)
#define ACTUAL_GetEx_Cisco_A_CiscoEnum(pStruct, element, value) (ACTUAL_Enum_Cisco_A_CiscoEnum)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_GetMember_Cisco_A_CiscoEnum(pMember) (ACTUAL_Enum_Cisco_A_CiscoEnum*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_A_CiscoEnum)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_MethodEnum
{
    ACTUAL_MethodEnum_Cisco_CiscoAction = 0,
    ACTUAL_MethodEnum_Cisco_A_CiscoAction = 1
} ACTUAL_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_METHOD_CISCO_CISCOACTION__
#define __ACTUAL_METHOD_CISCO_A_CISCOACTION__


/*
 * Methods
 */

extern void ACTUAL_Method_Cisco_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Cisco_A_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * Module
 */

ACTUAL_EXPORT const HDK_MOD_Module* ACTUAL_Module(void);

/* Dynamic server module export */
ACTUAL_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_H__ */
