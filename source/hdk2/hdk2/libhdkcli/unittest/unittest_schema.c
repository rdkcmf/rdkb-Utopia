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

#include "unittest_schema.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/Unittest/",
    /* 1 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolArg0 = 0 */ { 0, "BoolArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolInputArg = 1 */ { 0, "BoolInputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember = 2 */ { 0, "BoolMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethod = 3 */ { 0, "ComplexUnittestMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResponse = 4 */ { 0, "ComplexUnittestMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResult = 5 */ { 0, "ComplexUnittestMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember = 6 */ { 0, "DateTimeMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember = 7 */ { 0, "EnumMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod = 8 */ { 0, "HttpGetMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResponse = 9 */ { 0, "HttpGetMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResult = 10 */ { 0, "HttpGetMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInput = 11 */ { 0, "HttpGetMethod_WithInput" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResponse = 12 */ { 0, "HttpGetMethod_WithInputResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResult = 13 */ { 0, "HttpGetMethod_WithInputResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember = 14 */ { 0, "IPAddressMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_IntArg0 = 15 */ { 0, "IntArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember = 16 */ { 0, "IntMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg = 17 */ { 0, "IntOutputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg0 = 18 */ { 0, "IntOutputArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember = 19 */ { 0, "MACAddressMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethod = 20 */ { 0, "NoAuthMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResponse = 21 */ { 0, "NoAuthMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResult = 22 */ { 0, "NoAuthMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethod = 23 */ { 0, "NoInputOutputPathMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResponse = 24 */ { 0, "NoInputOutputPathMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResult = 25 */ { 0, "NoInputOutputPathMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethod = 26 */ { 0, "NoInputStructMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResponse = 27 */ { 0, "NoInputStructMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResult = 28 */ { 0, "NoInputStructMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStringInputArg = 29 */ { 0, "OptionalStringInputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStructOutputArg = 30 */ { 0, "OptionalStructOutputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethod = 31 */ { 0, "SimpleUnittestMethod" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResponse = 32 */ { 0, "SimpleUnittestMethodResponse" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResult = 33 */ { 0, "SimpleUnittestMethodResult" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArg0 = 34 */ { 0, "StringArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember = 35 */ { 0, "StringArrayMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StringInputArg = 36 */ { 0, "StringInputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember = 37 */ { 0, "StringMember" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StringOutArg = 38 */ { 0, "StringOutArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StructArg0 = 39 */ { 0, "StructArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StructInputArg = 40 */ { 0, "StructInputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg = 41 */ { 0, "StructOutputArg" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg0 = 42 */ { 0, "StructOutputArg0" },
    /* UNITTEST_SCHEMA_Element_Cisco_Unittest_string = 43 */ { 0, "string" },
    /* UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body = 44 */ { 1, "Body" },
    /* UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 45 */ { 1, "Envelope" },
    /* UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header = 46 */ { 1, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration http://cisco.com/Unittest/ComplexUnittestMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_ComplexUnittestMethodResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_Error1",
    "ERROR_Error2",
    "ERROR_Error3",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/HttpGetMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_HttpGetMethodResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/HttpGetMethod_WithInputResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_HttpGetMethod_WithInputResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/NoAuthMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_NoAuthMethodResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/NoInputOutputPathMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_NoInputOutputPathMethodResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/NoInputStructMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_NoInputStructMethodResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/SimpleUnittestMethodResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_SimpleUnittestMethodResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/Unittest/UnittestEnum
 */

static const HDK_XML_EnumValue s_enum_Cisco_Unittest_UnittestEnum[] =
{
    "EnumValue0",
    "EnumValue1",
    "EnumValue2",
    "EnumValue3",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_Cisco_Unittest_ComplexUnittestMethodResult,
    s_enum_Cisco_Unittest_HttpGetMethodResult,
    s_enum_Cisco_Unittest_HttpGetMethod_WithInputResult,
    s_enum_Cisco_Unittest_NoAuthMethodResult,
    s_enum_Cisco_Unittest_NoInputOutputPathMethodResult,
    s_enum_Cisco_Unittest_NoInputStructMethodResult,
    s_enum_Cisco_Unittest_SimpleUnittestMethodResult,
    s_enum_Cisco_Unittest_UnittestEnum
};


/*
 * Method http://cisco.com/Unittest/ComplexUnittestMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_ComplexUnittestMethod_Input[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethod, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StructInputArg, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringInputArg, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolInputArg, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStringInputArg, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 8 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember, HDK_XML_BuiltinType_Bool, 0 },
    /* 11 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember, HDK_XML_BuiltinType_DateTime, 0 },
    /* 12 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 13 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 14 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 },
    /* 15 */ { 4, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember, HDK_XML_BuiltinType_Struct, 0 },
    /* 16 */ { 15, UNITTEST_SCHEMA_Element_Cisco_Unittest_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_ComplexUnittestMethod_Input[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethod,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_ComplexUnittestMethod_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_ComplexUnittestMethod_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_ComplexUnittestMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringOutArg, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_OptionalStructOutputArg, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 8 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember, HDK_XML_BuiltinType_Bool, 0 },
    /* 11 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember, HDK_XML_BuiltinType_DateTime, 0 },
    /* 12 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 13 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 14 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 },
    /* 15 */ { 7, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember, HDK_XML_BuiltinType_Struct, 0 },
    /* 16 */ { 15, UNITTEST_SCHEMA_Element_Cisco_Unittest_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_ComplexUnittestMethod_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_ComplexUnittestMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_ComplexUnittestMethod_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/Unittest/HttpGetMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_HttpGetMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg0, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_HttpGetMethod_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_HttpGetMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_HttpGetMethod_Output,
    s_enumTypes
};

/*
 * Method http://cisco.com/Unittest/HttpGetMethod-WithInput
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_HttpGetMethod_WithInput_Input[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInput, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntArg0, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_HttpGetMethod_WithInput_Input[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInput,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_HttpGetMethod_WithInput_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_HttpGetMethod_WithInput_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_HttpGetMethod_WithInput_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntOutputArg0, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_HttpGetMethod_WithInput_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_HttpGetMethod_WithInput_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_HttpGetMethod_WithInput_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/Unittest/NoAuthMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_NoAuthMethod_Input[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethod, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArg0, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_NoAuthMethod_Input[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethod,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_NoAuthMethod_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_NoAuthMethod_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_NoAuthMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_NoAuthMethod_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_NoAuthMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_NoAuthMethod_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/Unittest/NoInputOutputPathMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_NoInputOutputPathMethod_Input[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethod, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArg0, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_StructArg0, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntArg0, HDK_XML_BuiltinType_Int, 0 },
    /* 4 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember, HDK_XML_BuiltinType_Int, 0 },
    /* 6 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember, HDK_XML_BuiltinType_DateTime, 0 },
    /* 8 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 9 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 10 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 },
    /* 11 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember, HDK_XML_BuiltinType_Struct, 0 },
    /* 12 */ { 11, UNITTEST_SCHEMA_Element_Cisco_Unittest_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_NoInputOutputPathMethod_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_NoInputOutputPathMethod_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_NoInputOutputPathMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg0, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember, HDK_XML_BuiltinType_String, 0 },
    /* 4 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember, HDK_XML_BuiltinType_DateTime, 0 },
    /* 7 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 8 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 9 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 },
    /* 10 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 10, UNITTEST_SCHEMA_Element_Cisco_Unittest_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_NoInputOutputPathMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_NoInputOutputPathMethod_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/Unittest/NoInputStructMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_NoInputStructMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StructOutputArg, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringMember, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntMember, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolMember, HDK_XML_BuiltinType_Bool, 0 },
    /* 9 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_DateTimeMember, HDK_XML_BuiltinType_DateTime, 0 },
    /* 10 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_IPAddressMember, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 11 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_MACAddressMember, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 12 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_EnumMember, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_UnittestEnum, 0 },
    /* 13 */ { 5, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArrayMember, HDK_XML_BuiltinType_Struct, 0 },
    /* 14 */ { 13, UNITTEST_SCHEMA_Element_Cisco_Unittest_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_NoInputStructMethod_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_NoInputStructMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_NoInputStructMethod_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/Unittest/SimpleUnittestMethod
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_SimpleUnittestMethod_Input[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethod, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_StringArg0, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_BoolArg0, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_IntArg0, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_SimpleUnittestMethod_Input[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethod,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_SimpleUnittestMethod_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_SimpleUnittestMethod_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Unittest_SimpleUnittestMethod_Output[] =
{
    /* 0 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResult, UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Unittest_SimpleUnittestMethod_Output[] =
{
    UNITTEST_SCHEMA_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Unittest_SimpleUnittestMethod_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Unittest_SimpleUnittestMethod_Output,
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
        "http://cisco.com/Unittest/ComplexUnittestMethod",
        0,
        &s_schema_Cisco_Unittest_ComplexUnittestMethod_Input,
        &s_schema_Cisco_Unittest_ComplexUnittestMethod_Output,
        s_elementPath_Cisco_Unittest_ComplexUnittestMethod_Input,
        s_elementPath_Cisco_Unittest_ComplexUnittestMethod_Output,
        0,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_ComplexUnittestMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_ComplexUnittestMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_ComplexUnittestMethodResult_REBOOT
    },
    {
        "GET",
        "/HNAP1",
        "http://cisco.com/Unittest/HttpGetMethod",
        0,
        0,
        &s_schema_Cisco_Unittest_HttpGetMethod_Output,
        0,
        s_elementPath_Cisco_Unittest_HttpGetMethod_Output,
        HDK_MOD_MethodOption_NoInputStruct,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethodResult_OK
    },
    {
        "GET",
        "/HNAP1",
        "http://cisco.com/Unittest/HttpGetMethod-WithInput",
        0,
        &s_schema_Cisco_Unittest_HttpGetMethod_WithInput_Input,
        &s_schema_Cisco_Unittest_HttpGetMethod_WithInput_Output,
        s_elementPath_Cisco_Unittest_HttpGetMethod_WithInput_Input,
        s_elementPath_Cisco_Unittest_HttpGetMethod_WithInput_Output,
        0,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_HttpGetMethod_WithInputResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_HttpGetMethod_WithInputResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_HttpGetMethod_WithInputResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/Unittest/NoAuthMethod",
        0,
        &s_schema_Cisco_Unittest_NoAuthMethod_Input,
        &s_schema_Cisco_Unittest_NoAuthMethod_Output,
        s_elementPath_Cisco_Unittest_NoAuthMethod_Input,
        s_elementPath_Cisco_Unittest_NoAuthMethod_Output,
        HDK_MOD_MethodOption_NoBasicAuth,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_NoAuthMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoAuthMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoAuthMethodResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/Unittest/NoInputOutputPathMethod",
        0,
        &s_schema_Cisco_Unittest_NoInputOutputPathMethod_Input,
        &s_schema_Cisco_Unittest_NoInputOutputPathMethod_Output,
        0,
        0,
        0,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputOutputPathMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputOutputPathMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputOutputPathMethodResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/Unittest/NoInputStructMethod",
        0,
        0,
        &s_schema_Cisco_Unittest_NoInputStructMethod_Output,
        0,
        s_elementPath_Cisco_Unittest_NoInputStructMethod_Output,
        HDK_MOD_MethodOption_NoInputStruct,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_NoInputStructMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_NoInputStructMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_NoInputStructMethodResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/Unittest/SimpleUnittestMethod",
        0,
        &s_schema_Cisco_Unittest_SimpleUnittestMethod_Input,
        &s_schema_Cisco_Unittest_SimpleUnittestMethod_Output,
        s_elementPath_Cisco_Unittest_SimpleUnittestMethod_Input,
        s_elementPath_Cisco_Unittest_SimpleUnittestMethod_Output,
        0,
        UNITTEST_SCHEMA_Element_Cisco_Unittest_SimpleUnittestMethodResult,
        UNITTEST_SCHEMA_EnumType_Cisco_Unittest_SimpleUnittestMethodResult,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK,
        UNITTEST_SCHEMA_Enum_Cisco_Unittest_SimpleUnittestMethodResult_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Events
 */

static const HDK_MOD_Event s_events[] =
{
    HDK_MOD_EventsEnd
};


/*
 * Services
 */

static const HDK_MOD_Service s_services[] =
{
    HDK_MOD_ServicesEnd
};


/*
 * Module
 */

static const HDK_MOD_Module s_module =
{
    0,
    "Module",
    s_services,
    s_methods,
    s_events,
    0
};

const HDK_MOD_Module* UNITTEST_SCHEMA_Module(void)
{
    return &s_module;
}
