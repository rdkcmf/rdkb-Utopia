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

#include "unittest.h"
#include "unittest_module.h"
#include "unittest_util.h"

#include "hdk_srv.h"

#include <string.h>


/* Server context */
typedef struct _ServerContext
{
    int fReboot;
} ServerContext;


/* Server auth callback */
static int ServerAuth(void* pServerCtx, const char* pszUser, const char* pszPassword)
{
    /* Unused parameters */
    (void)pServerCtx;

    return strcmp(pszUser, "admin") == 0 &&
        strcmp(pszPassword, "password") == 0;
}


/* Server HNAP result callback */
static void ServerHNAPResult(void* pServerCtx, HDK_XML_Struct* pOutput,
                             HDK_XML_Element resultElement, HDK_XML_Type resultType,
                             int resultOK, int resultReboot)
{
    /* Unused parameters */
    (void)pServerCtx;
    (void)pOutput;
    (void)resultElement;
    (void)resultType;
    (void)resultOK;
    (void)resultReboot;

    /* Reboot? */
    if (((ServerContext*)pServerCtx)->fReboot &&
        HDK_XML_GetEx_Enum(pOutput, resultElement, resultType, resultOK) == resultOK)
    {
        HDK_XML_Set_Enum(pOutput, resultElement, resultType, resultReboot);
    }
}


/* Server unit test helper function */
void ServerTestHelper(
    const char* pszNetworkObjectID,
    const char* pszMethod,
    const char* pszLocation,
    const char* pszSOAPAction,
    const char* pszAuth,
    const char* pszRequest,
    int options)
{
    ServerTestHelperEx(pszNetworkObjectID, pszMethod, pszLocation, pszSOAPAction, pszAuth, pszRequest, options, 0, 0);
}


/* Server unit test helper function (extended) */
void ServerTestHelperEx(
    const char* pszNetworkObjectID,
    const char* pszMethod,
    const char* pszLocation,
    const char* pszSOAPAction,
    const char* pszAuth,
    const char* pszRequest,
    int options,
    HDK_SRV_ADIGetFn pfnADIGet,
    HDK_SRV_ADISetFn pfnADISet)
{
    HDK_SRV_ModuleContext moduleCtx;
    HDK_SRV_ModuleContext moduleCtx2;
    HDK_SRV_ModuleContext* moduleCtxs[3];
    HDK_XML_InputStream_BufferContext inputBufferCtx;
    ServerContext serverCtx;
    int fHandled;
    int fError;

    /* Module */
    memset(&moduleCtx, 0, sizeof(moduleCtx));
    moduleCtx.pModule = GetModule();
    moduleCtx.pfnADIGet = pfnADIGet;
    moduleCtx.pfnADISet = pfnADISet;

    /* Module2 */
    memset(&moduleCtx2, 0, sizeof(moduleCtx2));
    moduleCtx2.pModule = GetModule2();

    /* Module list */
    moduleCtxs[0] = &moduleCtx;
    moduleCtxs[1] = &moduleCtx2;
    moduleCtxs[2] = HDK_SRV_ModuleContextEnd;

    /* Input stream context */
    memset(&inputBufferCtx, 0, sizeof(inputBufferCtx));
    inputBufferCtx.pBuf = pszRequest;
    inputBufferCtx.cbBuf = (unsigned int)strlen(inputBufferCtx.pBuf);

    /* Handle the request */
    memset(&serverCtx, 0, sizeof(serverCtx));
    serverCtx.fReboot = !!(options & STHO_Reboot);
    fHandled = HDK_SRV_HandleRequest(
        &fError,
        &serverCtx,
        moduleCtxs,
        pszNetworkObjectID,
        pszMethod,
        pszLocation,
        pszSOAPAction,
        pszAuth,
        inputBufferCtx.cbBuf + ((options & STHO_BadContentLength) ? 10 : 0),
        HDK_XML_InputStream_Buffer,
        &inputBufferCtx,
        UnittestStream,
        UnittestStreamCtx,
        0,
        ServerAuth,
        ServerHNAPResult,
        0);

    UnittestLog1("\nHandled = %d\n", fHandled);
    UnittestLog1("Error = %d\n", fError);
}
