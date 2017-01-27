/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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

    module: swctl_hal.h

    ---------------------------------------------------------------

    description:

        This header file gives the function call prototypes and 
        structure definitions used for the RDK-Broadband swctl for Puma6 only
  
    --------------------------------------------------------------- 

    author:

        zhicheng_qiu@cable.comcast.com 

    ---------------------------------------------------------------
*/

#ifndef __SWCTL_HAL_H__
#define __SWCTL_HAL_H__


#ifndef INT
#define INT   int
#endif


INT swctl(
	const int command, 		//-c numeric command number
	const int port, 		//-p Port number
	const int vid, 			//-v VLAN number (1-4095);   					-1: unset
	const int membertag, 	//-m Member port�s egress VLAN tag processing; 	-1: unset
	const int vlanmode,		//-q VLAN mode. Always use 1=FALLBACK; 			-1: unset,  1:fallback
	const int efm,			//-r  External Switch Enable Egress Flood Mitigation ,   -1: unset , 4: enable
	const char *mac,		//-s mac address xx:xx:xx:xx:xx:xx.   			null: unset 
	const char *magic 		//-b  magic number;   							null: unset, 0x007b: magic number
	);
	
	
//example: 	
//	swctl -c 16 -p 7 -v 100 -m 2 -q 1   	==> swctl(16, 7, 100, 2, 1, -1, null, null);
//	swctl -c 4 -p 5							==> swctl(4, 5, -1, -1, -1, null, null); 
//	swctl -c 23 -p 7 -s 01:00:5E:7F:FF:FA	==> swctl(23, 7, -1, -1, -1, "01:00:5E:7F:FF:FA", null);
//	swctl -c 11 -p 2 -r 4 -b 0x007b			==> swctl(11, 2, -1, -1, 4, "0x007b", null);

#endif
