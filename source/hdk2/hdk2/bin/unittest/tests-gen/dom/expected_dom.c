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
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_DOM_Element_Cisco_CiscoStruct = 0 */ { 0, "CiscoStruct" },
    /* ACTUAL_DOM_Element_Cisco_CiscoStructToo = 1 */ { 0, "CiscoStructToo" },
    /* ACTUAL_DOM_Element_Cisco_a = 2 */ { 0, "a" },
    /* ACTUAL_DOM_Element_Cisco_b = 3 */ { 0, "b" },
    /* ACTUAL_DOM_Element_Cisco_c = 4 */ { 0, "c" },
    /* ACTUAL_DOM_Element_Cisco_d = 5 */ { 0, "d" },
    /* ACTUAL_DOM_Element_Cisco_e = 6 */ { 0, "e" },
    /* ACTUAL_DOM_Element_Cisco_s = 7 */ { 0, "s" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Struct http://cisco.com/HNAPExt/CiscoStruct
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoStruct[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Cisco_CiscoStruct, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_Cisco_c, HDK_XML_BuiltinType_Long, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoStruct =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoStruct,
    0
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_CiscoStruct()
{
    return &s_schema_Cisco_CiscoStruct;
}


/*
 * Struct http://cisco.com/HNAPExt/CiscoStructToo
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_CiscoStructToo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Cisco_CiscoStructToo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_Cisco_s, HDK_XML_BuiltinType_Struct, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_Cisco_d, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 3 */ { 0, ACTUAL_DOM_Element_Cisco_e, HDK_XML_BuiltinType_String, 0 },
    /* 4 */ { 1, ACTUAL_DOM_Element_Cisco_a, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 1, ACTUAL_DOM_Element_Cisco_b, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 1, ACTUAL_DOM_Element_Cisco_c, HDK_XML_BuiltinType_Long, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_Cisco_CiscoStructToo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_CiscoStructToo,
    0
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_CiscoStructToo()
{
    return &s_schema_Cisco_CiscoStructToo;
}
