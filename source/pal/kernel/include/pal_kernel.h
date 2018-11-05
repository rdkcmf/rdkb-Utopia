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

/**********************************************************************
 * FileName:   pal_kernel.h
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for linksys IGD
 *****************************************************************************/
/*$Id: pal_kernel.h,v 1.2 2009/05/13 08:52:59 jianxiao Exp $
 *
 *$Log: pal_kernel.h,v $
 *Revision 1.2  2009/05/13 08:52:59  jianxiao
 *Modify the prefix from IGD to PAL
 *
 *Revision 1.1  2009/05/13 03:13:13  jianxiao
 *create orignal version
 *

 *
 **/
#ifndef PAL_KERNEL_H
#define PAL_KERNEL_H

#include <stdlib.h>

#include "pal_def.h"

#define IP_ADDRESS_LEN	16
#define MAC_ADDRESS_LEN	6
#define IF_NAME_LEN	16

/************************************************************
* Function: PAL_get_if_IpAddress
*
*  Parameters: 
*	   ifName:		   IN. the interface name. 
* 
*	   IpAddress:   INOUT. the ipaddress of the interface 
* 
*  Description:
*	  This function get the ipaddress of the interface ifName.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
INT32 PAL_get_if_IpAddress(IN const CHAR *ifName, INOUT CHAR IpAddress[IP_ADDRESS_LEN]);
/************************************************************
* Function: PAL_get_if_MacAddress
*
*  Parameters: 
*	   ifName:		   IN. the interface name. 
* 
*	   IpAddress:   INOUT. the mac of the interface 
* 
*  Description:
*	  This function get the mac of the interface ifName.  
*
*  Return Values: INT32
*	   0 if successful else error code.
************************************************************/
INT32 PAL_get_if_MacAddress(IN const CHAR *ifName, INOUT CHAR MacAddress[MAC_ADDRESS_LEN]);
#endif/*PAL_KERNEL_H*/

