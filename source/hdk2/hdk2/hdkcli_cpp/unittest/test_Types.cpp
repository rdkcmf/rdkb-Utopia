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

#include "unittest.h"
#include "unittest_tests.h"
#include "unittest_util.h"
#include "unittest_client.h"

#include "unittest_schema.h"

#include "hdk_cli_cpp.h"

#include <errno.h>

#define PRINTF_COMPARISON(_lhs, _rhs) \
    printf(#_lhs" %c= " #_rhs "\n", (_lhs == _rhs) ? '=' : '!')


#define PRINTF_IPV4ADDRESS(_ip) \
    { \
        printf(#_ip ": %s\n", _ip.ToString().c_str()); \
        printf("  IsBlank: %s\n", _ip.IsBlank() ? "true" : "false"); \
    }

void Test_IPv4Address(void)
{
    HDK_XML_IPAddress hdkIp;
    hdkIp.a = 1;
    hdkIp.b = 2;
    hdkIp.c = 3;
    hdkIp.d = 4;

    HDK::IPv4Address ipBroadcast(0xFFFFFFFF);
    HDK::IPv4Address ipZero((unsigned long)0);

    HDK::IPv4Address ipDefaultConstructor;
    PRINTF_IPV4ADDRESS(ipDefaultConstructor);
    PRINTF_COMPARISON(ipDefaultConstructor, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipDefaultConstructor, ipBroadcast);
    PRINTF_COMPARISON(ipDefaultConstructor, ipZero);
    printf("\n");

    HDK::IPv4Address ipHdkConstructor(&hdkIp);
    PRINTF_IPV4ADDRESS(ipHdkConstructor);
    PRINTF_COMPARISON(ipHdkConstructor, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipHdkConstructor, ipBroadcast);
    PRINTF_COMPARISON(ipHdkConstructor, ipZero);
    printf("\n");

    HDK::IPv4Address ipHdkNullConstructor((const HDK_XML_IPAddress*)NULL);
    PRINTF_IPV4ADDRESS(ipHdkNullConstructor);
    PRINTF_COMPARISON(ipHdkNullConstructor, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipHdkNullConstructor, ipBroadcast);
    PRINTF_COMPARISON(ipHdkNullConstructor, ipZero);
    printf("\n");

    HDK::IPv4Address ipHdkRefConstructor(hdkIp);
    PRINTF_IPV4ADDRESS(ipHdkRefConstructor);
    PRINTF_COMPARISON(ipHdkRefConstructor, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipHdkRefConstructor, ipBroadcast);
    PRINTF_COMPARISON(ipHdkRefConstructor, ipZero);
    printf("\n");

    HDK::IPv4Address ipCopyConstructor(ipHdkRefConstructor);
    PRINTF_IPV4ADDRESS(ipCopyConstructor);
    PRINTF_COMPARISON(ipCopyConstructor, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipCopyConstructor, ipBroadcast);
    PRINTF_COMPARISON(ipCopyConstructor, ipZero);
    PRINTF_COMPARISON(ipCopyConstructor, ipHdkRefConstructor);
    printf("\n");

    // ipBroadcast uses the unsigned long constructor
    PRINTF_IPV4ADDRESS(ipBroadcast);
    PRINTF_COMPARISON(ipBroadcast, HDK::IPv4Address::Blank());
    PRINTF_COMPARISON(ipBroadcast, ipZero);
    printf("\n");

    PRINTF_IPV4ADDRESS(HDK::IPv4Address::Blank());
    printf("\n");

    printf("FromString():\n");

    static const char* s_pszFromStringInputs[] =
    {
        "0.0.0.0",
        "000.000.000.000",
        "00.0.000.0",
        "255.255.255.255",
        "0001.2.2.3",
        "120.121.122.123",
        "999.999.999.999",
        "2..2.1",
        "255.255.256.255",
        "44455555233434.1.1.255",
        "a.10.1.0",
        "1.2.3.4.",
        "1.23.4.78a",
        "1.23.4.78hi mom",
        "1.2",
        "",
        "      ",
        "  \n \t \t",
        " \t   6 \n b",
        "   1.2.3.4",
        "5.52.53.54   ",
        "\t 10.0.0.64 \n",
        "foo",
        "0x23.0x12.0xa.0x5",
        "192.168.0.1",
        "192.169.0.1/12",
        "2,2,2,2",
        "19.12.44,79",
        "d192.168.0.1",
        "19 2. 1 6 8 . 0  \n . \t 1 ",
        "-1.2.3.2",
        NULL
    };

    for (size_t ix = 0; ix < sizeof(s_pszFromStringInputs) / sizeof(*s_pszFromStringInputs); ix++)
    {
        HDK::IPv4Address ip;
        bool fValid = ip.FromString(s_pszFromStringInputs[ix]);
        printf("  '%s' --> %svalid", s_pszFromStringInputs[ix], fValid ? "" : "NOT ");
        if (fValid)
        {
            printf(" (%s)", ip.ToString().c_str());
        }
        printf("\n");
    }
}

#define PRINTF_MACADDRESS(_mac) \
    { \
        printf(#_mac ": %s\n", _mac.ToString().c_str()); \
        printf("  IsBlank: %s\n", _mac.IsBlank() ? "true" : "false"); \
    }

void Test_MACAddress(void)
{
    HDK_XML_MACAddress hdkMac;
    hdkMac.a = 1;
    hdkMac.b = 2;
    hdkMac.c = 3;
    hdkMac.d = 4;
    hdkMac.e = 5;
    hdkMac.f = 6;

    static unsigned char s_broadcast[] =
    {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    };
    static unsigned char s_zero[] =
    {
        0x00,0x00,0x00,0x00,0x00,0x00
    };

    HDK::MACAddress macBroadcast(s_broadcast);
    HDK::MACAddress macZero(s_zero);

    HDK::MACAddress macDefaultConstructor;
    PRINTF_MACADDRESS(macDefaultConstructor);
    PRINTF_COMPARISON(macDefaultConstructor, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macDefaultConstructor, macBroadcast);
    PRINTF_COMPARISON(macDefaultConstructor, macZero);
    printf("\n");

    HDK::MACAddress macHdkConstructor(&hdkMac);
    PRINTF_MACADDRESS(macHdkConstructor);
    PRINTF_COMPARISON(macHdkConstructor, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macHdkConstructor, macBroadcast);
    PRINTF_COMPARISON(macHdkConstructor, macZero);
    printf("\n");

    HDK::MACAddress macHdkNullConstructor((const HDK_XML_MACAddress*)NULL);
    PRINTF_MACADDRESS(macHdkNullConstructor);
    PRINTF_COMPARISON(macHdkNullConstructor, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macHdkNullConstructor, macBroadcast);
    PRINTF_COMPARISON(macHdkNullConstructor, macZero);
    printf("\n");

    HDK::MACAddress macHdkRefConstructor(hdkMac);
    PRINTF_MACADDRESS(macHdkRefConstructor);
    PRINTF_COMPARISON(macHdkRefConstructor, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macHdkRefConstructor, macBroadcast);
    PRINTF_COMPARISON(macHdkRefConstructor, macZero);
    printf("\n");

    HDK::MACAddress macCopyConstructor(macHdkRefConstructor);
    PRINTF_MACADDRESS(macCopyConstructor);
    PRINTF_COMPARISON(macCopyConstructor, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macCopyConstructor, macBroadcast);
    PRINTF_COMPARISON(macCopyConstructor, macZero);
    PRINTF_COMPARISON(macCopyConstructor, macHdkRefConstructor);
    printf("\n");

    // macBroadcast uses the const unsigned char* constructor
    PRINTF_MACADDRESS(macBroadcast);
    PRINTF_COMPARISON(macBroadcast, HDK::MACAddress::Blank());
    PRINTF_COMPARISON(macBroadcast, macZero);
    printf("\n");

    PRINTF_MACADDRESS(HDK::MACAddress::Blank());
    printf("\n");

    printf("FromString():\n");

    static const char* s_pszFromStringInputs[] =
    {
        "00:00:00:00:00:00",
        "0:0:0:0:0:0",
        ":::::",
        "000:0000:000:000:000:000",
        "6:5:4:3:2:1",
        "00:1C:4b:35:42:E1",
        "FF:FF:FF:FF:FF:FF",
        "00:1D:4B:G5:42:X1",
        "00:1D:4B:E5:42.21",
        "00:1D:4B:E5:42.21:09",
        "00:1D:4B:E5:42.f00",
        "00:1D:4B:E5:42:fFF",
        "00:1D::E05:42:f0",
        " \t  00:13:FF:33:10:c0",
        "00:33:41:2B:A3:d2 \t  \n ",
        "  \t  \n 00:1D:F4:E0:11:A3 \t  \n ",
        "00:1D:42:f0",
        "0022334455",
        "",
        "foo",
        "  0   0:1\nC:4b:\t3 5:42:    E1 ",
        NULL
    };

    for (size_t ix = 0; ix < sizeof(s_pszFromStringInputs) / sizeof(*s_pszFromStringInputs); ix++)
    {
        HDK::MACAddress mac;
        bool fValid = mac.FromString(s_pszFromStringInputs[ix]);
        printf("  '%s' --> %svalid", s_pszFromStringInputs[ix], fValid ? "" : "NOT ");
        if (fValid)
        {
            printf(" (%s)", mac.ToString().c_str());
        }
        printf("\n");
    }
}

#define PRINTF_BLOB(_blob) \
    printf(#_blob ": {%u}\n", _blob.get_Size()); \
    { \
        printf("  "); \
        for (size_t ix = 0; ix < _blob.get_Size(); ix++) \
        { \
            unsigned char byte = (unsigned char)_blob.get_Data()[ix]; \
            printf("%02x", byte); \
        } \
        printf("\n"); \
    }

void Test_Blob(void)
{
    static unsigned char s_bytesDeadBeef [] =
    {
        0xDE,0xAD,0xBE,0xEF
    };

    static unsigned char s_bytesDeadDeadBeef [] =
    {
        0xDE,0xAD,0xDE,0xAD,0xBE,0xEF
    };

    static unsigned char s_bytesFoooFo [] =
    {
        0xF0,0x00,0xF0
    };

    HDK::Blob blobDeadDeadBeef((const char*)s_bytesDeadDeadBeef, sizeof(s_bytesDeadDeadBeef) / sizeof(*s_bytesDeadDeadBeef));
    HDK::Blob blobFooFo((const char*)s_bytesFoooFo, sizeof(s_bytesFoooFo) / sizeof(*s_bytesFoooFo));

    HDK::Blob blobDefaultConstructor;
    PRINTF_BLOB(blobDefaultConstructor);
    PRINTF_COMPARISON(blobDefaultConstructor, blobDeadDeadBeef);
    PRINTF_COMPARISON(blobDefaultConstructor, blobFooFo);
    printf("\n");

    HDK::Blob blobRawConstructor((const char*)s_bytesDeadBeef, sizeof(s_bytesDeadBeef) / sizeof(*s_bytesDeadBeef));
    PRINTF_BLOB(blobRawConstructor);
    PRINTF_COMPARISON(blobRawConstructor.get_Data(), (const char*)s_bytesDeadBeef);
    PRINTF_COMPARISON(blobRawConstructor, blobDeadDeadBeef);
    PRINTF_COMPARISON(blobRawConstructor, blobFooFo);
    printf("\n");

    HDK::Blob blobCopyConstructor(blobRawConstructor);
    PRINTF_BLOB(blobCopyConstructor);
    PRINTF_COMPARISON(blobCopyConstructor.get_Data(), blobRawConstructor.get_Data());
    PRINTF_COMPARISON(blobCopyConstructor, blobDeadDeadBeef);
    PRINTF_COMPARISON(blobCopyConstructor, blobFooFo);
    PRINTF_COMPARISON(blobCopyConstructor, blobRawConstructor);
    printf("\n");

    HDK::Blob blobAssignmentOperator;
    blobAssignmentOperator = blobRawConstructor;
    PRINTF_BLOB(blobAssignmentOperator);
    PRINTF_COMPARISON(blobAssignmentOperator.get_Data(), blobRawConstructor.get_Data());
    PRINTF_COMPARISON(blobAssignmentOperator, blobDeadDeadBeef);
    PRINTF_COMPARISON(blobCopyConstructor, blobFooFo);
    PRINTF_COMPARISON(blobAssignmentOperator, blobRawConstructor);
}

#define PRINTF_STRUCT(_struct) \
    printf(#_struct "\n"); \
    { \
        printf("  IsNull: %s\n", _struct.IsNull() ? "true" : "false"); \
        printf("  Element: %s (%d)\n", (HDK_XML_BuiltinElement_Unknown != _struct.Element()) ? s_elements[_struct.Element()].pszElement : "", _struct.Element()); \
    }

void Test_Struct(void)
{
    /* Schema tree */
    static HDK_XML_SchemaNode s_schemaNodes[] = {
        { /* 0 */ 0, Element_Struct, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_int, HDK_XML_BuiltinType_Int, 0 },
        { /* 1 */ 0, Element_long, HDK_XML_BuiltinType_Long, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    HDK_XML_Struct sHdk;
    HDK_XML_Struct_Init(&sHdk);
    sHdk.node.element = Element_Struct;

    HDK_XML_Set_Int(&sHdk, Element_int, 5);
    HDK_XML_Set_Int(&sHdk, Element_long, -123);

    HDK::Struct structDefaultConstructor;
    PRINTF_STRUCT(structDefaultConstructor);
    printf("\n");

    HDK::Struct structElementConstructor(Element_Struct);
    PRINTF_STRUCT(structElementConstructor);
    SerializeTestHelper(structElementConstructor, s_schemaNodes);
    printf("\n");

    HDK::Struct structNull((HDK_XML_Struct*)NULL);
    PRINTF_STRUCT(structNull);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structNull, (const HDK_XML_Struct*)HDK::Struct::Null());
    printf("\n");

    HDK::Struct structHdkStructConstructor(&sHdk);
    PRINTF_STRUCT(structHdkStructConstructor);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structHdkStructConstructor, &sHdk);
    SerializeTestHelper(structHdkStructConstructor, s_schemaNodes);
    printf("\n");

    HDK::Struct structCopyConstructor(structHdkStructConstructor);
    PRINTF_STRUCT(structCopyConstructor);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structCopyConstructor, (const HDK_XML_Struct*)structHdkStructConstructor);
    SerializeTestHelper(structCopyConstructor, s_schemaNodes);
    printf("\n");

    HDK::Struct structCopyConstructorFromNull(structNull);
    PRINTF_STRUCT(structCopyConstructorFromNull);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structCopyConstructorFromNull, (const HDK_XML_Struct*)structNull);
    printf("\n");

    HDK::Struct structAssignmentOperator;
    structAssignmentOperator = structHdkStructConstructor;
    PRINTF_STRUCT(structAssignmentOperator);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structAssignmentOperator, (const HDK_XML_Struct*)structHdkStructConstructor);
    SerializeTestHelper(structAssignmentOperator, s_schemaNodes);
    printf("\n");

    HDK::Struct structAssignmentOperatorFromNull;
    structAssignmentOperatorFromNull = structNull;
    PRINTF_STRUCT(structAssignmentOperatorFromNull);
    PRINTF_COMPARISON((const HDK_XML_Struct*)structAssignmentOperatorFromNull, (const HDK_XML_Struct*)structNull);
    printf("\n");

    HDK_XML_Struct_Free(&sHdk);

    UnittestStruct unittestStruct;
    PRINTF_STRUCT(unittestStruct);
    PrintUnittestStruct(unittestStruct);
    printf("\n");
}

void Test_DOM(void)
{
    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_Struct, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_int, HDK_XML_BuiltinType_Int, 0 },
        { /* 2 */ 0, Element_string, HDK_XML_BuiltinType_String, 0 },
        { /* 3 */ 0, Element_Struct, HDK_XML_BuiltinType_Struct, 0 },
        { /* 4 */ 3, Element_string, HDK_XML_BuiltinType_String, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    static const char s_pszFileContents[] =
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      "<Struct xmlns=\"http://cisco.com/HDK/Unittest/Client/cpp/\">\n"
      " <int>45</int>\n"
      " <string>This is a string in my DOM unittest</string>\n"
      " <Struct>\n"
      "  <string>a substruct</string>\n"
      " </Struct>\n"
      "</Struct>\n";

    HDK_XML_Schema schema;
    memset(&schema, 0, sizeof(schema));
    schema.ppszNamespaces = s_namespaces;
    schema.pElements = s_elements;
    schema.pSchemaNodes = schemaNodes;
    schema.pEnumTypes = s_enumTypes;

    const char* pszTmp  = "unittest.tmp";
    FILE* pfhTmp = fopen(pszTmp, "wb+");
    if (!pfhTmp)
    {
        UnittestLog2("fopen(%s) failed with error %d\n", pszTmp, errno);
        return;
    }

    UnittestStruct unittestStruct;
    if (fwrite(s_pszFileContents, sizeof(char), sizeof(s_pszFileContents) - 1, pfhTmp))
    {
        rewind(pfhTmp);
        if (!unittestStruct.DeserializeFromFile(&schema, pfhTmp))
        {
            UnittestLog("Failed to deserialize input from tmp file\n");
        }
    }
    else
    {
        UnittestLog1("fwrite() failed with error %d\n", errno);
    }

    fclose(pfhTmp);
    (void)remove(pszTmp);

    PrintUnittestStruct(unittestStruct);
    printf("\n");

    if (!unittestStruct.SerializeToFile(&schema, stdout, 0))
    {
        UnittestLog("Failed to serialize struct to file\n");
    }
}

#define PRINTF_DEVICE(_device) \
    printf(#_device "\n"); \
    { \
        printf("   Username: %s\n", _device.Username()); \
        printf("   Password: %s\n", _device.Password()); \
        printf("   Host: %s\n", _device.Host()); \
        printf("   Port: %u\n", _device.Port()); \
        printf("   UseSsl: %s\n", _device.UseSsl() ? "true" : "false"); \
    }

#define UPDATE_AND_PRINT_DEVICE_STRING(_device, _string, _value) \
    printf(#_device "." #_string ": %s -> " #_value "\n", _device._string()); \
    if (!_device._string(#_value)) \
    { \
        UnittestLog1("Failed to update " #_device "." #_string "(%s)\n", _value); \
    } \
    else \
    { \
        printf(#_device "." #_string "(): %s\n", _device._string()); \
    }

void Test_Device(void)
{
    HDK::HNAPDevice deviceDefaultConstructor;
    PRINTF_DEVICE(deviceDefaultConstructor);

    HDK::HNAPDevice deviceWithHost("some_host_string");
    PRINTF_DEVICE(deviceWithHost);

    HDK::HNAPDevice deviceWithHostUsernamePassword("This be my host", "This be my password", "Arr ...");
    PRINTF_DEVICE(deviceWithHostUsernamePassword);

    HDK::HNAPDevice deviceWithHostPortUsernamePassword("hostess", 6554, "admin", "not password");
    PRINTF_DEVICE(deviceWithHostPortUsernamePassword);

    HDK::HNAPDevice deviceWithIpPort(0xFFFFFFFF, 79);
    PRINTF_DEVICE(deviceWithHostPortUsernamePassword);

    HDK::HNAPDevice deviceWithIpPortUsernamePassword(HDK::IPv4Address((unsigned long int)0), 445, "", "");
    PRINTF_DEVICE(deviceWithIpPortUsernamePassword);

    HDK::HNAPDevice deviceWithIpv4PortUsernamePassword(HDK::IPv4Address::Blank(), 0, NULL, NULL);
    PRINTF_DEVICE(deviceWithIpv4PortUsernamePassword);

    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Username, "NEW_USERNAME_VALUE");
    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Username, "ANOTHER_NEW_USERNAME_VALUE");

    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Password, "NEW_PAsSWord_VALUE");
    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Password, "ANOTHER_NEW_PasswoRDd_VALUE");

    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Host, "___New_hOsTT__VALUE");
    UPDATE_AND_PRINT_DEVICE_STRING(deviceDefaultConstructor, Host, "ANOTH___ADASIOJD#&$#$__Host_Value..");
}


#define PRINTF_UUID(_uuid) \
    { \
        printf(#_uuid ": %s\n", _uuid.ToString().c_str()); \
        printf("  IsZero(): %s\n", _uuid.IsZero() ? "true" : "false"); \
    }

void Test_UUID(void)
{
    HDK_XML_UUID hdkUUID;
    for (size_t ix = 0; ix < (sizeof(hdkUUID.bytes) / sizeof(*hdkUUID.bytes)); ix++)
    {
        hdkUUID.bytes[ix] = (unsigned char)ix;
    }

    HDK_XML_UUID hdkUUIDZero;
    memset(&hdkUUIDZero, 0, sizeof(hdkUUIDZero));

    HDK::UUID uuidZero(&hdkUUIDZero);

    HDK::UUID uuidDefaultConstructor;
    PRINTF_UUID(uuidDefaultConstructor);
    PRINTF_COMPARISON(uuidDefaultConstructor, uuidZero);
    printf("\n");

    HDK::UUID uuidHdkConstructor(&hdkUUID);
    PRINTF_UUID(uuidHdkConstructor);
    PRINTF_COMPARISON(uuidHdkConstructor, uuidZero);
    printf("\n");

    HDK::UUID uuidHdkNullConstructor((const HDK_XML_UUID*)NULL);
    PRINTF_UUID(uuidHdkNullConstructor);
    PRINTF_COMPARISON(uuidHdkNullConstructor, uuidZero);
    printf("\n");

    HDK::UUID uuidHdkRefConstructor(hdkUUID);
    PRINTF_UUID(uuidHdkRefConstructor);
    PRINTF_COMPARISON(uuidHdkRefConstructor, uuidZero);
    printf("\n");

    HDK::UUID uuidCopyConstructor(uuidHdkRefConstructor);
    PRINTF_UUID(uuidCopyConstructor);
    PRINTF_COMPARISON(uuidCopyConstructor, uuidZero);
    PRINTF_COMPARISON(uuidCopyConstructor, uuidHdkRefConstructor);
    printf("\n");

    PRINTF_UUID(HDK::UUID::Zero());
    printf("\n");

    printf("FromString():\n");

    static const char* s_pszFromStringInputs[] =
    {
        "00000000-0000-0000-0000-000000000000",
        "00000000-0000-0000-0000-00000000000",
        "01234567-89AB-CDEF-1234-0123456789AB",
        "01234567-89AB-CDEF-1234-G123456789AB",
        "0-0-7-2",
        "    01234567-89AB-1234-CDEF-0123456789AB",
        " \t \n01234567-89AB-3334-CDEF-0123456789AB \t \n",
        "asfawdt iajsgl;ksjfklalsdfkasl;dfla;ldas"
        "",
        "      ",
        "  \n \t \t",
        " \t   6 \n b",
        "0   1234567 - 89  AB -CDEF \n -0123456789AB",
        "- - - -",
        NULL
    };

    for (size_t ix = 0; ix < sizeof(s_pszFromStringInputs) / sizeof(*s_pszFromStringInputs); ix++)
    {
        HDK::UUID uuid;
        bool fValid = uuid.FromString(s_pszFromStringInputs[ix]);
        printf("  '%s' --> %svalid", s_pszFromStringInputs[ix], fValid ? "" : "NOT ");
        if (fValid)
        {
            printf(" (%s)", uuid.ToString().c_str());
        }
        printf("\n");
    }
}
