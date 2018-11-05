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

#ifndef __HDK_XML_TYPE_H__
#define __HDK_XML_TYPE_H__

#include "hdk_xml.h"

/* Member types */

typedef struct _HDK_XML_Member_Blob
{
    HDK_XML_Member node;
    char* pValue;
    unsigned int cbValue;
} HDK_XML_Member_Blob;

typedef struct _HDK_XML_Member_Bool
{
    HDK_XML_Member node;
    int fValue;
} HDK_XML_Member_Bool;

typedef struct _HDK_XML_Member_DateTime
{
    HDK_XML_Member node;
    time_t tValue;
} HDK_XML_Member_DateTime;

typedef struct _HDK_XML_Member_Int
{
    HDK_XML_Member node;
    HDK_XML_Int iValue;
} HDK_XML_Member_Int;

typedef struct _HDK_XML_Member_Long
{
    HDK_XML_Member node;
    HDK_XML_Long llValue;
} HDK_XML_Member_Long;

typedef struct _HDK_XML_Member_String
{
    HDK_XML_Member node;
    char* pszValue;
} HDK_XML_Member_String;

typedef struct _HDK_XML_Member_IPAddress
{
    HDK_XML_Member node;
    HDK_XML_IPAddress ipAddress;
} HDK_XML_Member_IPAddress;

typedef struct _HDK_XML_Member_MACAddress
{
    HDK_XML_Member node;
    HDK_XML_MACAddress macAddress;
} HDK_XML_Member_MACAddress;

typedef struct _HDK_XML_Member_UUID
{
    HDK_XML_Member node;
    HDK_XML_UUID uuid;
} HDK_XML_Member_UUID;

/* Built-in type function typedefs */
typedef HDK_XML_Member* (*HDK_XML_TypeFn_New)(void);
typedef void (*HDK_XML_TypeFn_Free)(HDK_XML_Member* pMember);
typedef int (*HDK_XML_TypeFn_Copy)(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc);
typedef int (*HDK_XML_TypeFn_Deserialize)(HDK_XML_Member* pMember, const char* pszValue);
typedef int (*HDK_XML_TypeFn_Serialize)(unsigned int* pcbStream, HDK_XML_OutputStreamFn pStreamFn, void* pStreamCtx,
                                        const HDK_XML_Member* pMember);

/* Type function accessors */
extern HDK_XML_Member* HDK_XML_CallTypeFn_New(HDK_XML_Type type);
extern void HDK_XML_CallTypeFn_Free(HDK_XML_Type type, HDK_XML_Member* pMember);
extern int HDK_XML_CallTypeFn_Copy(HDK_XML_Type type, HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc);
extern int HDK_XML_CallTypeFn_Deserialize(HDK_XML_Type type, const HDK_XML_EnumType* pEnumTypes,
                                          HDK_XML_Member* pMember, const char* pszValue);
extern int HDK_XML_CallTypeFn_Serialize(HDK_XML_Type type, const HDK_XML_EnumType* pEnumTypes,
                                        unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                        const HDK_XML_Member* pMember);

/* Structure building helper function */
extern void HDK_XML_Struct_AddMember(HDK_XML_Struct* pStruct, HDK_XML_Member* pMember,
                                     HDK_XML_Element element, HDK_XML_Type type);

#endif /* __HDK_XML_TYPE_H__ */
