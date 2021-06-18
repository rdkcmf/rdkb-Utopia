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
 * FileName:   pal_kernel.c
 * Author:      Jianrong xiao(jianxiao@cisco.com)
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for the IGD utiles
 *****************************************************************************/
/*$Id: pal_kernel.c,v 1.2 2009/05/13 08:53:22 jianxiao Exp $
 *
 *$Log: pal_kernel.c,v $
 *Revision 1.2  2009/05/13 08:53:22  jianxiao
 *Modify the prefix from IGD to PAL
 *
 *Revision 1.1  2009/05/13 03:13:17  jianxiao
 *create orignal version
 *

 *
 **/
#include <unistd.h>
#include <string.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pal_def.h"
#include "pal_kernel.h"


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
INT32 PAL_get_if_IpAddress(IN const CHAR *ifName, INOUT CHAR IpAddress[IP_ADDRESS_LEN])
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
  	INT32 fd;
  	INT32 ret = -1;

  	if(NULL == ifName)
    	return ret;

  	if((fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
    	return ret;

  	if(fd >= 0 )
  	{
	/*CID 135600 : BUFFER_SIZE_WARNING */
        strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name)-1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    	ifr.ifr_addr.sa_family = AF_INET;
    	if(ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    	{
      		saddr = (struct sockaddr_in *)&ifr.ifr_addr;
      		strncpy(IpAddress, inet_ntoa(saddr->sin_addr), strlen(inet_ntoa(saddr->sin_addr))+1);
      		ret = 0;
    	}
  	}

  	close(fd);
  	return ret;
}
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
INT32 PAL_get_if_MacAddress(IN const CHAR *ifName, INOUT CHAR MacAddress[MAC_ADDRESS_LEN])
{
  	struct ifreq ifr;
  	INT32 fd;
  	INT32 ret = -1;

  	if(NULL == ifName)
    	return ret;

  	if((fd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
   		return ret;
  
  	if(fd >= 0 )
  	{
	/* CID 135636 : BUFFER_SIZE_WARNING */
        strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name)-1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
    	ifr.ifr_addr.sa_family = AF_INET;
    	if(ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
    	{
      		memcpy(MacAddress, &ifr.ifr_ifru.ifru_hwaddr.sa_data, MAC_ADDRESS_LEN);
      		ret = 0;
    	}
  	}

  	close(fd);
  	return ret;
}
