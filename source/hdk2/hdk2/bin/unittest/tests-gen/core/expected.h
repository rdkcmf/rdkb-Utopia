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
    ACTUAL_Element_Action = 0,
    ACTUAL_Element_ActionResponse = 1,
    ACTUAL_Element_ActionResult = 2,
    ACTUAL_Element_Member = 3,
    ACTUAL_Element_Sub_ActionSub = 4,
    ACTUAL_Element_Sub_ActionSubResponse = 5,
    ACTUAL_Element_Sub_ActionSubResult = 6,
    ACTUAL_Element_Sub_MemberSub = 7,
    ACTUAL_Element_Cisco_ActionExt = 8,
    ACTUAL_Element_Cisco_ActionExtResponse = 9,
    ACTUAL_Element_Cisco_ActionExtResult = 10,
    ACTUAL_Element_Cisco_MemberExt = 11,
    ACTUAL_Element_Cisco_Sub_ActionExtSub = 12,
    ACTUAL_Element_Cisco_Sub_ActionExtSubResponse = 13,
    ACTUAL_Element_Cisco_Sub_ActionExtSubResult = 14,
    ACTUAL_Element_Cisco_Sub_MemberExtSub = 15,
    ACTUAL_Element_Purenetworks_HNAP_ActionLegacy2 = 16,
    ACTUAL_Element_Purenetworks_HNAP_ActionLegacy2Response = 17,
    ACTUAL_Element_Purenetworks_HNAP_ActionLegacy2Result = 18,
    ACTUAL_Element_Purenetworks_HNAP_MemberLegacy2 = 19,
    ACTUAL_Element_PN_Sub_ActionLegacy2Sub = 20,
    ACTUAL_Element_PN_Sub_ActionLegacy2SubResponse = 21,
    ACTUAL_Element_PN_Sub_ActionLegacy2SubResult = 22,
    ACTUAL_Element_PN_Sub_MemberLegacy2Sub = 23,
    ACTUAL_Element_PN_ActionLegacy = 24,
    ACTUAL_Element_PN_ActionLegacyResponse = 25,
    ACTUAL_Element_PN_ActionLegacyResult = 26,
    ACTUAL_Element_PN_MemberLegacy = 27,
    ACTUAL_Element_Purenetworks_HNAP1_Sub_ActionSubLegacy = 28,
    ACTUAL_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResponse = 29,
    ACTUAL_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResult = 30,
    ACTUAL_Element_Purenetworks_HNAP1_Sub_MemberLegacySub = 31,
    ACTUAL_Element_Purenetworks_ActionLegacyExt = 32,
    ACTUAL_Element_Purenetworks_ActionLegacyExtResponse = 33,
    ACTUAL_Element_Purenetworks_ActionLegacyExtResult = 34,
    ACTUAL_Element_Purenetworks_MemberLegacyExt = 35,
    ACTUAL_Element_Purenetworks_Sub_ActionLegacyExtSub = 36,
    ACTUAL_Element_Purenetworks_Sub_ActionLegacyExtSubResponse = 37,
    ACTUAL_Element_Purenetworks_Sub_ActionLegacyExtSubResult = 38,
    ACTUAL_Element_Purenetworks_Sub_MemberLegacyExtSub = 39,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body = 40,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 41,
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header = 42
} ACTUAL_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_EnumType
{
    ACTUAL_EnumType_ActionResult = -1,
    ACTUAL_EnumType_Sub_ActionSubResult = -2,
    ACTUAL_EnumType_Cisco_ActionExtResult = -3,
    ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult = -4,
    ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result = -5,
    ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult = -6,
    ACTUAL_EnumType_PN_ActionLegacyResult = -7,
    ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult = -8,
    ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult = -9,
    ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult = -10
} ACTUAL_EnumType;


/*
 * Enumeration http://cisco.com/HNAP/ActionResult
 */

typedef enum _ACTUAL_Enum_ActionResult
{
    ACTUAL_Enum_ActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_ActionResult_OK = 0,
    ACTUAL_Enum_ActionResult_ERROR = 1
} ACTUAL_Enum_ActionResult;

#define ACTUAL_Set_ActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_ActionResult, 0 ? ACTUAL_Enum_ActionResult_OK : (value))
#define ACTUAL_Append_ActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_ActionResult, 0 ? ACTUAL_Enum_ActionResult_OK : (value))
#define ACTUAL_Get_ActionResult(pStruct, element) (ACTUAL_Enum_ActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_ActionResult)
#define ACTUAL_GetEx_ActionResult(pStruct, element, value) (ACTUAL_Enum_ActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_ActionResult, 0 ? ACTUAL_Enum_ActionResult_OK : (value))
#define ACTUAL_GetMember_ActionResult(pMember) (ACTUAL_Enum_ActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_ActionResult)


