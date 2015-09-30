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
#include "hdk_xml_log.h"

#include <string.h>


/*
 * XML string encoding - returns non-zero for success, zero otherwise
 */

int HDK_XML_OutputStream_EncodeXML(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_EncodeXML_Context* pEncodeCtx = (HDK_XML_OutputStream_EncodeXML_Context*)pStreamCtx;
    unsigned int cbTotal = 0;

    /* XML encode the string to the stream */
    const char* pszValue = pBuf;
    const char* pszValueEnd = pBuf + cbBuf;
    while (pszValue != pszValueEnd)
    {
        unsigned int cbStream;

        /* Search for XML entities */
        const char* pszEntity = 0;
        const char* psz;
        for (psz = pszValue; psz != pszValueEnd; ++psz)
        {
            if (*psz == '\"')
            {
                pszEntity = "&quot;";
                break;
            }
            else if (*psz == '&')
            {
                pszEntity = "&amp;";
                break;
            }
            else if (*psz == '\'')
            {
                pszEntity = "&apos;";
                break;
            }
            else if (*psz == '<')
            {
                pszEntity = "&lt;";
                break;
            }
            else if (*psz == '>')
            {
                pszEntity = "&gt;";
                break;
            }
        }

        /* Stream the preceding text */
        if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, pszValue, (unsigned int)(psz - pszValue)))
        {
            return 0;
        }
        cbTotal += cbStream;

        /* Stream the XML entity, if any */
        if (pszEntity)
        {
            unsigned int cbEntity = (unsigned int)strlen(pszEntity);
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, pszEntity, cbEntity))
            {
                return 0;
            }
            cbTotal += cbStream;

            /* Increment past the entity */
            pszValue = psz + 1;
        }
        else
        {
            break;
        }
    }

    /* Return the result */
    if (pcbStream)
    {
        *pcbStream = cbTotal;
    }
    return 1;
}


/*
 * CSV string encoding - returns non-zero for success, zero otherwise
 */

int HDK_XML_OutputStream_EncodeCSV(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_EncodeCSV_Context* pEncodeCtx = (HDK_XML_OutputStream_EncodeCSV_Context*)pStreamCtx;
    unsigned int cbTotal = 0;

    /* CSV encode the string to the stream */
    const char* pszValue = pBuf;
    const char* pszValueEnd = pBuf + cbBuf;
    while (pszValue != pszValueEnd)
    {
        unsigned int cbStream;

        /* Search for CSV entities */
        const char* pszEntity = 0;
        const char* psz;
        for (psz = pszValue; psz != pszValueEnd; ++psz)
        {
            if (*psz == '\\')
            {
                pszEntity = "\\\\";
                break;
            }
            else if (*psz == ',')
            {
                pszEntity = "\\,";
                break;
            }
        }

        /* Stream the preceding text */
        if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, pszValue, (unsigned int)(psz - pszValue)))
        {
            return 0;
        }
        cbTotal += cbStream;

        /* Stream the CSV entity, if any */
        if (pszEntity)
        {
            unsigned int cbEntity = (unsigned int)strlen(pszEntity);
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, pszEntity, cbEntity))
            {
                return 0;
            }
            cbTotal += cbStream;

            /* Increment past the entity */
            pszValue = psz + 1;
        }
        else
        {
            break;
        }
    }

    /* Return the result */
    if (pcbStream)
    {
        *pcbStream = cbTotal;
    }
    return 1;
}


/*
 * CSV string decoding - returns non-zero for success, zero otherwise
 */

int HDK_XML_OutputStream_DecodeCSV(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_DecodeCSV_Context* pDecodeCtx = (HDK_XML_OutputStream_DecodeCSV_Context*)pStreamCtx;

    unsigned int cbTotal = 0;

    int fEscaped = 0;
    unsigned int ix;
    for (ix = 0; ix < cbBuf; ++ix)
    {
        unsigned int cbStream;

        /* Escape character? */
        if ('\\' == pBuf[ix])
        {
            fEscaped = !fEscaped;
            if (fEscaped)
            {
                /* Skip the escape character */
                continue;
            }
        }

        /* Invalid escaped character? */
        if (fEscaped && !((',' == pBuf[ix]) || ('\\' == pBuf[ix])))
        {
            HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Invalid CSV escaped character '%c'\n", pBuf[ix]);
            return 0;
        }

        fEscaped = 0;

        if (!pDecodeCtx->pfnStream(&cbStream, pDecodeCtx->pStreamCtx, &pBuf[ix], 1))
        {
            return 0;
        }

        cbTotal += cbStream;
    }

    if (pcbStream)
    {
        *pcbStream = cbTotal;
    }

    return 1;
}


/*
 * Base 64 encoding - returns non-zero for success, zero otherwise
 */

