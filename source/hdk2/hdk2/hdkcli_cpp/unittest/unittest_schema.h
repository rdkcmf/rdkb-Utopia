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

#pragma once

#include "hdk_xml.h"

/*
 * Elements
 */

typedef enum _Element
{
    Element_Blob,
    Element_BlobArray,
    Element_BoolArray,
    Element_DateTimeArray,
    Element_Enum,
    Element_EnumArray,
    Element_IPAddress,
    Element_IPAddressArray,
    Element_IntArray,
    Element_LongArray,
    Element_MACAddress,
    Element_MACAddressArray,
    Element_StringArray,
    Element_Struct,
    Element_StructArray,
    Element_UUID,
    Element_UUIDArray,
    Element_bool,
    Element_datetime,
    Element_int,
    Element_long,
    Element_string
} Element;

/* Namespace table */
extern const HDK_XML_Namespace s_namespaces[];

/* Elements table */
extern const HDK_XML_ElementNode s_elements[];

/* Enum table */
extern const HDK_XML_EnumType s_enumTypes[];

/*
 * Enum types enumeration
 */

typedef enum _EnumType
{
    EnumType_UnittestEnum = -1
} EnumType;

typedef enum _UnittestEnum
{
    UnittestEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UnittestEnum_UnittestEnum_Value0 = 0,
    UnittestEnum_UnittestEnum_Value1 = 1,
    UnittestEnum_UnittestEnum_Value2 = 2,
    UnittestEnum_UnittestEnum_Value3 = 3,
    UnittestEnum_UnittestEnum_Value4 = 4
} UnittestEnum;
