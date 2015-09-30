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

#include "actual.h"

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
    /* ACTUAL_Element_Cisco_CiscoAction = 0 */ { 0, "CiscoAction" },
    /* ACTUAL_Element_Cisco_CiscoActionResponse = 1 */ { 0, "CiscoActionResponse" },
    /* ACTUAL_Element_Cisco_CiscoActionResult = 2 */ { 0, "CiscoActionResult" },
    /* ACTUAL_Element_Cisco_CiscoCSVableStruct = 3 */ { 0, "CiscoCSVableStruct" },
    /* ACTUAL_Element_Cisco_CiscoStruct = 4 */ { 0, "CiscoStruct" },
    /* ACTUAL_Element_Cisco_a = 5 */ { 0, "a" },
    /* ACTUAL_Element_Cisco_as = 6 */ { 0, "as" },
    /* ACTUAL_Element_Cisco_b = 7 */ { 0, "b" },
    /* ACTUAL_Element_Cisco_bs = 8 */ { 0, "bs" },
    /* ACTUAL_Element_Cisco_c = 9 */ { 0, "c" },
    /* ACTUAL_Element_Cisco_cs = 10 */ { 0, "cs" },
    /* ACTUAL_Element_Cisco_csvInts = 11 */ { 0, "csvInts" },
    /* ACTUAL_Element_Cisco_csvStruct = 12 */ { 0, "csvStruct" },
    /* ACTUAL_Element_Cisco_csvStructs = 13 */ { 0, "csvStructs" },
    /* ACTUAL_Element_Cisco_int = 14 */ { 0, "int" },
    /* ACTUAL_Element_Cisco_string = 15 */ { 0, "string" },
    /* ACTUAL_Element_Cisco_A_CiscoAction = 16 */ { 1, "CiscoAction" },
    /* ACTUAL_Element_Cisco_A_CiscoActionResponse = 17 */ { 1, "CiscoActionResponse" },
    /* ACTUAL_Element_Cisco_A_CiscoActionResult = 18 */ { 1, "CiscoActionResult" },
    /* ACTUAL_Element_Cisco_A_CiscoStruct = 19 */ { 1, "CiscoStruct" },
    /* ACTUAL_Element_Cisco_A_a = 20 */ { 1, "a" },
    /* ACTUAL_Element_Cisco_A_as = 21 */ { 1, "as" },
    /* ACTUAL_Element_Cisco_A_b = 22 */ { 1, "b" },
    /* ACTUAL_Element_Cisco_A_bs = 23 */ { 1, "bs" },
    /* ACTUAL_Element_Cisco_A_c = 24 */ { 1, "c" },
    /* ACTUAL_Element_Cisco_A_cs = 25 */ { 1, "cs" },
    /* ACTUAL_Element_Cisco_A_int = 26 */ { 1, "int" },
    /* ACTUAL_Element_Cisco_A_string = 27 */ { 1, "string" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body = 28 */ { 2, "Body" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 29 */ { 2, "Envelope" },
    /* ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header = 30 */ { 2, "Header" },
    HDK_XML_Schema_ElementsEnd
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
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_CiscoAction, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 3, ACTUAL_Element_Cisco_csvInts, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_CSV },
    /* 6 */ { 3, ACTUAL_Element_Cisco_csvStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_CSV },
    /* 7 */ { 3, ACTUAL_Element_Cisco_csvStructs, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_CSV },
    /* 8 */ { 4, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 9 */ { 4, ACTUAL_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 10 */ { 4, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 11 */ { 4, ACTUAL_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 12 */ { 4, ACTUAL_Element_Cisco_c, ACTUAL_EnumType_Cisco_CiscoEnum, 0 },
    /* 13 */ { 4, ACTUAL_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 14 */ { 5, ACTUAL_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 6, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 16 */ { 6, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 17 */ { 6, ACTUAL_Element_Cisco_c, HDK_XML_BuiltinType_DateTime, 0 },
    /* 18 */ { 7, ACTUAL_Element_Cisco_CiscoCSVableStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 19 */ { 9, ACTUAL_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 20 */ { 11, ACTUAL_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 21 */ { 13, ACTUAL_Element_Cisco_string, ACTUAL_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 22 */ { 18, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 23 */ { 18, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 24 */ { 18, ACTUAL_Element_Cisco_c, HDK_XML_BuiltinType_DateTime, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction_Input[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_CiscoAction,
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
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_CiscoActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_Element_Cisco_CiscoActionResult, ACTUAL_EnumType_Cisco_CiscoActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_Element_Cisco_c, ACTUAL_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_Element_Cisco_string, ACTUAL_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction_Output[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_CiscoActionResponse,
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
 * Method http://cisco.com/HNAPExt/A/CiscoAction
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_A_CiscoAction_Input[] =
{
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_A_CiscoAction, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_Element_Cisco_A_a, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 4, ACTUAL_Element_Cisco_A_a, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 4, ACTUAL_Element_Cisco_A_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 4, ACTUAL_Element_Cisco_A_b, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 4, ACTUAL_Element_Cisco_A_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 4, ACTUAL_Element_Cisco_A_c, ACTUAL_EnumType_Cisco_A_CiscoEnum, 0 },
    /* 10 */ { 4, ACTUAL_Element_Cisco_A_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_Element_Cisco_A_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 8, ACTUAL_Element_Cisco_A_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 13 */ { 10, ACTUAL_Element_Cisco_A_string, ACTUAL_EnumType_Cisco_A_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_A_CiscoAction_Input[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_A_CiscoAction,
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
    /* 0 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_Element_Cisco_A_CiscoActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_Element_Cisco_A_CiscoActionResult, ACTUAL_EnumType_Cisco_A_CiscoActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_Element_Cisco_A_b, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_Element_Cisco_A_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_Element_Cisco_A_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_Element_Cisco_A_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_Element_Cisco_A_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_Element_Cisco_A_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_Element_Cisco_A_c, ACTUAL_EnumType_Cisco_A_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_Element_Cisco_A_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_Element_Cisco_A_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_Element_Cisco_A_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_Element_Cisco_A_string, ACTUAL_EnumType_Cisco_A_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_A_CiscoAction_Output[] =
{
    ACTUAL_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_Element_Cisco_A_CiscoActionResponse,
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
        ACTUAL_Method_Cisco_CiscoAction,
        &s_schema_Cisco_CiscoAction_Input,
        &s_schema_Cisco_CiscoAction_Output,
        s_elementPath_Cisco_CiscoAction_Input,
        s_elementPath_Cisco_CiscoAction_Output,
        0,
        ACTUAL_Element_Cisco_CiscoActionResult,
        ACTUAL_EnumType_Cisco_CiscoActionResult,
        ACTUAL_Enum_Cisco_CiscoActionResult_OK,
        ACTUAL_Enum_Cisco_CiscoActionResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/A/CiscoAction",
        ACTUAL_Method_Cisco_A_CiscoAction,
        &s_schema_Cisco_A_CiscoAction_Input,
        &s_schema_Cisco_A_CiscoAction_Output,
        s_elementPath_Cisco_A_CiscoAction_Input,
        s_elementPath_Cisco_A_CiscoAction_Output,
        0,
        ACTUAL_Element_Cisco_A_CiscoActionResult,
        ACTUAL_EnumType_Cisco_A_CiscoActionResult,
        ACTUAL_Enum_Cisco_A_CiscoActionResult_OK,
        ACTUAL_Enum_Cisco_A_CiscoActionResult_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Service Methods
 */

static const HDK_MOD_Method* s_service_Cisco_CiscoService_Methods[] =
{
    &s_methods[ACTUAL_MethodEnum_Cisco_CiscoAction],
    0
};


/*
 * Service Events
 */

static const HDK_MOD_Event* s_service_Cisco_CiscoService_Events[] =
{
    0
};


/*
 * Services
 */

static const HDK_MOD_Service s_services[] =
{
    {
        "http://cisco.com/HNAPExt/CiscoService",
        s_service_Cisco_CiscoService_Methods,
        s_service_Cisco_CiscoService_Events
    },
    HDK_MOD_ServicesEnd
};


/*
 * Module
 */

/* c0785edb-2255-4a38-9302-bb129ffb2f71 */
static const HDK_XML_UUID s_uuid_NOID =
{
    { 0xc0, 0x78, 0x5e, 0xdb, 0x22, 0x55, 0x4a, 0x38, 0x93, 0x02, 0xbb, 0x12, 0x9f, 0xfb, 0x2f, 0x71 }
};

static const HDK_MOD_Module s_module =
{
    &s_uuid_NOID,
    0,
    s_services,
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
