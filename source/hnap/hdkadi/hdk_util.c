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
 * hdk_util.c - Stateless utility functions
 */

#include "hdk_util.h"

#include <stdio.h>
#include "safec_lib_common.h"

char* HDK_Util_IPToStr(char pszStr[16], HDK_XML_IPAddress* pIPAddress)
{
    errno_t safec_rc = -1;
    if (!pIPAddress){
        return 0;
    }

    safec_rc = sprintf_s(pszStr, 16,"%u.%u.%u.%u",
                pIPAddress->a, pIPAddress->b,
                pIPAddress->c, pIPAddress->d);
    if(safec_rc < EOK)
    {
          ERR_CHK(safec_rc);
          return 0;
    }
    return pszStr;
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
    errno_t safec_rc = -1;
    if (!pMacAddress){
        return 0;
    }
    safec_rc = sprintf_s(pszStr, 20,"%02X:%02X:%02X:%02X:%02X:%02X",
                pMacAddress->a, pMacAddress->b,
                pMacAddress->c, pMacAddress->d,
                pMacAddress->e, pMacAddress->f);
    if(safec_rc < EOK)
    {
        ERR_CHK(safec_rc);
        return 0;
    }
    return pszStr;
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