/*
 * Enumeration http://cisco.com/HNAP/Sub/ActionSubResult
 */

typedef enum _ACTUAL_Enum_Sub_ActionSubResult
{
    ACTUAL_Enum_Sub_ActionSubResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Sub_ActionSubResult_OK = 0,
    ACTUAL_Enum_Sub_ActionSubResult_ERROR = 1
} ACTUAL_Enum_Sub_ActionSubResult;

#define ACTUAL_Set_Sub_ActionSubResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Sub_ActionSubResult, 0 ? ACTUAL_Enum_Sub_ActionSubResult_OK : (value))
#define ACTUAL_Append_Sub_ActionSubResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Sub_ActionSubResult, 0 ? ACTUAL_Enum_Sub_ActionSubResult_OK : (value))
#define ACTUAL_Get_Sub_ActionSubResult(pStruct, element) (ACTUAL_Enum_Sub_ActionSubResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Sub_ActionSubResult)
#define ACTUAL_GetEx_Sub_ActionSubResult(pStruct, element, value) (ACTUAL_Enum_Sub_ActionSubResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Sub_ActionSubResult, 0 ? ACTUAL_Enum_Sub_ActionSubResult_OK : (value))
#define ACTUAL_GetMember_Sub_ActionSubResult(pMember) (ACTUAL_Enum_Sub_ActionSubResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Sub_ActionSubResult)


/*
 * Enumeration http://cisco.com/HNAPExt/ActionExtResult
 */

typedef enum _ACTUAL_Enum_Cisco_ActionExtResult
{
    ACTUAL_Enum_Cisco_ActionExtResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_ActionExtResult_OK = 0,
    ACTUAL_Enum_Cisco_ActionExtResult_ERROR = 1
} ACTUAL_Enum_Cisco_ActionExtResult;

#define ACTUAL_Set_Cisco_ActionExtResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_ActionExtResult, 0 ? ACTUAL_Enum_Cisco_ActionExtResult_OK : (value))
#define ACTUAL_Append_Cisco_ActionExtResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_ActionExtResult, 0 ? ACTUAL_Enum_Cisco_ActionExtResult_OK : (value))
#define ACTUAL_Get_Cisco_ActionExtResult(pStruct, element) (ACTUAL_Enum_Cisco_ActionExtResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_ActionExtResult)
#define ACTUAL_GetEx_Cisco_ActionExtResult(pStruct, element, value) (ACTUAL_Enum_Cisco_ActionExtResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_ActionExtResult, 0 ? ACTUAL_Enum_Cisco_ActionExtResult_OK : (value))
#define ACTUAL_GetMember_Cisco_ActionExtResult(pMember) (ACTUAL_Enum_Cisco_ActionExtResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_ActionExtResult)


/*
 * Enumeration http://cisco.com/HNAPExt/Sub/ActionExtSubResult
 */

typedef enum _ACTUAL_Enum_Cisco_Sub_ActionExtSubResult
{
    ACTUAL_Enum_Cisco_Sub_ActionExtSubResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Cisco_Sub_ActionExtSubResult_OK = 0,
    ACTUAL_Enum_Cisco_Sub_ActionExtSubResult_ERROR = 1
} ACTUAL_Enum_Cisco_Sub_ActionExtSubResult;

#define ACTUAL_Set_Cisco_Sub_ActionExtSubResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult, 0 ? ACTUAL_Enum_Cisco_Sub_ActionExtSubResult_OK : (value))
#define ACTUAL_Append_Cisco_Sub_ActionExtSubResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult, 0 ? ACTUAL_Enum_Cisco_Sub_ActionExtSubResult_OK : (value))
#define ACTUAL_Get_Cisco_Sub_ActionExtSubResult(pStruct, element) (ACTUAL_Enum_Cisco_Sub_ActionExtSubResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult)
#define ACTUAL_GetEx_Cisco_Sub_ActionExtSubResult(pStruct, element, value) (ACTUAL_Enum_Cisco_Sub_ActionExtSubResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult, 0 ? ACTUAL_Enum_Cisco_Sub_ActionExtSubResult_OK : (value))
#define ACTUAL_GetMember_Cisco_Sub_ActionExtSubResult(pMember) (ACTUAL_Enum_Cisco_Sub_ActionExtSubResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Cisco_Sub_ActionExtSubResult)


/*
 * Enumeration http://purenetworks.com/HNAP/ActionLegacy2Result
 */

