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
#include "hdk_xml_type.h"
#include "hdk_xml_log.h"

#include <ctype.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Struct initialization/free
 */

void HDK_XML_Struct_Init(HDK_XML_Struct* pStruct)
{
    pStruct->node.element = HDK_XML_BuiltinElement_Unknown;
    pStruct->node.type = HDK_XML_BuiltinType_Struct;
    pStruct->node.pNext = 0;
    pStruct->pHead = 0;
    pStruct->pTail = 0;
}

void HDK_XML_Struct_Free(HDK_XML_Struct* pStruct)
{
    /* Free the struct members */
    HDK_XML_Member* pMember = pStruct->pHead;
    while (pMember)
    {
        HDK_XML_Member* pMemberNext = pMember->pNext;
        HDK_XML_CallTypeFn_Free(pMember->type, pMember);
        pMember = pMemberNext;
    }

    /* Clear the struct */
    pStruct->pHead = 0;
    pStruct->pTail = 0;
}


/*
 * Generic member accessors
 */

HDK_XML_Member* HDK_XML_Set_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_Member* pMember)
{
    HDK_XML_Member* pMemberDst = HDK_XML_Get_Member(pStruct, element, pMember->type);
    if (pMemberDst)
    {
        return HDK_XML_SetEx_Member(pMemberDst, pMember);
    }
    else
    {
        return HDK_XML_Append_Member(pStruct, element, pMember);
    }
}

HDK_XML_Member* HDK_XML_SetEx_Member(HDK_XML_Member* pMemberDst, const HDK_XML_Member* pMember)
{
    if (pMemberDst->type != pMember->type ||
        !HDK_XML_CallTypeFn_Copy(pMember->type, pMemberDst, pMember))
    {
        HDK_XML_CallTypeFn_Free(pMember->type, pMemberDst);
        pMemberDst = 0;
    }
    return pMemberDst;
}

HDK_XML_Member* HDK_XML_Append_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_Member* pMember)
{
    HDK_XML_Member* pMemberDst = HDK_XML_CallTypeFn_New(pMember->type);
    if (pMemberDst)
    {
        if (!HDK_XML_CallTypeFn_Copy(pMember->type, pMemberDst, pMember))
        {
            HDK_XML_CallTypeFn_Free(pMember->type, pMemberDst);
            pMemberDst = 0;
        }
        else
        {
            HDK_XML_Struct_AddMember(pStruct, pMemberDst, element, pMember->type);
        }
    }
    return pMemberDst;
}

HDK_XML_Member* HDK_XML_Get_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type)
{
    if (pStruct)
    {
        HDK_XML_Member* pMember;
        for (pMember = pStruct->pHead; pMember; pMember = pMember->pNext)
        {
            if (pMember->element == element && pMember->type == type)
            {
                return pMember;
            }
        }
    }
    return 0;
}


/*
 * Struct type
 */

static HDK_XML_Member* typeFn_New_Struct(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Struct));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Struct;
        ((HDK_XML_Struct*)pMember)->pHead = 0;
        ((HDK_XML_Struct*)pMember)->pTail = 0;
    }
    return pMember;
}

static void typeFn_Free_Struct(HDK_XML_Member* pMember)
{
    HDK_XML_Struct_Free((HDK_XML_Struct*)pMember);
    free(pMember);
}

static int typeFn_Copy_Struct(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    HDK_XML_Member* pChildSrc;

    /* Free the dest struct */
    HDK_XML_Struct_Free((HDK_XML_Struct*)pMember);

    /* Copy each member */
    for (pChildSrc = ((HDK_XML_Struct*)pMemberSrc)->pHead; pChildSrc; pChildSrc = pChildSrc->pNext)
    {
        HDK_XML_Member* pChild = HDK_XML_CallTypeFn_New(pChildSrc->type);
        if (!pChild || !HDK_XML_CallTypeFn_Copy(pChildSrc->type, pChild, pChildSrc))
        {
            return 0;
        }
        HDK_XML_Struct_AddMember((HDK_XML_Struct*)pMember, pChild, pChildSrc->element, pChildSrc->type);
    }

    return 1;
}

HDK_XML_Struct* HDK_XML_Set_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Struct* pMember = HDK_XML_Get_Struct(pStruct, element);
    if (pMember)
    {
        HDK_XML_Struct_Free(pMember);
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Struct(pStruct, element);
    }
}

HDK_XML_Struct* HDK_XML_SetEx_Struct(HDK_XML_Struct* pStructDst, HDK_XML_Element element, const HDK_XML_Struct* pStruct)
{
    return (HDK_XML_Struct*)HDK_XML_Set_Member(pStructDst, element, (HDK_XML_Member*)pStruct);
}

HDK_XML_Struct* HDK_XML_Append_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Struct* pMember = (HDK_XML_Struct*)HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Struct);
    if (pMember)
    {
        pMember->pHead = 0;
        pMember->pTail = 0;
        HDK_XML_Struct_AddMember(pStruct, (HDK_XML_Member*)pMember, element, HDK_XML_BuiltinType_Struct);
    }
    return pMember;
}

HDK_XML_Struct* HDK_XML_AppendEx_Struct(HDK_XML_Struct* pStructDst, HDK_XML_Element element, const HDK_XML_Struct* pStruct)
{
    return (HDK_XML_Struct*)HDK_XML_Append_Member(pStructDst, element, (HDK_XML_Member*)pStruct);
}

HDK_XML_Struct* HDK_XML_Get_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return (HDK_XML_Struct*)HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Struct);
}

HDK_XML_Struct* HDK_XML_GetMember_Struct(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_Struct ? (HDK_XML_Struct*)pMember : 0);
}


/*
 * Blank type
 */

static HDK_XML_Member* typeFn_New_Blank(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Blank;
    }
    return pMember;
}

static void typeFn_Free_Blank(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_Blank(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    /* Unused parameters */
    (void)pMember;
    (void)pMemberSrc;

    return 1;
}

HDK_XML_Member* HDK_XML_Set_Blank(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Blank);
    if (pMember)
    {
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Blank(pStruct, element);
    }
}

