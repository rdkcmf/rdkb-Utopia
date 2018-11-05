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
 * hdk_util.h - Stateless utility functions.
 */

#ifndef __HDK_UTIL_H__
#define __HDK_UTIL_H__

#include "hdk_xml.h"


/* Helper functions to convert between strings and IPs. */
extern char* HDK_Util_IPToStr(char pszStr[16], HDK_XML_IPAddress* pIPAddress);
extern HDK_XML_IPAddress* HDK_Util_StrToIP(HDK_XML_IPAddress* pIPAddress, char* pszStr);

/* Helper function to convert between strings and MACs. */
extern char* HDK_Util_MACToStr(char pszStr[20], HDK_XML_MACAddress* pMacAddress);
extern HDK_XML_MACAddress* HDK_Util_StrToMAC(HDK_XML_MACAddress* pMACAddress, char* pszStr);

#endif /* __HDK_UTIL__H__ */
