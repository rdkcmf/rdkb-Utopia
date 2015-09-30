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

#include "hdk_srv.h"
#include "malloc_interposer.h"

#ifdef _MSC_VER
#  include <fcntl.h>
#  include <io.h>
#  include <Windows.h>
#else
#  include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Parse request HTTP headers */
static FILE* ParseHeaders(const char* pszRequest,
                          char* pszSoapAction, unsigned int cbSoapAction,
                          unsigned int* pcbContentLength)
{
    /* Open the request file */
    FILE* pfhRequest = fopen(pszRequest, "rb");
    if (pfhRequest)
    {
        char szLine[512];
        int fSoapAction = 0;
        int posContent;
        int posEnd;

        /* Parse HTTP headers */
        while (fgets(szLine, sizeof(szLine), pfhRequest))
        {
            if (strcmp(szLine, "\n") == 0 || strcmp(szLine, "\r\n") == 0)
            {
                break;
            }
            else if (strstr(szLine, "SOAPAction:") == szLine)
            {
                char* pszValue = szLine + sizeof("SOAPAction:") - 1;
                unsigned int cbValue = (unsigned int)strlen(pszValue) + 1;
                if (cbValue > cbSoapAction)
                {
                    szLine[cbSoapAction - 1] = 0;
                    cbValue = cbSoapAction;
                }
                memcpy(pszSoapAction, pszValue, cbValue);
                fSoapAction = 1;
            }
        }

        /* Compute the content length */
        if (!fSoapAction ||
            (posContent = ftell(pfhRequest)) == -1 ||
            fseek(pfhRequest, 0, SEEK_END) == -1 ||
            (posEnd = ftell(pfhRequest)) == -1 ||
            fseek(pfhRequest, posContent, SEEK_SET) == -1)
        {
            fclose(pfhRequest);
            pfhRequest = 0;
        }
        else
        {
            *pcbContentLength = posEnd - posContent;
        }
    }

    return pfhRequest;
}


