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
#include "unittest_module.h"
#include "unittest_tests.h"
#include "unittest_util.h"

#include "hdk_srv.h"

#include <string.h>


/*
 * Method Bar
 */

void Method_Bar(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;

    HDK_XML_Set_Int(pOutput, Element_Int, *HDK_XML_Get_Int(pInput, Element_Int) + 10);
}


/*
 * Method Foo
 */

void Method_Foo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int* pInt;

    /* Unused parameters */
    (void)pMethodCtx;

    /* Integer input? */
    pInt = HDK_XML_Get_Int(pInput, Element_Int);
    if (pInt)
    {
        /* Set error? */
        if (*pInt & 0x01)
        {
            Set_FooResult(pOutput, Element_FooResult, FooResult_ERROR);
        }

        /* Set result? */
        if (*pInt & 0x02)
        {
            HDK_XML_Set_Int(pOutput, Element_Int, *pInt + 10);
        }

        /* Invalid element? */
        if (*pInt & 0x04)
        {
            HDK_XML_Set_Int(pOutput, Element_A, *pInt + 10);
        }
    }
}


/*
 * Tests...
 */

void Test_AuthBad()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic Zm9vOmJhcg==",           /* "foo:bar" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        "</Foo>\n", 0);
}


void Test_AuthInvalid()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ",    /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        "</Foo>\n", 0);
}


void Test_AuthNone()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        0,

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        "</Foo>\n", 0);
}


void Test_BadContentLength()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        "</Foo>\n", STHO_BadContentLength);
}


void Test_ElementPath()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Bar",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
        " <soap:Body>\n"
        "  <Bar xmlns=\"http://cisco.com/HNAP1/\">\n"
        "   <Int>7</Int>\n"
        "  </Bar>\n"
        " </soap:Body>\n"
        "</soap:Envelope>\n", 0);
}

void Test_HNAPError()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>3</Int>\n"
        "</Foo>\n", 0);
}


void Test_HNAPGet()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "GET",
        "/HNAP1/",
        0,
        "Basic Zm9vOmJhcg==",           /* "foo:bar" */
        "", 0);
}


void Test_HNAPGetFile()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "GET",
        "/HNAP1/Foo",
        0,
        "Basic Zm9vOmJhcg==",           /* "foo:bar" */
        "", 0);
}


void Test_HNAPGetFileSlash()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "GET",
        "/HNAP1/Foo/",
        0,
        "Basic Zm9vOmJhcg==",           /* "foo:bar" */
        "", 0);
}


void Test_HNAPGetNoSlash()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "GET",
        "/HNAP1",
        0,
        "Basic Zm9vOmJhcg==",           /* "foo:bar" */
        "", 0);
}


void Test_HNAPPost()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        "</Foo>\n", 0);
}


void Test_HNAPReboot()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", STHO_Reboot);
}


void Test_InvalidRequest()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n", 0);
}


void Test_InvalidRequest2()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>2</Int>\n"
        " <Int>2</Int>\n"
        "</Foo>\n", 0);
}


void Test_InvalidResponse()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>4</Int>\n"
        "</Foo>\n", 0);
}


void Test_MultipleModules()
{
    ServerTestHelper(
        "c473989b-d162-4fb2-983c-0883149c0416",
        "POST",
        "/HNAP2/",
        "http://cisco.com/HNAP2/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", 0);
}


void Test_NoLocationMatch()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP99/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", 0);
}


void Test_NonHNAP()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/NonHNAP/",
        "http://cisco.com/NonHNAP/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Int>1</Int>\n"
        "</Foo>\n", 0);
}

void Test_NullDeviceId()
{
    ServerTestHelper(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", 0);
}

void Test_NullNetworkObjectId()
{
    ServerTestHelper(
        0,
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", 0);
}

void Test_UnknownNetworkObjectId()
{
    ServerTestHelper(
        "Some bogus network id",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/Foo",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Foo xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</Foo>\n", 0);
}