HDK_XML_Member* HDK_XML_Append_Blank(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Blank);
    if (pMember)
    {
        HDK_XML_Struct_AddMember(pStruct, (HDK_XML_Member*)pMember, element, HDK_XML_BuiltinType_Blank);
    }
    return pMember;
}


/*
 * Blob type
 */

static HDK_XML_Member* typeFn_New_Blob(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_Blob));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Blob;
        ((HDK_XML_Member_Blob*)pMember)->pValue = 0;
        ((HDK_XML_Member_Blob*)pMember)->cbValue = 0;
    }
    return pMember;
}

static void typeFn_Free_Blob(HDK_XML_Member* pMember)
{
    free(((HDK_XML_Member_Blob*)pMember)->pValue);
    free(pMember);
}

static int typeFn_Copy_Blob_Helper(HDK_XML_Member* pMember, const char* pValue, unsigned int cbValue)
{
    /* Duplicate the blob */
    char* pValueNew = (char*)malloc(cbValue);
    if (pValueNew)
    {
        memcpy(pValueNew, pValue, cbValue);

        /* Free the old blob */
        free(((HDK_XML_Member_Blob*)pMember)->pValue);

        /* Set the new blob */
        ((HDK_XML_Member_Blob*)pMember)->pValue = pValueNew;
        ((HDK_XML_Member_Blob*)pMember)->cbValue = cbValue;

        return 1;
    }

    return 0;
}

static int typeFn_Copy_Blob(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    return typeFn_Copy_Blob_Helper(pMember, ((HDK_XML_Member_Blob*)pMemberSrc)->pValue,
                                   ((HDK_XML_Member_Blob*)pMemberSrc)->cbValue);
}

static int typeFn_Deserialize_Blob(HDK_XML_Member* pMember, const char* pszValue)
{
    char* pValueNew;

    /* Determine the blob size */
    unsigned int cbValue = (unsigned int)strlen(pszValue);
    unsigned int cbDecoded;
    HDK_XML_OutputStream_EncodeBase64_Context base64Ctx;
    memset(&base64Ctx, 0, sizeof(base64Ctx));
    base64Ctx.pfnStream = HDK_XML_OutputStream_Null;
    if (!HDK_XML_OutputStream_DecodeBase64(&cbDecoded, &base64Ctx, pszValue, cbValue) ||
        !HDK_XML_OutputStream_DecodeBase64Done(&base64Ctx))
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize blob value:\n%s\n", pszValue);
        return 0;
    }

    /* Allocate the blob buffer */
    pValueNew = (char*)malloc(cbDecoded);
    if (pValueNew)
    {
        /* Base64 decode the blob */
        HDK_XML_OutputStream_BufferContext bufferCtx;
        memset(&bufferCtx, 0, sizeof(bufferCtx));
        bufferCtx.pBuf = pValueNew;
        bufferCtx.cbBuf = cbDecoded;

        base64Ctx.pfnStream = HDK_XML_OutputStream_Buffer;
        base64Ctx.pStreamCtx = &bufferCtx;
        base64Ctx.state = 0;
        base64Ctx.prev = 0;
        if (!HDK_XML_OutputStream_DecodeBase64(&cbDecoded, &base64Ctx, pszValue, cbValue) ||
            !HDK_XML_OutputStream_DecodeBase64Done(&base64Ctx))
        {
            /* Free the new (failed) node */
            free(pValueNew);

            return 0;
        }

        /* Free the old blob */
        free(((HDK_XML_Member_Blob*)pMember)->pValue);

        /* Set the new blob */
        ((HDK_XML_Member_Blob*)pMember)->pValue = pValueNew;
        ((HDK_XML_Member_Blob*)pMember)->cbValue = cbDecoded;
    }

    return 1;
}

static int typeFn_Serialize_Blob(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Member* pMember)
{
    /* Base64 encode the blob */
    char* pValue = ((HDK_XML_Member_Blob*)pMember)->pValue;
    unsigned int cbValue = ((HDK_XML_Member_Blob*)pMember)->cbValue;
    unsigned int cbEncoded1;
    unsigned int cbEncoded2;
    HDK_XML_OutputStream_EncodeBase64_Context base64Ctx;
    memset(&base64Ctx, 0, sizeof(base64Ctx));
    base64Ctx.pfnStream = pfnStream;
    base64Ctx.pStreamCtx = pStreamCtx;
    if (!HDK_XML_OutputStream_EncodeBase64(&cbEncoded1, &base64Ctx, pValue, cbValue) ||
        !HDK_XML_OutputStream_EncodeBase64Done(&cbEncoded2, &base64Ctx))
    {
        return 0;
    }

    /* Return the result */
    *pcbStream = cbEncoded1 + cbEncoded2;
    return 1;
}

HDK_XML_Member* HDK_XML_Set_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pValue, unsigned int cbValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Blob);
    if (pMember)
    {
        return (typeFn_Copy_Blob_Helper(pMember, pValue, cbValue) ? pMember : 0);
    }
    else
    {
        return HDK_XML_Append_Blob(pStruct, element, pValue, cbValue);
    }
}

HDK_XML_Member* HDK_XML_Append_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pValue, unsigned int cbValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Blob);
    if (pMember)
    {
        if (typeFn_Copy_Blob_Helper(pMember, pValue, cbValue))
        {
            HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_Blob);
        }
        else
        {
            HDK_XML_CallTypeFn_Free(HDK_XML_BuiltinType_Blob, pMember);
            pMember = 0;
        }
    }

    return (HDK_XML_Member*)pMember;
}

char* HDK_XML_Get_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, unsigned int* pcbValue)
{
    return HDK_XML_GetMember_Blob(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Blob), pcbValue);
}

char* HDK_XML_GetMember_Blob(HDK_XML_Member* pMember, unsigned int* pcbValue)
{
    if (pMember && pMember->type == HDK_XML_BuiltinType_Blob)
    {
        *pcbValue = ((HDK_XML_Member_Blob*)pMember)->cbValue;
        return ((HDK_XML_Member_Blob*)pMember)->pValue;
    }
    else
    {
        return 0;
    }
}


/*
 * Bool type
 */