typedef enum _ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result
{
    ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK = 0,
    ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result_ERROR = 1
} ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result;

#define ACTUAL_Set_Purenetworks_HNAP_ActionLegacy2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result, 0 ? ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK : (value))
#define ACTUAL_Append_Purenetworks_HNAP_ActionLegacy2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result, 0 ? ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK : (value))
#define ACTUAL_Get_Purenetworks_HNAP_ActionLegacy2Result(pStruct, element) (ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result)
#define ACTUAL_GetEx_Purenetworks_HNAP_ActionLegacy2Result(pStruct, element, value) (ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result, 0 ? ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK : (value))
#define ACTUAL_GetMember_Purenetworks_HNAP_ActionLegacy2Result(pMember) (ACTUAL_Enum_Purenetworks_HNAP_ActionLegacy2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Purenetworks_HNAP_ActionLegacy2Result)


/*
 * Enumeration http://purenetworks.com/HNAP/Sub/ActionLegacy2SubResult
 */

typedef enum _ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult
{
    ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult_OK = 0,
    ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult_ERROR = 1
} ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult;

#define ACTUAL_Set_PN_Sub_ActionLegacy2SubResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult, 0 ? ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult_OK : (value))
#define ACTUAL_Append_PN_Sub_ActionLegacy2SubResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult, 0 ? ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult_OK : (value))
#define ACTUAL_Get_PN_Sub_ActionLegacy2SubResult(pStruct, element) (ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult)
#define ACTUAL_GetEx_PN_Sub_ActionLegacy2SubResult(pStruct, element, value) (ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult, 0 ? ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult_OK : (value))
#define ACTUAL_GetMember_PN_Sub_ActionLegacy2SubResult(pMember) (ACTUAL_Enum_PN_Sub_ActionLegacy2SubResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_PN_Sub_ActionLegacy2SubResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/ActionLegacyResult
 */

typedef enum _ACTUAL_Enum_PN_ActionLegacyResult
{
    ACTUAL_Enum_PN_ActionLegacyResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_PN_ActionLegacyResult_OK = 0,
    ACTUAL_Enum_PN_ActionLegacyResult_ERROR = 1
} ACTUAL_Enum_PN_ActionLegacyResult;

#define ACTUAL_Set_PN_ActionLegacyResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_PN_ActionLegacyResult, 0 ? ACTUAL_Enum_PN_ActionLegacyResult_OK : (value))
#define ACTUAL_Append_PN_ActionLegacyResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_PN_ActionLegacyResult, 0 ? ACTUAL_Enum_PN_ActionLegacyResult_OK : (value))
#define ACTUAL_Get_PN_ActionLegacyResult(pStruct, element) (ACTUAL_Enum_PN_ActionLegacyResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_PN_ActionLegacyResult)
#define ACTUAL_GetEx_PN_ActionLegacyResult(pStruct, element, value) (ACTUAL_Enum_PN_ActionLegacyResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_PN_ActionLegacyResult, 0 ? ACTUAL_Enum_PN_ActionLegacyResult_OK : (value))
#define ACTUAL_GetMember_PN_ActionLegacyResult(pMember) (ACTUAL_Enum_PN_ActionLegacyResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_PN_ActionLegacyResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/Sub/ActionSubLegacyResult
 */

typedef enum _ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult
{
    ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK = 0,
    ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_ERROR = 1
} ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult;

#define ACTUAL_Set_Purenetworks_HNAP1_Sub_ActionSubLegacyResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult, 0 ? ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK : (value))
#define ACTUAL_Append_Purenetworks_HNAP1_Sub_ActionSubLegacyResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult, 0 ? ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK : (value))
#define ACTUAL_Get_Purenetworks_HNAP1_Sub_ActionSubLegacyResult(pStruct, element) (ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult)
#define ACTUAL_GetEx_Purenetworks_HNAP1_Sub_ActionSubLegacyResult(pStruct, element, value) (ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult, 0 ? ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK : (value))
#define ACTUAL_GetMember_Purenetworks_HNAP1_Sub_ActionSubLegacyResult(pMember) (ACTUAL_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult)


/*
 * Enumeration http://purenetworks.com/HNAPExt/ActionLegacyExtResult
 */

typedef enum _ACTUAL_Enum_Purenetworks_ActionLegacyExtResult
{
    ACTUAL_Enum_Purenetworks_ActionLegacyExtResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Purenetworks_ActionLegacyExtResult_OK = 0,
    ACTUAL_Enum_Purenetworks_ActionLegacyExtResult_ERROR = 1
} ACTUAL_Enum_Purenetworks_ActionLegacyExtResult;

