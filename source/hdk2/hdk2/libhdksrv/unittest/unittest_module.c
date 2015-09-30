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

#include "unittest_module.h"

#include <string.h>


/*
 * Elements
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAP1/",
    /* 1 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};

static const HDK_XML_ElementNode s_elements[] =
{
    /* Element_A */ { 0, "A" },
    /* Element_ADI */ { 0, "ADI" },
    /* Element_ADIGet */ { 0, "ADIGet" },
    /* Element_ADIGetResponse */ { 0, "ADIGetResponse" },
    /* Element_ADISet */ { 0, "ADISet" },
    /* Element_ADISetResponse */ { 0, "ADISetResponse" },
    /* Element_Bar */ { 0, "Bar" },
    /* Element_BarResponse */ { 0, "BarResponse" },
    /* Element_BarResult */ { 0, "BarResult" },
    /* Element_Foo */ { 0, "Foo" },
    /* Element_FooResponse */ { 0, "FooResponse" },
    /* Element_FooResult */ { 0, "FooResult" },
    /* Element_Int */ { 0, "Int" },
    /* Element_Struct */ { 0, "Struct" },
    /* HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body */ { 1, "Body" },
    /* HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Envelope */ { 1, "Envelope" },
    /* HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Header */ { 1, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration types (enum)
 */

typedef enum _EnumType
{
    EnumType_BarResult = -1,
    EnumType_FooResult = -2
} EnumType;


/*
 * Enumeration BarResult
 */

static const HDK_XML_EnumValue s_BarResult_Strings[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};

HDK_XML_Member* Set_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value)
{
    return HDK_XML_Set_Enum(pStruct, element, EnumType_BarResult, value);
}

HDK_XML_Member* Append_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value)
{
    return HDK_XML_Append_Enum(pStruct, element, EnumType_BarResult, value);
}

BarResult* Get_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return (BarResult*)HDK_XML_Get_Enum(pStruct, element, EnumType_BarResult);
}

BarResult Get_BarResultEx(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value)
{
    return (BarResult)HDK_XML_GetEx_Enum(pStruct, element, EnumType_BarResult, value);
}

BarResult* Get_BarResultMember(HDK_XML_Member* pMember)
{
    return (BarResult*)HDK_XML_GetMember_Enum(pMember, EnumType_BarResult);
}


/*
 * Enumeration FooResult
 */

static const HDK_XML_EnumValue s_FooResult_Strings[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};

HDK_XML_Member* Set_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value)
{
    return HDK_XML_Set_Enum(pStruct, element, EnumType_FooResult, value);
}

HDK_XML_Member* Append_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value)
{
    return HDK_XML_Append_Enum(pStruct, element, EnumType_FooResult, value);
}

FooResult* Get_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return (FooResult*)HDK_XML_Get_Enum(pStruct, element, EnumType_FooResult);
}

FooResult Get_FooResultEx(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value)
{
    return (FooResult)HDK_XML_GetEx_Enum(pStruct, element, EnumType_FooResult, value);
}

FooResult* Get_FooResultMember(HDK_XML_Member* pMember)
{
    return (FooResult*)HDK_XML_GetMember_Enum(pMember, EnumType_FooResult);
}


/*
 * Enumeration types
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_BarResult_Strings,
    s_FooResult_Strings
};


/*
 * Method Bar
 */

