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

#include "actual_dom.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAPExt/",
    /* 1 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_DOM_Element_Cisco_CiscoAction = 0 */ { 0, "CiscoAction" },
    /* ACTUAL_DOM_Element_Cisco_CiscoActionResponse = 1 */ { 0, "CiscoActionResponse" },
    /* ACTUAL_DOM_Element_Cisco_CiscoActionResult = 2 */ { 0, "CiscoActionResult" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 3 */ { 1, "Body" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 4 */ { 1, "Envelope" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 5 */ { 1, "Header" },
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
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_Cisco_CiscoActionResult
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

const HDK_MOD_Module* ACTUAL_DOM_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
