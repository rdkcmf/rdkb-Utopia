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
#include "unittest_util.h"

#include <string.h>


/*
 * Namespaces
 */

const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HDK/Unittest/Client/cpp/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

const HDK_XML_ElementNode s_elements[] =
{
    /* Element_Blob */ { 0, "Blob" },
    /* Element_BlobArray */ { 0, "BlobArray" },
    /* Element_BoolArray */ { 0, "BoolArray" },
    /* Element_DateTimeArray */ { 0, "DateTimeArray" },
    /* Element_EnumMember */ { 0, "Enum" },
    /* Element_EnumArray */ { 0, "EnumArray" },
    /* Element_IPAddress */ { 0, "IPAddress" },
    /* Element_IPAddressArray */ { 0, "IPAddressArray" },
    /* Element_IntArray */ { 0, "IntArray" },
    /* Element_LongArray */ { 0, "LongArray" },
    /* Element_MACAddress */ { 0, "MACAddress" },
    /* Element_MACAddressArray */ { 0, "MACAddressArray" },
    /* Element_StringArray */ { 0, "StringArray" },
    /* Element_Struct */ { 0, "Struct" },
    /* Element_StructArray */ { 0, "StructArray" },
    /* Element_UUID */ { 0, "UUID" },
    /* Element_UUIDArray */ { 0, "UUIDArray" },
    /* Element_bool */ { 0, "bool" },
    /* Element_datetime */ { 0, "datetime" },
    /* Element_int */ { 0, "int" },
    /* Element_long */ { 0, "long" },
    /* Element_string */ { 0, "string" },
    HDK_XML_Schema_ElementsEnd
};

static const HDK_XML_EnumValue s_enum_UnittestEnum[] =
{
    "UnittestEnum_Value0",
    "UnittestEnum_Value1",
    "UnittestEnum_Value2",
    "UnittestEnum_Value3",
    "UnittestEnum_Value4",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_UnittestEnum
};
