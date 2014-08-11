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
 * Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 *
 * October 2008 - Sridhar Ramaswamy
 */

#ifndef _SYSCFG_MTD_H_
#define _SYSCFG_MTD_H_

int mtd_get_hdrsize ();
long int mtd_get_devicesize ();
int mtd_write_from_file (const char *mtd_device, const char *file);
int load_from_mtd (const char *mtd_device);
int commit_to_mtd (const char *mtd_device);
int mtd_hdr_check (const char *mtd_device);

#endif // _SYSCFG_MTD_H_