static HDK_XML_Member* typeFn_New_Bool(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_Bool));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Bool;
    }
    return pMember;
}

static void typeFn_Free_Bool(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_Bool(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_Bool*)pMember)->fValue = ((HDK_XML_Member_Bool*)pMemberSrc)->fValue;
    return 1;
}

static int typeFn_Deserialize_Bool(HDK_XML_Member* pMember, const char* pszValue)
{
    if (strcmp(pszValue, "true") == 0)
    {
        ((HDK_XML_Member_Bool*)pMember)->fValue = 1;
        return 1;
    }
    else if (strcmp(pszValue, "false") == 0)
    {
        ((HDK_XML_Member_Bool*)pMember)->fValue = 0;
        return 1;
    }
    else
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize Bool value:\n%s\n", pszValue);
        return 0;
    }
}

static int typeFn_Serialize_Bool(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Member* pMember)
{
    int fValue = ((HDK_XML_Member_Bool*)pMember)->fValue;
    char* pszValue = (fValue ? (char*)"true" : (char*)"false");

    return pfnStream(pcbStream, pStreamCtx, pszValue, (unsigned int)strlen(pszValue));
}

HDK_XML_Member* HDK_XML_Set_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Bool);
    if (pMember)
    {
        ((HDK_XML_Member_Bool*)pMember)->fValue = fValue;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Bool(pStruct, element, fValue);
    }
}

HDK_XML_Member* HDK_XML_Append_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Bool);
    if (pMember)
    {
        ((HDK_XML_Member_Bool*)pMember)->fValue = fValue;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_Bool);
    }
    return pMember;
}

int* HDK_XML_Get_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_Bool(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Bool));
}

int HDK_XML_GetEx_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fDefault)
{
    int* pfValue = HDK_XML_Get_Bool(pStruct, element);
    return (pfValue ? *pfValue : fDefault);
}

int* HDK_XML_GetMember_Bool(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_Bool ? &((HDK_XML_Member_Bool*)pMember)->fValue : 0);
}


/*
 * DateTime type
 */

static HDK_XML_Member* typeFn_New_DateTime(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_DateTime));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_DateTime;
    }
    return pMember;
}

static void typeFn_Free_DateTime(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_DateTime(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_DateTime*)pMember)->tValue = ((HDK_XML_Member_DateTime*)pMemberSrc)->tValue;
    return 1;
}

static int typeFn_Deserialize_DateTime(HDK_XML_Member* pMember, const char* pszValue)
{
    const char* p;
    const char* pStart;
    int year, mon, mday, hour, min, sec;
    int fUTC = 0;
    int tzFactor = 1;
    int tzHour = 0;
    int tzMin = 0;
    int fResult = 0;

    /* Skip whitespace */
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* Parse date/time: \d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2} */
    pStart = p;
    if (isdigit((unsigned char)*p) && isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == '-' &&
        isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == '-' &&
        isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == 'T' &&
        isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == ':' &&
        isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == ':' &&
        isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p))
    {
        /* Get the component integers */
        year = atoi(pStart);
        mon = atoi(pStart + 5);
        mday = atoi(pStart + 8);
        hour = atoi(pStart + 11);
        min = atoi(pStart + 14);
        sec = atoi(pStart + 17);

        /* Time zone offset? */
        if (*++p == '+' || *p == '-')
        {
            fUTC = 1;
            if (*p == '-')
            {
                tzFactor = -1;
            }

            /* Parse offset: \d{2}:\d{2} */
            if (isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p) && *++p == ':' &&
                isdigit((unsigned char)*++p) && isdigit((unsigned char)*++p))
            {
                tzHour = atoi(pStart + 20);
                tzMin = atoi(pStart + 23);

                /* Skip whitespace */
                for (++p; isspace((unsigned char)*p); ++p) {}
                fResult = !*p;
            }
        }
        else
        {
            /* UTC time? */
            if (*p == 'Z')
            {
                fUTC = 1;
                ++p;
            }

            /* Skip whitespace */
            for (; isspace((unsigned char)*p); ++p) {}
            fResult = !*p;
        }

        /* Make the time */
        if (fResult)
        {
            time_t result = HDK_XML_mktime(year, mon, mday, hour, min, sec, fUTC);
            if (result != -1)
            {
                result += tzFactor * (tzHour * 60 * 60 + tzMin * 60);
                ((HDK_XML_Member_DateTime*)pMember)->tValue = result;
            }
            else
            {
                fResult = 0;
            }
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize DateTime value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_DateTime(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                     const HDK_XML_Member* pMember)
{
    char szValue[32];
    struct tm t;
    HDK_XML_gmtime(((HDK_XML_Member_DateTime*)pMember)->tValue, &t);

    sprintf(szValue, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_DateTime);
    if (pMember)
    {
        ((HDK_XML_Member_DateTime*)pMember)->tValue = tValue;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_DateTime(pStruct, element, tValue);
    }
}

HDK_XML_Member* HDK_XML_Append_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_DateTime);
    if (pMember)
    {
        ((HDK_XML_Member_DateTime*)pMember)->tValue = tValue;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_DateTime);
    }
    return pMember;
}

time_t* HDK_XML_Get_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_DateTime(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_DateTime));
}

time_t HDK_XML_GetEx_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tDefault)
{
    time_t* ptValue = HDK_XML_Get_DateTime(pStruct, element);
    return (ptValue ? *ptValue : tDefault);
}

time_t* HDK_XML_GetMember_DateTime(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_DateTime ? &((HDK_XML_Member_DateTime*)pMember)->tValue : 0);
}


/*
 * DateTime helper functions
 */

time_t HDK_XML_mktime(int year, int mon, int mday, int hour, int min, int sec, int fUTC)
{
    time_t result;
    struct tm t;

    /* Compute local time_t */
    t.tm_year = year - 1900;
    t.tm_mon = mon - 1;
    t.tm_mday = mday;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = -1;
    result = mktime(&t);

    /* If UTC time, translate to local time_t */
    if (result != -1 && fUTC)
    {
        /* Compute time_t difference between the local time and UTC time */
        time_t utc;
        struct tm tUTC;
        HDK_XML_gmtime(result, &tUTC);
        tUTC.tm_isdst = t.tm_isdst;
        utc = mktime(&tUTC);
        if (utc == -1)
        {
            result = -1;
        }
        else
        {
            result += (result - utc);
        }
    }

    return result;
}

