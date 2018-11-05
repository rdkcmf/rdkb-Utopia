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

#ifndef __UNITTEST_H__
#define __UNITTEST_H__

#include <stdio.h>

/* Logging helpers */
#define UnittestLog(s) \
    printf(s)
#define UnittestLog1(s, a1) \
    printf(s, a1)
#define UnittestLog2(s, a1, a2) \
    printf(s, a1, a2)
#define UnittestLog3(s, a1, a2, a3) \
    printf(s, a1, a2, a3)
#define UnittestLog4(s, a1, a2, a3, a4) \
    printf(s, a1, a2, a3, a4)
#define UnittestLog5(s, a1, a2, a3, a4, a5) \
    printf(s, a1, a2, a3, a4, a5)
#define UnittestLog6(s, a1, a2, a3, a4, a5, a6) \
    printf(s, a1, a2, a3, a4, a5, a6)

/* Unittest stream functions */
#define UnittestStream HDK_XML_OutputStream_File
#define UnittestStreamCtx stdout

/* Unit test function definition */
typedef void (*UnittestFn)();

/* Unit test node definition */
typedef struct _UnittestNode
{
    const char* pszTestName;
    UnittestFn pfnTest;
} UnittestNode;

/* Unit test table */
extern UnittestNode g_Unittests[];

#endif /* __UNITTEST_H__ */
