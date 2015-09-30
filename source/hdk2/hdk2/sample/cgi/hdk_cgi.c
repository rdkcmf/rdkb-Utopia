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

#include "hdk_cgi_context.h"
#include "hdk_srv.h"

#ifdef HDK_CGI_FCGI
#include <fcgi_stdio.h>
#else
#include <stdio.h>
#endif
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>


/* Maximum total server request memory allocation */
#ifndef HDK_CGI_MAX_ALLOC
#define HDK_CGI_MAX_ALLOC (12 * 1024 * 1024)
#endif

/* Helper macro for static content responses */
#define PRINTF_CONTENT(s) printf("Content-Length: %d\r\n\r\n" s, (int)sizeof(s) - 1)


/* Server input stream functions */
static int HDK_CGI_InputStream(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf)
{
    *pcbStream = (unsigned int)fread(pBuf, 1, cbBuf, (FILE*)pStreamCtx);
    return (*pcbStream == cbBuf || !ferror((FILE*)pStreamCtx));
}


/* Server output stream functions */
static int HDK_CGI_OutputStream(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }
    return (cbBuf ? fwrite((char*)pBuf, 1, cbBuf, (FILE*)pStreamCtx) == cbBuf : 1);
}


/* Main entry point */
int main(void)
{
#ifdef _MSC_VER
    /* Ensure stdin and stdout are in binary mode */
    (void)setmode(fileno(stdin), _O_BINARY);
    (void)setmode(fileno(stdout), _O_BINARY);
#endif

#ifdef HDK_CGI_FCGI
    /* Wait for fastcgi request */
    while (FCGI_Accept() >= 0)
#endif
    {
        int fHandled = 0;
        void* pServerCtx = 0;

        /* CGI environment variables */
        char* pszRequestMethod = getenv("REQUEST_METHOD");
        char* pszRequestURI = getenv("REQUEST_URI");
        char* pszContentLength = getenv("CONTENT_LENGTH");
        unsigned int cbContentLength = (pszContentLength ? strtoul(pszContentLength, 0, 0) : 0);
        char* pszSOAPAction = getenv("HTTP_SOAPACTION");
        char* pszHTTPAuth = getenv("HTTP_AUTHORIZATION");
        char* pszNetworkObjectID = getenv("X-NETWORKOBJECTID");

        /* Initialize the server context */
        pServerCtx = HDK_CGI_ServerContext_Init();
        if (pServerCtx)
        {
            int fError = 0;

            /* Initialize the server module contexts */
            HDK_SRV_ModuleContext** ppModuleCtxs = HDK_CGI_ServerModules_Init(pServerCtx);
            if (ppModuleCtxs)
            {
                /* Handle the request */
                fHandled = HDK_SRV_HandleRequest(
                    &fError,
                    pServerCtx,
                    ppModuleCtxs,
                    pszNetworkObjectID,
                    pszRequestMethod,
                    pszRequestURI,
                    pszSOAPAction,
                    pszHTTPAuth,
                    cbContentLength,
                    HDK_CGI_InputStream,
                    stdin,
                    HDK_CGI_OutputStream,
                    stdout,
                    "Status: ",
                    HDK_CGI_Authenticate,
                    HDK_CGI_HNAPResult,
                    HDK_CGI_MAX_ALLOC);

                /* Free the server module contexts */
                HDK_CGI_ServerModules_Free(ppModuleCtxs);
            }

            /* Free the server context */
            HDK_CGI_ServerContext_Free(pServerCtx, fHandled && !fError);
        }

        /* Request not handled? */
        if (!fHandled)
        {
            printf(
                "Status: 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n");
            PRINTF_CONTENT(
                "<html>\n"
                "<head>\n"
                "<title>404 Not Found</title>\n"
                "</head>\n"
                "<body>\n"
                "404 Not Found\n"
                "</body>\n"
                "</html>\n");
        }

        /* Flush out the response */
        fflush(stdout);
    }

    exit(0);
}