void HDK_XML_localtime(time_t t, struct tm* ptm)
{
#ifdef _MSC_VER
    localtime_s(ptm, &t);
#else
    localtime_r(&t, ptm);
#endif
}

void HDK_XML_gmtime(time_t t, struct tm* ptm)
{
#ifdef _MSC_VER
    gmtime_s(ptm, &t);
#else
    gmtime_r(&t, ptm);
#endif
}


/*
 * Int type
 */

static HDK_XML_Member* typeFn_New_Int(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_Int));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Int;
    }
    return pMember;
}

static void typeFn_Free_Int(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_Int(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_Int*)pMember)->iValue = ((HDK_XML_Member_Int*)pMemberSrc)->iValue;
    return 1;
}

static int typeFn_Deserialize_Int(HDK_XML_Member* pMember, const char* pszValue)
{
    int fResult = 0;

    /* Skip whitespace */
    const char* p;
    const char* pStart;
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* Positive/negative? */
    pStart = p;
    if (*p == '-' || *p == '+')
    {
        ++p;
    }

    /* Must be at least one digit */
    if (isdigit((unsigned char)*p))
    {
        /* Skip digits */
        for (; isdigit((unsigned char)*p); ++p) {}

        /* Skip whitespace */
        for (; isspace((unsigned char)*p); ++p) {}

        /* Get the integer */
        if (!*p)
        {
            ((HDK_XML_Member_Int*)pMember)->iValue = atoi(pStart);
            fResult = 1;
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize Int value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_Int(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                const HDK_XML_Member* pMember)
{
    char szValue[32];
    HDK_XML_Int iValue = ((HDK_XML_Member_Int*)pMember)->iValue;

    sprintf(szValue, "%d", (int)iValue);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Int);
    if (pMember)
    {
        ((HDK_XML_Member_Int*)pMember)->iValue = iValue;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Int(pStruct, element, iValue);
    }
}

HDK_XML_Member* HDK_XML_Append_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Int);
    if (pMember)
    {
        ((HDK_XML_Member_Int*)pMember)->iValue = iValue;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_Int);
    }
    return pMember;
}

HDK_XML_Int* HDK_XML_Get_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_Int(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Int));
}

HDK_XML_Int HDK_XML_GetEx_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iDefault)
{
    HDK_XML_Int* piValue = HDK_XML_Get_Int(pStruct, element);
    return (piValue ? *piValue : iDefault);
}

HDK_XML_Int* HDK_XML_GetMember_Int(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_Int ? &((HDK_XML_Member_Int*)pMember)->iValue : 0);
}


/*
 * Long type
 */

static HDK_XML_Member* typeFn_New_Long(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_Long));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_Long;
    }
    return pMember;
}

static void typeFn_Free_Long(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_Long(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_Long*)pMember)->llValue = ((HDK_XML_Member_Long*)pMemberSrc)->llValue;
    return 1;
}

static int typeFn_Deserialize_Long(HDK_XML_Member* pMember, const char* pszValue)
{
    int fResult = 0;

    /* Skip whitespace */
    const char* p;
    const char* pStart;
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* Positive/negative? */
    pStart = p;
    if (*p == '-' || *p == '+')
    {
        ++p;
    }

    /* Must be at least one digit */
    if (isdigit((unsigned char)*p))
    {
        /* Skip digits */
        for (; isdigit((unsigned char)*p); ++p) {}

        /* Skip whitespace */
        for (; isspace((unsigned char)*p); ++p) {}

        /* Get the integer */
        if (!*p)
        {
#ifdef _MSC_VER
            ((HDK_XML_Member_Long*)pMember)->llValue = _atoi64(pStart);
#else
            ((HDK_XML_Member_Long*)pMember)->llValue = atoll(pStart);
#endif
            fResult = 1;
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize Long value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_Long(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Member* pMember)
{
    char szValue[32];
    HDK_XML_Long llValue = ((HDK_XML_Member_Long*)pMember)->llValue;

    sprintf(szValue, "%lld", (long long int)llValue);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Long);
    if (pMember)
    {
        ((HDK_XML_Member_Long*)pMember)->llValue = llValue;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Long(pStruct, element, llValue);
    }
}

HDK_XML_Member* HDK_XML_Append_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_Long);
    if (pMember)
    {
        ((HDK_XML_Member_Long*)pMember)->llValue = llValue;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_Long);
    }
    return pMember;
}

HDK_XML_Long* HDK_XML_Get_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_Long(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_Long));
}

HDK_XML_Long HDK_XML_GetEx_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llDefault)
{
    HDK_XML_Long* pllValue = HDK_XML_Get_Long(pStruct, element);
    return (pllValue ? *pllValue : llDefault);
}

HDK_XML_Long* HDK_XML_GetMember_Long(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_Long ? &((HDK_XML_Member_Long*)pMember)->llValue : 0);
}


/*
 * String type
 */

static HDK_XML_Member* typeFn_New_String(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_String));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_String;
        ((HDK_XML_Member_String*)pMember)->pszValue = 0;
    }
    return pMember;
}

static void typeFn_Free_String(HDK_XML_Member* pMember)
{
    free(((HDK_XML_Member_String*)pMember)->pszValue);
    free(pMember);
}

static int typeFn_Copy_String_Helper(HDK_XML_Member* pMember, const char* pszValue)
{
    /* Duplicate the string */
    size_t cbValue = strlen(pszValue) + 1;
    char* pszValueNew = (char*)malloc(cbValue);
    if (pszValueNew)
    {
        strcpy(pszValueNew, pszValue);

        /* Free the old string */
        free(((HDK_XML_Member_String*)pMember)->pszValue);

        /* Set the new string */
        ((HDK_XML_Member_String*)pMember)->pszValue = pszValueNew;

        return 1;
    }
    else
    {
        return 0;
    }
}

