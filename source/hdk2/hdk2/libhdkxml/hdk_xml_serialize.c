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

#include "hdk_xml.h"
#include "hdk_xml_schema.h"
#include "hdk_xml_type.h"

#include <string.h>


/* Helper macro for streaming static text */
#define STREAM_TEXT(s) \
    STREAM_TEXT_HELPER(s, sizeof(s) - 1)

#define STREAM_STRING(s) \
    STREAM_TEXT_HELPER(s, strlen(s))

#define STREAM_TEXT_HELPER(s, cb) \
    cbString = (unsigned int)cb; \
    if (!pfnStream(&cbString, pStreamCtx, s, cbString)) \
    { \
        return 0; \
    } \
    cbStream += cbString


/* Helper function for HDK_XML_Serialize */
static int HDK_XML_Serialize_Helper(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                    const HDK_XML_SchemaInternal* pSchema, HDK_XML_Member* pMember, int options,
                                    int ixSchemaNode, int fFirstNode, unsigned int* pCSVMembersSerialized)
{
    unsigned int cbStream = 0;
    unsigned int cbString;
    HDK_XML_Struct* pStruct;

    /* Get the schema node and element node for the member and its parent */
    const HDK_XML_SchemaNode* pMemberNode = HDK_XML_Schema_GetNode(pSchema, ixSchemaNode);
    const HDK_XML_ElementNode* pMemberElement = HDK_XML_Schema_GetElementNode(pSchema, pMember->element);
    const HDK_XML_SchemaNode* pParentNode = HDK_XML_Schema_GetNode(pSchema, pMemberNode->ixParent);
    const HDK_XML_ElementNode* pParentElement = HDK_XML_Schema_GetElementNode(pSchema, pParentNode->element);

    /* Can't serialize a struct as non-XML... */
    if (pMember->type == HDK_XML_BuiltinType_Struct)
    {
        options &= ~HDK_XML_SerializeOption_NoXML;
    }

    /* Serialize the element open */
    if (!(options & HDK_XML_SerializeOption_CSV))
    {
        if (!fFirstNode ||
            !(options & HDK_XML_SerializeOption_NoXML))
        {
            STREAM_TEXT("<");
            STREAM_STRING(pMemberElement->pszElement);
            if (fFirstNode ||
                pParentElement->ixNamespace != pMemberElement->ixNamespace)
            {
                const char* pszNamespace = HDK_XML_Schema_GetNamespace(pSchema, pMemberElement->ixNamespace);
                STREAM_TEXT(" xmlns=\"");
                STREAM_STRING(pszNamespace);
                STREAM_TEXT("\">");
            }
            else
            {
                STREAM_TEXT(">");
            }
        }
    }

    /* Serialize the element value */
    pStruct = HDK_XML_GetMember_Struct(pMember);
    if (pStruct)
    {
        unsigned int ixChildBegin;
        unsigned int ixChildEnd;

        /* Output a newline after the struct open tag if not a csv struct */
        if (!(options & HDK_XML_SerializeOption_NoNewlines) &&
            !(options & HDK_XML_SerializeOption_CSV) &&
            !(pMemberNode->prop & HDK_XML_SchemaNodeProperty_CSV))
        {
            STREAM_TEXT("\n");
        }

        /* Get the struct's child tree nodes */
        if (HDK_XML_Schema_GetChildNodes(&ixChildBegin, &ixChildEnd, pSchema, ixSchemaNode))
        {
            /* Iterate the child tree nodes */
            HDK_XML_Member* pChildBegin = pStruct->pHead;
            unsigned int ixChild;
            for (ixChild = ixChildBegin; ixChild < ixChildEnd; ++ixChild)
            {
                /* Output the matching members */
                const HDK_XML_SchemaNode* pChildNode = HDK_XML_Schema_GetNode(pSchema, ixChild);
                HDK_XML_Member* pChild;
                for (pChild = pChildBegin; pChild; pChild = pChild->pNext)
                {
                    if (pChild->element == pChildNode->element)
                    {
                        /* Serialize the child member */
                        if (!(options & HDK_XML_SerializeOption_ErrorOutput) ||
                            (pChildNode->prop & HDK_XML_SchemaNodeProperty_ErrorOutput))
                        {
                            int childOptions = options;

                            /* Always serialize children as CSV when directed by schema */
                            if (pMemberNode->prop & HDK_XML_SchemaNodeProperty_CSV)
                            {
                                childOptions |= HDK_XML_SerializeOption_CSV;
                            }

                            if (!HDK_XML_Serialize_Helper(&cbString, pfnStream, pStreamCtx, pSchema, pChild,
                                                          childOptions, ixChild, 0, pCSVMembersSerialized))
                            {
                                return 0;
                            }
                            cbStream += cbString;
                        }

                        /* Update child begin - we won't match this member again */
                        if (pChild == pChildBegin)
                        {
                            pChildBegin = pChild->pNext;
                        }

                        /* Stop if not unbounded */
                        if (!(pChildNode->prop & HDK_XML_SchemaNodeProperty_Unbounded))
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    else
    {
        HDK_XML_OutputStreamFn pfnMemberStream = pfnStream;
        void* pMemberStreamCtx = pStreamCtx;

        HDK_XML_OutputStream_EncodeCSV_Context csvEncodeCtx;
        HDK_XML_OutputStream_EncodeXML_Context xmlEncodeCtx;

        if (options & HDK_XML_SerializeOption_CSV)
        {
            /* Comma-seperate members after the first member of a CSV struct */
            if (*pCSVMembersSerialized > 0)
            {
                STREAM_TEXT(",");
            }

            (*pCSVMembersSerialized)++;

            /* Wrap the member stream with the CSV stream encoder */
            memset(&csvEncodeCtx, 0, sizeof(csvEncodeCtx));
            csvEncodeCtx.pfnStream = pfnMemberStream;
            csvEncodeCtx.pStreamCtx = pMemberStreamCtx;

            pfnMemberStream = HDK_XML_OutputStream_EncodeCSV;
            pMemberStreamCtx = &csvEncodeCtx;
        }

        /* Serialize the member */
        if (!(options & HDK_XML_SerializeOption_NoXML))
        {
            /* Wrap the member stream with the XML stream encoder */
            memset(&xmlEncodeCtx, 0, sizeof(xmlEncodeCtx));
            xmlEncodeCtx.pfnStream = pfnMemberStream;
            xmlEncodeCtx.pStreamCtx = pMemberStreamCtx;

            pfnMemberStream = HDK_XML_OutputStream_EncodeXML;
            pMemberStreamCtx = &xmlEncodeCtx;
        }

        if (!HDK_XML_CallTypeFn_Serialize(pMember->type, pSchema->pEnumTypes,
                                          &cbString, pfnMemberStream, pMemberStreamCtx, pMember))
        {
            return 0;
        }

        cbStream += cbString;
    }

    if (!(options & HDK_XML_SerializeOption_CSV))
    {
        /* Serialize the element close */
        if (!fFirstNode ||
            !(options & HDK_XML_SerializeOption_NoXML))
        {
            STREAM_TEXT("</");
            STREAM_STRING(pMemberElement->pszElement);
            if (!(options & HDK_XML_SerializeOption_NoNewlines))
            {
                STREAM_TEXT(">\n");
            }
            else
            {
                STREAM_TEXT(">");
            }
        }

        *pCSVMembersSerialized = 0;
    }

    /* Return the result */
    if (pcbStream)
    {
        *pcbStream = cbStream;
    }
    return 1;
}


/* Structure XML serialization */
int HDK_XML_Serialize(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                      const HDK_XML_Schema* pSchema, const HDK_XML_Struct* pStruct, int options)
{
    return HDK_XML_SerializeEx(pcbStream, pfnStream, pStreamCtx,
                               pSchema, (const HDK_XML_Member*)pStruct, options, 0);
}


/* Structure XML serialization (specify schema starting location) */
int HDK_XML_SerializeEx(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                        const HDK_XML_Schema* pSchema, const HDK_XML_Member* pMember, int options,
                        unsigned int ixSchemaNode)
{
    unsigned int cbStream = 0;
    unsigned int cbString;
    HDK_XML_SchemaInternal schema;

    unsigned int membersSerialized = 0;

    /* Initilialize the internal schema */
    HDK_XML_Schema_Init(&schema, pSchema);

    /* Output the XML header */
    if (!(options & HDK_XML_SerializeOption_NoXML) &&
        !(options & HDK_XML_SerializeOption_CSV))
    {
        STREAM_TEXT("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        if (!(options & HDK_XML_SerializeOption_NoNewlines))
        {
            STREAM_TEXT("\n");
        }
    }

    /* Output the struct */
    if (!HDK_XML_Serialize_Helper(&cbString, pfnStream, pStreamCtx,
                                  &schema, (HDK_XML_Member*)pMember, options, ixSchemaNode, 1, &membersSerialized))
    {
        return 0;
    }
    cbStream += cbString;

    /* Return the result */
    if (pcbStream)
    {
        *pcbStream = cbStream;
    }
    return 1;
}

int HDK_XML_Serialize_Blob(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                           const char* pData, unsigned int cbData, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_Blob blobMember;
    blobMember.node.type = HDK_XML_BuiltinType_Blob;
    blobMember.pValue = (char*)pData;
    blobMember.cbValue = cbData;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_Blob, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&blobMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_Bool(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                           int fValue, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_Bool boolMember;
    memset(&boolMember, 0, sizeof(boolMember));
    boolMember.node.type = HDK_XML_BuiltinType_Bool;
    boolMember.fValue = fValue;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_Bool, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&boolMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_DateTime(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                               time_t tValue, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_DateTime datetimeMember;
    memset(&datetimeMember, 0, sizeof(datetimeMember));
    datetimeMember.node.type = HDK_XML_BuiltinType_DateTime;
    datetimeMember.tValue = tValue;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_DateTime, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&datetimeMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_Int(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                          HDK_XML_Int iValue, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_Int intMember;
    memset(&intMember, 0, sizeof(intMember));
    intMember.node.type = HDK_XML_BuiltinType_Int;
    intMember.iValue = iValue;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_Int, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&intMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_Long(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                           HDK_XML_Long llValue, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_Long longMember;
    memset(&longMember, 0, sizeof(longMember));
    longMember.node.type = HDK_XML_BuiltinType_Long;
    longMember.llValue = llValue;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_Long, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&longMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_String(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                             const char* pszValue, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_String stringMember;
    memset(&stringMember, 0, sizeof(stringMember));
    stringMember.node.type = HDK_XML_BuiltinType_String;
    stringMember.pszValue = (char*)pszValue;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_String, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&stringMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_IPAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                const HDK_XML_IPAddress* pIPAddress, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_IPAddress ipMember;
    memset(&ipMember, 0, sizeof(ipMember));
    ipMember.node.type = HDK_XML_BuiltinType_IPAddress;
    ipMember.ipAddress = *pIPAddress;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_IPAddress, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&ipMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_MACAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_MACAddress* pMACAddress, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_MACAddress macMember;
    memset(&macMember, 0, sizeof(macMember));
    macMember.node.type = HDK_XML_BuiltinType_MACAddress;
    macMember.macAddress = *pMACAddress;

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_MACAddress, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&macMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}

int HDK_XML_Serialize_UUID(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                           const HDK_XML_UUID* pUUID, int fNullTerminate)
{
    int iReturn = 0;

    HDK_XML_Member_UUID uuidMember;
    memset(&uuidMember, 0, sizeof(uuidMember));
    uuidMember.node.type = HDK_XML_BuiltinType_UUID;
    memcpy(&uuidMember.uuid, pUUID, sizeof(uuidMember.uuid));

    if (HDK_XML_CallTypeFn_Serialize(HDK_XML_BuiltinType_UUID, 0,
                                     pcbStream, pfnStream, pStreamCtx,
                                     (const HDK_XML_Member*)&uuidMember))
    {
        unsigned int cbStreamNull = 0;
        iReturn = (fNullTerminate) ? pfnStream(&cbStreamNull, pStreamCtx, "", 1) : 1;

        if (pcbStream)
        {
            *pcbStream += cbStreamNull;
        }
    }

    return iReturn;
}