/* HDK ADI unit test helper application */
int main(int argc, char* argv[])
{
    int fHandled = 0;
    int fError = 0;

    /*
        Clear all malloc interposer stats up to this point, as some library initializers may allocate data
        which would be erroneously reported as a memory leak (since memory leaks are reported prior the libraries
        being unloaded and their destructors called.)
    */
    clear_interposer_stats();

    /* Ensure stdin and stdout are in binary mode */
#ifdef _MSC_VER
    (void)setmode(fileno(stdin), _O_BINARY);
    (void)setmode(fileno(stdout), _O_BINARY);
#endif

    /* Usage */
    if (argc != 4 && argc != 5 && argc != 7)
    {
        printf("Usage: %s <module> <method> <location> [<request> [<input-state> <output-state>]]\n", argv[0]);
        fError = 1;
    }
    else
    {
        const char* pszModule = argv[1];
        const char* pszMethod = argv[2];
        const char* pszLocation = argv[3];
        const char* pszRequest = (argc >= 5 ? argv[4] : 0);
        const char* pszInputState = (argc >= 6 ? argv[5] : 0);
        const char* pszOutputState = (argc >= 7 ? argv[6] : 0);
        int fRequest = (pszRequest && *pszRequest && strcmp(pszRequest, "-") != 0);

        /* Load the module */
        HDK_MOD_ModuleFn pfnModule;
        HDK_SRV_MODULE_HANDLE pModuleHandle = HDK_SRV_OpenModule(&pfnModule, pszModule);
        if (!pModuleHandle)
        {
            printf("Error: Couldn't load module \"%s\"\n", pszModule);
            fError = 1;
        }
        else
        {
            HDK_SRV_ModuleContext moduleCtx;
            HDK_SRV_Simulator_ModuleCtx simulatorCtx;
            const char* pszSoapAction;
            char szSoapAction[512];
            unsigned int cbContentLength = 0;
            HDK_XML_InputStreamFn pfnRequestStream;
            void* pRequestStreamCtx = 0;
            HDK_XML_InputStream_BufferContext requestBufferCtx;

            /* Initialize the module context */
            memset(&moduleCtx, 0, sizeof(moduleCtx));
            moduleCtx.pModule = pfnModule();
            moduleCtx.pModuleCtx = &simulatorCtx;
            moduleCtx.pfnADIGet = HDK_SRV_Simulator_ADIGet;
            moduleCtx.pfnADISet = HDK_SRV_Simulator_ADISet;

            /* Initialize the simulator context */
            HDK_SRV_Simulator_Init(&simulatorCtx);

            /* Request stream */
            if (fRequest)
            {
                /* Parse the request headers */
                pfnRequestStream = HDK_XML_InputStream_File;
                pRequestStreamCtx = (void*)ParseHeaders(pszRequest, szSoapAction, sizeof(szSoapAction), &cbContentLength);
                if (!pRequestStreamCtx)
                {
                    printf("Error: Couldn't load request file \"%s\"\n", pszRequest);
                    fError = 1;
                }
                pszSoapAction = szSoapAction;
            }
            else
            {
                /* Empty request */
                pfnRequestStream = HDK_XML_InputStream_Buffer;
                memset(&requestBufferCtx, 0, sizeof(requestBufferCtx));
                pRequestStreamCtx = &requestBufferCtx;
                pszSoapAction = 0;
                cbContentLength = 0;
            }

            /* Read the simulator state file */
            if (!fError && pszInputState)
            {
                int fState = 0;
                FILE* pfhState = fopen(pszInputState, "rb");
                if (pfhState)
                {
                    fState = HDK_SRV_Simulator_Read(&simulatorCtx, HDK_XML_InputStream_File, pfhState);
                    fclose(pfhState);
                }
                if (!fState)
                {
                    printf("Error: Couldn't read input state file \"%s\"\n", pszInputState);
                    fError = 1;
                }
            }

            /* Execute the request */
            if (!fError)
            {
                HDK_SRV_ModuleContext* moduleCtxs[2];

                /* Serialize the network object ID */
                const char* pszNOID = 0;
                HDK_XML_OutputStream_BufferContext streamCtx;
                unsigned int cbStream;
                memset(&streamCtx, 0, sizeof(streamCtx));
                if (moduleCtx.pModule->pNOID &&
                    HDK_XML_Serialize_UUID(&cbStream, HDK_XML_OutputStream_GrowBuffer, &streamCtx, moduleCtx.pModule->pNOID, 1))
                {
                    pszNOID = streamCtx.pBuf;
                }

                /* Handle the request */
                moduleCtxs[0] = &moduleCtx;
                moduleCtxs[1] = HDK_SRV_ModuleContextEnd;
                fHandled = HDK_SRV_HandleRequest(
                    0,
                    0,
                    moduleCtxs,
                    pszNOID,
                    pszMethod,
                    pszLocation,
                    pszSoapAction,
                    "Basic YWRtaW46cGFzc3dvcmQ=",   /* admin:password */
                    cbContentLength,
                    pfnRequestStream,
                    pRequestStreamCtx,
                    HDK_XML_OutputStream_File,
                    (void*)stdout,
                    0,
                    0,
                    0,
                    0);
                if (!fHandled)
                {
                    printf("Request not handled.\n");
                }

                /* Free the serialized network object ID */
                free(streamCtx.pBuf);

                /* Free the unittest module context */
                if (!fError && pszOutputState)
                {
                    int fState = 0;
                    FILE* pfhState = fopen(pszOutputState, "wb");
                    if (pfhState)
                    {
                        fState = HDK_SRV_Simulator_Write(&simulatorCtx, HDK_XML_OutputStream_File, pfhState);
                        fclose(pfhState);
                    }
                    if (!fState)
                    {
                        printf("Error: Couldn't write output state file \"%s\"\n", pszOutputState);
                        fError = 1;
                    }
                }
            }

            /* Close the request stream */
            if (fRequest && pRequestStreamCtx)
            {
                fclose((FILE*)pRequestStreamCtx);
            }

            /* Free the simulator context */
            HDK_SRV_Simulator_Free(&simulatorCtx);
        }

        /* Close the module */
        if (pModuleHandle)
        {
            HDK_SRV_CloseModule(pModuleHandle);
        }
    }

    /* Return the result */
    exit(fError);
}
