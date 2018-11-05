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
    TEST(ADIGet),
    TEST(ADIGetBad),
    TEST(ADIGetNoImpl),
    TEST(ADISet),
    TEST(ADISetNoImpl),
    TEST(AuthBad),
    TEST(AuthInvalid),
    TEST(AuthNone),
    TEST(BadContentLength),
    TEST(ElementPath),
    TEST(HNAPError),
    TEST(HNAPGet),
    TEST(HNAPGetFile),
    TEST(HNAPGetFileSlash),
    TEST(HNAPGetNoSlash),
    TEST(HNAPPost),
    TEST(HNAPReboot),
    TEST(InvalidRequest),
    TEST(InvalidRequest2),
    TEST(InvalidResponse),
    TEST(MultipleModules),
    TEST(NoLocationMatch),
    TEST(NonHNAP),
    TEST(NullDeviceId),
    TEST(NullNetworkObjectId),
    TEST(UnknownNetworkObjectId),
    { 0, 0 }
};
