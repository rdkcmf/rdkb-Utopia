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

#include "actual.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAPExt",
    /* 1 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_Element_Cisco_MyAction = 0 */ { 0, "MyAction" },
    /* ACTUAL_Element_Cisco_MyActionResponse = 1 */ { 0, "MyActionResponse" },
    /* ACTUAL_Element_Cisco_MyActionResult = 2 */ { 0, "MyActionResult" },
    /* ACTUAL_Element_Cisco_MyStruct = 3 */ { 0, "MyStruct" },
    /* ACTUAL_Element_Cisco_a = 4 */ { 0, "a" },
    /* ACTUAL_Element_Cisco_b = 5 */ { 0, "b" },
    /* ACTUAL_Element_Cisco_c = 6 */ { 0, "c" },
    /* ACTUAL_Element_Cisco_d = 7 */ { 0, "d" },
    /* ACTUAL_Element_Cisco_e = 8 */ { 0, "e" },
    /* ACTUAL_Element_Cisco_f = 9 */ { 0, "f" },
    /* ACTUAL_Element_Cisco_g = 10 */ { 0, "g" },
    /* ACTUAL_Element_Cisco_h = 11 */ { 0, "h" },
    /* ACTUAL_Element_Cisco_i = 12 */ { 0, "i" },
    /* ACTUAL_Element_Cisco_int = 13 */ { 0, "int" },
    /* ACTUAL_Element_Cisco_j = 14 */ { 0, "j" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body = 15 */ { 1, "Body" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 16 */ { 1, "Envelope" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header = 17 */ { 1, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/MyActionResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_MyActionResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_Cisco_MyActionResult
};


/*
 * Method http://cisco.com/HNAPExt/MyAction
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_MyAction_Input[] =
{
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_MyAction, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 3, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_MyAction_Input[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_MyAction,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_MyAction_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_MyAction_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_MyAction_Output[] =
{
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_MyActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_Element_Cisco_MyActionResult, ACTUAL_EnumType_Cisco_MyActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_Element_Cisco_c, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 3, ACTUAL_Element_Cisco_d, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 3, ACTUAL_Element_Cisco_e, HDK_XML_BuiltinType_Struct, 0 },
    /* 8 */ { 3, ACTUAL_Element_Cisco_f, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 3, ACTUAL_Element_Cisco_g, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 10 */ { 3, ACTUAL_Element_Cisco_h, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 11 */ { 3, ACTUAL_Element_Cisco_i, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 12 */ { 3, ACTUAL_Element_Cisco_j, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 13 */ { 6, ACTUAL_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 7, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 15 */ { 7, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_Int, 0 },
    /* 16 */ { 8, ACTUAL_Element_Cisco_MyStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 17 */ { 10, ACTUAL_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 18 */ { 11, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 19 */ { 11, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 20 */ { 12, ACTUAL_Element_Cisco_MyStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 21 */ { 16, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 22 */ { 16, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_Int, 0 },
    /* 23 */ { 20, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 24 */ { 20, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_MyAction_Output[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_MyActionResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_MyAction_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_MyAction_Output,
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
        "http://cisco.com/HNAPExt/MyAction",
        ACTUAL_Method_Cisco_MyAction,
        &s_schema_Cisco_MyAction_Input,
        &s_schema_Cisco_MyAction_Output,
        s_elementPath_Cisco_MyAction_Input,
        s_elementPath_Cisco_MyAction_Output,
        0,
        ACTUAL_Element_Cisco_MyActionResult,
        ACTUAL_EnumType_Cisco_MyActionResult,
        ACTUAL_Enum_Cisco_MyActionResult_OK,
        ACTUAL_Enum_Cisco_MyActionResult_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Module
 */

static const HDK_MOD_Module s_module =
{
    0,
    0,
    0,
    s_methods,
    0,
    0
};

const HDK_MOD_Module* ACTUAL_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