#define ACTUAL_Set_Purenetworks_ActionLegacyExtResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult, 0 ? ACTUAL_Enum_Purenetworks_ActionLegacyExtResult_OK : (value))
#define ACTUAL_Append_Purenetworks_ActionLegacyExtResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult, 0 ? ACTUAL_Enum_Purenetworks_ActionLegacyExtResult_OK : (value))
#define ACTUAL_Get_Purenetworks_ActionLegacyExtResult(pStruct, element) (ACTUAL_Enum_Purenetworks_ActionLegacyExtResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult)
#define ACTUAL_GetEx_Purenetworks_ActionLegacyExtResult(pStruct, element, value) (ACTUAL_Enum_Purenetworks_ActionLegacyExtResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult, 0 ? ACTUAL_Enum_Purenetworks_ActionLegacyExtResult_OK : (value))
#define ACTUAL_GetMember_Purenetworks_ActionLegacyExtResult(pMember) (ACTUAL_Enum_Purenetworks_ActionLegacyExtResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Purenetworks_ActionLegacyExtResult)


/*
 * Enumeration http://purenetworks.com/HNAPExt/Sub/ActionLegacyExtSubResult
 */

typedef enum _ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult
{
    ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK = 0,
    ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_ERROR = 1
} ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult;

#define ACTUAL_Set_Purenetworks_Sub_ActionLegacyExtSubResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult, 0 ? ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK : (value))
#define ACTUAL_Append_Purenetworks_Sub_ActionLegacyExtSubResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult, 0 ? ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK : (value))
#define ACTUAL_Get_Purenetworks_Sub_ActionLegacyExtSubResult(pStruct, element) (ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult)
#define ACTUAL_GetEx_Purenetworks_Sub_ActionLegacyExtSubResult(pStruct, element, value) (ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult, 0 ? ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK : (value))
#define ACTUAL_GetMember_Purenetworks_Sub_ActionLegacyExtSubResult(pMember) (ACTUAL_Enum_Purenetworks_Sub_ActionLegacyExtSubResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_MethodEnum
{
    ACTUAL_MethodEnum_Action = 0,
    ACTUAL_MethodEnum_Sub_ActionSub = 1,
    ACTUAL_MethodEnum_Cisco_ActionExt = 2,
    ACTUAL_MethodEnum_Cisco_Sub_ActionExtSub = 3,
    ACTUAL_MethodEnum_Purenetworks_HNAP_ActionLegacy2 = 4,
    ACTUAL_MethodEnum_PN_Sub_ActionLegacy2Sub = 5,
    ACTUAL_MethodEnum_PN_ActionLegacy = 6,
    ACTUAL_MethodEnum_Purenetworks_HNAP1_Sub_ActionSubLegacy = 7,
    ACTUAL_MethodEnum_Purenetworks_ActionLegacyExt = 8,
    ACTUAL_MethodEnum_Purenetworks_Sub_ActionLegacyExtSub = 9
} ACTUAL_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_METHOD_ACTION__
#define __ACTUAL_METHOD_SUB_ACTIONSUB__
#define __ACTUAL_METHOD_CISCO_ACTIONEXT__
#define __ACTUAL_METHOD_CISCO_SUB_ACTIONEXTSUB__
#define __ACTUAL_METHOD_PURENETWORKS_HNAP_ACTIONLEGACY2__
#define __ACTUAL_METHOD_PN_SUB_ACTIONLEGACY2SUB__
#define __ACTUAL_METHOD_PN_ACTIONLEGACY__
#define __ACTUAL_METHOD_PURENETWORKS_HNAP1_SUB_ACTIONSUBLEGACY__
#define __ACTUAL_METHOD_PURENETWORKS_ACTIONLEGACYEXT__
#define __ACTUAL_METHOD_PURENETWORKS_SUB_ACTIONLEGACYEXTSUB__


/*
 * Methods
 */

extern void ACTUAL_Method_Action(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Sub_ActionSub(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Cisco_ActionExt(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Cisco_Sub_ActionExtSub(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Purenetworks_HNAP_ActionLegacy2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_PN_Sub_ActionLegacy2Sub(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_PN_ActionLegacy(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Purenetworks_HNAP1_Sub_ActionSubLegacy(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Purenetworks_ActionLegacyExt(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_Method_Purenetworks_Sub_ActionLegacyExtSub(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * Module
 */

ACTUAL_EXPORT const HDK_MOD_Module* ACTUAL_Module(void);

/* Dynamic server module export */
ACTUAL_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_H__ */
