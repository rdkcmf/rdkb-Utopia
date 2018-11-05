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

#ifndef __UNITTEST_TESTS_H__
#define __UNITTEST_TESTS_H__

/* Unit tests */
extern void Test_ADIGet(void);
extern void Test_ADIGetBad(void);
extern void Test_ADIGetNoImpl(void);
extern void Test_ADISet(void);
extern void Test_ADISetNoImpl(void);
extern void Test_AuthBad(void);
extern void Test_AuthInvalid(void);
extern void Test_AuthNone(void);
extern void Test_BadContentLength(void);
extern void Test_ElementPath(void);
extern void Test_HNAPError(void);
extern void Test_HNAPGet(void);
extern void Test_HNAPGetFile(void);
extern void Test_HNAPGetFileSlash(void);
extern void Test_HNAPGetNoSlash(void);
extern void Test_HNAPPost(void);
extern void Test_HNAPReboot(void);
extern void Test_InvalidRequest(void);
extern void Test_InvalidRequest2(void);
extern void Test_InvalidResponse(void);
extern void Test_MultipleModules(void);
extern void Test_NoLocationMatch(void);
extern void Test_NonHNAP(void);
extern void Test_NullDeviceId(void);
extern void Test_NullNetworkObjectId(void);
extern void Test_UnknownNetworkObjectId(void);

#endif /* __UNITTEST_TESTS_H__ */
