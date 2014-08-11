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

#include "actual_dom.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAPExt/",
    /* 1 */ "http://cisco.com/HNAPExt/A/",
    /* 2 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_DOM_Element_Cisco_CiscoAction = 0 */ { 0, "CiscoAction" },
    /* ACTUAL_DOM_Element_Cisco_CiscoAction2 = 1 */ { 0, "CiscoAction2" },
    /* ACTUAL_DOM_Element_Cisco_CiscoAction2Response = 2 */ { 0, "CiscoAction2Response" },
    /* ACTUAL_DOM_Element_Cisco_CiscoAction2Result = 3 */ { 0, "CiscoAction2Result" },
    /* ACTUAL_DOM_Element_Cisco_CiscoActionResponse = 4 */ { 0, "CiscoActionResponse" },
    /* ACTUAL_DOM_Element_Cisco_CiscoActionResult = 5 */ { 0, "CiscoActionResult" },
    /* ACTUAL_DOM_Element_Cisco_CiscoEvent = 6 */ { 0, "CiscoEvent" },
    /* ACTUAL_DOM_Element_Cisco_CiscoStruct = 7 */ { 0, "CiscoStruct" },
    /* ACTUAL_DOM_Element_Cisco_a = 8 */ { 0, "a" },
    /* ACTUAL_DOM_Element_Cisco_as = 9 */ { 0, "as" },
    /* ACTUAL_DOM_Element_Cisco_b = 10 */ { 0, "b" },
    /* ACTUAL_DOM_Element_Cisco_bool = 11 */ { 0, "bool" },
    /* ACTUAL_DOM_Element_Cisco_bs = 12 */ { 0, "bs" },
    /* ACTUAL_DOM_Element_Cisco_c = 13 */ { 0, "c" },
    /* ACTUAL_DOM_Element_Cisco_cs = 14 */ { 0, "cs" },
    /* ACTUAL_DOM_Element_Cisco_extra = 15 */ { 0, "extra" },
    /* ACTUAL_DOM_Element_Cisco_in = 16 */ { 0, "in" },
    /* ACTUAL_DOM_Element_Cisco_int = 17 */ { 0, "int" },
    /* ACTUAL_DOM_Element_Cisco_out = 18 */ { 0, "out" },
    /* ACTUAL_DOM_Element_Cisco_someFlag = 19 */ { 0, "someFlag" },
    /* ACTUAL_DOM_Element_Cisco_string = 20 */ { 0, "string" },
    /* ACTUAL_DOM_Element_Cisco_stuff = 21 */ { 0, "stuff" },
    /* ACTUAL_DOM_Element_Cisco_x = 22 */ { 0, "x" },
    /* ACTUAL_DOM_Element_Cisco_A_CiscoAction = 23 */ { 1, "CiscoAction" },
    /* ACTUAL_DOM_Element_Cisco_A_CiscoActionResponse = 24 */ { 1, "CiscoActionResponse" },
    /* ACTUAL_DOM_Element_Cisco_A_CiscoActionResult = 25 */ { 1, "CiscoActionResult" },
    /* ACTUAL_DOM_Element_Cisco_A_CiscoStruct = 26 */ { 1, "CiscoStruct" },
    /* ACTUAL_DOM_Element_Cisco_A_a = 27 */ { 1, "a" },
    /* ACTUAL_DOM_Element_Cisco_A_as = 28 */ { 1, "as" },
    /* ACTUAL_DOM_Element_Cisco_A_b = 29 */ { 1, "b" },
    /* ACTUAL_DOM_Element_Cisco_A_bs = 30 */ { 1, "bs" },
    /* ACTUAL_DOM_Element_Cisco_A_c = 31 */ { 1, "c" },
    /* ACTUAL_DOM_Element_Cisco_A_cs = 32 */ { 1, "cs" },
    /* ACTUAL_DOM_Element_Cisco_A_int = 33 */ { 1, "int" },
    /* ACTUAL_DOM_Element_Cisco_A_string = 34 */ { 1, "string" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 35 */ { 2, "Body" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 36 */ { 2, "Envelope" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 37 */ { 2, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoAction2Result
 */

static const HDK_XML_EnumValue s_enum_Cisco_CiscoAction2Result[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoActionResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_CiscoActionResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/CiscoEnum
 */

static const HDK_XML_EnumValue s_enum_Cisco_CiscoEnum[] =
{
    "Value1",
    "Value2",
    "Value3",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoActionResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_A_CiscoActionResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/A/CiscoEnum
 */

static const HDK_XML_EnumValue s_enum_Cisco_A_CiscoEnum[] =
{
    "Value1",
    "Value2",
    "Value3",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_Cisco_CiscoAction2Result,
    s_enum_Cisco_CiscoActionResult,
    s_enum_Cisco_CiscoEnum,
    s_enum_Cisco_A_CiscoActionResult,
    s_enum_Cisco_A_CiscoEnum
};


/*
 * Method http://cisco.com/HNAPExt/CiscoAction
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoAction_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_CiscoAction, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 4, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 4, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 4, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 4, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 4, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 10 */ { 4, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 8, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 13 */ { 10, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_CiscoAction,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoAction_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoAction_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoAction_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_CiscoActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_CiscoActionResult, ACTUAL_DOM_EnumType_Cisco_CiscoActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_CiscoActionResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoAction_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoAction_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAPExt/CiscoAction2
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoAction2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_CiscoAction2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_in, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_Cisco_x, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 4, ACTUAL_DOM_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_CiscoAction2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoAction2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoAction2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoAction2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_CiscoAction2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_CiscoAction2Result, ACTUAL_DOM_EnumType_Cisco_CiscoAction2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_Cisco_out, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_Cisco_extra, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 5, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 5, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 5, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 5, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 5, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 5, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 6, ACTUAL_DOM_Element_Cisco_bool, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 8, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 10, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 16 */ { 12, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_CiscoAction2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoAction2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoAction2_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAPExt/A/CiscoAction
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_A_CiscoAction_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_A_CiscoAction, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_A_a, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 4, ACTUAL_DOM_Element_Cisco_A_a, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 4, ACTUAL_DOM_Element_Cisco_A_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 4, ACTUAL_DOM_Element_Cisco_A_b, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 4, ACTUAL_DOM_Element_Cisco_A_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 4, ACTUAL_DOM_Element_Cisco_A_c, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, 0 },
    /* 10 */ { 4, ACTUAL_DOM_Element_Cisco_A_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_Cisco_A_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 8, ACTUAL_DOM_Element_Cisco_A_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 13 */ { 10, ACTUAL_DOM_Element_Cisco_A_string, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_A_CiscoAction_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_A_CiscoAction,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_A_CiscoAction_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_A_CiscoAction_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_A_CiscoAction_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_A_CiscoActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_A_CiscoActionResult, ACTUAL_DOM_EnumType_Cisco_A_CiscoActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_Cisco_A_b, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_Cisco_A_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_Cisco_A_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_Cisco_A_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_Cisco_A_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_Cisco_A_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_Cisco_A_c, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_DOM_Element_Cisco_A_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_DOM_Element_Cisco_A_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_DOM_Element_Cisco_A_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_DOM_Element_Cisco_A_string, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_A_CiscoAction_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_A_CiscoActionResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_A_CiscoAction_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_A_CiscoAction_Output,
    s_enumTypes
};


/*
 * Methods
 */

static const HDK_MOD_Method s_methods[] =
{
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/CiscoAction",
        ACTUAL_DOM_Method_Cisco_CiscoAction,
        &s_schema_Cisco_CiscoAction_Input,
        &s_schema_Cisco_CiscoAction_Output,
        s_elementPath_Cisco_CiscoAction_Input,
        s_elementPath_Cisco_CiscoAction_Output,
        0,
        ACTUAL_DOM_Element_Cisco_CiscoActionResult,
        ACTUAL_DOM_EnumType_Cisco_CiscoActionResult,
        ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK,
        ACTUAL_DOM_Enum_Cisco_CiscoActionResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/CiscoAction2",
        ACTUAL_DOM_Method_Cisco_CiscoAction2,
        &s_schema_Cisco_CiscoAction2_Input,
        &s_schema_Cisco_CiscoAction2_Output,
        s_elementPath_Cisco_CiscoAction2_Input,
        s_elementPath_Cisco_CiscoAction2_Output,
        0,
        ACTUAL_DOM_Element_Cisco_CiscoAction2Result,
        ACTUAL_DOM_EnumType_Cisco_CiscoAction2Result,
        ACTUAL_DOM_Enum_Cisco_CiscoAction2Result_OK,
        ACTUAL_DOM_Enum_Cisco_CiscoAction2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/A/CiscoAction",
        ACTUAL_DOM_Method_Cisco_A_CiscoAction,
        &s_schema_Cisco_A_CiscoAction_Input,
        &s_schema_Cisco_A_CiscoAction_Output,
        s_elementPath_Cisco_A_CiscoAction_Input,
        s_elementPath_Cisco_A_CiscoAction_Output,
        0,
        ACTUAL_DOM_Element_Cisco_A_CiscoActionResult,
        ACTUAL_DOM_EnumType_Cisco_A_CiscoActionResult,
        ACTUAL_DOM_Enum_Cisco_A_CiscoActionResult_OK,
        ACTUAL_DOM_Enum_Cisco_A_CiscoActionResult_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Event http://cisco.com/HNAPExt/CiscoEvent
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoEvent[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Cisco_CiscoEvent, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_Cisco_stuff, HDK_XML_BuiltinType_Struct, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_Cisco_someFlag, HDK_XML_BuiltinType_Bool, 0 },
    /* 3 */ { 1, ACTUAL_DOM_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 10 */ { 5, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 11 */ { 7, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 9, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoEvent =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoEvent,
    s_enumTypes
};


/*
 * Events
 */

static const HDK_MOD_Event s_events[] =
{
    {
        "http://cisco.com/HNAPExt/CiscoEvent",
        &s_schema_Cisco_CiscoEvent
    },
    HDK_MOD_EventsEnd
};


/*
 * Service Methods
 */

static const HDK_MOD_Method* s_service_Cisco_CiscoService2_Methods[] =
{
    &s_methods[ACTUAL_DOM_MethodEnum_Cisco_CiscoAction2],
    0
};
static const HDK_MOD_Method* s_service_Cisco_CiscoServiceOne_Methods[] =
{
    &s_methods[ACTUAL_DOM_MethodEnum_Cisco_CiscoAction],
    0
};


/*
 * Service Events
 */

static const HDK_MOD_Event* s_service_Cisco_CiscoService2_Events[] =
{
    &s_events[ACTUAL_DOM_EventEnum_Cisco_CiscoEvent],
    0
};
static const HDK_MOD_Event* s_service_Cisco_CiscoServiceOne_Events[] =
{
    0
};


/*
 * Services
 */

static const HDK_MOD_Service s_services[] =
{
    {
        "http://cisco.com/HNAPExt/CiscoService2",
        s_service_Cisco_CiscoService2_Methods,
        s_service_Cisco_CiscoService2_Events
    },
    {
        "http://cisco.com/HNAPExt/CiscoServiceOne",
        s_service_Cisco_CiscoServiceOne_Methods,
        s_service_Cisco_CiscoServiceOne_Events
    },
    HDK_MOD_ServicesEnd
};


/*
 * Struct http://cisco.com/HNAPExt/CiscoStruct
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoStruct[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_Cisco_c, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, 0 },
    /* 6 */ { 0, ACTUAL_DOM_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 2, ACTUAL_DOM_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 8 */ { 4, ACTUAL_DOM_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 9 */ { 6, ACTUAL_DOM_Element_Cisco_string, ACTUAL_DOM_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoStruct =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoStruct,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_CiscoStruct()
{
    return &s_schema_Cisco_CiscoStruct;
}


/*
 * Struct http://cisco.com/HNAPExt/A/CiscoStruct
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_A_CiscoStruct[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Cisco_A_CiscoStruct, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_Cisco_A_a, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_Cisco_A_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_Cisco_A_b, HDK_XML_BuiltinType_String, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_Cisco_A_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_Cisco_A_c, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, 0 },
    /* 6 */ { 0, ACTUAL_DOM_Element_Cisco_A_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 2, ACTUAL_DOM_Element_Cisco_A_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 8 */ { 4, ACTUAL_DOM_Element_Cisco_A_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 9 */ { 6, ACTUAL_DOM_Element_Cisco_A_string, ACTUAL_DOM_EnumType_Cisco_A_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_A_CiscoStruct =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_A_CiscoStruct,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_A_CiscoStruct()
{
    return &s_schema_Cisco_A_CiscoStruct;
}


/*
 * Module
 */

/* 6c4d0bba-0679-46de-a43b-9da35b3ba1e4 */
static const HDK_XML_UUID s_uuid_NOID =
{
    { 0x6c, 0x4d, 0x0b, 0xba, 0x06, 0x79, 0x46, 0xde, 0xa4, 0x3b, 0x9d, 0xa3, 0x5b, 0x3b, 0xa1, 0xe4 }
};

static const HDK_MOD_Module s_module =
{
    &s_uuid_NOID,
    0,
    s_services,
    s_methods,
    s_events,
    0
};

const HDK_MOD_Module* ACTUAL_DOM_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
