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

#include "hnapd_context.h"
#include "hdk_srv.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#  include <Windows.h>
#else /* def _MSC_VER */
#  include <unistd.h>
#  include <netinet/in.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#endif /* def _MSC_VER */


#ifdef _MSC_VER
#  define HNAPD_SOCKET SOCKET
#  define HNAPD_strcasecmp _stricmp
#  define HNAPD_closesocket closesocket
#  define HNAPD_socklen_t int
#else /* ndef _MSC_VER */
#  define HNAPD_SOCKET int
#  define HNAPD_strcasecmp strcasecmp
#  define HNAPD_closesocket close
#  define HNAPD_socklen_t socklen_t
#endif /* def _MSC_VER */


/* Maximum total server request memory allocation */
#ifndef HNAPD_MAX_ALLOC
#  define HNAPD_MAX_ALLOC (12 * 1024 * 1024)
#endif


/*
 * Socket input/output stream
 */

typedef struct _SocketStreamContext
{
    HNAPD_SOCKET fd;
} SocketStreamContext;

static int ReadSocket(unsigned int* pcbRead, void* pStreamCtx, char* pBuf, unsigned int cbBuf)
{
    int cbRecv = recv(((SocketStreamContext*)pStreamCtx)->fd, pBuf, cbBuf, 0);
    if (cbRecv >= 0)
    {
        *pcbRead = (unsigned int)cbRecv;
        return 1;
    }
    return 0;
}

static int WriteSocket(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }
    return cbBuf ? send(((SocketStreamContext*)pStreamCtx)->fd, pBuf, cbBuf, 0) : 1;
}


/* Parse the HTTP request headers */
static void HTTP_ParseHeaders(HNAPD_SOCKET fdSocket, char* pszBuf, unsigned int cbBuf,
                              char** ppszNetworkObjectID,
                              char** ppszRequestMethod, char** ppszRequestURI,
                              unsigned int* pContentLength, char** ppszSOAPAction,
                              char** ppszHTTPAuth)
{
    char* pBuf;
    char* pBufEnd;
    int cNewline;
    int fHaveContent;
    char* pszNetworkObjectID = 0;
    char* pszRequestMethod = 0;
    char* pszRequestURI = 0;
    char* pszContentLength = 0;
    char* pszSOAPAction = 0;
    char* pszHTTPAuth = 0;

    /* Search for the blank line preceding the content */
    pBufEnd = pszBuf + cbBuf;
    cNewline = 0;
    fHaveContent = 0;
    for (pBuf = pszBuf; pBuf < pBufEnd; ++pBuf)
    {
        /* Read 1 byte into the buffer */
        unsigned int cbRead = 0;
        SocketStreamContext streamCtx;
        streamCtx.fd = fdSocket;
        if (!ReadSocket(&cbRead, &streamCtx, pBuf, 1))
        {
            break;
        }

        /* Count consecutive newlines */
        if (*pBuf == '\n')
        {
            if (++cNewline == 2)
            {
                fHaveContent = 1;
                break;
            }
        }
        else if (cNewline > 0 && *pBuf != '\r')
        {
            cNewline = 0;
        }
    }

    /* Parse HTTP headers */
    if (fHaveContent)
    {
        char* pszName = 0;
        char* pszValue = 0;
        pBufEnd = pBuf;

        /* Find the HTTP method and location */
        for (pBuf = pszBuf; pBuf < pBufEnd; ++pBuf)
        {
            if (*pBuf == '\n' || *pBuf == '\r')
            {
                break;
            }
            else if (!pszRequestMethod && *pBuf == ' ')
            {
                pszRequestMethod = pszBuf;
                *pBuf = '\0';
            }
            else if (pszRequestMethod && !pszValue && *pBuf != ' ')
            {
                pszValue = pBuf;
            }
            else if (pszRequestMethod && pszValue && *pBuf == ' ')
            {
                pszRequestURI = pszValue;
                *pBuf = '\0';
                break;
            }
        }

        /* Parse the HTTP header name/value pairs */
        pszValue = 0;
        for ( ; pBuf < pBufEnd; ++pBuf)
        {
            if (*pBuf == '\n' || *pBuf == '\r')
            {
                /* Found name/value pair? */
                if (pszName && pszValue)
                {
                    /* Skip whitespace & quotes*/
                    for ( ; *(pBuf - 1) == ' ' && pBuf >= pszValue; pBuf--) {}
                    for ( ; *(pBuf - 1) == '"' && pBuf >= pszValue; pBuf--) {}

                    /* Null-terminate the value string */
                    *pBuf = '\0';

                    /* Handle HTTP headers... */
                    if (HNAPD_strcasecmp(pszName, "Content-Length") == 0)
                    {
                        pszContentLength = pszValue;
                    }
                    else if (HNAPD_strcasecmp(pszName, "Authorization") == 0)
                    {
                        pszHTTPAuth = pszValue;
                    }
                    else if (HNAPD_strcasecmp(pszName, "SOAPAction") == 0)
                    {
                        pszSOAPAction = pszValue;
                    }
                    else if (HNAPD_strcasecmp(pszName, "X-NetworkObjectID") == 0)
                    {
                        pszNetworkObjectID = pszValue;
                    }
                }

                /* Start searching for the next name/value pair */
                pszName = pBuf + 1;
                pszValue = 0;
            }
            else if (!pszValue && *pBuf == ':')
            {
                /* Null-terminate the name string */
                *pBuf++ = '\0';

                /* Skip whitespace & quotes*/
                for ( ; *pBuf == ' ' && pBuf < pBufEnd; pBuf++) {}
                for ( ; *pBuf == '"' && pBuf < pBufEnd; pBuf++) {}
                pszValue = pBuf;
            }
        }
    }

    /* Return the results */
    *ppszNetworkObjectID = pszNetworkObjectID;
    *ppszRequestMethod = pszRequestMethod;
    *ppszRequestURI = pszRequestURI;
    *pContentLength = (pszContentLength ? atoi(pszContentLength) : 0);
    *ppszSOAPAction = pszSOAPAction;
    *ppszHTTPAuth = pszHTTPAuth;
}


