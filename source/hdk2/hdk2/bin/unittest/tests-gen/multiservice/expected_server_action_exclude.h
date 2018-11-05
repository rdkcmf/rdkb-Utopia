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

#ifndef __ACTUAL_SERVER_ACTION_EXCLUDE_H__
#define __ACTUAL_SERVER_ACTION_EXCLUDE_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_SERVER_ACTION_EXCLUDE_BUILD
#      define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_SERVER_ACTION_EXCLUDE_BUILD
#      define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_Element
{
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2 = 0,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Response = 1,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Result = 2,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoEvent = 3,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoStruct = 4,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_a = 5,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_as = 6,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_b = 7,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bool = 8,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bs = 9,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_c = 10,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_cs = 11,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_extra = 12,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_in = 13,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_int = 14,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_out = 15,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_someFlag = 16,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string = 17,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_stuff = 18,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_x = 19,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body = 20,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 21,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Header = 22
} ACTUAL_SERVER_ACTION_EXCLUDE_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_EnumType
{
    ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result = -1,
    ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum = -2
} ACTUAL_SERVER_ACTION_EXCLUDE_EnumType;


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoAction2Result
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result
{
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK = 0,
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_ERROR = 1
} ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result;

#define ACTUAL_SERVER_ACTION_EXCLUDE_Set_Cisco_CiscoAction2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_Append_Cisco_CiscoAction2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_Get_Cisco_CiscoAction2Result(pStruct, element) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result)
#define ACTUAL_SERVER_ACTION_EXCLUDE_GetEx_Cisco_CiscoAction2Result(pStruct, element, value) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_GetMember_Cisco_CiscoAction2Result(pMember) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result)


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoEnum
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum
{
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value1 = 0,
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value2 = 1,
    ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value3 = 2
} ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum;

#define ACTUAL_SERVER_ACTION_EXCLUDE_Set_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_Append_Cisco_CiscoEnum(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_Get_Cisco_CiscoEnum(pStruct, element) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum)
#define ACTUAL_SERVER_ACTION_EXCLUDE_GetEx_Cisco_CiscoEnum(pStruct, element, value) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 ? ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum_Value1 : (value))
#define ACTUAL_SERVER_ACTION_EXCLUDE_GetMember_Cisco_CiscoEnum(pMember) (ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoEnum*)HDK_XML_GetMember_Enum(pMember, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_MethodEnum
{
    ACTUAL_SERVER_ACTION_EXCLUDE_MethodEnum_Cisco_CiscoAction2 = 0
} ACTUAL_SERVER_ACTION_EXCLUDE_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_SERVER_ACTION_EXCLUDE_METHOD_CISCO_CISCOACTION2__


/*
 * Methods
 */

extern void ACTUAL_SERVER_ACTION_EXCLUDE_Method_Cisco_CiscoAction2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * Event enumeration
 */

typedef enum _ACTUAL_SERVER_ACTION_EXCLUDE_EventEnum
{
    ACTUAL_SERVER_ACTION_EXCLUDE_EventEnum_Cisco_CiscoEvent = 0
} ACTUAL_SERVER_ACTION_EXCLUDE_EventEnum;


/*
 * Module
 */

ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT const HDK_MOD_Module* ACTUAL_SERVER_ACTION_EXCLUDE_Module(void);

/* Dynamic server module export */
ACTUAL_SERVER_ACTION_EXCLUDE_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_SERVER_ACTION_EXCLUDE_H__ */