static int typeFn_Copy_String(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    return typeFn_Copy_String_Helper(pMember, ((HDK_XML_Member_String*)pMemberSrc)->pszValue);
}

static int typeFn_Deserialize_String(HDK_XML_Member* pMember, const char* pszValue)
{
    return typeFn_Copy_String_Helper(pMember, pszValue);
}

static int typeFn_Serialize_String(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                   const HDK_XML_Member* pMember)
{
    char* pszValue = ((HDK_XML_Member_String*)pMember)->pszValue;

    return pfnStream(pcbStream, pStreamCtx, pszValue, (unsigned int)strlen(pszValue));
}

HDK_XML_Member* HDK_XML_Set_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_String);
    if (pMember)
    {
        return (typeFn_Copy_String_Helper(pMember, pszValue) ? pMember : 0);
    }
    else
    {
        return HDK_XML_Append_String(pStruct, element, pszValue);
    }
}

HDK_XML_Member* HDK_XML_Append_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_String);
    if (pMember)
    {
        if (typeFn_Copy_String_Helper(pMember, pszValue))
        {
            HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_String);
        }
        else
        {
            HDK_XML_CallTypeFn_Free(HDK_XML_BuiltinType_String, pMember);
            pMember = 0;
        }
    }
    return pMember;
}

char* HDK_XML_Get_String(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_String(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_String));
}

const char* HDK_XML_GetEx_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszDefault)
{
    char* pszValue = HDK_XML_Get_String(pStruct, element);
    return (pszValue ? pszValue : pszDefault);
}

char* HDK_XML_GetMember_String(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_String ? ((HDK_XML_Member_String*)pMember)->pszValue : 0);
}


/*
 * IPAddress type
 */

static HDK_XML_Member* typeFn_New_IPAddress(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_IPAddress));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_IPAddress;
    }
    return pMember;
}

static void typeFn_Free_IPAddress(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_IPAddress(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_IPAddress*)pMember)->ipAddress.a = ((HDK_XML_Member_IPAddress*)pMemberSrc)->ipAddress.a;
    ((HDK_XML_Member_IPAddress*)pMember)->ipAddress.b = ((HDK_XML_Member_IPAddress*)pMemberSrc)->ipAddress.b;
    ((HDK_XML_Member_IPAddress*)pMember)->ipAddress.c = ((HDK_XML_Member_IPAddress*)pMemberSrc)->ipAddress.c;
    ((HDK_XML_Member_IPAddress*)pMember)->ipAddress.d = ((HDK_XML_Member_IPAddress*)pMemberSrc)->ipAddress.d;
    return 1;
}

static int typeFn_Deserialize_IPAddress(HDK_XML_Member* pMember, const char* pszValue)
{
    int fResult = 0;

    /* Skip whitespace */
    const char* p;
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* Blank values are allowed */
    if (!*p)
    {
        HDK_XML_IPAddress* pIPAddress = &((HDK_XML_Member_IPAddress*)pMember)->ipAddress;
        memset(pIPAddress, 0, sizeof(*pIPAddress));
        fResult = 1;
    }
    else if (isdigit((unsigned char)*p))
    {
        /* First int */
        const char* pA = p;
        for (; isdigit((unsigned char)*p); ++p) {}
        if (*p == '.' && p - pA <= 3 && isdigit((unsigned char)*++p))
        {
            /* Second int */
            const char* pB = p;
            for (; isdigit((unsigned char)*p); ++p) {}
            if (*p == '.' && p - pB <= 3 && isdigit((unsigned char)*++p))
            {
                /* Third int */
                const char* pC = p;
                for (; isdigit((unsigned char)*p); ++p) {}
                if (*p == '.' && p - pC <= 3 && isdigit((unsigned char)*++p))
                {
                    /* Fourth int */
                    const char* pD = p;
                    for (; isdigit((unsigned char)*p); ++p) {}
                    if (p - pD <= 3)
                    {
                        /* Skip whitespace */
                        for (; isspace((unsigned char)*p); ++p) {}

                        /* Get the component integers */
                        if (!*p)
                        {
                            int a = atoi(pA);
                            int b = atoi(pB);
                            int c = atoi(pC);
                            int d = atoi(pD);
                            if (a <= 255 && b <= 255 && c <= 255 && d <= 255)
                            {
                                HDK_XML_IPAddress* pIPAddress = &((HDK_XML_Member_IPAddress*)pMember)->ipAddress;
                                pIPAddress->a = (unsigned char)a;
                                pIPAddress->b = (unsigned char)b;
                                pIPAddress->c = (unsigned char)c;
                                pIPAddress->d = (unsigned char)d;
                                fResult = 1;
                            }
                        }
                    }
                }
            }
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize IPAddress value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_IPAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                      const HDK_XML_Member* pMember)
{
    char szValue[32];
    HDK_XML_IPAddress* pIPAddress = &((HDK_XML_Member_IPAddress*)pMember)->ipAddress;

    sprintf(szValue, "%d.%d.%d.%d",
            (int)pIPAddress->a, (int)pIPAddress->b, (int)pIPAddress->c, (int)pIPAddress->d);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pIPAddress)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_IPAddress);
    if (pMember)
    {
        ((HDK_XML_Member_IPAddress*)pMember)->ipAddress = *pIPAddress;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_IPAddress(pStruct, element, pIPAddress);
    }
}

HDK_XML_Member* HDK_XML_Append_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pIPAddress)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_IPAddress);
    if (pMember)
    {
        ((HDK_XML_Member_IPAddress*)pMember)->ipAddress = *pIPAddress;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_IPAddress);
    }
    return pMember;
}

HDK_XML_IPAddress* HDK_XML_Get_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_IPAddress(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_IPAddress));
}

const HDK_XML_IPAddress* HDK_XML_GetEx_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pDefault)
{
    HDK_XML_IPAddress* pValue = HDK_XML_Get_IPAddress(pStruct, element);
    return (pValue ? pValue : pDefault);
}

HDK_XML_IPAddress* HDK_XML_GetMember_IPAddress(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_IPAddress ? &((HDK_XML_Member_IPAddress*)pMember)->ipAddress : 0);
}

