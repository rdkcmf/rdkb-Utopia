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
 * Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
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

/*
 * hdk_util.c - Stateless utility functions
 */

#include "hdk_util.h"

#include <stdio.h>


char* HDK_Util_IPToStr(char pszStr[16], HDK_XML_IPAddress* pIPAddress)
{
    if (!pIPAddress)
    {
        return 0;
    }
    else
    {
        sprintf(pszStr, "%u.%u.%u.%u",
                pIPAddress->a, pIPAddress->b,
                pIPAddress->c, pIPAddress->d);

        return pszStr;
    }
}

HDK_XML_IPAddress* HDK_Util_StrToIP(HDK_XML_IPAddress* pIPAddress, char* pszStr)
{
    unsigned int a, b, c, d;

    if (!pIPAddress || !pszStr ||
        sscanf(pszStr, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255)
    {
        return 0;
    }
    else
    {
        pIPAddress->a = a;
        pIPAddress->b = b;
        pIPAddress->c = c;
        pIPAddress->d = d;

        return pIPAddress;
    }
}

char* HDK_Util_MACToStr(char pszStr[20], HDK_XML_MACAddress* pMacAddress)
{
    if (!pMacAddress)
    {
        return 0;
    }
    else
    {
        sprintf(pszStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                pMacAddress->a, pMacAddress->b,
                pMacAddress->c, pMacAddress->d,
                pMacAddress->e, pMacAddress->f);

        return pszStr;
    }
}

HDK_XML_MACAddress* HDK_Util_StrToMAC(HDK_XML_MACAddress* pMACAddress, char* pszStr)
{
    unsigned int a, b, c, d, e, f;

    if (!pMACAddress || !pszStr ||
        sscanf(pszStr, "%02X:%02X:%02X:%02X:%02X:%02X", &a, &b, &c, &d, &e, &f) != 6 ||
        a > 255 || b > 255 || c > 255 || d > 255 || e > 255 || f > 255)
    {
        return 0;
    }
    else
    {
        pMACAddress->a = a;
        pMACAddress->b = b;
        pMACAddress->c = c;
        pMACAddress->d = d;
        pMACAddress->e = e;
        pMACAddress->f = f;

        return pMACAddress;
    }
}
