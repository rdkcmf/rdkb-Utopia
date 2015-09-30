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

#ifndef __ACTUAL_CLIENT_MODULE_H__
#define __ACTUAL_CLIENT_MODULE_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_CLIENT_MODULE_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_CLIENT_MODULE_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_CLIENT_MODULE_EXPORT ACTUAL_CLIENT_MODULE_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_CLIENT_MODULE_BUILD
#      define ACTUAL_CLIENT_MODULE_EXPORT ACTUAL_CLIENT_MODULE_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_CLIENT_MODULE_EXPORT ACTUAL_CLIENT_MODULE_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_CLIENT_MODULE_BUILD
#      define ACTUAL_CLIENT_MODULE_EXPORT ACTUAL_CLIENT_MODULE_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_CLIENT_MODULE_EXPORT ACTUAL_CLIENT_MODULE_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_CLIENT_MODULE_Element
{
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoAction = 0,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoAction2 = 1,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoAction2Response = 2,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoAction2Result = 3,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoActionResponse = 4,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoActionResult = 5,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoEvent = 6,
    ACTUAL_CLIENT_MODULE_Element_Cisco_CiscoStruct = 7,
    ACTUAL_CLIENT_MODULE_Element_Cisco_a = 8,
    ACTUAL_CLIENT_MODULE_Element_Cisco_as = 9,
    ACTUAL_CLIENT_MODULE_Element_Cisco_b = 10,
    ACTUAL_CLIENT_MODULE_Element_Cisco_bool = 11,
    ACTUAL_CLIENT_MODULE_Element_Cisco_bs = 12,
    ACTUAL_CLIENT_MODULE_Element_Cisco_c = 13,
    ACTUAL_CLIENT_MODULE_Element_Cisco_cs = 14,
    ACTUAL_CLIENT_MODULE_Element_Cisco_extra = 15,
    ACTUAL_CLIENT_MODULE_Element_Cisco_in = 16,
    ACTUAL_CLIENT_MODULE_Element_Cisco_int = 17,
    ACTUAL_CLIENT_MODULE_Element_Cisco_out = 18,
    ACTUAL_CLIENT_MODULE_Element_Cisco_someFlag = 19,
    ACTUAL_CLIENT_MODULE_Element_Cisco_string = 20,
    ACTUAL_CLIENT_MODULE_Element_Cisco_stuff = 21,
    ACTUAL_CLIENT_MODULE_Element_Cisco_x = 22,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_CiscoAction = 23,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_CiscoActionResponse = 24,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_CiscoActionResult = 25,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_CiscoStruct = 26,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_a = 27,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_as = 28,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_b = 29,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_bs = 30,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_c = 31,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_cs = 32,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_int = 33,
    ACTUAL_CLIENT_MODULE_Element_Cisco_A_string = 34,
    ACTUAL_CLIENT_MODULE_Element_Schemas_xmlsoap_org_soap_envelope_Body = 35,
    ACTUAL_CLIENT_MODULE_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 36,
    ACTUAL_CLIENT_MODULE_Element_Schemas_xmlsoap_org_soap_envelope_Header = 37
} ACTUAL_CLIENT_MODULE_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_CLIENT_MODULE_EnumType
{
    ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result = -1,
    ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult = -2,
    ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum = -3,
    ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult = -4,
    ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum = -5
} ACTUAL_CLIENT_MODULE_EnumType;


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoAction2Result
 */

typedef enum _ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result
{
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result_OK = 0,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result_ERROR = 1
} ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result;

#define ACTUAL_CLIENT_MODULE_Set_Cisco_CiscoAction2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_CLIENT_MODULE_Append_Cisco_CiscoAction2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_CLIENT_MODULE_Get_Cisco_CiscoAction2Result(pStruct, element) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result)
#define ACTUAL_CLIENT_MODULE_GetEx_Cisco_CiscoAction2Result(pStruct, element, value) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_CLIENT_MODULE_GetMember_Cisco_CiscoAction2Result(pMember) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoAction2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoAction2Result)


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoActionResult
 */

typedef enum _ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult
{
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult_OK = 0,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult_ERROR = 1
} ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult;

#define ACTUAL_CLIENT_MODULE_Set_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_Append_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_Get_Cisco_CiscoActionResult(pStruct, element) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult)
#define ACTUAL_CLIENT_MODULE_GetEx_Cisco_CiscoActionResult(pStruct, element, value) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_GetMember_Cisco_CiscoActionResult(pMember) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoActionResult)


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoEnum
 */

typedef enum _ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum
{
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value1 = 0,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value2 = 1,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value3 = 2
} ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum;

#define ACTUAL_CLIENT_MODULE_Set_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_Append_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_Get_Cisco_CiscoEnum(pStruct, element) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum)
#define ACTUAL_CLIENT_MODULE_GetEx_Cisco_CiscoEnum(pStruct, element, value) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_GetMember_Cisco_CiscoEnum(pMember) (ACTUAL_CLIENT_MODULE_Enum_Cisco_CiscoEnum*)HDK_XML_GetMember_Enum(pMember, ACTUAL_CLIENT_MODULE_EnumType_Cisco_CiscoEnum)


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoActionResult
 */

typedef enum _ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult
{
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult_OK = 0,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult_ERROR = 1
} ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult;

#define ACTUAL_CLIENT_MODULE_Set_Cisco_A_CiscoActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_Append_Cisco_A_CiscoActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_Get_Cisco_A_CiscoActionResult(pStruct, element) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult)
#define ACTUAL_CLIENT_MODULE_GetEx_Cisco_A_CiscoActionResult(pStruct, element, value) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult_OK : (value))
#define ACTUAL_CLIENT_MODULE_GetMember_Cisco_A_CiscoActionResult(pMember) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoActionResult)


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoEnum
 */

typedef enum _ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum
{
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value1 = 0,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value2 = 1,
    ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value3 = 2
} ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum;

#define ACTUAL_CLIENT_MODULE_Set_Cisco_A_CiscoEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_Append_Cisco_A_CiscoEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_Get_Cisco_A_CiscoEnum(pStruct, element) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum)
#define ACTUAL_CLIENT_MODULE_GetEx_Cisco_A_CiscoEnum(pStruct, element, value) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum, 0 ? ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum_Value1 : (value))
#define ACTUAL_CLIENT_MODULE_GetMember_Cisco_A_CiscoEnum(pMember) (ACTUAL_CLIENT_MODULE_Enum_Cisco_A_CiscoEnum*)HDK_XML_GetMember_Enum(pMember, ACTUAL_CLIENT_MODULE_EnumType_Cisco_A_CiscoEnum)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_CLIENT_MODULE_MethodEnum
{
    ACTUAL_CLIENT_MODULE_MethodEnum_Cisco_CiscoAction = 0,
    ACTUAL_CLIENT_MODULE_MethodEnum_Cisco_CiscoAction2 = 1,
    ACTUAL_CLIENT_MODULE_MethodEnum_Cisco_A_CiscoAction = 2
} ACTUAL_CLIENT_MODULE_MethodEnum;


/*
 * Event enumeration
 */

typedef enum _ACTUAL_CLIENT_MODULE_EventEnum
{
    ACTUAL_CLIENT_MODULE_EventEnum_Cisco_CiscoEvent = 0
} ACTUAL_CLIENT_MODULE_EventEnum;


/*
 * Module
 */

ACTUAL_CLIENT_MODULE_EXPORT const HDK_MOD_Module* ACTUAL_CLIENT_MODULE_Module(void);

#endif /* __ACTUAL_CLIENT_MODULE_H__ */
