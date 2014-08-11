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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* C I/O FILE* output stream function (pStreamCtx is a FILE*) */
int HDK_XML_OutputStream_File(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }
    return (cbBuf ? fwrite(pBuf, 1, cbBuf, (FILE*)pStreamCtx) == cbBuf : 1);
}


/* Buffer output stream function (pStreamCtx is HDK_XML_OutputStream_BufferContext*) */
int HDK_XML_OutputStream_Buffer(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_BufferContext* pBufferCtx = (HDK_XML_OutputStream_BufferContext*)pStreamCtx;

    /* Copy the characters into the buffer */
    unsigned int ix;
    for (ix = 0; pBufferCtx->ixCur < pBufferCtx->cbBuf && ix < cbBuf; ++pBufferCtx->ixCur, ++ix)
    {
        pBufferCtx->pBuf[pBufferCtx->ixCur] = pBuf[ix];
    }

    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }

    return ix == cbBuf;
}


/* Grow-buffer output stream function (pStreamCtx is HDK_XML_OutputStream_BufferContext*) */
int HDK_XML_OutputStream_GrowBuffer(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_BufferContext* pBufferCtx = (HDK_XML_OutputStream_BufferContext*)pStreamCtx;

    /* Need to grow the buffer?  Add 1 byte pad for null termination. */
    unsigned int ixCurNew = pBufferCtx->ixCur + cbBuf + 1;
    if (!pBufferCtx->pBuf || ixCurNew >= pBufferCtx->cbBuf)
    {
        char* pBufNew;

        /* Determine the new buffer size */
        unsigned int cbBufNew = (pBufferCtx->pBuf ? 2 * pBufferCtx->cbBuf : pBufferCtx->cbBuf);
        if (cbBufNew < ixCurNew)
        {
            cbBufNew = ixCurNew;
        }

        /* Realloc the buffer */
        pBufNew = (char*)realloc(pBufferCtx->pBuf, cbBufNew);
        if (!pBufNew)
        {
            return 0;
        }
        pBufferCtx->pBuf = pBufNew;
        pBufferCtx->cbBuf = cbBufNew;
    }

    /* Copy the characters into the buffer */
    memcpy(pBufferCtx->pBuf + pBufferCtx->ixCur, pBuf, cbBuf);
    pBufferCtx->ixCur += cbBuf;

    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }

    return 1;
}


/* Null output stream function implementation (pStreamCtx is not used) */
int HDK_XML_OutputStream_Null(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    /* Unused parameters */
    (void)pStreamCtx;
    (void)pBuf;
    (void)cbBuf;

    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }

    return 1;
}


/* C I/O FILE* input stream function (pStreamCtx is a FILE*) */
int HDK_XML_InputStream_File(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf)
{
    *pcbStream = (unsigned int)fread(pBuf, 1, cbBuf, (FILE*)pStreamCtx);
    return (*pcbStream == cbBuf || !ferror((FILE*)pStreamCtx));
}


/* Buffer input stream function (pStreamCtx is HDK_XML_InputStream_BufferContext*) */
int HDK_XML_InputStream_Buffer(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf)
{
    HDK_XML_OutputStream_BufferContext* pBufferCtx = (HDK_XML_OutputStream_BufferContext*)pStreamCtx;
    unsigned int cbRead;

    /* Copy the characters into the buffer */
    cbRead = pBufferCtx->cbBuf - pBufferCtx->ixCur;
    if (cbBuf < cbRead)
    {
        cbRead = cbBuf;
    }
    if (cbRead)
    {
        memcpy(pBuf, pBufferCtx->pBuf + pBufferCtx->ixCur, cbRead);
        pBufferCtx->ixCur += cbRead;
    }
    *pcbStream = cbRead;
    return 1;
}