int HDK_XML_IsEqual_IPAddress(const HDK_XML_IPAddress* pIPAddress1, const HDK_XML_IPAddress* pIPAddress2)
{
    return (pIPAddress1->a == pIPAddress2->a &&
            pIPAddress1->b == pIPAddress2->b &&
            pIPAddress1->c == pIPAddress2->c &&
            pIPAddress1->d == pIPAddress2->d);
}


/*
 * MACAddress type
 */

static HDK_XML_Member* typeFn_New_MACAddress(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_MACAddress));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_MACAddress;
    }
    return pMember;
}

static void typeFn_Free_MACAddress(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_MACAddress(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.a = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.a;
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.b = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.b;
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.c = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.c;
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.d = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.d;
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.e = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.e;
    ((HDK_XML_Member_MACAddress*)pMember)->macAddress.f = ((HDK_XML_Member_MACAddress*)pMemberSrc)->macAddress.f;
    return 1;
}

static int typeFn_Deserialize_MACAddress(HDK_XML_Member* pMember, const char* pszValue)
{
    int fResult = 0;

    /* Skip whitespace */
    const char* p;
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* Blank values are allowed */
    if (!*p)
    {
        HDK_XML_MACAddress* pMACAddress = &((HDK_XML_Member_MACAddress*)pMember)->macAddress;
        memset(pMACAddress, 0, sizeof(*pMACAddress));
        fResult = 1;
    }
    else
    {
        /* MAC address: \x{2}:\x{2}:\x{2}:\x{2}:\x{2}:\x{2} */
        const char* pStart = p;
        if (isxdigit((unsigned char)*p) && isxdigit((unsigned char)*++p) && *++p == ':' &&
            isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == ':' &&
            isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == ':' &&
            isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == ':' &&
            isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == ':' &&
            isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p))
        {
            /* Skip whitespace */
            for (++p; isspace((unsigned char)*p); ++p) {}
            if (!*p)
            {
                /* Get the component integers */
                HDK_XML_MACAddress* pMACAddress = &((HDK_XML_Member_MACAddress*)pMember)->macAddress;
                char* pEnd;
                pMACAddress->a = (unsigned char)strtol(pStart, &pEnd, 16);
                pMACAddress->b = (unsigned char)strtol(pStart + 3, &pEnd, 16);
                pMACAddress->c = (unsigned char)strtol(pStart + 6, &pEnd, 16);
                pMACAddress->d = (unsigned char)strtol(pStart + 9, &pEnd, 16);
                pMACAddress->e = (unsigned char)strtol(pStart + 12, &pEnd, 16);
                pMACAddress->f = (unsigned char)strtol(pStart + 15, &pEnd, 16);
                fResult = 1;
            }
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize MACAddress value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_MACAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                       const HDK_XML_Member* pMember)
{
    char szValue[32];
    HDK_XML_MACAddress* pMACAddress = &((HDK_XML_Member_MACAddress*)pMember)->macAddress;

    sprintf(szValue, "%02X:%02X:%02X:%02X:%02X:%02X",
            (int)pMACAddress->a, (int)pMACAddress->b, (int)pMACAddress->c,
            (int)pMACAddress->d, (int)pMACAddress->e, (int)pMACAddress->f);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pMACAddress)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_MACAddress);
    if (pMember)
    {
        ((HDK_XML_Member_MACAddress*)pMember)->macAddress = *pMACAddress;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_MACAddress(pStruct, element, pMACAddress);
    }
}

HDK_XML_Member* HDK_XML_Append_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pMACAddress)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_MACAddress);
    if (pMember)
    {
        ((HDK_XML_Member_MACAddress*)pMember)->macAddress = *pMACAddress;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_MACAddress);
    }
    return pMember;
}

HDK_XML_MACAddress* HDK_XML_Get_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_MACAddress(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_MACAddress));
}

const HDK_XML_MACAddress* HDK_XML_GetEx_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pDefault)
{
    HDK_XML_MACAddress* pValue = HDK_XML_Get_MACAddress(pStruct, element);
    return (pValue ? pValue : pDefault);
}

HDK_XML_MACAddress* HDK_XML_GetMember_MACAddress(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_MACAddress ? &((HDK_XML_Member_MACAddress*)pMember)->macAddress : 0);
}

int HDK_XML_IsEqual_MACAddress(const HDK_XML_MACAddress* pMACAddress1, const HDK_XML_MACAddress* pMACAddress2)
{
    return (pMACAddress1->a == pMACAddress2->a &&
            pMACAddress1->b == pMACAddress2->b &&
            pMACAddress1->c == pMACAddress2->c &&
            pMACAddress1->d == pMACAddress2->d &&
            pMACAddress1->e == pMACAddress2->e &&
            pMACAddress1->f == pMACAddress2->f);
}


/*
 * UUID type
 */

static HDK_XML_Member* typeFn_New_UUID(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_UUID));
    if (pMember)
    {
        pMember->type = HDK_XML_BuiltinType_UUID;
    }
    return pMember;
}

static void typeFn_Free_UUID(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_UUID(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    memcpy(&((HDK_XML_Member_UUID*)pMember)->uuid, &((HDK_XML_Member_UUID*)pMemberSrc)->uuid,
           sizeof(((HDK_XML_Member_UUID*)pMember)->uuid));
    return 1;
}

static int typeFn_Deserialize_UUID(HDK_XML_Member* pMember, const char* pszValue)
{
    int fResult = 0;
    const char* pStart;

    /* Skip whitespace */
    const char* p;
    for (p = pszValue; isspace((unsigned char)*p); ++p) {}

    /* UUID: \x{8}-\x{4}-\x{4}-\x{4}-\x{12} */
    pStart = p;
    if (isxdigit((unsigned char)*p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == '-' &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == '-' &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == '-' &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) && *++p == '-' &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p) &&
        isxdigit((unsigned char)*++p) && isxdigit((unsigned char)*++p))
    {
        /* Skip whitespace */
        for (++p; isspace((unsigned char)*p); ++p) {}
        if (!*p)
        {
            /* Get the component integers */
            unsigned int val[16];
            if (16 == sscanf(pStart, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                             &val[0], &val[1], &val[2], &val[3], &val[4], &val[5], &val[6], &val[7],
                             &val[8], &val[9], &val[10], &val[11], &val[12], &val[13], &val[14], &val[15]))
            {
                int i;
                for (i = 0; i < 16; ++i)
                {
                    ((HDK_XML_Member_UUID*)pMember)->uuid.bytes[i] = (unsigned char)val[i];
                }
                fResult = 1;
            }
        }
    }

#ifdef HDK_LOGGING
    if (!fResult)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize UUID value:\n%s\n", pszValue);
    }
