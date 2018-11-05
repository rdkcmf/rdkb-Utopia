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
#include "unittest_tests.h"


/* Unit tests table helper macro */
#define TEST(testFn) \
    { #testFn, Test_##testFn }


/* Unit test table */
UnittestNode g_Unittests[] =
{
    TEST(InputStreamBuffer),
    TEST(OutputStreamBuffer),
    TEST(ParseBlob),
    TEST(ParseBool),
    TEST(ParseCSV),
    TEST(ParseDateTime),
    TEST(ParseEx),
    TEST(ParseInt),
    TEST(ParseInvalidTypes),
    TEST(ParseInvalidXML),
    TEST(ParseIPAddress),
    TEST(ParseLong),
    TEST(ParseMACAddress),
    TEST(ParseMaxAlloc),
    TEST(ParseProperties),
    TEST(ParseSimple),
    TEST(ParseUUID),
    TEST(ParseValidTypes),
    TEST(ParseValidLocalDateTime),
    TEST(SerializeBlob),
    TEST(SerializeBool),
    TEST(SerializeCSV),
    TEST(SerializeDateTime),
    TEST(SerializeErrorOutput),
    TEST(SerializeInt),
    TEST(SerializeIPAddress),
    TEST(SerializeLong),
    TEST(SerializeMACAddress),
    TEST(SerializeString),
    TEST(SerializeUUID),
    TEST(Struct),
    TEST(StructBlank),
    TEST(StructBlob),
    TEST(StructBool),
    TEST(StructDateTime),
    TEST(StructEnum),
    TEST(StructInt),
    TEST(StructIPAddress),
    TEST(StructLong),
    TEST(StructMACAddress),
    TEST(StructMember),
    TEST(StructString),
    TEST(TypeInt),
    TEST(TypeLong),
    { 0, 0 }
};
