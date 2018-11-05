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
 *    FileName:    pal_def.h
 *      Author:    CBG CRDC HN DEV team
 *        Date:   2009-05-12
 * Description:   The common type definition for all modules
 *****************************************************************************/
/*$Id: pal_def.h,v 1.3 2009/05/21 09:27:21 zhangli Exp $
 *
 *$Log: pal_def.h,v $
 *Revision 1.3  2009/05/21 09:27:21  zhangli
 *Added some common marco definition for the network stack
 *
 *Revision 1.2  2009/05/12 09:30:58  zhangli
 *Fixed some words
 *
 *Revision 1.1  2009/05/12 09:26:06  zhangli
 *Created the original version
 *
 *
 **/

#ifndef _PAL_DEF_
#define _PAL_DEF_

/******************************************************************************
 * Basic data types definition
 *****************************************************************************/
#ifndef UCHAR
#define UCHAR      unsigned char
#endif

#ifndef CHAR
#define CHAR       char
#endif


#ifndef UINT8
#define UINT8      unsigned char
#endif

#ifndef INT8
#define INT8       char
#endif

#ifndef UINT16
#define UINT16     unsigned short
#endif

#ifndef INT16
#define INT16      short
#endif

#ifndef UINT32
#define UINT32     unsigned int
#endif

#ifndef INT32
#define INT32      int
#endif

#ifndef BOOL
#define BOOL       unsigned char

#define BOOL_TRUE  1
#define BOOL_FALSE 0
#endif

#ifndef LOCAL
#define LOCAL      static
#endif

#ifndef LONG
#define LONG       long
#endif

#ifndef ULONG
#define ULONG      unsigned long
#endif

#ifndef VOID
#define VOID void
#endif

/******************************************************************************
 * Common marco definition for the direction of the function parameter
 *****************************************************************************/
#ifndef IN
	#define IN
#endif

#ifndef OUT
	#define OUT
#endif

#ifndef INOUT
	#define INOUT
#endif

/******************************************************************************
 * Common marco definition for the network stack
 *****************************************************************************/
#ifndef ETHER_ADDR_LEN
    #define ETHER_ADDR_LEN		6	//Ethernnet mac address lengh
#endif

#ifndef IPV4_ADDR_LEN
    #define IPV4_ADDR_LEN  		16	//IP V4 Address length
#endif

#ifndef MAC_ADDR_LEN
    #define MAC_ADDR_LEN  		18	//Ethernet MAC Address length in string
#endif


#endif //_PAL_DEF_

