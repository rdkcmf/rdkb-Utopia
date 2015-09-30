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
#include "unittest_schema.h"
#include "unittest_tests.h"
#include "unittest_util.h"

#include "hdk_xml.h"

#include <string.h>
#include <stdlib.h>


void Test_ParseSimple()
{
    HDK_XML_Struct sTemp;
    char* pBlob;
    unsigned int cbBlob;
    HDK_XML_Struct* pStruct;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Int, 0 },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_String, 0 },
        { /* 3 */ 0, Element_d, HDK_XML_BuiltinType_Blob, 0 },
        { /* 4 */ 0, Element_e, HDK_XML_BuiltinType_Bool, 0 },
        { /* 5 */ 0, Element_f, HDK_XML_BuiltinType_DateTime, 0 },
        { /* 6 */ 0, Element_g, HDK_XML_BuiltinType_Long, 0 },
        { /* 7 */ 0, Element_h, HDK_XML_BuiltinType_IPAddress, 0 },
        { /* 8 */ 0, Element_i, HDK_XML_BuiltinType_MACAddress, 0 },
        { /* 9 */ 0, Element_j, HDK_XML_BuiltinType_UUID, 0 },
        { /* 10 */ 0, Element_k, HDK_XML_BuiltinType_Struct, 0 },
        { /* 11 */ 10, Element_i, HDK_XML_BuiltinType_Int, 0 },
        { /* 12 */ 10, Element_Cisco_Foo, HDK_XML_BuiltinType_Struct, 0 },
        { /* 13 */ 12, Element_a, HDK_XML_BuiltinType_Int, 0 },
        { /* 14 */ 12, Element_Cisco_Bar, HDK_XML_BuiltinType_Int, 0 },
        { /* 15 */ 12, Element_NoNamespace, HDK_XML_BuiltinType_Int, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Parse the XML */
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>a &amp; b &lt; c &gt; d &quot; e &apos; f</c>\n"
        " <d>SGVsbG8sIFdvcmxkIQo=</d>\n"
        " <e>true</e>\n"
        " <k>\n"
        "  <i>11</i>\n"
        "  <Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        "   <a xmlns=\"http://cisco.com/\">11</a>\n"
        "   <Bar>9</Bar>\n"
        "   <NoNamespace xmlns=\"\">15</NoNamespace>\n"
        "  </Foo>\n"
        " </k>\n"
        " <f>2009-12-01T17:00:00Z</f>\n"
        " <g>19</g>\n"
        " <h>192.168.1.114</h>\n"
        " <i>00:0C:F1:92:4C:D8</i>\n"
        " <j>00112233-4455-6677-8899-aAbBcCdDeEfF</j>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Output parsed members */
    UnittestLog("\n");
    UnittestLog1("Member b = %d\n", *HDK_XML_Get_Int(&sTemp, Element_b));
    UnittestLog1("Member c = \"%s\"\n", HDK_XML_Get_String(&sTemp, Element_c));
    pBlob = HDK_XML_Get_Blob(&sTemp, Element_d, &cbBlob);
    pBlob[cbBlob - 1] = 0;
    UnittestLog1("Member d = %s\n", pBlob);
    UnittestLog1("Member e = %d\n", *HDK_XML_Get_Bool(&sTemp, Element_e));
    UnittestLog1("Member f = %s\n", DateTimeToString(*HDK_XML_Get_DateTime(&sTemp, Element_f), 1));
    UnittestLog1("Member g = %lld\n", (long long int)*HDK_XML_Get_Long(&sTemp, Element_g));
    UnittestLog1("Member h = %s\n", IPAddressToString(HDK_XML_Get_IPAddress(&sTemp, Element_h)));
    UnittestLog1("Member i = %s\n", MACAddressToString(HDK_XML_Get_MACAddress(&sTemp, Element_i)));
    UnittestLog1("Member j = %s\n", UUIDToString(HDK_XML_Get_UUID(&sTemp, Element_j)));
    pStruct = HDK_XML_Get_Struct(&sTemp, Element_k);
    UnittestLog1("Member k.i = %d\n", *HDK_XML_Get_Int(pStruct, Element_i));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_ParseEx()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 2 */ 1, Element_c, HDK_XML_BuiltinType_Struct, 0 },
        { /* 3 */ 1, Element_d, HDK_XML_BuiltinType_Int, 0 },
        { /* 4 */ 1, Element_f, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
        { /* 5 */ 2, Element_e, HDK_XML_BuiltinType_Int, 0 },
        { /* 6 */ 2, Element_c, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 7 */ 6, Element_d, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 8 */ 7, Element_e, HDK_XML_BuiltinType_Int, 0 },
        { /* 9 */ 7, Element_f, HDK_XML_BuiltinType_String, 0 },
        { /* 10 */ 7, Element_g, HDK_XML_BuiltinType_Bool, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Tests... */

    UnittestLog("\n========================================\n");
    UnittestLog("Struct as XML:\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 1, 0, 0,
        "<b xmlns=\"http://cisco.com/\">\n"
        " <c>\n"
        "  <e>18</e>\n"
        " </c>\n"
        " <d>17</d>\n"
        "</b>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Struct as non-XML, no-newlines:\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 1, ParseTestHelper_Parse_NoXML | ParseTestHelper_Serialize_NoNewlines | ParseTestHelper_Serialize_NoXML, 0,
        "<b xmlns=\"http://cisco.com/\">\n"
        " <c>\n"
        "  <e>18</e>\n"
        " </c>\n"
        " <d>17</d>\n"
        "</b>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Struct as CSV:\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 6, ParseTestHelper_Parse_NoXML | ParseTestHelper_Parse_CSV | ParseTestHelper_Serialize_CSV | ParseTestHelper_Serialize_NoXML, 0,
        "<c xmlns=\"http://cisco.com/\">1,a tuple list,true,2,the second \\, element in the list &amp;,false,145,yet more,true</c>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Non-struct XML as XML (non-member):\n\n");
    HDK_XML_Struct_Free(&sTemp);
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 3, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<d xmlns=\"http://cisco.com/\">17</d>",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Non-struct XML as XML member:\n\n");
    HDK_XML_Struct_Free(&sTemp);
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 3, ParseTestHelper_Parse_NoXML | ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<d xmlns=\"http://cisco.com/\">17</d>",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Non-struct non-XML as member:\n\n");
    HDK_XML_Struct_Free(&sTemp);
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 3, ParseTestHelper_Parse_NoXML | ParseTestHelper_Serialize_NoXML, 0,
        "17",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("String non-XML as member:\n\n");
    HDK_XML_Struct_Free(&sTemp);
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 4, ParseTestHelper_Parse_NoXML | ParseTestHelper_Serialize_NoXML, 0,
        "This & That",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Empty non-XML as member (invalid):\n\n");
    HDK_XML_Struct_Free(&sTemp);
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 3, ParseTestHelper_Parse_NoXML | ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_ParseProperties()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 3 */ 0, Element_d, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 4 */ 0, Element_e, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 5 */ 0, Element_f, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
        { /* 6 */ 1, Element_a, HDK_XML_BuiltinType_Int, 0 },
        { /* 7 */ 2, Element_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
        { /* 8 */ 3, Element_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 9 */ 4, Element_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Tests... */

    UnittestLog("[1..1] Valid (single):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>\n"
        "  <a>3</a>\n"
        " </b>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[1..1] Invalid (missing):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>\n"
        " </b>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..1] Valid (single):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <c>\n"
        "  <a>3</a>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..1] Valid (missing):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <c>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..1] Invalid (multiple):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <c>\n"
        "  <a>3</a>\n"
        "  <a>4</a>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[1..Inf] Invalid (missing):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <d>\n"
        " </d>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[1..Inf] Valid (single):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <d>\n"
        "  <a>3</a>\n"

        " </d>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[1..Inf] Valid (multiple):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <d>\n"
        "  <a>3</a>\n"
        "  <a>4</a>\n"
        " </d>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..Inf] Valid (missing):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <e>\n"
        " </e>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..Inf] Valid (single):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <e>\n"
        "  <a>3</a>\n"
        " </e>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("[0..Inf] Valid (multiple):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <e>\n"
        "  <a>3</a>\n"
        "  <a>4</a>\n"
        " </e>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\n========================================\n");
    UnittestLog("Any element:\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <f>\n"
        "  <apple>\n"
        "   <peach fuzz=\"1\" />\n"
        "  </apple>\n"
        "  <banana>\n"
        "   <pear>\n"
        "    <delicious>true</delicious>\n"
        "   </pear>\n"
        "  </banana>\n"
        " </f>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}

void Test_ParseCSV()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_Cisco_Foo, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_a, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_CSV },
        { /* 2 */ 0, Element_b, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_CSV },
        { /* 3 */ 0, Element_c, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_CSV },
        { /* 4 */ 0, Element_d, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_CSV },
        { /* 5 */ 1, Element_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 6 */ 2, Element_a, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 7 */ 3, Element_a, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_CSV /* this should be ignored for non-structs */ },
        { /* 8 */ 3, Element_b, HDK_XML_BuiltinType_Bool, 0 },
        { /* 9 */ 3, Element_c, HDK_XML_BuiltinType_String, 0 },
        { /* 10 */ 3, Element_d, HDK_XML_BuiltinType_String, 0 },
        { /* 11 */ 3, Element_e, HDK_XML_BuiltinType_DateTime,  HDK_XML_SchemaNodeProperty_Optional },
        { /* 12 */ 3, Element_f, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
        { /* 13 */ 4, Element_a, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 14 */ 13, Element_a, HDK_XML_BuiltinType_Int, 0 },
        { /* 15 */ 13, Element_b, HDK_XML_BuiltinType_String, 0 },
        { /* 15 */ 13, Element_c, HDK_XML_BuiltinType_DateTime, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Tests... */

    UnittestLog("Valid (homogenous lists):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <a xmlns=\"http://cisco.com/\">  1, 2,3 ,  4  ,5  </a>\n"
        " <b xmlns=\"http://cisco.com/\">This,    is a    ,'\\,', seperated, list  of  , strings, \\\\This\\\\is\\\\a\\\\path\\\\on\\\\Windows, /  </b>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Valid (empty homogenous lists):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <a xmlns=\"http://cisco.com/\"></a>\n"
        " <b xmlns=\"http://cisco.com/\"></b>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Valid (single item homogenous lists):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <a xmlns=\"http://cisco.com/\">14</a>\n"
        " <b xmlns=\"http://cisco.com/\">\\, test \\\\</b>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

        UnittestLog("Valid (empty string list):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <b xmlns=\"http://cisco.com/\">,,,,,,,,,,</b>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Valid (heterogenous lists):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">2,true,, my comma\\,,2009-12-01T17:00:00Z</c>\n"
        " <d xmlns=\"http://cisco.com/\">1,struct 1,2009-12-01T17:00:00Z,22,struct 2,2010-02-01T17:00:00Z,333,struct \\,3,2011-03-01T17:00:00Z</d>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Valid (optional item):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">55,false,,</c>"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (homogenous list type):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <a xmlns=\"http://cisco.com/\">  1, foo, boom  </a>\n"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (heterogenous list type):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">234,not a bool,,</c>"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (tuple list type):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <d xmlns=\"http://cisco.com/\">234,ok ,2009-12-01T17:00:00Z,15, bad, xxyy</d>"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (heterogenous missing element):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">55,false</c>"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (heterogenous extraneous element):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">55,false,foo  ,bar!! ,2009-12-01T17:00:00Z,more,</c>"
        "</Foo>\n",
        ParseTestHelperEnd);

    UnittestLog("Invalid (optional item mismatch):\n\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        " <c xmlns=\"http://cisco.com/\">2,true,, my comma\\,,This string should be preceeded by an (optional) datetime value</c>"
        "</Foo>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


typedef enum _Enum_A
{
    Enum_A_foo = 0,
    Enum_A_bar
} Enum_A;

static HDK_XML_EnumValue Enum_A_Strings[] =
{
    "foo",
    "bar",
    HDK_XML_Schema_EnumTypeValuesEnd
};

typedef enum _Enum_B
{
    Enum_B_bonk = 0,
    Enum_B_thud,
    Enum_B_
} Enum_B;

static HDK_XML_EnumValue Enum_B_Strings[] =
{
    "bonk",
    "thud",
    "",
    HDK_XML_Schema_EnumTypeValuesEnd
};


void Test_ParseValidTypes()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Schema schemaTmp;

    /* Enum Types */
    typedef enum _EnumTypes
    {
        EnumType_A = -1,
        EnumType_B = -2
    } EnumTypes;
    HDK_XML_EnumType enumTypes[] =
    {
        Enum_A_Strings,
        Enum_B_Strings
    };

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 3 */ 0, Element_d, HDK_XML_BuiltinType_Blob, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 4 */ 0, Element_e, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 5 */ 0, Element_f, HDK_XML_BuiltinType_DateTime, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 6 */ 0, Element_g, HDK_XML_BuiltinType_Long, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 7 */ 0, Element_h, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 8 */ 0, Element_i, HDK_XML_BuiltinType_MACAddress, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 9 */ 0, Element_j, HDK_XML_BuiltinType_UUID, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 10 */ 0, Element_k, HDK_XML_BuiltinType_Struct, 0 },
        { /* 11 */ 10, Element_a, EnumType_A, HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 12 */ 10, Element_b, EnumType_B, HDK_XML_SchemaNodeProperty_Unbounded },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Parse the XML */
    ParseTestHelper(
        &sTemp, schemaNodes, enumTypes, 0, 0, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <b>-7</b>\n"
        " <b>2147483647</b>\n"
        " <b>-2147483648</b>\n"
        " <b>  7</b>\n"
        " <b>8  </b>\n"
        " <b>  9  </b>\n"
        " <c>a &amp; b &lt; c &gt; d &quot; e &apos; f</c>\n"
        " <c>&amp;&lt;&gt;&quot;&apos;</c>\n"
        " <c>  abcdefg  </c>\n"
        " <c></c>\n"
        " <d>SGVsbG8sIFdvcmxkIQo=</d>\n"
        " <d>SGVsbG8sIFdvcmxkISEK</d>\n"
        " <d>SGVsbG8sIFdvcmxkISEhCg==</d>\n"
        " <d>  SGVsbG8sIFdvcmxkIQo=</d>\n"
        " <d>SGVsbG8sIFdvcmxkIQo=  </d>\n"
        " <d>  SGVsbG8sIFdvcmxkIQo=  </d>\n"
        " <d></d>\n"
        " <e>true</e>\n"
        " <e>false</e>\n",
        " <f>2009-12-01T17:00:00Z</f>\n"
        " <f>2009-12-01T17:00:00-08:00</f>\n"
        " <f>  2009-12-01T17:00:01Z</f>\n"
        " <f>2009-12-01T17:00:02Z  </f>\n"
        " <f>  2009-12-01T17:00:03Z  </f>\n"
        " <g>19</g>\n"
        " <g>-19</g>\n"
        " <g>9223372036854775807</g>\n"
        " <g>-9223372036854775808</g>\n"
        " <g>  19</g>\n"
        " <g>20  </g>\n"
        " <g>  21  </g>\n"
        " <h>192.168.1.114</h>\n"
        " <h>  192.168.1.115</h>\n"
        " <h>192.168.1.116  </h>\n"
        " <h>  192.168.1.117  </h>\n",
        " <h></h>\n"
        " <i>00:0C:F1:92:4C:D8</i>\n"
        " <i>  00:0C:F1:92:4C:D9</i>\n"
        " <i>00:0C:F1:92:4C:Da  </i>\n"
        " <i>  00:0C:F1:92:4C:Db  </i>\n"
        " <i></i>\n"
        " <j>00112233-4455-6677-8899-aAbBcCdDeEfF</j>\n"
        " <j>  00112233-4455-6677-8899-aAbBcCdDeEf0</j>\n"
        " <j>00112233-4455-6677-8899-aAbBcCdDeEf1  </j>\n"
        " <j>  00112233-4455-6677-8899-aAbBcCdDeEf2  </j>\n"
        " <k>\n"
        "  <a>foo</a>\n"
        "  <a>bar</a>\n"
        "  <a>Foo</a>\n"
        "  <a></a>\n"
        "  <b>bonk</b>\n"

        "  <b>thud</b>\n"
        "  <b></b>\n"
        "  <b>Bonk</b>\n"
        "  <b>foo</b>\n"
        " </k>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Get an enumeration value string */
    memset(&schemaTmp, 0, sizeof(schemaTmp));
    schemaTmp.pEnumTypes = enumTypes;
    UnittestLog1("\nEnumeration value string: %s\n",
                 HDK_XML_Schema_EnumValueString(&schemaTmp, EnumType_B, Enum_B_thud));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_ParseValidLocalDateTime()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 0 */ 0, Element_f, HDK_XML_BuiltinType_DateTime, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Parse the XML */
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <f>2009-12-01T17:00:00</f>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Log the parsed local DateTime */
    UnittestLog1("Member f = %s\n", DateTimeToString(*HDK_XML_Get_DateTime(&sTemp, Element_f), 0));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_ParseInvalidTypes()
{
    /* Type/value pair struct */
    typedef struct _TypeValue
    {
        HDK_XML_BuiltinType type;
        const char* pszValue;
    } TypeValue;

    /* Test cases */
#define TEST(t, s) { HDK_XML_BuiltinType_##t, s }
    TypeValue values[] =
    {
        TEST(Int, ""),
        TEST(Int, "adfasdfasdfasdf"),
        TEST(Int, "7asdf"),
        TEST(Int, "asdf7"),

        TEST(Blob, "SGVsbG8sIFdvcmxkIQo"),
        TEST(Blob, "SGVsbG8sIFdvcmxkISEK="),
        TEST(Blob, "SGVsbG8sIFdvcmxkISEhCg="),
        TEST(Blob, "SGVsbG8sI@FdvcmxkISEhCg=="),

        TEST(Bool, ""),
        TEST(Bool, "adfasdfasdfasdf"),
        TEST(Bool, "True"),
        TEST(Bool, "False"),
        TEST(Bool, " false"),
        TEST(Bool, "false "),
        TEST(Bool, " false "),

        TEST(DateTime, ""),
        TEST(DateTime, "adfasdfasdfasdf"),
        TEST(DateTime, "2009-12-02-04:28:00"),
        TEST(DateTime, "2009-12-02T04:28:00Z08:00"),
        TEST(DateTime, "2009-12-02T04:28:00-08"),
        TEST(DateTime, "2009-12T04:28:00Z"),
        TEST(DateTime, "2009-12-02"),

        TEST(Long, ""),
        TEST(Long, "adfasdfasdfasdf"),
        TEST(Long, "7asdf"),
        TEST(Long, "asdf7"),

        TEST(IPAddress, "adfasdfasdfasdf"),
        TEST(IPAddress, "192.168.1.1asdf"),
        TEST(IPAddress, "192"),
        TEST(IPAddress, "192.168.1"),
        TEST(IPAddress, "192.168.1."),
        TEST(IPAddress, "192.168.1:1"),

        TEST(MACAddress, "adfasdfasdfasdf"),
        TEST(MACAddress, "00:01:02:03:04:05asdf"),
        TEST(MACAddress, "00"),
        TEST(MACAddress, "00:01:02:03:04"),
        TEST(MACAddress, "00:01:02:03:04:"),
        TEST(MACAddress, "00:01:02:03:04.05"),

        TEST(UUID, ""),
        TEST(UUID, "adfasdfasdfasdf"),
        TEST(UUID, "00"),
        TEST(UUID, "00112233-4455-6677-8899-aAbBcCdDeEfFasdf"),
        TEST(UUID, "00112233-4455-6677-8899-aAbBcCdDeEf"),
        TEST(UUID, "00112233-4455-6677-8899.aAbBcCdDeEfF"),
        TEST(UUID, "00112233-4455-6677-8899-")
    };
#undef TEST

    /* Run the tests */
    TypeValue* valuesEnd = values + sizeof(values) / sizeof(*values);
    TypeValue* pValue;
    for (pValue = values; pValue != valuesEnd; ++pValue)
    {
        HDK_XML_Struct sTemp;

        /* Schema tree */
        HDK_XML_SchemaNode schemaNodes[] = {
            { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
            { /* 1 */ 0, Element_b, -1, 0 },
            HDK_XML_Schema_SchemaNodesEnd
        };
        schemaNodes[1].type = pValue->type;

        /* Initialize the struct */
        HDK_XML_Struct_Init(&sTemp);

        /* Test */
        UnittestLog3("%s%s \"%s\":\n", (pValue != values ? "\n" : ""), BuiltinTypeToString(pValue->type), pValue->pszValue);
        ParseTestHelper(
            &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<a xmlns=\"http://cisco.com/\">\n"
            " <b>", pValue->pszValue, "</b>\n"
            "</a>\n",
            ParseTestHelperEnd);

        /* Free the struct */
        HDK_XML_Struct_Free(&sTemp);
    }
}


void Test_ParseInvalidXML()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Int, 0 },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_Struct, 0 },
        { /* 3 */ 0, Element_Cisco_Foo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
        { /* 4 */ 2, Element_d, HDK_XML_BuiltinType_Int, 0 },
        { /* 5 */ 2, Element_Cisco_Bar, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
        { /* 6 */ 3, Element_a, HDK_XML_BuiltinType_Int, 0 },
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Tests... */

    UnittestLog("Missing end element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nMissing start element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " 7</b>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nMalformed element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nMismatched elements:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</c>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nUnknown element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <asdf>11</asdf>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nUnexpected top-level element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<f xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        "</f>\n",
        ParseTestHelperEnd);

    UnittestLog("\nUnexpected element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <f>11</f>\n"
        " </c>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nMissing final element:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n",
        ParseTestHelperEnd);

    UnittestLog("\nWrong namespace:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        " <Bar>9</Bar>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nWrong namespace (nested):\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 0,
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>7</b>\n"
        " <c>\n"
        "  <d>11</d>\n"
        " </c>\n"
        " <Foo xmlns=\"http://cisco.com/HNAPExt/\">\n"
        "  <a>9</a>\n"
        " </Foo>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_ParseMaxAlloc()
{
    HDK_XML_Struct sTemp;

    /* Schema tree */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_String,  HDK_XML_SchemaNodeProperty_Unbounded},
        HDK_XML_Schema_SchemaNodesEnd
    };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Tests... */

    UnittestLog("Non-zero max alloc success:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, 0, 256,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>1</b>\n"
        " <b>2</b>\n"
        " <b>3</b>\n"
        " <b>4</b>\n"
        " <b>5</b>\n"
        " <b>6</b>\n"
        "</a>\n",
        ParseTestHelperEnd);

    UnittestLog("\nNon-zero max alloc failure:\n");
    ParseTestHelper(
        &sTemp, schemaNodes, 0, 0, ParseTestHelper_NoSerialize | ParseTestHelper_NoValidate, 256,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<a xmlns=\"http://cisco.com/\">\n"
        " <b>1</b>\n"
        " <b>2</b>\n"
        " <b>3</b>\n"
        " <b>4</b>\n"
        " <b>5</b>\n"
        " <b>6</b>\n"
        " <b>0123456789012345678901234567890123456789</b>\n"
        " <b>0123456789012345678901234567890123456789</b>\n"
        " <b>0123456789012345678901234567890123456789</b>\n"
        " <b>0123456789012345678901234567890123456789</b>\n"
        " <b>0123456789012345678901234567890123456789</b>\n"
        "</a>\n",
        ParseTestHelperEnd);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}

void Test_ParseBlob()
{
    const char* s_input[] =
      {
          "3q2+7xXwDQ==",
          "3q2+7xXwDQ=",
          "0x43",
          "efasd3+",
          " ",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        char* pData = 0;
        unsigned int cbData = 0;

        int fAlloc;
        for (fAlloc = 1; fAlloc >= 0; fAlloc--)
        {
            if (HDK_XML_Parse_Blob(pData, &cbData, s_input[ix]))
            {
                if (fAlloc)
                {
                    pData = (char*)malloc(cbData);
                }
                else
                {
                    UnittestLog1("Parsed '%s' into blob\n", s_input[ix]);
                }
            }
            else
            {
                UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
                break;
            }
        }

        free(pData);
    }
}

void Test_ParseBool()
{
    const char* s_input[] =
      {
          "1",
          "0",
          "True",
          "FaLse",
          "tru",
          "true",
          "false",
          "334234",
          "\\ ",
          "\t",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        int f;
        if (HDK_XML_Parse_Bool(&f, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into boolean\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseDateTime()
{
    const char* s_input[] =
      {
          "1970-01-01T06:30:52Z",
          "1970-01-01T06:30:61Z",
          "1970-01-01T06:30:52",
          "asdasdwe",
          "\t",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        time_t t;
        if (HDK_XML_Parse_DateTime(&t, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into time_t\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseInt()
{
    const char* s_input[] =
      {
          "-2932",
          "0",
          "12445452",
          "foo",
          "\n",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        HDK_XML_Int i;
        if (HDK_XML_Parse_Int(&i, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into HDK_XML_Int\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseIPAddress()
{
    const char* s_input[] =
      {
          "0.0.3.2",
          "4",
          "?df?.?asd\\",
          "0x32.2.3.5",
          "\t",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        HDK_XML_IPAddress ip;
        if (HDK_XML_Parse_IPAddress(&ip, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into HDK_XML_IPAddress\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseLong()
{
    const char* s_input[] =
      {
          "-12382734932",
          "0",
          "124453453452",
          "foo",
          "\n",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        HDK_XML_Long l;
        if (HDK_XML_Parse_Long(&l, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into HDK_XML_Long\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseMACAddress()
{
    const char* s_input[] =
      {
          "00:00:ac:ed:33:01",
          "0x00:00:aa:dd:33:01",
          "\n",
          ""
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        HDK_XML_MACAddress mac;
        if (HDK_XML_Parse_MACAddress(&mac, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into HDK_XML_MACAddress\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}

void Test_ParseUUID()
{
    const char* s_input[] =
      {
          "abcDEf12-0342-ade2-225f-aabbc3258904",
          "   abcDEf12-0342-ade2-225f-aabbc3258904  \n",
          "abcDEf12-0342-ade2-225f-sabbc3258904",
          "\n",
          "0"
      };

    size_t ix;
    for (ix = 0; ix < sizeof(s_input) / sizeof(*s_input); ix++)
    {
        HDK_XML_UUID uuid;
        if (HDK_XML_Parse_UUID(&uuid, s_input[ix]))
        {
            UnittestLog1("Parsed '%s' into HDK_XML_UUID\n", s_input[ix]);
        }
        else
        {
            UnittestLog1("Failed to parse '%s'\n", s_input[ix]);
        }
    }
}
