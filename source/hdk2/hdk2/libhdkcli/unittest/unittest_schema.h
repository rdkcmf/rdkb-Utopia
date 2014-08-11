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

#ifndef __UNITTEST_SCHEMA_H__
#define __UNITTEST_SCHEMA_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define UNITTEST_SCHEMA_EXPORT_PREFIX extern "C"
#else
#  define UNITTEST_SCHEMA_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define UNITTEST_SCHEMA_EXPORT UNITTEST_SCHEMA_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef UNITTEST_SCHEMA_BUILD
#      define UNITTEST_SCHEMA_EXPORT UNITTEST_SCHEMA_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define UNITTEST_SCHEMA_EXPORT UNITTEST_SCHEMA_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef UNITTEST_SCHEMA_BUILD
#      define UNITTEST_SCHEMA_EXPORT UNITTEST_SCHEMA_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define UNITTEST_SCHEMA_EXPORT UNITTEST_SCHEMA_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _UNITTEST_SCHEMA_Element
{
    UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolArg0 = 0,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolInputArg = 1,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember = 2,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethod = 3,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResponse = 4,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResult = 5,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember = 6,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember = 7,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod = 8,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResponse = 9,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResult = 10,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInput = 11,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResponse = 12,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResult = 13,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember = 14,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_IntArg0 = 15,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember = 16,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg = 17,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg0 = 18,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember = 19,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethod = 20,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResponse = 21,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResult = 22,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethod = 23,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResponse = 24,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResult = 25,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethod = 26,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResponse = 27,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResult = 28,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStringInputArg = 29,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStructOutputArg = 30,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethod = 31,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResponse = 32,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResult = 33,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArg0 = 34,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember = 35,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StringInputArg = 36,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember = 37,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StringOutArg = 38,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StructArg0 = 39,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StructInputArg = 40,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg = 41,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg0 = 42,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_string = 43,
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body = 44,
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 45,
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header = 46
} UNITTEST_SCHEMA_Element;


/*
 * Enum types enumeration
 */

typedef enum _UNITTEST_SCHEMA_EnumType
{
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult = -1,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult = -2,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult = -3,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult = -4,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult = -5,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult = -6,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult = -7,
    UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum = -8
} UNITTEST_SCHEMA_EnumType;


/*
 * Enumeration http://cisco.com/Unittest/ComplexUnittestMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_REBOOT = 1,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_ERROR = 2,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_ERROR_Error1 = 3,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_ERROR_Error2 = 4,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_ERROR_Error3 = 5
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_ComplexUnittestMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_ComplexUnittestMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_ComplexUnittestMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_ComplexUnittestMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_ComplexUnittestMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/HttpGetMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_HttpGetMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_HttpGetMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_HttpGetMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_HttpGetMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_HttpGetMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/HttpGetMethod_WithInputResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_HttpGetMethod_WithInputResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_HttpGetMethod_WithInputResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_HttpGetMethod_WithInputResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_HttpGetMethod_WithInputResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_HttpGetMethod_WithInputResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult)


/*
 * Enumeration http://cisco.com/Unittest/NoAuthMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_NoAuthMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_NoAuthMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_NoAuthMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_NoAuthMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_NoAuthMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/NoInputOutputPathMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_NoInputOutputPathMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_NoInputOutputPathMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_NoInputOutputPathMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_NoInputOutputPathMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_NoInputOutputPathMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/NoInputStructMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_NoInputStructMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_NoInputStructMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_NoInputStructMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_NoInputStructMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_NoInputStructMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/SimpleUnittestMethodResult
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_ERROR = 1
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_SimpleUnittestMethodResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_SimpleUnittestMethodResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_SimpleUnittestMethodResult(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_SimpleUnittestMethodResult(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_SimpleUnittestMethodResult(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult)


/*
 * Enumeration http://cisco.com/Unittest/UnittestEnum
 */

typedef enum _UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum
{
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue0 = 0,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue1 = 1,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue2 = 2,
    UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue3 = 3
} UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum;

#define UNITTEST_SCHEMA_Set_Cisco_Unittest_UnittestEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue0 : (value))
#define UNITTEST_SCHEMA_Append_Cisco_Unittest_UnittestEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue0 : (value))
#define UNITTEST_SCHEMA_Get_Cisco_Unittest_UnittestEnum(pStruct, element) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum*)HDK_XML_Get_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum)
#define UNITTEST_SCHEMA_GetEx_Cisco_Unittest_UnittestEnum(pStruct, element, value) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum)HDK_XML_GetEx_Enum(pStruct, element, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 ? UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum_EnumValue0 : (value))
#define UNITTEST_SCHEMA_GetMember_Cisco_Unittest_UnittestEnum(pMember) (UNITTEST_SCHEMA_Enum_Cisco_Unittest_UnittestEnum*)HDK_XML_GetMember_Enum(pMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum)


/*
 * Method enumeration
 */

typedef enum _UNITTEST_SCHEMA_MethodEnum
{
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_ComplexUnittestMethod = 0,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_HttpGetMethod = 1,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_HttpGetMethod_WithInput = 2,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_NoAuthMethod = 3,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_NoInputOutputPathMethod = 4,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_NoInputStructMethod = 5,
    UNITTEST_SCHEMA_MethodEnum_Cisco_Unittest_SimpleUnittestMethod = 6
} UNITTEST_SCHEMA_MethodEnum;


/*
 * Module
 */

UNITTEST_SCHEMA_EXPORT const HDK_MOD_Module* UNITTEST_SCHEMA_Module(void);

#endif /* __UNITTEST_SCHEMA_H__ */