#endif

    return fResult;
}

static int typeFn_Serialize_UUID(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Member* pMember)
{
    char szValue[48];
    HDK_XML_UUID* pUUID = &((HDK_XML_Member_UUID*)pMember)->uuid;

    sprintf(szValue, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            (int)pUUID->bytes[0], (int)pUUID->bytes[1], (int)pUUID->bytes[2], (int)pUUID->bytes[3],
            (int)pUUID->bytes[4], (int)pUUID->bytes[5], (int)pUUID->bytes[6], (int)pUUID->bytes[7],
            (int)pUUID->bytes[8], (int)pUUID->bytes[9], (int)pUUID->bytes[10], (int)pUUID->bytes[11],
            (int)pUUID->bytes[12], (int)pUUID->bytes[13], (int)pUUID->bytes[14], (int)pUUID->bytes[15]);

    return pfnStream(pcbStream, pStreamCtx, szValue, (unsigned int)strlen(szValue));
}

HDK_XML_Member* HDK_XML_Set_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pUUID)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_UUID);
    if (pMember)
    {
        memcpy(&((HDK_XML_Member_UUID*)pMember)->uuid, pUUID, sizeof(((HDK_XML_Member_UUID*)pMember)->uuid));
        return pMember;
    }
    else
    {
        return HDK_XML_Append_UUID(pStruct, element, pUUID);
    }
}

HDK_XML_Member* HDK_XML_Append_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pUUID)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(HDK_XML_BuiltinType_UUID);
    if (pMember)
    {
        memcpy(&((HDK_XML_Member_UUID*)pMember)->uuid, pUUID, sizeof(((HDK_XML_Member_UUID*)pMember)->uuid));
        HDK_XML_Struct_AddMember(pStruct, pMember, element, HDK_XML_BuiltinType_UUID);
    }
    return pMember;
}

HDK_XML_UUID* HDK_XML_Get_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    return HDK_XML_GetMember_UUID(HDK_XML_Get_Member(pStruct, element, HDK_XML_BuiltinType_UUID));
}

const HDK_XML_UUID* HDK_XML_GetEx_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pDefault)
{
    HDK_XML_UUID* pValue = HDK_XML_Get_UUID(pStruct, element);
    return (pValue ? pValue : pDefault);
}

HDK_XML_UUID* HDK_XML_GetMember_UUID(HDK_XML_Member* pMember)
{
    return (pMember && pMember->type == HDK_XML_BuiltinType_UUID ? &((HDK_XML_Member_UUID*)pMember)->uuid : 0);
}

int HDK_XML_IsEqual_UUID(const HDK_XML_UUID* pUUID1, const HDK_XML_UUID* pUUID2)
{
    return memcmp(pUUID1->bytes, pUUID2->bytes, sizeof(pUUID1->bytes)) == 0;
}


/*
 * Enumeration type
 */

typedef struct _HDK_XML_Member_Enum
{
    HDK_XML_Member node;
    int iValue;
} HDK_XML_Member_Enum;

static HDK_XML_Member* typeFn_New_Enum(void)
{
    HDK_XML_Member* pMember = (HDK_XML_Member*)malloc(sizeof(HDK_XML_Member_Enum));
    if (pMember)
    {
        pMember->type = -1;
    }
    return pMember;
}

static void typeFn_Free_Enum(HDK_XML_Member* pMember)
{
    free(pMember);
}

static int typeFn_Copy_Enum(HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    ((HDK_XML_Member_Enum*)pMember)->iValue = ((HDK_XML_Member_Enum*)pMemberSrc)->iValue;
    return 1;
}

static int HDK_XML_TypeFn_Deserialize_Enum(HDK_XML_Member* pMember, const char* pszValue, HDK_XML_EnumType ppszValues)
{
    HDK_XML_EnumType ppszValue;
    for (ppszValue = ppszValues; *ppszValue; ++ppszValue)
    {
        if (**ppszValue == *pszValue && strcmp(*ppszValue, pszValue) == 0)
        {
            ((HDK_XML_Member_Enum*)pMember)->iValue = (int)(ppszValue - ppszValues);
            return 1;
        }
    }

    HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to deserialize enum value:\n%s\n", pszValue);

    /* Unknown enumeration value */
    ((HDK_XML_Member_Enum*)pMember)->iValue = HDK_XML_Enum_Unknown;
    return 1;
}

static int HDK_XML_TypeFn_Serialize_Enum(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                         const HDK_XML_Member* pMember, HDK_XML_EnumType ppszValues)
{
    int iValue = ((HDK_XML_Member_Enum*)pMember)->iValue;
    HDK_XML_EnumValue pszValue = (iValue == HDK_XML_Enum_Unknown ? "" : *(ppszValues + iValue));

    return pfnStream(pcbStream, pStreamCtx, pszValue, (unsigned int)strlen(pszValue));
}

HDK_XML_Member* HDK_XML_Set_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iValue)
{
    HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, type);
    if (pMember)
    {
        ((HDK_XML_Member_Enum*)pMember)->iValue = iValue;
        return pMember;
    }
    else
    {
        return HDK_XML_Append_Enum(pStruct, element, type, iValue);
    }
}

HDK_XML_Member* HDK_XML_Append_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iValue)
{
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(type);
    if (pMember)
    {
        ((HDK_XML_Member_Enum*)pMember)->iValue = iValue;
        HDK_XML_Struct_AddMember(pStruct, pMember, element, type);
    }
    return pMember;
}

