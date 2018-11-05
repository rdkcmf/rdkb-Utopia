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

#include "hdk_cli_cpp.h"

#include "unittest_schema.h"

typedef HDK::EnumArray<UnittestEnum, EnumType_UnittestEnum, Element_Enum> UnittestEnumArray;

extern HDK_XML_SchemaNode s_schemaUnittestStruct[];

class UnittestSubStruct : public HDK::Struct
{
public:
    UnittestSubStruct() :
        Struct()
    {
    }
    UnittestSubStruct(HDK_XML_Struct* pStruct) :
        Struct(pStruct)
    {
    }

    const char* get_String() const
    {
        return HDK_XML_GetEx_String(GetStruct(), Element_string, 0);
    }

    void set_String(const char* value)
    {
        (void)HDK_XML_Set_String(GetStruct(), Element_string, value);
    }
};

class UnittestStruct : public HDK::Struct
{
public:
    UnittestStruct() :
        Struct()
    {
    }
    UnittestStruct(HDK_XML_Struct* pStruct) :
        Struct(pStruct)
    {
    }

    UnittestEnum get_Enum() const
    {
        return (UnittestEnum)HDK_XML_GetEx_Enum(GetStruct(), Element_Enum, EnumType_UnittestEnum, UnittestEnum__UNKNOWN__);
    }

    void set_Enum(UnittestEnum value)
    {
        (void)HDK_XML_Set_Enum(GetStruct(), Element_Enum, EnumType_UnittestEnum, value);
    }

    int get_Int() const
    {
        return HDK_XML_GetEx_Int(GetStruct(), Element_int, 0);
    }

    void set_Int(int value)
    {
        (void)HDK_XML_Set_Int(GetStruct(), Element_int, value);
    }

    long long int get_Long() const
    {
        return HDK_XML_GetEx_Long(GetStruct(), Element_long, 0);
    }

    void set_Long(long long int value)
    {
        (void)HDK_XML_Set_Long(GetStruct(), Element_long, value);
    }

    HDK::IPv4Address get_IPAddress() const
    {
        return HDK_XML_GetEx_IPAddress(GetStruct(), Element_IPAddress, 0);
    }

    void set_IPAddress(const HDK::IPv4Address & value)
    {
        (void)HDK_XML_Set_IPAddress(GetStruct(), Element_IPAddress, value);
    }

    HDK::MACAddress get_MACAddress() const
    {
        return HDK_XML_GetEx_MACAddress(GetStruct(), Element_MACAddress, 0);
    }

    void set_MACAddress(const HDK::MACAddress & value)
    {
        (void)HDK_XML_Set_MACAddress(GetStruct(), Element_MACAddress, value);
    }

    UnittestEnumArray get_EnumArray() const
    {
        return HDK_XML_Get_Struct(GetStruct(), Element_EnumArray);
    }

    const char* get_String() const
    {
        return HDK_XML_GetEx_String(GetStruct(), Element_string, 0);
    }

    void set_String(const char* value)
    {
        (void)HDK_XML_Set_String(GetStruct(), Element_string, value);
    }

    UnittestSubStruct get_SubStruct() const
    {
        return HDK_XML_Get_Struct(GetStruct(), Element_Struct);
    }

    void set_SubStruct(const UnittestSubStruct & value) const
    {
        (void)HDK_XML_SetEx_Struct(GetStruct(), Element_Struct, value);
    }

}; // class UnittestStruct : public Struct

extern void PrintUnittestStruct(const UnittestStruct & s);

typedef HDK::StructArray<UnittestStruct, Element_Struct> UnittestStructArray;
