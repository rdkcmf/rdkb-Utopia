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

#pragma once

#include "hdk_xml.h"

/*
 * Elements
 */

typedef enum _Element
{
    Element_Blob,
    Element_BlobArray,
    Element_BoolArray,
    Element_DateTimeArray,
    Element_Enum,
    Element_EnumArray,
    Element_IPAddress,
    Element_IPAddressArray,
    Element_IntArray,
    Element_LongArray,
    Element_MACAddress,
    Element_MACAddressArray,
    Element_StringArray,
    Element_Struct,
    Element_StructArray,
    Element_UUID,
    Element_UUIDArray,
    Element_bool,
    Element_datetime,
    Element_int,
    Element_long,
    Element_string
} Element;

/* Namespace table */
extern const HDK_XML_Namespace s_namespaces[];

/* Elements table */
extern const HDK_XML_ElementNode s_elements[];

/* Enum table */
extern const HDK_XML_EnumType s_enumTypes[];

/*
 * Enum types enumeration
 */

typedef enum _EnumType
{
    EnumType_UnittestEnum = -1
} EnumType;

typedef enum _UnittestEnum
{
    UnittestEnum__UNKNOWN__ = HDK_XML_Enum_Unknown,
    UnittestEnum_UnittestEnum_Value0 = 0,
    UnittestEnum_UnittestEnum_Value1 = 1,
    UnittestEnum_UnittestEnum_Value2 = 2,
    UnittestEnum_UnittestEnum_Value3 = 3,
    UnittestEnum_UnittestEnum_Value4 = 4
} UnittestEnum;
