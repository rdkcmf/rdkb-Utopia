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

#include "actual_server_action_exclude.h"

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
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2 = 0 */ { 0, "CiscoAction2" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Response = 1 */ { 0, "CiscoAction2Response" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Result = 2 */ { 0, "CiscoAction2Result" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoEvent = 3 */ { 0, "CiscoEvent" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoStruct = 4 */ { 0, "CiscoStruct" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_a = 5 */ { 0, "a" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_as = 6 */ { 0, "as" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_b = 7 */ { 0, "b" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bool = 8 */ { 0, "bool" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bs = 9 */ { 0, "bs" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_c = 10 */ { 0, "c" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_cs = 11 */ { 0, "cs" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_extra = 12 */ { 0, "extra" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_in = 13 */ { 0, "in" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_int = 14 */ { 0, "int" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_out = 15 */ { 0, "out" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_someFlag = 16 */ { 0, "someFlag" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string = 17 */ { 0, "string" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_stuff = 18 */ { 0, "stuff" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_x = 19 */ { 0, "x" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body = 20 */ { 1, "Body" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 21 */ { 1, "Envelope" },
    /* ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Header = 22 */ { 1, "Header" },
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
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_Cisco_CiscoAction2Result,
    s_enum_Cisco_CiscoEnum
};


/*
 * Method http://cisco.com/HNAPExt/CiscoAction2
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoAction2_Input[] =
{
    /* 0 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_in, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_x, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 4, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_c, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 8, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 12, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction2_Input[] =
{
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2,
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
    /* 0 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Result, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_out, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_extra, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_c, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 },
    /* 12 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 6, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bool, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 8, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 10, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 16 */ { 12, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_CiscoAction2_Output[] =
{
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Response,
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
 * Methods
 */

static const HDK_MOD_Method s_methods[] =
{
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/CiscoAction2",
        ACTUAL_SERVER_ACTION_EXCLUDE_Method_Cisco_CiscoAction2,
        &s_schema_Cisco_CiscoAction2_Input,
        &s_schema_Cisco_CiscoAction2_Output,
        s_elementPath_Cisco_CiscoAction2_Input,
        s_elementPath_Cisco_CiscoAction2_Output,
        0,
        ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoAction2Result,
        ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoAction2Result,
        ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK,
        ACTUAL_SERVER_ACTION_EXCLUDE_Enum_Cisco_CiscoAction2Result_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Event http://cisco.com/HNAPExt/CiscoEvent
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoEvent[] =
{
    /* 0 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoEvent, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_stuff, HDK_XML_BuiltinType_Struct, 0 },
    /* 2 */ { 0, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_someFlag, HDK_XML_BuiltinType_Bool, 0 },
    /* 3 */ { 1, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 4 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_as, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_bs, HDK_XML_BuiltinType_Struct, 0 },
    /* 8 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_c, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, 0 },
    /* 9 */ { 3, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_cs, HDK_XML_BuiltinType_Struct, 0 },
    /* 10 */ { 5, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 11 */ { 7, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 9, ACTUAL_SERVER_ACTION_EXCLUDE_Element_Cisco_string, ACTUAL_SERVER_ACTION_EXCLUDE_EnumType_Cisco_CiscoEnum, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
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
    &s_methods[ACTUAL_SERVER_ACTION_EXCLUDE_MethodEnum_Cisco_CiscoAction2],
    0
};


/*
 * Service Events
 */

static const HDK_MOD_Event* s_service_Cisco_CiscoService2_Events[] =
{
    &s_events[ACTUAL_SERVER_ACTION_EXCLUDE_EventEnum_Cisco_CiscoEvent],
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
    HDK_MOD_ServicesEnd
};


/*
 * Module
 */

static const HDK_MOD_Module s_module =
{
    0,
    0,
    s_services,
    s_methods,
    s_events,
    0
};

const HDK_MOD_Module* ACTUAL_SERVER_ACTION_EXCLUDE_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