static const HDK_XML_SchemaNode s_schemaNodes_Bar_Input[] =
{
    /* 0 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement },
    /* 1 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, 0 },
    /* 2 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 3 */ { 1, Element_Bar, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, Element_Int, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Bar_Input[] =
{
    HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    Element_Bar,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Bar_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Bar_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Bar_Output[] =
{
    /* 0 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement },
    /* 1 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, 0 },
    /* 2 */ { 0, HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 3 */ { 1, Element_BarResponse, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, Element_BarResult, EnumType_BarResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, Element_Int, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Bar_Output[] =
{
    HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    Element_BarResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Bar_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Bar_Output,
    s_enumTypes
};


/*
 * Method Foo
 */

static const HDK_XML_SchemaNode s_schemaNodes_Foo_Input[] =
{
    /* 0 */ { 0, Element_Foo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, Element_Int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Foo_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Foo_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Foo_Output[] =
{
    /* 0 */ { 0, Element_FooResponse, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, Element_FooResult, EnumType_FooResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 2 */ { 0, Element_Int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Foo_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Foo_Output,
    s_enumTypes
};


/*
 * Method ADIGet
 */

static const HDK_XML_SchemaNode s_schemaNodes_ADIGet_Input[] =
{
    /* 0 */ { 0, Element_ADIGet, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADIGet_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADIGet_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_ADIGet_Output[] =
{
    /* 0 */ { 0, Element_ADIGetResponse, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, Element_Struct, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, Element_Int, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, Element_Int, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADIGet_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADIGet_Output,
    s_enumTypes
};


/*
 * Method ADISet
 */

static const HDK_XML_SchemaNode s_schemaNodes_ADISet_Input[] =
{
    /* 0 */ { 0, Element_ADISet, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, Element_Struct, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, Element_Int, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, Element_Int, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADISet_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADISet_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_ADISet_Output[] =
{
    /* 0 */ { 0, Element_ADISetResponse, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADISet_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADISet_Output,
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
        "http://cisco.com/HNAP1/Bar",
        Method_Bar,
        &s_schema_Bar_Input,
        &s_schema_Bar_Output,
        s_elementPath_Bar_Input,
        s_elementPath_Bar_Output,
        0,
        Element_BarResult,
        EnumType_BarResult,
        BarResult_OK,
        BarResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP1/Foo",
        Method_Foo,
        &s_schema_Foo_Input,
        &s_schema_Foo_Output,
        0,
        0,
        0,
        Element_FooResult,
        EnumType_FooResult,
        FooResult_OK,
        FooResult_REBOOT
    },
    {
        "GET",
        "/HNAP1",
        0,
        Method_Foo,
        &s_schema_Foo_Input,
        &s_schema_Foo_Output,
        0,
        0,
        HDK_MOD_MethodOption_NoBasicAuth | HDK_MOD_MethodOption_NoInputStruct,
        Element_FooResult,
        EnumType_FooResult,
        FooResult_OK,
        FooResult_REBOOT
    },
    {
        "GET",
        "/HNAP1/Foo",
        0,
        Method_Foo,
        &s_schema_Foo_Input,
        &s_schema_Foo_Output,
        0,
        0,
        HDK_MOD_MethodOption_NoLocationSlash | HDK_MOD_MethodOption_NoBasicAuth | HDK_MOD_MethodOption_NoInputStruct,
        Element_FooResult,
        EnumType_FooResult,
        FooResult_OK,
        FooResult_REBOOT
    },
    {
        "POST",
        "/NonHNAP",
        "http://cisco.com/NonHNAP/Foo",
        Method_Foo,
        &s_schema_Foo_Input,
        &s_schema_Foo_Output,
        0,
        0,
        0,
        HDK_XML_BuiltinElement_Unknown,
        0,
        0,
        0
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP1/ADIGet",
        Method_ADIGet,
        &s_schema_ADIGet_Input,
        &s_schema_ADIGet_Output,
        0,
        0,
        0,
        HDK_XML_BuiltinElement_Unknown,
        0,
        0,
        0
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP1/ADISet",
        Method_ADISet,
        &s_schema_ADISet_Input,
        &s_schema_ADISet_Output,
        0,
        0,
        0,
        HDK_XML_BuiltinElement_Unknown,
        0,
        0,
        0
    },
    HDK_MOD_MethodsEnd
};


static const HDK_MOD_Method s_methods2[] =
{
    {
        "POST",
        "/HNAP2",
        "http://cisco.com/HNAP2/Foo",
        Method_Foo,
        &s_schema_Foo_Input,
        &s_schema_Foo_Output,
        0,
        0,
        0,
        Element_FooResult,
        EnumType_FooResult,
        FooResult_OK,
        FooResult_REBOOT
    },
    {
        "POST",
        "/HNAP2",
        "http://cisco.com/HNAP2/ADIGet",
        Method_ADIGet,
        &s_schema_ADIGet_Input,
        &s_schema_ADIGet_Output,
        0,
        0,
        0,
        HDK_XML_BuiltinElement_Unknown,
        0,
        0,
        0
    },
    {
        "POST",
        "/HNAP2",
        "http://cisco.com/HNAP2/ADISet",
        Method_ADISet,
        &s_schema_ADISet_Input,
        &s_schema_ADISet_Output,
        0,
        0,
        0,
        HDK_XML_BuiltinElement_Unknown,
        0,
        0,
        0
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

static const HDK_MOD_Event s_events2[] =
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

static const HDK_MOD_Service s_services2[] =
{
    HDK_MOD_ServicesEnd
};


/*
 * ADI
 */

static const HDK_XML_SchemaNode s_schemaNodes_ADI[] =
{
    /* 0 */ { 0, Element_ADI, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, Element_Int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 2 */ { 0, Element_Struct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 3 */ { 2, Element_Int, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADI =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADI,
    s_enumTypes
};


/*
 * Module
 */
/* 198b0022-070c-4bb9-91da-3e7e01a38944 */
static const HDK_XML_UUID s_uuidNOID1 =
{
    { 0x19,0x8b,0x00,0x22,0x07,0x0c,0x4b,0xb9,0x91,0xda,0x3e,0x7e,0x01,0xa3,0x89,0x44 }
};

static const HDK_MOD_Module s_module =
{
    &s_uuidNOID1,
    "Module",
    s_services,
    s_methods,
    s_events,
    &s_schema_ADI
};

/* c473989b-d162-4fb2-983c-0883149c0416 */
static const HDK_XML_UUID s_uuidNOID2 =
{
    { 0xc4,0x73,0x98,0x9b,0xd1,0x62,0x4f,0xb2,0x98,0x3c,0x08,0x83,0x14,0x9c,0x04,0x16 }
};

static const HDK_MOD_Module s_module2 =
{
    &s_uuidNOID2,
    "Module 2",
    s_services2,
    s_methods2,
    s_events2,
    0
};

const HDK_MOD_Module* GetModule()
{
    return &s_module;
}

const HDK_MOD_Module* GetModule2()
{
    return &s_module2;
}
