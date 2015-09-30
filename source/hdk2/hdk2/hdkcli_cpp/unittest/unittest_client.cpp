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

#include "unittest_client.h"
#include "unittest_util.h"

#ifdef GCC_WARN_EFFCPP
/* Disable warnings about non-virtual destructors.  This is required to compile this file with the -Weffc++ flag, but only supported on GCC 4.2+ */
#  pragma GCC diagnostic ignored "-Weffc++"
#endif

typedef HDK::EnumArray<UnittestEnum, EnumType_UnittestEnum, Element_Enum> UnittestEnumArray;

HDK_XML_SchemaNode s_schemaUnittestStruct[] = {
    { /* 0 */ 0, Element_StructArray, HDK_XML_BuiltinType_Struct, 0 },
    { /* 1 */ 0, Element_Struct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Unbounded },
    { /* 2 */ 1, Element_Enum, EnumType_UnittestEnum, 0 },
    { /* 3 */ 1, Element_int, HDK_XML_BuiltinType_Int, 0 },
    { /* 4 */ 1, Element_long, HDK_XML_BuiltinType_Long, 0 },
    { /* 5 */ 1, Element_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    { /* 6 */ 1, Element_MACAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    { /* 7 */ 1, Element_EnumArray, HDK_XML_BuiltinType_Struct, 0 },
    { /* 8 */ 1, Element_string, HDK_XML_BuiltinType_String, 0 },
    { /* 9 */ 1, Element_Struct, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    { /* 10 */ 7, Element_Enum, EnumType_UnittestEnum, HDK_XML_SchemaNodeProperty_Unbounded },
    { /* 11 */ 9, Element_string, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

void PrintUnittestStruct(const UnittestStruct & s)
{
    printf("{\n");
    printf("    Enum: %s\n", EnumToString(EnumType_UnittestEnum, s.get_Enum()));
    printf("    Int: %d\n", s.get_Int());
    printf("    Long: %lld\n", s.get_Long());

    char szIP[16] = { 0 };
    s.get_IPAddress().ToString(szIP);
    printf("    IPAddress: %s\n", szIP);

    char szMAC[18] = { 0 };
    s.get_MACAddress().ToString(szMAC);
    printf("    MACAddress: %s\n", szMAC);

    printf("    EnumArray: [");
    bool fNeedComma = false;

    for (UnittestEnumArray::EnumArrayIter iter = s.get_EnumArray().begin(); iter != s.get_EnumArray().end(); iter++)
    {
        printf("%s %s", fNeedComma ? "," : "", EnumToString(EnumType_UnittestEnum, iter.value()));
        fNeedComma = true;
    }
    printf(" ]\n");


    printf("    String: %s\n", s.get_String());

    UnittestSubStruct subStruct = s.get_SubStruct();
    if (!subStruct.IsNull())
    {
        printf("    SubStruct:\n");
        printf("    {\n");
        printf("        String: %s\n", subStruct.get_String());
        printf("    }\n");
    }
    printf("}");
}
