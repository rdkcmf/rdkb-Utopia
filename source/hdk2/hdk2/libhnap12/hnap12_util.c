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

#include "hnap12_util.h"

#include <string.h>


/*
 * IPAdress Helpers
 */

int HNAP12_UTL_IPAddress_IsValid(HDK_XML_IPAddress* pIP, HDK_XML_IPAddress* pSubnet)
{
    if (pIP && pSubnet &&

        /* Is pIP not 0.0.0.0? */
        (pIP->a || pIP->b || pIP->c || pIP->d) &&

        /* Is the pSubnet valid */
        HNAP12_UTL_IPAddress_IsValidSubnet(pSubnet) &&

        /* Is pIP not the broadcast address for pSubnet? */
        (((unsigned char)((pIP->a & pSubnet->a) | ~pSubnet->a) != pIP->a) ||
         ((unsigned char)((pIP->b & pSubnet->b) | ~pSubnet->b) != pIP->b) ||
         ((unsigned char)((pIP->c & pSubnet->c) | ~pSubnet->c) != pIP->c) ||
         ((unsigned char)((pIP->d & pSubnet->d) | ~pSubnet->d) != pIP->d)))
    {
        return 1;
    }

    return 0;
}

int HNAP12_UTL_IPAddress_IsValidRange(HDK_XML_IPAddress* pIP, HDK_XML_IPAddress* pSubnet,
                                      HDK_XML_IPAddress* pIPFirst, HDK_XML_IPAddress* pIPLast)
{
    unsigned int uiFirstIpDec;
    unsigned int uiLastIpDec;

    if (pIP && pSubnet && pIPFirst && pIPLast &&

        /* Is the first ip within the subnet? */
        HNAP12_UTL_IPAddress_IsWithinSubnet(pIP, pSubnet, pIPFirst) &&

        /* Is the last ip within the subnet? */
        HNAP12_UTL_IPAddress_IsWithinSubnet(pIP, pSubnet, pIPLast))
    {

        /*
         * Convert the IP address to a decimal value by multiplying each
         * octet by the appropriate power of 256 and summing them together.
         * This makes it easy to determine if the last IP is greater than
         * the first.
         */
        uiFirstIpDec = (pIPFirst->a << 24) + (pIPFirst->b << 16) + (pIPFirst->c << 8) + pIPFirst->d;
        uiLastIpDec = (pIPLast->a << 24) + (pIPLast->b << 16) + (pIPLast->c << 8) + pIPLast->d;

        if (uiFirstIpDec <= uiLastIpDec)
        {
            return 1;
        }
    }

    return 0;
}

int HNAP12_UTL_IPAddress_IsValidSubnet(HDK_XML_IPAddress* pSubnet)
{
    unsigned int uiIP;

    if (!pSubnet ||

        /* Is the Class A octet is not 255, then this is not a valid subnet */
        (pSubnet->a != 255))
    {
        return 0;
    }

    /*
     * Convert the subnet mask to a decimal value by multiplying each
     * octet by the appropriate power of 256 and summing them together.
     */
    uiIP = (pSubnet->a << 24) + (pSubnet->b << 16) + (pSubnet->c << 8) + pSubnet->d;

    /* Shift value until all of the 0's on right are gone */
    for (; uiIP > 0 && !(uiIP & 0x01); uiIP >>= 1) {}

    /* Keep shifting to the right, making sure that the value is always odd */
    for (; uiIP > 0;)
    {
        if (!(uiIP & 0x01))
        {
            return 0;
        }

        uiIP >>= 1;
    }

    return 1;
}

int HNAP12_UTL_IPAddress_IsWithinSubnet(HDK_XML_IPAddress* pIP, HDK_XML_IPAddress* pSubnet,
                                        HDK_XML_IPAddress* pIPAddress)
{
    if (pIP && pSubnet && pIPAddress &&

        /* Is pSubnet valid? */
        HNAP12_UTL_IPAddress_IsValidSubnet(pSubnet) &&

        /* Is pIPAddress in subnet of pIP/pSubnet? */
        (pIP->a & pSubnet->a) == (pIPAddress->a & pSubnet->a) &&
        (pIP->b & pSubnet->b) == (pIPAddress->b & pSubnet->b) &&
        (pIP->c & pSubnet->c) == (pIPAddress->c & pSubnet->c) &&
        (pIP->d & pSubnet->d) == (pIPAddress->d & pSubnet->d))
    {
        return 1;
    }

    return 0;
}


/*
 * Password/SSID/character Helpers
 */

int HNAP12_UTL_AdminPassword_IsValid(char* pszPassword)
{
    if (pszPassword)
    {
        return strlen(pszPassword) >= 1 && HNAP12_UTL_Ascii_IsValid(pszPassword);
    }

    return 0;
}

int HNAP12_UTL_Password_IsValid(char* pszPassword)
{
    if (pszPassword)
    {
        size_t len = strlen(pszPassword);
        size_t lenOK = strspn(pszPassword, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
        return len >= 4 && len <= 32 && len == lenOK;
    }

    return 0;
}

int HNAP12_UTL_SSID_IsValid(char* pszSSID)
{
    if (pszSSID)
    {
        size_t len = strlen(pszSSID);
        return len >= 1 && len <= 32;
    }

    return 0;
}

int HNAP12_UTL_Ascii_IsValid(char* pszStr)
{
    if (pszStr)
    {
        return strlen(pszStr) == strspn(pszStr, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    }

    return 0;
}

int HNAP12_UTL_Hex_IsValid(char* pszStr)
{
    if (pszStr)
    {
        return strlen(pszStr) == strspn(pszStr, "0123456789ABCDEFabcdef");
    }

    return 0;
}


/*
 * WEP/WPA Helpers
 */

int HNAP12_UTL_WPAKey_IsValid(char* pszKey, int fAllowHex)
{
    int fIsAscii;

    /* Validate the key length */
    size_t lenKey = strlen(pszKey);
    if (fAllowHex && lenKey == 64)
    {
        fIsAscii = 0;
    }
    else if (lenKey >= 8 && lenKey <= 64)
    {
        fIsAscii = 1;
    }
    else
    {
        return 0;
    }

    /* Validate the key's characters */
    if ((fIsAscii && HNAP12_UTL_Ascii_IsValid(pszKey)) ||
        (!fIsAscii && HNAP12_UTL_Hex_IsValid(pszKey)))
    {
        return 1;
    }

    return 0;
}

int HNAP12_UTL_WEPKey_IsValid(char* pszKey)
{
    /* Validate the key length */
    size_t lenKey = strlen(pszKey);
    if ((lenKey == 10 || lenKey == 26) &&

        /* Valid hex? */
        HNAP12_UTL_Hex_IsValid(pszKey))
    {
        return 1;
    }

    return 0;
}
