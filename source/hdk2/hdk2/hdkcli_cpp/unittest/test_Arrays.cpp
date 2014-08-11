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

#include "unittest.h"
#include "unittest_tests.h"
#include "unittest_util.h"

#include "unittest_schema.h"
#include "unittest_client.h"

#include "hdk_cli_cpp.h"

#define PRINTF_ARRAY(_array) \
  printf(#_array " (size: %llu, empty: %s)\n", (long long unsigned)_array.size(), _array.empty() ? "true" : "false")

#ifdef GCC_WARN_EFFCPP
/* Disable warnings about non-virtual destructors.  This is required to compile this file with the -Weffc++ flag, but only supported on GCC 4.2+ */
#  pragma GCC diagnostic ignored "-Weffc++"
#endif

void Test_StructArray(void)
{
    UnittestStructArray structArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &structArrayRead, s_schemaUnittestStruct,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<StructArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <Struct>\n"
        "  <Enum>UnittestEnum_Value3</Enum>\n"
        "  <int>1984</int>\n"
        "  <long>-3334</long>\n"
        "  <IPAddress>239.255.255.254</IPAddress>\n"
        "  <MACAddress>00:11:bb:5a:c4:01</MACAddress>\n"
        "  <EnumArray>\n"
        "   <Enum>UnittestEnum_Value1</Enum>\n"
        "   <Enum>UnittestEnum_Value4</Enum>\n"
        "   <Enum>UnittestEnum_Value0</Enum>\n"
        "   <Enum>UnittestEnum_Value3</Enum>\n"
        "  </EnumArray>\n"
        "  <string>This is the first struct in the struct array!</string>\n"
        " </Struct>\n"
        " <Struct>\n"
        "  <Enum>UnittestEnum_Value0</Enum>\n"
        "  <int>4815</int>\n"
        "  <long>162342</long>\n"
        "  <IPAddress>4.8.15.16</IPAddress>\n"
        "  <MACAddress>23:42:23:16:15:80</MACAddress>\n"
        "  <EnumArray>\n"
        "   <Enum>UnittestEnum_Value2</Enum>\n"
        "   <Enum>UnittestEnum_Value2</Enum>\n"
        "   <Enum>UnittestEnum_Value1</Enum>\n"
        "   <Enum>UnittestEnum_Value0</Enum>\n"
        "  </EnumArray>\n"
        "  <string>This is the second (yep, #2) struct in the struct array!</string>\n"
        "  <Struct>\n"
        "   <string>This is a string in the optional sub-struct element</string>\n"
        "  </Struct>\n"
        " </Struct>\n"
        " <Struct>\n"
        "  <Enum>UnittestEnum_Value4</Enum>\n"
        "  <int>1122</int>\n"
        "  <long>2233</long>\n"
        "  <IPAddress>55.1.19.21</IPAddress>\n"
        "  <MACAddress>00:FF:00:BE:EF:11</MACAddress>\n"
        "  <EnumArray>\n"
        "   <Enum>UnittestEnum_Value4</Enum>\n"
        "   <Enum>UnittestEnum_Value4</Enum>\n"
        "   <Enum>UnittestEnum_Value4</Enum>\n"
        "   <Enum>UnittestEnum_Value4</Enum>\n"
        "  </EnumArray>\n"
        "  <string>This is the third (yes 3rd) struct in the struct array!</string>\n"
        " </Struct>\n"
        "</StructArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(structArrayRead);

    // Test that the assignment operator made a deep copy.
    UnittestStructArray structArrayCopy;
    structArrayCopy = structArrayRead;

    // This should not alter structArrayRead...
    structArrayCopy.append(structArrayRead.begin().value());

    PRINTF_ARRAY(structArrayCopy);
    SerializeTestHelper(structArrayCopy, s_schemaUnittestStruct);

    printf("[");
    bool fNeedComma = false;
    for (UnittestStructArray::StructArrayIter iter = structArrayRead.begin(); iter != structArrayRead.end(); iter++)
    {
        printf("%s\n", fNeedComma ? "," : "");
        PrintUnittestStruct(iter.value());
        fNeedComma = true;
    }
    printf("\n]\n");

    UnittestStructArray structArrayWrite(Element_StructArray);

    PRINTF_ARRAY(structArrayWrite);
    SerializeTestHelper(structArrayWrite, s_schemaUnittestStruct);
}

void Test_EnumArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_EnumArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_Enum, EnumType_UnittestEnum, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    UnittestEnumArray enumArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &enumArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<EnumArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <Enum>UnittestEnum_Value0</Enum>\n"
        " <Enum>UnittestEnum_Value0</Enum>\n"
        " <Enum>UnittestEnum_Value1</Enum>\n"
        " <Enum>UnittestEnum_Value1</Enum>\n"
        "</EnumArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(enumArrayRead);

    printf("[");
    bool fNeedComma = false;
    for (UnittestEnumArray::EnumArrayIter iter = enumArrayRead.begin(); iter != enumArrayRead.end(); iter++)
    {
        printf("%s\n%s", fNeedComma ? "," : "", EnumToString(EnumType_UnittestEnum, iter.value()));
        fNeedComma = true;
    }
    printf("\n]\n");

    UnittestEnumArray enumArrayWrite(Element_EnumArray);

    PRINTF_ARRAY(enumArrayWrite);
    SerializeTestHelper(enumArrayWrite, s_schemaNodes);

    static UnittestEnum s_valuesToWrite[] =
    {
        UnittestEnum_UnittestEnum_Value0,
        UnittestEnum_UnittestEnum_Value1,
        UnittestEnum_UnittestEnum_Value0,
        UnittestEnum_UnittestEnum_Value2,
        UnittestEnum_UnittestEnum_Value0,
        UnittestEnum_UnittestEnum_Value3,
        UnittestEnum_UnittestEnum_Value0,
        UnittestEnum_UnittestEnum_Value4,
        UnittestEnum_UnittestEnum_Value2,
        UnittestEnum_UnittestEnum_Value3,
        UnittestEnum_UnittestEnum_Value1,
        UnittestEnum_UnittestEnum_Value0
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!enumArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    UnittestEnumArray enumArrayCopy;
    enumArrayCopy = enumArrayWrite;

    // This should not alter enumArrayWrite...
    enumArrayCopy.append(enumArrayWrite.begin().value());

    PRINTF_ARRAY(enumArrayWrite);
    SerializeTestHelper(enumArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(enumArrayCopy);
    SerializeTestHelper(enumArrayCopy, s_schemaNodes);
}

void Test_BlobArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_BlobArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_Blob, HDK_XML_BuiltinType_Blob, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::BlobArray<Element_Blob> blobArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &blobArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<BlobArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <Blob>VGhpcyBpcyBhIGJsb2I=</Blob>\n"
        " <Blob>VGhpcyBpcyBhbHNvIGEgYmxvYiAuLi4uIDwgPiAmICEgLiB+</Blob>\n"
        " <Blob>Vf9EM1I=</Blob>\n"
        "</BlobArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(blobArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::BlobArray<Element_Blob>::BlobArrayIter iter = blobArrayRead.begin(); iter != blobArrayRead.end(); iter++)
    {
        HDK::Blob blob(iter.value());
        char* pszBlob = (char*)malloc(blob.get_Size() + 1);
        if (pszBlob)
        {
            memcpy(pszBlob, blob, blob.get_Size());
            pszBlob[blob.get_Size()] = '\0';
            printf("%s\n%s", fNeedComma ? "," : "", pszBlob);
            fNeedComma = true;
        }

        free(pszBlob);
    }
    printf("\n]\n");

    HDK::BlobArray<Element_Blob> blobArrayWrite(Element_BlobArray);

    PRINTF_ARRAY(blobArrayWrite);
    SerializeTestHelper(blobArrayWrite, s_schemaNodes);

    static const char* s_valuesToWrite[] =
    {
        "This is a blob",
        "This is also a blob .... < > & ! . ~",
        "\x24\x25\x26\x27\x28"
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        HDK::Blob blob(s_valuesToWrite[ix], (unsigned int)strlen(s_valuesToWrite[ix]));
        if (!blobArrayWrite.append(blob))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::BlobArray<Element_Blob> blobArrayCopy;
    blobArrayCopy = blobArrayWrite;

    // This should not alter blobArrayWrite...
    blobArrayCopy.append(blobArrayWrite.begin().value());

    PRINTF_ARRAY(blobArrayWrite);
    SerializeTestHelper(blobArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(blobArrayCopy);
    SerializeTestHelper(blobArrayCopy, s_schemaNodes);
}

void Test_BoolArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_BoolArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_bool, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::BoolArray<Element_bool> boolArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &boolArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<BoolArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <bool>true</bool>\n"
        " <bool>false</bool>\n"
        " <bool>false</bool>\n"
        " <bool>true</bool>\n"
        " <bool>false</bool>\n"
        "</BoolArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(boolArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::BoolArray<Element_bool>::BoolArrayIter iter = boolArrayRead.begin(); iter != boolArrayRead.end(); iter++)
    {
        printf("%s\n%s", fNeedComma ? "," : "", iter.value() ? "true" : "false");
        fNeedComma = true;
    }
    printf("\n]\n");

    HDK::BoolArray<Element_bool> boolArrayWrite(Element_BoolArray);

    PRINTF_ARRAY(boolArrayWrite);
    SerializeTestHelper(boolArrayWrite, s_schemaNodes);

    static bool s_valuesToWrite[] =
    {
        true,
        false,
        false,
        true,
        false,
        true,
        true
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!boolArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::BoolArray<Element_bool> boolArrayCopy;
    boolArrayCopy = boolArrayWrite;

    // This should not alter boolArrayWrite...
    boolArrayCopy.append(boolArrayWrite.begin().value());

    PRINTF_ARRAY(boolArrayWrite);
    SerializeTestHelper(boolArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(boolArrayCopy);
    SerializeTestHelper(boolArrayCopy, s_schemaNodes);
}

void Test_DateTimeArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_DateTimeArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_datetime, HDK_XML_BuiltinType_DateTime, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::DateTimeArray<Element_datetime> datetimeArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &datetimeArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<DateTimeArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <datetime>2009-12-01T17:00:00Z</datetime>\n"
        " <datetime>2010-11-00T16:01:40Z</datetime>\n"
        " <datetime>2001-02-05T15:20:12Z</datetime>\n"
        "</DateTimeArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(datetimeArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::DateTimeArray<Element_datetime>::DateTimeArrayIter iter = datetimeArrayRead.begin(); iter != datetimeArrayRead.end(); iter++)
    {
        printf("%s\n%s", fNeedComma ? "," : "", DateTimeToString(iter.value(), 1));
        fNeedComma = true;
    }
    printf("\n]\n");

    HDK::DateTimeArray<Element_datetime> datetimeArrayWrite(Element_DateTimeArray);

    PRINTF_ARRAY(datetimeArrayWrite);
    SerializeTestHelper(datetimeArrayWrite, s_schemaNodes);

    static time_t s_valuesToWrite[] =
    {
        0,
        1234345,
        34645454,
        3444433,
        1111111,
        987656534
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!datetimeArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::DateTimeArray<Element_datetime> datetimeArrayCopy;
    datetimeArrayCopy = datetimeArrayWrite;

    // This should not alter datetimeArrayWrite...
    datetimeArrayCopy.append(datetimeArrayWrite.begin().value());

    PRINTF_ARRAY(datetimeArrayWrite);
    SerializeTestHelper(datetimeArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(datetimeArrayCopy);
    SerializeTestHelper(datetimeArrayCopy, s_schemaNodes);
}

void Test_IntArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_IntArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::IntArray<Element_int> intArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &intArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<IntArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <int>0</int>\n"
        " <int>2222</int>\n"
        " <int>-2342</int>\n"
        " <int>20</int>\n"
        "</IntArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(intArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::IntArray<Element_int>::IntArrayIter iter = intArrayRead.begin(); iter != intArrayRead.end(); iter++)
    {
        printf("%s\n%d", fNeedComma ? "," : "", iter.value());
        fNeedComma = true;
    }
    printf("\n]\n");


    HDK::IntArray<Element_int> intArrayWrite(Element_IntArray);

    PRINTF_ARRAY(intArrayWrite);
    SerializeTestHelper(intArrayWrite, s_schemaNodes);

    static HDK_XML_Int s_valuesToWrite[] =
    {
        4, 8, 15, 16, 23, 42
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!intArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::IntArray<Element_int> intArrayCopy;
    intArrayCopy = intArrayWrite;

    // This should not alter intArrayWrite...
    intArrayCopy.append(intArrayWrite.begin().value());

    PRINTF_ARRAY(intArrayWrite);
    SerializeTestHelper(intArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(intArrayCopy);
    SerializeTestHelper(intArrayCopy, s_schemaNodes);
}

void Test_LongArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_LongArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_long, HDK_XML_BuiltinType_Long, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::LongArray<Element_long> longArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &longArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<LongArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <long>0</long>\n"
        " <long>12333213123123</long>\n"
        " <long>-234234234</long>\n"
        " <long>2</long>\n"
        " <long>14</long>\n"
        "</LongArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(longArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::LongArray<Element_long>::LongArrayIter iter = longArrayRead.begin(); iter != longArrayRead.end(); iter++)
    {
        printf("%s\n%lld", fNeedComma ? "," : "", (long long int)iter.value());
        fNeedComma = true;
    }
    printf("\n]\n");

    HDK::LongArray<Element_long> longArrayWrite(Element_LongArray);

    PRINTF_ARRAY(longArrayWrite);
    SerializeTestHelper(longArrayWrite, s_schemaNodes);

    static HDK_XML_Long s_valuesToWrite[] =
    {
        4, 8, 15, 16, 23, 42
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!longArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::LongArray<Element_long> longArrayCopy;
    longArrayCopy = longArrayWrite;

    // This should not alter longArrayWrite...
    longArrayCopy.append(longArrayWrite.begin().value());

    PRINTF_ARRAY(longArrayWrite);
    SerializeTestHelper(longArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(longArrayCopy);
    SerializeTestHelper(longArrayCopy, s_schemaNodes);
}

void Test_StringArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_StringArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::StringArray<Element_string> stringArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &stringArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<StringArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <string>This is a string</string>\n"
        " <string>in an array of strings...</string>\n"
        " <string>hooray...</string>\n"
        " <string>still more strings</string>\n"
        " <string>... last one!</string>\n"
        "</StringArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(stringArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::StringArray<Element_string>::StringArrayIter iter = stringArrayRead.begin(); iter != stringArrayRead.end(); iter++)
    {
        printf("%s\n\"%s\"", fNeedComma ? "," : "", iter.value());
        fNeedComma = true;
    }
    printf("\n]\n");

    HDK::StringArray<Element_string> stringArrayWrite(Element_StringArray);

    PRINTF_ARRAY(stringArrayWrite);
    SerializeTestHelper(stringArrayWrite, s_schemaNodes);

    static const char* s_valuesToWrite[] =
    {
        "Why",
        "are manholes round?",
        "...",
        "Because manhole covers are.",
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        if (!stringArrayWrite.append(s_valuesToWrite[ix]))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::StringArray<Element_string> stringArrayCopy;
    stringArrayCopy = stringArrayWrite;

    // This should not alter stringArrayWrite...
    stringArrayCopy.append(stringArrayWrite.begin().value());

    PRINTF_ARRAY(stringArrayWrite);
    SerializeTestHelper(stringArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(stringArrayCopy);
    SerializeTestHelper(stringArrayCopy, s_schemaNodes);
}

void Test_IPv4AddressArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_IPAddressArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_IPAddress, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::IPAddressArray<Element_IPAddress> ipaddressArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &ipaddressArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<IPAddressArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <IPAddress>255.255.255.255</IPAddress>\n"
        " <IPAddress>0.0.0.0</IPAddress>\n"
        " <IPAddress>127.0.0.1</IPAddress>\n"
        " <IPAddress>192.168.0.1</IPAddress>\n"
        " <IPAddress>10.0.0.1</IPAddress>\n"
        "</IPAddressArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(ipaddressArrayRead);
    printf("[");
    bool fNeedComma = false;

    char pszIP[16];
    for (HDK::IPAddressArray<Element_IPAddress>::IPAddressArrayIter iter = ipaddressArrayRead.begin(); iter != ipaddressArrayRead.end(); iter++)
    {
        if (iter.value().ToString(pszIP))
        {
            printf("%s\n\"%s\"", fNeedComma ? "," : "", pszIP);
            fNeedComma = true;
        }
    }
    printf("\n]\n");

    HDK::IPAddressArray<Element_IPAddress> ipaddressArrayWrite(Element_IPAddress);

    PRINTF_ARRAY(ipaddressArrayWrite);
    SerializeTestHelper(ipaddressArrayWrite, s_schemaNodes);

    static const char* s_valuesToWrite[] =
    {
        "111.111.111.112",
        "222.222.222.223",
        "12.34.56.78",
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        HDK::IPv4Address ip;
        if (!ip.FromString(s_valuesToWrite[ix]))
        {
            UnittestLog1("FromString(%s) failed\n", s_valuesToWrite[ix]);
            break;
        }
        if (!ipaddressArrayWrite.append(ip))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::IPAddressArray<Element_IPAddress> ipaddressArrayCopy;
    ipaddressArrayCopy = ipaddressArrayWrite;

    // This should not alter ipaddressArrayWrite...
    ipaddressArrayCopy.append(ipaddressArrayWrite.begin().value());

    PRINTF_ARRAY(ipaddressArrayWrite);
    SerializeTestHelper(ipaddressArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(ipaddressArrayCopy);
    SerializeTestHelper(ipaddressArrayCopy, s_schemaNodes);
}

void Test_MACAddressArray(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_MACAddressArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_MACAddress, HDK_XML_BuiltinType_MACAddress, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::MACAddressArray<Element_MACAddress> macaddressArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &macaddressArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<MACAddressArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <MACAddress>00:11:22:33:44:55</MACAddress>\n"
        " <MACAddress>67:89:1A:BC:DE:F0</MACAddress>\n"
        " <MACAddress>00:01:e4:b6:77:01</MACAddress>\n"
        "</MACAddressArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(macaddressArrayRead);
    printf("[");
    bool fNeedComma = false;

    char pszMAC[18];
    for (HDK::MACAddressArray<Element_MACAddress>::MACAddressArrayIter iter = macaddressArrayRead.begin(); iter != macaddressArrayRead.end(); iter++)
    {
        if (iter.value().ToString(pszMAC))
        {
            printf("%s\n\"%s\"", fNeedComma ? "," : "", pszMAC);
            fNeedComma = true;
        }
    }
    printf("\n]\n");

    HDK::MACAddressArray<Element_MACAddress> macaddressArrayWrite(Element_MACAddress);

    PRINTF_ARRAY(macaddressArrayWrite);
    SerializeTestHelper(macaddressArrayWrite, s_schemaNodes);

    static const char* s_valuesToWrite[] =
    {
        "33:54:fe:A2:57:00",
        "10:21:32:43:54:65",
        "00:00:0c:90:e2:33",
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        HDK::MACAddress mac;
        if (!mac.FromString(s_valuesToWrite[ix]))
        {
            UnittestLog1("FromString(%s) failed\n", s_valuesToWrite[ix]);
            break;
        }
        if (!macaddressArrayWrite.append(mac))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::MACAddressArray<Element_MACAddress> macaddressArrayCopy;
    macaddressArrayCopy = macaddressArrayWrite;

    // This should not alter macaddressArrayWrite...
    macaddressArrayCopy.append(macaddressArrayWrite.begin().value());

    PRINTF_ARRAY(macaddressArrayWrite);
    SerializeTestHelper(macaddressArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(macaddressArrayCopy);
    SerializeTestHelper(macaddressArrayCopy, s_schemaNodes);
}

void Test_ArrayIter(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_IntArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::IntArray<Element_int> intArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &intArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<IntArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <int>1</int>\n"
        " <int>2</int>\n"
        " <int>3</int>\n"
        " <int>4</int>\n"
        "</IntArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(intArrayRead);
    printf("[");
    bool fNeedComma = false;
    for (HDK::IntArray<Element_int>::IntArrayIter iter = intArrayRead.begin(); iter != intArrayRead.end(); iter++)
    {
        printf("%s\n%d", fNeedComma ? "," : "", iter.value());
        fNeedComma = true;
    }
    printf("\n]\n");


    HDK::IntArray<Element_int>::IntArrayIter iter = intArrayRead.begin();

    printf("iter.value() == %d\n", iter.value());
    iter++;
    printf("iter.value() == %d\n", iter.value());
    ++iter;
    printf("iter.value() == %d\n", iter.value());
}

void Test_UUIDArray(void)
{

    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_UUIDArray, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_UUID, HDK_XML_BuiltinType_UUID, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK::UUIDArray<Element_UUID> uuidArrayRead;

    /* Parse the XML */
    ParseTestHelper(
        &uuidArrayRead, s_schemaNodes,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<UUIDArray xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
        " <UUID>00000000-0000-0000-0000-000000000000</UUID>\n"
        " <UUID>11111111-1111-1111-1111-111111111111</UUID>\n"
        " <UUID>22222222-2222-2222-2222-222222222222</UUID>\n"
        " <UUID>33333333-3333-3333-3333-333333333333</UUID>\n"
        "</UUIDArray>\n",
        ParseTestHelperEnd);

    PRINTF_ARRAY(uuidArrayRead);
    printf("[");
    bool fNeedComma = false;

    char pszUUID[37];
    for (HDK::UUIDArray<Element_UUID>::UUIDArrayIter iter = uuidArrayRead.begin(); iter != uuidArrayRead.end(); iter++)
    {
        if (iter.value().ToString(pszUUID))
        {
            printf("%s\n\"%s\"", fNeedComma ? "," : "", pszUUID);
            fNeedComma = true;
        }
    }
    printf("\n]\n");

    HDK::UUIDArray<Element_UUID> uuidArrayWrite(Element_UUID);

    PRINTF_ARRAY(uuidArrayWrite);
    SerializeTestHelper(uuidArrayWrite, s_schemaNodes);

    static const char* s_valuesToWrite[] =
    {
        "00002510-0330-0dfa-4234-900000000009",
        "34f02a17-0330-ccaa-2234-900000000bbb",
        "cccccccc-dddd-eeee-defa-ffffffffffff",
    };

    for (size_t ix = 0; ix < sizeof(s_valuesToWrite) / sizeof(*s_valuesToWrite); ix++)
    {
        HDK::UUID uuid;
        if (!uuid.FromString(s_valuesToWrite[ix]))
        {
            UnittestLog1("FromString(%s) failed\n", s_valuesToWrite[ix]);
            break;
        }
        if (!uuidArrayWrite.append(uuid))
        {
            UnittestLog("append() failed\n");
            break;
        }
    }

    // Test that the assignment operator made a deep copy.
    HDK::UUIDArray<Element_UUID> uuidArrayCopy;
    uuidArrayCopy = uuidArrayWrite;

    // This should not alter uuidArrayWrite...
    uuidArrayCopy.append(uuidArrayWrite.begin().value());

    PRINTF_ARRAY(uuidArrayWrite);
    SerializeTestHelper(uuidArrayWrite, s_schemaNodes);

    PRINTF_ARRAY(uuidArrayCopy);
    SerializeTestHelper(uuidArrayCopy, s_schemaNodes);
}
