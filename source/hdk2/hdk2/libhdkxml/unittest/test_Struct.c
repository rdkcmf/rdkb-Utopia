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

#include "hdk_xml.h"


void Test_Struct()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    HDK_XML_Struct* pStruct;
    HDK_XML_Struct* pStruct2;
    HDK_XML_Struct* pStruct3;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append struct */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Struct(&sTemp, Element_b) ? 1 : 0));
    pStruct = HDK_XML_Set_Struct(&sTemp, Element_b);
    UnittestLog1("Set result: %d (expect 1)\n", (pStruct ? 1 : 0));
    pMember = HDK_XML_Set_Int(pStruct, Element_a, 39);
    UnittestLog1("Set int in struct: %d (expect 39)\n", *HDK_XML_GetMember_Int(pMember));
    pStruct2 = HDK_XML_Set_Struct(pStruct, Element_b);
    UnittestLog1("Set struct in struct: %d (expect 1)\n", (pStruct2 ? 1 : 0));
    pMember = HDK_XML_Set_Int(pStruct2, Element_a, 40);
    UnittestLog1("Set int in struct: %d (expect 40)\n", *HDK_XML_GetMember_Int(pMember));
    pStruct2 = HDK_XML_AppendEx_Struct(&sTemp, Element_b, pStruct);
    UnittestLog1("AppendEx result: %d (expect 1)\n", (pStruct2 ? 1 : 0));
    pMember = HDK_XML_Set_Int(pStruct2, Element_a, 41);
    UnittestLog1("Set int in 2nd struct: %d (expect 41)\n", *HDK_XML_GetMember_Int(pMember));
    UnittestLog1("Get outer int: %d (expect 39)\n", *HDK_XML_Get_Int(pStruct, Element_a));
    UnittestLog1("Get 2nd outer int: %d (expect 41)\n", *HDK_XML_Get_Int(pStruct2, Element_a));
    UnittestLog1("Get inner int: %d (expect 40)\n", *HDK_XML_Get_Int(HDK_XML_Get_Struct(pStruct, Element_b), Element_a));
    UnittestLog1("Get 2nd inner int: %d (expect 40)\n", *HDK_XML_Get_Int(HDK_XML_Get_Struct(pStruct2, Element_b), Element_a));
    pStruct3 = HDK_XML_Append_Struct(&sTemp, Element_b);
    UnittestLog1("Append result: %d (expect 1)\n", (pStruct3 ? 1 : 0));
    pStruct3 = HDK_XML_Set_Struct(&sTemp, Element_b);
    UnittestLog2("Set result: %d (expect 1), equal 1st struct? %d (expect 1)\n",
                 (pStruct3 ? 1 : 0), (pStruct == pStruct3 ? 1 : 0));
    UnittestLog1("Get outer int: %d (expect 0)\n", (HDK_XML_Get_Int(pStruct, Element_a) ? 1 : 0));
    pStruct = HDK_XML_SetEx_Struct(&sTemp, Element_b, pStruct2);
    UnittestLog2("SetEx result: %d (expect 1), equal 1st struct? %d (expect 1)\n",
                 (pStruct ? 1 : 0), (pStruct == pStruct3 ? 1 : 0));
    UnittestLog1("Get outer int: %d (expect 41)\n", *HDK_XML_Get_Int(pStruct, Element_a));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructBlank()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    HDK_XML_Member* pMember2;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Set/Append blank */
    pMember = HDK_XML_Set_Blank(&sTemp, Element_a);
    UnittestLog1("Set result: %d (expect 1)\n", (pMember ? 1 : 0));
    pMember2 = HDK_XML_Set_Blank(&sTemp, Element_a);
    UnittestLog2("2nd set result: %d (expect 1), is first member? %d (expect 1)\n",
                 (pMember2 ? 1 : 0), (pMember2 == pMember ? 1 : 0));
    pMember2 = HDK_XML_Append_Blank(&sTemp, Element_a);
    UnittestLog2("Append result: %d (expect 1), is next member? %d (expect 1)\n",
                 (pMember2 ? 1 : 0), (pMember2 == pMember->pNext ? 1 : 0));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructBlob()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    char* pBuf;
    unsigned int cbBuf;
    char buf1[] = { 0x01, 0x00, 0x02 };
    char buf2[] = { 0x02, 0x00, 0x03, 0x04 };
    char buf3[] = { 0x03, 0x00, 0x04, 0x05, 0x06 };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append blob */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Blob(&sTemp, Element_a, &cbBuf) ? 1 : 0));
    pMember = HDK_XML_Set_Blob(&sTemp, Element_a, buf1, sizeof(buf1));
    pBuf = HDK_XML_GetMember_Blob(pMember, &cbBuf);
    UnittestLog4("Set result: %d - %d, %d, %d (expect 3 - 1, 0, 2)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2]);
    pBuf = HDK_XML_Get_Blob(&sTemp, Element_a, &cbBuf);
    UnittestLog4("Get after set: %d - %d, %d, %d (expect 3 - 1, 0, 2)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2]);
    pMember = HDK_XML_Set_Blob(&sTemp, Element_a, buf2, sizeof(buf2));
    pBuf = HDK_XML_GetMember_Blob(pMember, &cbBuf);
    UnittestLog5("2nd set result: %d - %d, %d, %d, %d (expect 4 - 2, 0, 3, 4)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
    pBuf = HDK_XML_Get_Blob(&sTemp, Element_a, &cbBuf);
    UnittestLog5("Get after 2nd set: %d - %d, %d, %d, %d (expect 4 - 2, 0, 3, 4)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
    pMember = HDK_XML_Append_Blob(&sTemp, Element_a, buf3, sizeof(buf3));
    pBuf = HDK_XML_GetMember_Blob(pMember, &cbBuf);
    UnittestLog6("Append result: %d - %d, %d, %d, %d, %d (expect 5 - 3, 0, 4, 5, 6)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4]);
    pBuf = HDK_XML_Get_Blob(&sTemp, Element_a, &cbBuf);
    UnittestLog5("Get after append: %d - %d, %d, %d, %d (expect 4 - 2, 0, 3, 4)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_Blob);
    pBuf = HDK_XML_GetMember_Blob(pMember->pNext, &cbBuf);
    UnittestLog6("Get next member: %d - %d, %d, %d, %d, %d (expect 5 - 3, 0, 4, 5, 6)\n",
                 cbBuf, pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4]);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructBool()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append bool */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Bool(&sTemp, Element_a) ? 1 : 0));
    UnittestLog1("GetEx before set: %d (expect 1)\n", HDK_XML_GetEx_Bool(&sTemp, Element_a, 1));
    pMember = HDK_XML_Set_Bool(&sTemp, Element_a, 1);
    UnittestLog1("Set result: %d (expect 1)\n", *HDK_XML_GetMember_Bool(pMember));
    UnittestLog1("Get after set: %d (expect 1)\n", *HDK_XML_Get_Bool(&sTemp, Element_a));
    UnittestLog1("GetEx after set: %d (expect 1)\n", HDK_XML_GetEx_Bool(&sTemp, Element_a, 0));
    pMember = HDK_XML_Set_Bool(&sTemp, Element_a, 0);
    UnittestLog1("2nd set result: %d (expect 0)\n", *HDK_XML_GetMember_Bool(pMember));
    UnittestLog1("Get after 2nd set: %d (expect 0)\n", *HDK_XML_Get_Bool(&sTemp, Element_a));
    pMember = HDK_XML_Append_Bool(&sTemp, Element_a, 1);
    UnittestLog1("Append result: %d (expect 1)\n", *HDK_XML_GetMember_Bool(pMember));
    UnittestLog1("Get after append: %d (expect 0)\n", *HDK_XML_Get_Bool(&sTemp, Element_a));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_Bool);
    UnittestLog1("Get next member: %d (expect 1)\n", *HDK_XML_GetMember_Bool(pMember->pNext));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructDateTime()
{
    time_t t1;
    time_t t2;
    struct tm tm2;
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Test the time helper functions */
    t1 = HDK_XML_mktime(2009, 12, 1, 9, 24, 0, 0);
    UnittestLog1("Local time: %d\n", (int)t1);
    HDK_XML_gmtime(t1, &tm2);
    UnittestLog3("GM time (struct): %d, %d, %d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);
    UnittestLog3(", %d, %d, %d\n", tm2.tm_hour, tm2.tm_min, tm2.tm_sec);
    t2 = HDK_XML_mktime(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday, tm2.tm_hour, tm2.tm_min, tm2.tm_sec, 1);
    UnittestLog1("GM time converted to local time: %d\n", (int)t2);

    /* Get/Set/Append datetime */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_DateTime(&sTemp, Element_a) ? 1 : 0));
    UnittestLog1("GetEx before set: %d (expect 39)\n", (int)HDK_XML_GetEx_DateTime(&sTemp, Element_a, 39));
    pMember = HDK_XML_Set_DateTime(&sTemp, Element_a, 39);
    UnittestLog1("Set result: %d (expect 39)\n", (int)*HDK_XML_GetMember_DateTime(pMember));
    UnittestLog1("Get after set: %d (expect 39)\n", (int)*HDK_XML_Get_DateTime(&sTemp, Element_a));
    UnittestLog1("GetEx after set: %d (expect 39)\n", (int)HDK_XML_GetEx_DateTime(&sTemp, Element_a, 40));
    pMember = HDK_XML_Set_DateTime(&sTemp, Element_a, 40);
    UnittestLog1("2nd set result: %d (expect 40)\n", (int)*HDK_XML_GetMember_DateTime(pMember));
    UnittestLog1("Get after 2nd set: %d (expect 40)\n", (int)*HDK_XML_Get_DateTime(&sTemp, Element_a));
    pMember = HDK_XML_Append_DateTime(&sTemp, Element_a, 41);
    UnittestLog1("Append result: %d (expect 41)\n", (int)*HDK_XML_GetMember_DateTime(pMember));
    UnittestLog1("Get after append: %d (expect 40)\n", (int)*HDK_XML_Get_DateTime(&sTemp, Element_a));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_DateTime);
    UnittestLog1("Get next member: %d (expect 41)\n", (int)*HDK_XML_GetMember_DateTime(pMember->pNext));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructEnum()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Enumeration enum */
    typedef enum _Enums
    {
        HDK_Enum_A = -1
    } Enums;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append enum */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Enum(&sTemp, Element_a, HDK_Enum_A) ? 1 : 0));
    UnittestLog1("GetEx before set: %d (expect 39)\n", HDK_XML_GetEx_Enum(&sTemp, Element_a, HDK_Enum_A, 39));
    pMember = HDK_XML_Set_Enum(&sTemp, Element_a, HDK_Enum_A, 39);
    UnittestLog1("Set result: %d (expect 39)\n", *HDK_XML_GetMember_Enum(pMember, HDK_Enum_A));
    UnittestLog1("Get after set: %d (expect 39)\n", *HDK_XML_Get_Enum(&sTemp, Element_a, HDK_Enum_A));
    UnittestLog1("GetEx after set: %d (expect 39)\n", HDK_XML_GetEx_Enum(&sTemp, Element_a, HDK_Enum_A, 40));
    pMember = HDK_XML_Set_Enum(&sTemp, Element_a, HDK_Enum_A, 40);
    UnittestLog1("2nd set result: %d (expect 40)\n", *HDK_XML_GetMember_Enum(pMember, HDK_Enum_A));
    UnittestLog1("Get after 2nd set: %d (expect 40)\n", *HDK_XML_Get_Enum(&sTemp, Element_a, HDK_Enum_A));
    pMember = HDK_XML_Append_Enum(&sTemp, Element_a, HDK_Enum_A, 41);
    UnittestLog1("Append result: %d (expect 41)\n", *HDK_XML_GetMember_Enum(pMember, HDK_Enum_A));
    UnittestLog1("Get after append: %d (expect 40)\n", *HDK_XML_Get_Enum(&sTemp, Element_a, HDK_Enum_A));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_Enum_A);
    UnittestLog1("Get next member: %d (expect 41)\n", *HDK_XML_GetMember_Enum(pMember->pNext, HDK_Enum_A));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructInt()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append int */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Int(&sTemp, Element_a) ? 1 : 0));
    UnittestLog1("GetEx before set: %d (expect 39)\n", HDK_XML_GetEx_Int(&sTemp, Element_a, 39));
    pMember = HDK_XML_Set_Int(&sTemp, Element_a, 39);
    UnittestLog1("Set result: %d (expect 39)\n", *HDK_XML_GetMember_Int(pMember));
    UnittestLog1("Get after set: %d (expect 39)\n", *HDK_XML_Get_Int(&sTemp, Element_a));
    UnittestLog1("GetEx after set: %d (expect 39)\n", HDK_XML_GetEx_Int(&sTemp, Element_a, 40));
    pMember = HDK_XML_Set_Int(&sTemp, Element_a, 40);
    UnittestLog1("2nd set result: %d (expect 40)\n", *HDK_XML_GetMember_Int(pMember));
    UnittestLog1("Get after 2nd set: %d (expect 40)\n", *HDK_XML_Get_Int(&sTemp, Element_a));
    pMember = HDK_XML_Append_Int(&sTemp, Element_a, 41);
    UnittestLog1("Append result: %d (expect 41)\n", *HDK_XML_GetMember_Int(pMember));
    UnittestLog1("Get after append: %d (expect 40)\n", *HDK_XML_Get_Int(&sTemp, Element_a));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_Int);
    UnittestLog1("Get next member: %d (expect 41)\n", *HDK_XML_GetMember_Int(pMember->pNext));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructIPAddress()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    const HDK_XML_IPAddress* pip;
    HDK_XML_IPAddress ip1 = { 192, 168, 1, 1 };
    HDK_XML_IPAddress ip2 = { 10, 0, 0, 1 };
    HDK_XML_IPAddress ip3 = { 169, 254, 1, 1 };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append IPAddress */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_IPAddress(&sTemp, Element_a) ? 1 : 0));
    pip = HDK_XML_GetEx_IPAddress(&sTemp, Element_a, &ip1);
    UnittestLog4("GetEx before set: %d.%d.%d.%d (expect 192.168.1.1)\n", pip->a, pip->b, pip->c, pip->d);
    pMember = HDK_XML_Set_IPAddress(&sTemp, Element_a, &ip1);
    pip = HDK_XML_GetMember_IPAddress(pMember);
    UnittestLog4("Set result: %d.%d.%d.%d (expect 192.168.1.1)\n", pip->a, pip->b, pip->c, pip->d);
    pip = HDK_XML_Get_IPAddress(&sTemp, Element_a);
    UnittestLog4("Get after set: %d.%d.%d.%d (expect 192.168.1.1)\n", pip->a, pip->b, pip->c, pip->d);
    pip = HDK_XML_GetEx_IPAddress(&sTemp, Element_a, &ip2);
    UnittestLog4("GetEx after set: %d.%d.%d.%d (expect 192.168.1.1)\n", pip->a, pip->b, pip->c, pip->d);
    pMember = HDK_XML_Set_IPAddress(&sTemp, Element_a, &ip2);
    pip = HDK_XML_GetMember_IPAddress(pMember);
    UnittestLog4("2nd set result: %d.%d.%d.%d (expect 10.0.0.1)\n", pip->a, pip->b, pip->c, pip->d);
    pip = HDK_XML_Get_IPAddress(&sTemp, Element_a);
    UnittestLog4("Get after 2nd set: %d.%d.%d.%d (expect 10.0.0.1)\n", pip->a, pip->b, pip->c, pip->d);
    pMember = HDK_XML_Append_IPAddress(&sTemp, Element_a, &ip3);
    pip = HDK_XML_GetMember_IPAddress(pMember);
    UnittestLog4("Append result: %d.%d.%d.%d (expect 169.254.1.1)\n", pip->a, pip->b, pip->c, pip->d);
    pip = HDK_XML_Get_IPAddress(&sTemp, Element_a);
    UnittestLog4("Get after append: %d.%d.%d.%d (expect 10.0.0.1)\n", pip->a, pip->b, pip->c, pip->d);
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_IPAddress);
    pip = HDK_XML_GetMember_IPAddress(pMember->pNext);
    UnittestLog4("Get next member: %d.%d.%d.%d (expect 169.254.1.1)\n", pip->a, pip->b, pip->c, pip->d);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructLong()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append long */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_Long(&sTemp, Element_a) ? 1 : 0));
    UnittestLog1("GetEx before set: %lld (expect 39)\n", (long long int)HDK_XML_GetEx_Long(&sTemp, Element_a, 39));
    pMember = HDK_XML_Set_Long(&sTemp, Element_a, 39);
    UnittestLog1("Set result: %lld (expect 39)\n", (long long int)*HDK_XML_GetMember_Long(pMember));
    UnittestLog1("Get after set: %lld (expect 39)\n", (long long int)*HDK_XML_Get_Long(&sTemp, Element_a));
    UnittestLog1("GetEx after set: %lld (expect 39)\n", (long long int)HDK_XML_GetEx_Long(&sTemp, Element_a, 40));
    pMember = HDK_XML_Set_Long(&sTemp, Element_a, 40);
    UnittestLog1("2nd set result: %lld (expect 40)\n", (long long int)*HDK_XML_GetMember_Long(pMember));
    UnittestLog1("Get after 2nd set: %lld (expect 40)\n", (long long int)*HDK_XML_Get_Long(&sTemp, Element_a));
    pMember = HDK_XML_Append_Long(&sTemp, Element_a, 41);
    UnittestLog1("Append result: %lld (expect 41)\n", (long long int)*HDK_XML_GetMember_Long(pMember));
    UnittestLog1("Get after append: %lld (expect 40)\n", (long long int)*HDK_XML_Get_Long(&sTemp, Element_a));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_Long);
    UnittestLog1("Get next member: %lld (expect 41)\n", (long long int)*HDK_XML_GetMember_Long(pMember->pNext));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructMACAddress()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    const HDK_XML_MACAddress* pmac;
    HDK_XML_MACAddress mac1 = { 1, 2, 3, 4, 5, 6 };
    HDK_XML_MACAddress mac2 = { 6, 5, 4, 3, 2, 1 };
    HDK_XML_MACAddress mac3 = { 9, 7, 5, 3, 1, 0 };

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append MACAddress */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_MACAddress(&sTemp, Element_a) ? 1 : 0));
    pmac = HDK_XML_GetEx_MACAddress(&sTemp, Element_a, &mac1);
    UnittestLog6("GetEx before set: %d.%d.%d.%d.%d.%d (expect 1.2.3.4.5.6)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pMember = HDK_XML_Set_MACAddress(&sTemp, Element_a, &mac1);
    pmac = HDK_XML_GetMember_MACAddress(pMember);
    UnittestLog6("Set result: %d.%d.%d.%d.%d.%d (expect 1.2.3.4.5.6)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pmac = HDK_XML_Get_MACAddress(&sTemp, Element_a);
    UnittestLog6("Get after set: %d.%d.%d.%d.%d.%d (expect 1.2.3.4.5.6)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pmac = HDK_XML_GetEx_MACAddress(&sTemp, Element_a, &mac2);
    UnittestLog6("GetEx after set: %d.%d.%d.%d.%d.%d (expect 1.2.3.4.5.6)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pMember = HDK_XML_Set_MACAddress(&sTemp, Element_a, &mac2);
    pmac = HDK_XML_GetMember_MACAddress(pMember);
    UnittestLog6("2nd set result: %d.%d.%d.%d.%d.%d (expect 6.5.4.3.2.1)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pmac = HDK_XML_Get_MACAddress(&sTemp, Element_a);
    UnittestLog6("Get after 2nd set: %d.%d.%d.%d.%d.%d (expect 6.5.4.3.2.1)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pMember = HDK_XML_Append_MACAddress(&sTemp, Element_a, &mac3);
    pmac = HDK_XML_GetMember_MACAddress(pMember);
    UnittestLog6("Append result: %d.%d.%d.%d.%d.%d (expect 9.7.5.3.1.0)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pmac = HDK_XML_Get_MACAddress(&sTemp, Element_a);
    UnittestLog6("Get after append: %d.%d.%d.%d.%d.%d (expect 6.5.4.3.2.1)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_MACAddress);
    pmac = HDK_XML_GetMember_MACAddress(pMember->pNext);
    UnittestLog6("Get next member: %d.%d.%d.%d.%d.%d (expect 9.7.5.3.1.0)\n", pmac->a, pmac->b, pmac->c, pmac->d, pmac->e, pmac->f);

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructMember()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;
    HDK_XML_Member* pMember2;
    HDK_XML_Member* pMember3;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append member */
    pMember = HDK_XML_Set_Int(&sTemp, Element_a, 17);
    pMember = HDK_XML_Set_Member(&sTemp, Element_b, pMember);
    UnittestLog1("Set result: %d (expect 17)\n", *HDK_XML_GetMember_Int(pMember));
    pMember2 = HDK_XML_Append_Member(&sTemp, Element_b, pMember);
    UnittestLog2("Append result: %d (expect 17), is first member? %d (expect 0)\n",
                 *HDK_XML_GetMember_Int(pMember2), (pMember2 == pMember ? 1 : 0));
    pMember3 = HDK_XML_Get_Member(&sTemp, Element_b, HDK_XML_BuiltinType_Int);
    UnittestLog1("Get is first member? %d (expect 1)\n", (pMember3 == pMember ? 1 : 0));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_StructString()
{
    HDK_XML_Struct sTemp;
    HDK_XML_Member* pMember;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Get/Set/Append string */
    UnittestLog1("Get before set: %d (expect 0)\n", (HDK_XML_Get_String(&sTemp, Element_a) ? 1 : 0));
    UnittestLog1("GetEx before set: \"%s\" (expect \"foo\")\n", HDK_XML_GetEx_String(&sTemp, Element_a, "foo"));
    pMember = HDK_XML_Set_String(&sTemp, Element_a, "foo");
    UnittestLog1("Set result: \"%s\" (expect \"foo\")\n", HDK_XML_GetMember_String(pMember));
    UnittestLog1("Get after set: \"%s\" (expect \"foo\")\n", HDK_XML_Get_String(&sTemp, Element_a));
    UnittestLog1("GetEx after set: \"%s\" (expect \"foo\")\n", HDK_XML_GetEx_String(&sTemp, Element_a, "bar"));
    pMember = HDK_XML_Set_String(&sTemp, Element_a, "bar");
    UnittestLog1("2nd set result: \"%s\" (expect \"bar\")\n", HDK_XML_GetMember_String(pMember));
    UnittestLog1("Get after 2nd set: \"%s\" (expect \"bar\")\n", HDK_XML_Get_String(&sTemp, Element_a));
    pMember = HDK_XML_Append_String(&sTemp, Element_a, "thud");
    UnittestLog1("Append result: \"%s\" (expect \"thud\")\n", HDK_XML_GetMember_String(pMember));
    UnittestLog1("Get after append: \"%s\" (expect \"bar\")\n", HDK_XML_Get_String(&sTemp, Element_a));
    pMember = HDK_XML_Get_Member(&sTemp, Element_a, HDK_XML_BuiltinType_String);
    UnittestLog1("Get next member: \"%s\" (expect \"thud\")\n", HDK_XML_GetMember_String(pMember->pNext));

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}
