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

#include "unittest.h"
#include "unittest_schema.h"
#include "unittest_tests.h"
#include "unittest_util.h"

#include "hdk_xml.h"

void Test_SerializeBlob()
{
    static const unsigned char s_pBytes[] = { 0xde, 0xad, 0xbe, 0xef, 0x15, 0xf0, 0x0d };

    static struct
    {
        const char* pData;
        unsigned int cbData;
    } s_blobs[] =
        {
            { (const char*)s_pBytes, (unsigned int)sizeof(s_pBytes) },
            { (const char*)s_pBytes, 4 },
            { (const char*)&s_pBytes[5], 2 },
            { 0, 0 }
        };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_blobs) / sizeof(*s_blobs); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_Blob(&cbSerialized, HDK_XML_OutputStream_File, stdout, s_blobs[ix].pData, s_blobs[ix].cbData, 0) &&
            HDK_XML_Serialize_Blob(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, s_blobs[ix].pData, s_blobs[ix].cbData, 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }
        else
        {
            UnittestLog2("Failed to serialize %u bytes of data at %p\n", s_blobs[ix].cbData, s_blobs[ix].pData);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeBool()
{
    static int s_bools[] =
      {
          0,
          1,
          324,
          -1,
          -234
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_bools) / sizeof(*s_bools); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_Bool(&cbSerialized, HDK_XML_OutputStream_File, stdout, s_bools[ix], 0) &&
            HDK_XML_Serialize_Bool(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, s_bools[ix], 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeDateTime()
{
    static time_t s_times[] =
      {
          0,
          1,
          23452,
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_times) / sizeof(*s_times); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_DateTime(&cbSerialized, HDK_XML_OutputStream_File, stdout, s_times[ix], 0) &&
            HDK_XML_Serialize_DateTime(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, s_times[ix], 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeErrorOutput()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_String, 0 },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_ErrorOutput },
        { /* 3 */ 0, Element_d, HDK_XML_BuiltinType_String, 0 },
        { /* 4 */ 0, Element_e, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
        { /* 5 */ 0, Element_f, HDK_XML_BuiltinType_String, 0 },
        { /* 6 */ 4, Element_b, HDK_XML_BuiltinType_String, 0 },
        { /* 7 */ 4, Element_c, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_ErrorOutput },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Parse the XML */
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_ErrorOutput, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>Four</b>\n"
        " <c>score</c>\n"
        " <d>and</d>\n"
        " <e>\n"
        "  <b>twenty</b>"
        "  <c>years</c>"
        " </e>\n"
        " <f>ago</f>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}

void Test_SerializeInt()
{
    static HDK_XML_Int s_ints[] =
      {
          0,
          -11,
          12345345,
          -992384
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_ints) / sizeof(*s_ints); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_Int(&cbSerialized, HDK_XML_OutputStream_File, stdout, s_ints[ix], 0) &&
            HDK_XML_Serialize_Int(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, s_ints[ix], 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeIPAddress()
{
    static HDK_XML_IPAddress s_ips[] =
      {
          { 0, 0, 0, 0 },
          { 255, 255, 255, 255 },
          { 192, 168, 0, 1 },
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_ips) / sizeof(*s_ips); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_IPAddress(&cbSerialized, HDK_XML_OutputStream_File, stdout, &(s_ips[ix]), 0) &&
            HDK_XML_Serialize_IPAddress(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, &(s_ips[ix]), 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeLong()
{
    static HDK_XML_Long s_longs[] =
      {
          0,
          -11,
          1234534512312LL,
          -992384L
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_longs) / sizeof(*s_longs); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_Long(&cbSerialized, HDK_XML_OutputStream_File, stdout, s_longs[ix], 0) &&
            HDK_XML_Serialize_Long(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, s_longs[ix], 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeMACAddress()
{
    static HDK_XML_MACAddress s_macs[] =
      {
          { 0, 0, 0, 0, 0, 0 },
          { 255, 255, 255, 255, 255, 255 },
          { 0x00, 0x01, 0x12, 0x23, 0x35, 0xFa },
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_macs) / sizeof(*s_macs); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_MACAddress(&cbSerialized, HDK_XML_OutputStream_File, stdout, &(s_macs[ix]), 0) &&
            HDK_XML_Serialize_MACAddress(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, &(s_macs[ix]), 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeString()
{
    static const char* s_strings[] =
      {
          "",
          "This is a string",
          "< & ' asd > ? \"",
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_strings) / sizeof(*s_strings); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_String(&cbSerialized, HDK_XML_OutputStream_File, stdout, (s_strings[ix]), 0) &&
            HDK_XML_Serialize_String(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, (s_strings[ix]), 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }
        else
        {
            UnittestLog1("Failed to serialize input '%s'\n", s_strings[ix] ? s_strings[ix] : "(null)");
        }

        UnittestLog("\n");
    }
}

void Test_SerializeUUID()
{
    static HDK_XML_UUID s_uuids[] =
      {
          { { 0x00,0x00,0x00,0x00,/*-*/0x00,0x00,/*-*/0x00,0x00,/*-*/0x00,0x00,/*-*/0x00,0x00,0x00,0x00,0x00,0x00 } },
          { { 0x11,0x21,0x31,0x41,/*-*/0x12,0x22,/*-*/0x13,0x23,/*-*/0x14,0x24,/*-*/0x15,0x25,0x35,0x45,0x55,0x65 } }
      };

    size_t ix;

    char szBuf[64];
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbSerialized;
    unsigned int cbSerializedNullTerm;

    bufferCtx.pBuf = szBuf;
    bufferCtx.cbBuf = sizeof(szBuf);

    for (ix = 0; ix < sizeof(s_uuids) / sizeof(*s_uuids); ix++)
    {
        bufferCtx.ixCur = 0;
        if (HDK_XML_Serialize_UUID(&cbSerialized, HDK_XML_OutputStream_File, stdout, &(s_uuids[ix]), 0) &&
            HDK_XML_Serialize_UUID(&cbSerializedNullTerm, HDK_XML_OutputStream_Buffer, &bufferCtx, &(s_uuids[ix]), 1))
        {
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerialized);

            UnittestLog1("%s", bufferCtx.pBuf);
            UnittestLog1("\nSerialized %u bytes of data\n", cbSerializedNullTerm);
        }

        UnittestLog("\n");
    }
}

void Test_SerializeCSV()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_CSV },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_Struct, 0 },
        { /* 3 */ 0, Element_d, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_CSV },
        { /* 4 */ 1, Element_a, HDK_XML_BuiltinType_Int, 0 },
        { /* 5 */ 1, Element_b, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_CSV /* ignored for non-struct types */ },
        { /* 7 */ 1, Element_c, HDK_XML_BuiltinType_Bool, 0 },
        { /* 8 */ 2, Element_a, HDK_XML_BuiltinType_Int, 0 },
        { /* 9 */ 2, Element_b, HDK_XML_BuiltinType_String, 0 },
        { /* 10 */ 2, Element_c, HDK_XML_BuiltinType_Bool, 0 },
        { /* 11 */ 3, Element_d, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Parse the XML */
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>365,Days in a year,true</b>\n"
        " <c>\n"
        "  <a>367</a>\n"
        "  <b>Days in a leap year</b>\n"
        "  <c>false</c>\n"
        " </c>\n"
        " <d>There are, &amp; \\,really \\,, 366 days, in a , leap year,</d>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}