int* HDK_XML_Get_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type)
{
    return HDK_XML_GetMember_Enum(HDK_XML_Get_Member(pStruct, element, type), type);
}

int HDK_XML_GetEx_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iDefault)
{
    int* piValue = HDK_XML_Get_Enum(pStruct, element, type);
    return (piValue ? *piValue : iDefault);
}

int* HDK_XML_GetMember_Enum(HDK_XML_Member* pMember, HDK_XML_Type type)
{
    return (pMember && pMember->type == type ? &((HDK_XML_Member_Enum*)pMember)->iValue : 0);
}


/*
 * Type interface
 */

static struct
{
    HDK_XML_TypeFn_New pfnNew;
    HDK_XML_TypeFn_Free pfnFree;
    HDK_XML_TypeFn_Copy pfnCopy;
    HDK_XML_TypeFn_Deserialize pfnDeserialize;
    HDK_XML_TypeFn_Serialize pfnSerialize;
} s_builtinTypes[] =
{
    { /* HDK_XML_Type_Struct */ typeFn_New_Struct, typeFn_Free_Struct, typeFn_Copy_Struct, 0, 0 },
    { /* HDK_XML_Type_Blank */ typeFn_New_Blank, typeFn_Free_Blank, typeFn_Copy_Blank, 0, 0 },
    { /* HDK_XML_Type_Blob */ typeFn_New_Blob, typeFn_Free_Blob, typeFn_Copy_Blob, typeFn_Deserialize_Blob, typeFn_Serialize_Blob },
    { /* HDK_XML_Type_Bool */ typeFn_New_Bool, typeFn_Free_Bool, typeFn_Copy_Bool, typeFn_Deserialize_Bool, typeFn_Serialize_Bool },
    { /* HDK_XML_Type_DateTime */ typeFn_New_DateTime, typeFn_Free_DateTime, typeFn_Copy_DateTime, typeFn_Deserialize_DateTime, typeFn_Serialize_DateTime },
    { /* HDK_XML_Type_Int */ typeFn_New_Int, typeFn_Free_Int, typeFn_Copy_Int, typeFn_Deserialize_Int, typeFn_Serialize_Int },
    { /* HDK_XML_Type_Long */ typeFn_New_Long, typeFn_Free_Long, typeFn_Copy_Long, typeFn_Deserialize_Long, typeFn_Serialize_Long },
    { /* HDK_XML_Type_String */ typeFn_New_String, typeFn_Free_String, typeFn_Copy_String, typeFn_Deserialize_String, typeFn_Serialize_String },
    { /* HDK_XML_Type_IPAddress */ typeFn_New_IPAddress, typeFn_Free_IPAddress, typeFn_Copy_IPAddress, typeFn_Deserialize_IPAddress, typeFn_Serialize_IPAddress },
    { /* HDK_XML_Type_MACAddress */ typeFn_New_MACAddress, typeFn_Free_MACAddress, typeFn_Copy_MACAddress, typeFn_Deserialize_MACAddress, typeFn_Serialize_MACAddress },
    { /* HDK_XML_Type_UUID */ typeFn_New_UUID, typeFn_Free_UUID, typeFn_Copy_UUID, typeFn_Deserialize_UUID, typeFn_Serialize_UUID },
};

HDK_XML_Member* HDK_XML_CallTypeFn_New(HDK_XML_Type type)
{
    HDK_XML_TypeFn_New pfnNew = (type < 0 ? typeFn_New_Enum : s_builtinTypes[type].pfnNew);
    return (pfnNew ? pfnNew() : 0);
}

void HDK_XML_CallTypeFn_Free(HDK_XML_Type type, HDK_XML_Member* pMember)
{
    HDK_XML_TypeFn_Free pfnFree = (type < 0 ? typeFn_Free_Enum : s_builtinTypes[type].pfnFree);
    if (pfnFree)
    {
        pfnFree(pMember);
    }
}

int HDK_XML_CallTypeFn_Copy(HDK_XML_Type type, HDK_XML_Member* pMember, const HDK_XML_Member* pMemberSrc)
{
    HDK_XML_TypeFn_Copy pfnCopy = (type < 0 ? typeFn_Copy_Enum : s_builtinTypes[type].pfnCopy);
    return (pfnCopy ? pfnCopy(pMember, pMemberSrc) : 0);
}

int HDK_XML_CallTypeFn_Deserialize(HDK_XML_Type type, const HDK_XML_EnumType* pEnumTypes,
                                   HDK_XML_Member* pMember, const char* pszValue)
{
    if (type < 0)
    {
        return HDK_XML_TypeFn_Deserialize_Enum(pMember, pszValue, pEnumTypes[-type - 1]);
    }
    else
    {
        HDK_XML_TypeFn_Deserialize pfnDeserialize = s_builtinTypes[type].pfnDeserialize;
        return (pfnDeserialize ? pfnDeserialize(pMember, pszValue) : 1);
    }
}

int HDK_XML_CallTypeFn_Serialize(HDK_XML_Type type, const HDK_XML_EnumType* pEnumTypes,
                                 unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Member* pMember)
{
    if (type < 0)
    {
        return HDK_XML_TypeFn_Serialize_Enum(pcbStream, pfnStream, pStreamCtx, pMember, pEnumTypes[-type - 1]);
    }
    else
    {
        HDK_XML_TypeFn_Serialize pfnSerialize = s_builtinTypes[type].pfnSerialize;
        if (pfnSerialize)
        {
            return pfnSerialize(pcbStream, pfnStream, pStreamCtx, pMember);
        }
        else
        {
            *pcbStream = 0;
            return 1;
        }
    }
}

void HDK_XML_Struct_AddMember(HDK_XML_Struct* pStruct, HDK_XML_Member* pMember,
                              HDK_XML_Element element, HDK_XML_Type type)
{
    pMember->element = element;
    pMember->type = type;
    pMember->pNext = 0;
    if (!pStruct->pHead)
    {
        pStruct->pHead = pMember;
        pStruct->pTail = pMember;
    }
    else
    {
        pStruct->pTail->pNext = pMember;
        pStruct->pTail = pMember;
    }
}