static char s_szBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int HDK_XML_OutputStream_EncodeBase64(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_EncodeBase64_Context* pEncodeCtx = (HDK_XML_OutputStream_EncodeBase64_Context*)pStreamCtx;
    unsigned int cbTotal = 0;

    const char* p;
    const char* pBufEnd = pBuf + cbBuf;
    for (p = pBuf; p != pBufEnd; ++p)
    {
        unsigned int cbStream;
        if (pEncodeCtx->state == 0)
        {
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, &s_szBase64[(*p & 0xFC) >> 2], 1))
            {
                return 0;
            }
            cbTotal += cbStream;
            pEncodeCtx->state = 1;
            pEncodeCtx->prev = ((*p & 0x03) << 4);
        }
        else if (pEncodeCtx->state == 1)
        {
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, &s_szBase64[pEncodeCtx->prev + ((*p & 0xF0) >> 4)], 1))
            {
                return 0;
            }
            cbTotal += cbStream;
            pEncodeCtx->state = 2;
            pEncodeCtx->prev = ((*p & 0x0F) << 2);
        }
        else if (pEncodeCtx->state == 2)
        {
            char c[2];
            c[0] = s_szBase64[pEncodeCtx->prev + ((*p & 0xC0) >> 6)];
            c[1] = s_szBase64[(*p & 0x3F)];
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, c, sizeof(c)))
            {
                return 0;
            }
            cbTotal += cbStream;
            pEncodeCtx->state = 0;
        }
    }

    if (pcbStream)
    {
        *pcbStream = cbTotal;
    }
    return 1;
}

int HDK_XML_OutputStream_EncodeBase64Done(unsigned int* pcbStream, HDK_XML_OutputStream_EncodeBase64_Context* pEncodeCtx)
{
    if (pEncodeCtx->state)
    {
        char c[3];
        c[0] = s_szBase64[pEncodeCtx->prev];
        c[1] = '=';
        c[2] = '=';
        if (!pEncodeCtx->pfnStream(pcbStream, pEncodeCtx->pStreamCtx, c, sizeof(c) + 1 - pEncodeCtx->state))
        {
            return 0;
        }
    }
    else
    {
        if (pcbStream)
        {
            *pcbStream = 0;
        }
    }

    return 1;
}


/*
 * Base64 decoding - returns non-zero for success, zero otherwise
 */

int HDK_XML_OutputStream_DecodeBase64(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_DecodeBase64_Context* pEncodeCtx = (HDK_XML_OutputStream_DecodeBase64_Context*)pStreamCtx;
    unsigned int cbTotal = 0;

    const char* p;
    const char* pBufEnd = pBuf + cbBuf;
    for (p = pBuf; p != pBufEnd; ++p)
    {
        unsigned int cbStream;
        int bits = (*p >= 'A' && *p <= 'Z' ? *p - 'A' :
                    (*p >= 'a' && *p <= 'z' ? *p - 'a' + 26 :
                     (*p >= '0' && *p <= '9' ? *p - '0' + 52 :
                      (*p == '+' ? 62 :
                       (*p == '/' ? 63 :
                        (*p == '=' ? -1 :
                         (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ? -2 : -3)))))));
        if (bits == -2)
        {
            continue;
        }
        if (bits == -3)
        {
            return 0;
        }

        if (pEncodeCtx->state == 0)
        {
            if (bits == -1)
            {
                return 0;
            }
            pEncodeCtx->state = 1;
            pEncodeCtx->prev = bits;
        }
        else if (pEncodeCtx->state == 1)
        {
            char byte;
            if (bits == -1)
            {
                return 0;
            }
            byte = (char)((pEncodeCtx->prev << 2) | ((bits & 0x30) >> 4));
            if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, &byte, 1))
            {
                return 0;
            }
            cbTotal += cbStream;
            pEncodeCtx->state = 2;
            pEncodeCtx->prev = bits;
        }
        else if (pEncodeCtx->state == 2)
        {
            int nextState = 3;
            if (bits == -1)
            {
                nextState = 4; /* == */
                bits = 0;
            }
            else
            {
                char byte = ((pEncodeCtx->prev & 0x0F) << 4) | ((bits & 0x3C) >> 2);
                if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, &byte, 1))
                {
                    return 0;
                }
                cbTotal += cbStream;
            }
            pEncodeCtx->state = nextState;
            pEncodeCtx->prev = bits;
        }
        else if (pEncodeCtx->state == 3)
        {
            int nextState = 0;
            if (bits == -1)
            {
                nextState = 5; /* = */
                bits = 0;
            }
            else
            {
                char byte = (char)(((pEncodeCtx->prev & 0x03) << 6) | bits);
                if (!pEncodeCtx->pfnStream(&cbStream, pEncodeCtx->pStreamCtx, &byte, 1))
                {
                    return 0;
                }
                cbTotal += cbStream;
            }
            pEncodeCtx->state = nextState;
            pEncodeCtx->prev = bits;
        }
        else if (pEncodeCtx->state == 4)
        {
            if (bits != -1)
            {
                return 0;
            }
            pEncodeCtx->state = 5;
        }
        else if (pEncodeCtx->state == 5)
        {
            return 0;
        }
    }

    if (pcbStream)
    {
        *pcbStream = cbTotal;
    }
    return 1;
}

int HDK_XML_OutputStream_DecodeBase64Done(HDK_XML_OutputStream_DecodeBase64_Context* pEncodeCtx)
{
    return (pEncodeCtx->state == 0 || pEncodeCtx->state == 5);
}
