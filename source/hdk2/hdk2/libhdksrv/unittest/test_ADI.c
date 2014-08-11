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
#include "unittest_module.h"
#include "unittest_tests.h"
#include "unittest_util.h"

#include "hdk_srv.h"

#include <string.h>


/*
 * Method ADIGet
 */

void Method_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the ADI values */
    if (!HDK_SRV_ADIGet(pMethodCtx, ADI_Int, pOutput, Element_Struct))
    {
        HDK_XML_Set_Int(pOutput, Element_Struct, 99);
    }
    if (!HDK_SRV_ADIGet(pMethodCtx, ADI_Struct, pOutput, Element_Int))
    {
        HDK_XML_Struct* pStruct = HDK_XML_Set_Struct(pOutput, Element_Int);
        if (pStruct)
        {
            HDK_XML_Set_Int(pStruct, Element_Int, 101);
        }
    }
}


/*
 * Method ADISet
 */

void Method_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pOutput;

    /* Get the ADI values */
    if (!HDK_SRV_ADISet(pMethodCtx, ADI_Int, pInput, Element_Struct))
    {
        UnittestLog("ADI_Int set failed!\n");
    }
    if (!HDK_SRV_ADISet(pMethodCtx, ADI_Struct, pInput, Element_Int))
    {
        UnittestLog("ADI_Struct set failed!\n");
    }
}


/*
 * ADI get/set implementations
 */

static const char* ADIGetHelper(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                const char* pszName, int fBad)
{
    const char* pszValue = 0;
    HDK_XML_Struct sTemp;
    int fValue = 0;
    HDK_SRV_ADIValue value = ADI_Int;
    HDK_XML_Element element = Element_Int;
    const char* pszValueBad = 0;

    /* Unused parameters */
    (void)pMethodCtx;

    /* Initialize the temporary struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the values */
    if (strcmp(pszNamespace, "http://cisco.com/HNAP1/") == 0 &&
        strcmp(pszName, "Int") == 0)
    {
        fValue = 1;
        value = ADI_Int;
        element = Element_Int;

        if (fBad)
        {
            pszValueBad = "asd9sd";
        }
        else
        {
            HDK_XML_Set_Int(&sTemp, element, 9);
        }
    }
    else if (strcmp(pszNamespace, "http://cisco.com/HNAP1/") == 0 &&
             strcmp(pszName, "Struct") == 0)
    {
        fValue = 1;
        value = ADI_Struct;
        element = Element_Struct;

        if (fBad)
        {
            pszValueBad = "11";
        }
        else
        {
            HDK_XML_Struct* pValue = HDK_XML_Set_Struct(&sTemp, element);
            if (pValue)
            {
                HDK_XML_Set_Int(pValue, Element_Int, 11);
            }
        }
    }

    /* Deserialize the value */
    if (fValue && pszValueBad)
    {
        pszValue = HDK_SRV_ADISerializeCopy(pszValueBad);
    }
    else
    {
        const HDK_MOD_Module* pModule = HDK_SRV_ADIModule(pMethodCtx);
        pszValue = HDK_SRV_ADISerialize(pModule, value, &sTemp, element);
    }

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);

    return pszValue;
}


static const char* ADIGet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                          const char* pszName)
{
    return ADIGetHelper(pMethodCtx, pszNamespace, pszName, 0);
}


static const char* ADIGetBad(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                             const char* pszName)
{
    return ADIGetHelper(pMethodCtx, pszNamespace, pszName, 1);
}


static int ADISet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                  const char* pszName, const char* pszValue)
{
    int fResult = 0;

    /* Unused parameters */
    (void)pMethodCtx;

    /* Report the values */
    if (strcmp(pszNamespace, "http://cisco.com/HNAP1/") == 0 &&
        strcmp(pszName, "Int") == 0)
    {
        UnittestLog1("ADI_Int = %s\n", pszValue);
        fResult = 1;
    }
    else if (strcmp(pszNamespace, "http://cisco.com/HNAP1/") == 0 &&
             strcmp(pszName, "Struct") == 0)
    {
        UnittestLog1("ADI_Struct = %s\n", pszValue);
        fResult = 1;
    }

    return fResult;
}


/*
 * Tests...
 */

void Test_ADIGet()
{
    ServerTestHelperEx(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/ADIGet",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<ADIGet xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</ADIGet>\n", 0,
        ADIGet, ADISet);
}


void Test_ADIGetBad()
{
    ServerTestHelperEx(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/ADIGet",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<ADIGet xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</ADIGet>\n", 0,
        ADIGetBad, ADISet);
}


void Test_ADIGetNoImpl()
{
    ServerTestHelperEx(
        "c473989b-d162-4fb2-983c-0883149c0416",
        "POST",
        "/HNAP2/",
        "http://cisco.com/HNAP2/ADIGet",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<ADIGet xmlns=\"http://cisco.com/HNAP1/\">\n"
        "</ADIGet>\n", 0,
        ADIGet, ADISet);
}


void Test_ADISet()
{
    ServerTestHelperEx(
        "198b0022-070c-4bb9-91da-3e7e01a38944",
        "POST",
        "/HNAP1/",
        "http://cisco.com/HNAP1/ADISet",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<ADISet xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Struct>7</Struct>\n"
        " <Int>\n"
        "  <Int>5</Int>\n"
        " </Int>\n"
        "</ADISet>\n", 0,
        ADIGet, ADISet);
}


void Test_ADISetNoImpl()
{
    ServerTestHelperEx(
        "c473989b-d162-4fb2-983c-0883149c0416",
        "POST",
        "/HNAP2/",
        "http://cisco.com/HNAP2/ADISet",
        "Basic YWRtaW46cGFzc3dvcmQ=",   /* "admin:password" */

        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<ADISet xmlns=\"http://cisco.com/HNAP1/\">\n"
        " <Struct>7</Struct>\n"
        " <Int>\n"
        "  <Int>5</Int>\n"
        " </Int>\n"
        "</ADISet>\n", 0,
        ADIGet, ADISet);
}
