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
    TEST(ArrayIter),
    TEST(BlobArray),
    TEST(Blob),
    TEST(BoolArray),
    TEST(DateTimeArray),
    TEST(DOM),
    TEST(Device),
    TEST(EnumArray),
    TEST(IntArray),
    TEST(IPv4AddressArray),
    TEST(IPv4Address),
    TEST(LongArray),
    TEST(MACAddressArray),
    TEST(MACAddress),
    TEST(StringArray),
    TEST(StructArray),
    TEST(Struct),
    TEST(UUIDArray),
    TEST(UUID),
    { 0, 0 }
};