/* Helper macro for static content responses */
#define WRITE_STRING(fd, s) WriteSocket(0, &streamCtx, s, sizeof(s) - 1)


/*
 * HTTP_HandleRequest - Handle the HTTP request
 */
static void HTTP_HandleRequest(HNAPD_SOCKET fdSocket)
{
    int fHandled = 0;
    void* pServerCtx;

    /* Parse the HTTP headers */
    char szHeaders[1024];
    char* pszNetworkObjectID = 0;
    char* pszRequestMethod = 0;
    char* pszRequestURI = 0;
    unsigned int cbContentLength = 0;
    char* pszSOAPAction = 0;
    char* pszHTTPAuth = 0;
    HTTP_ParseHeaders(fdSocket, szHeaders, sizeof(szHeaders), &pszNetworkObjectID,
                      &pszRequestMethod, &pszRequestURI, &cbContentLength, &pszSOAPAction, &pszHTTPAuth);

    /* Initialize the server context */
    pServerCtx = HNAPD_ServerContext_Init();
    if (pServerCtx)
    {
        int fError = 0;

        /* Initialize the server module contexts */
        HDK_SRV_ModuleContext** ppModuleCtxs = HNAPD_ServerModules_Init(pServerCtx);
        if (ppModuleCtxs)
        {
            SocketStreamContext streamCtx;
            streamCtx.fd = fdSocket;

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
                ReadSocket,
                &streamCtx,
                WriteSocket,
                &streamCtx,
                "HTTP/1.1 ",
                HNAPD_Authenticate,
                HNAPD_HNAPResult,
                HNAPD_MAX_ALLOC);

            /* Free the server module contexts */
            HNAPD_ServerModules_Free(ppModuleCtxs);
        }

        /* Free the server context */
        HNAPD_ServerContext_Free(pServerCtx, fHandled && !fError);
    }

    /* Request not handled? */
    if (!fHandled)
    {
        static const char g_psz404Body[] =
          "<html>\n"
          "<head>\n"
          "<title>404 Not Found</title>\n"
          "</head>\n"
          "<body>\n"
          "404 Not Found\n"
          "</body>\n"
          "</html>\n";

        char pszContentLengthHeader[64];
        int cbContentLengthHeader = sprintf(pszContentLengthHeader, "Content-Length: %d\r\n\r\n", (int)sizeof(g_psz404Body) - 1);

        SocketStreamContext streamCtx;
        streamCtx.fd = fdSocket;

        WRITE_STRING(streamCtx,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n");
        WriteSocket(0, &streamCtx,
                    pszContentLengthHeader,
                    cbContentLengthHeader);
        WRITE_STRING(streamCtx, g_psz404Body);
    }
}


/*
 * hnapd main entry point
 */
int main(int argc, char *argv[])
{
    int port;
    HNAPD_SOCKET fdSock;
    struct sockaddr_in addrServer;

#ifdef _MSC_VER
    WSADATA wsaData;
    if (0 != WSAStartup(MAKEWORD(2, 0), &wsaData))
    {
        perror("Error: Failure initializing winsock\n");
        exit(1);
    }
#endif /* def _MSC_VER */

    /* Command line */
    if (argc < 2)
    {
        perror("Error: No port provided\n");
        exit(1);
    }
    port = atoi(argv[1]);

    /* Bind to the socket */
    fdSock = socket(AF_INET, SOCK_STREAM, 0);
    if (fdSock < 0)
    {
        perror("Error: Failure opening socket\n");
        exit(1);
    }
    memset(&addrServer, 0, sizeof(addrServer));
    addrServer.sin_family = AF_INET;
    addrServer.sin_addr.s_addr = INADDR_ANY;
    addrServer.sin_port = htons((u_short)port);
    if (bind(fdSock, (struct sockaddr*)&addrServer, sizeof(addrServer)) < 0)
    {
        perror("Error: Failure binding socket");
        exit(1);
    }

    /* Loop forever... */
    while (1)
    {
        HNAPD_SOCKET fdRequest;
        HNAPD_socklen_t cbRequest;
        struct sockaddr_in addrRequest;

        /* Listen on the socket */
        listen(fdSock, 5);
        cbRequest = sizeof(addrRequest);
        fdRequest = accept(fdSock, (struct sockaddr*)&addrRequest, &cbRequest);
        if (fdRequest >= 0)
        {
            HTTP_HandleRequest(fdRequest);

            /* Close the socket */
            HNAPD_closesocket(fdRequest);
        }
    }

    exit(0);
}
