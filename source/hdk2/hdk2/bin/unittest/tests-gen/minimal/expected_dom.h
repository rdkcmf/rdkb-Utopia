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

#ifndef __ACTUAL_DOM_H__
#define __ACTUAL_DOM_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_DOM_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_DOM_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_DOM_Element
{
    ACTUAL_DOM_Element_Cisco_CiscoAction = 0,
    ACTUAL_DOM_Element_Cisco_CiscoActionResponse = 1,
    ACTUAL_DOM_Element_Cisco_CiscoActionResult = 2,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 3,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 4,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 5
} ACTUAL_DOM_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_DOM_EnumType
{
    ACTUAL_DOM_EnumType_Cisco_CiscoActionResult = -1
} ACTUAL_DOM_EnumType;


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoActionResult
 */

typedef enum _ACTUAL_DOM_Enum_Cisco_CiscoActionResult
{
    ACTUAL_DOM_Enum_Cisco_CiscoActionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK = 0,
    ACTUAL_DOM_Enum_Cisco_CiscoActionResult_ERROR = 1
} ACTUAL_DOM_Enum_Cisco_CiscoActionResult;

#define ACTUAL_DOM_Set_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_DOM_Append_Cisco_CiscoActionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_DOM_Get_Cisco_CiscoActionResult(pStruct, element) (ACTUAL_DOM_Enum_Cisco_CiscoActionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult)
#define ACTUAL_DOM_GetEx_Cisco_CiscoActionResult(pStruct, element, value) (ACTUAL_DOM_Enum_Cisco_CiscoActionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult, 0 ? ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK : (value))
#define ACTUAL_DOM_GetMember_Cisco_CiscoActionResult(pMember) (ACTUAL_DOM_Enum_Cisco_CiscoActionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_DOM_MethodEnum
{
    ACTUAL_DOM_MethodEnum_Cisco_CiscoAction = 0
} ACTUAL_DOM_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_DOM_METHOD_CISCO_CISCOACTION__


/*
 * Methods
 */

extern void ACTUAL_DOM_Method_Cisco_CiscoAction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * Module
 */

ACTUAL_DOM_EXPORT const HDK_MOD_Module* ACTUAL_DOM_Module(void);

/* Dynamic server module export */
ACTUAL_DOM_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_DOM_H__ */
