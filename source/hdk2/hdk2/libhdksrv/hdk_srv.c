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

#include "hdk_srv.h"
#include "hdk_srv_log.h"

#ifndef _MSC_VER
#  include <dlfcn.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef _MSC_VER
#  define HDK_SRV_strcasecmp _stricmp
#else
#  define HDK_SRV_strcasecmp strcasecmp
#endif

/* Helper function for streaming a null-terminated string */
static int StreamText(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx, const char* pszText)
{
    unsigned int cbText;
    if (!pfnStream(&cbText, pStreamCtx, pszText, (unsigned int)strlen(pszText)))
    {
        return 0;
    }
    *pcbStream += cbText;
    return 1;
}


/* Helper function for streaming an int */
static int StreamInt(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx, int iValue)
{
    char szBuf[32];
    sprintf(szBuf, "%d", iValue);
    return StreamText(pcbStream, pfnStream, pStreamCtx, szBuf);
}


/* Helper function for SendErrorResponse */
static int SendErrorResponseContent(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                    int fClient, const char* pszText)
{
    return
        StreamText(pcbStream, pfnStream, pStreamCtx,
                   "<?xml version=\"1.0\" ?>\n"
                   "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
                   "<soap:Body>\n"
                   "<soap:Fault>\n"
                   "<faultcode>soap:") &&
        StreamText(pcbStream, pfnStream, pStreamCtx, fClient ? "Client" : "Server") &&
        StreamText(pcbStream, pfnStream, pStreamCtx,
                   "</faultcode>\n"
                   "<faultstring>") &&
        StreamText(pcbStream, pfnStream, pStreamCtx, pszText) &&
        StreamText(pcbStream, pfnStream, pStreamCtx,
                   "</faultstring>\n"
                   "</soap:Fault>\n"
                   "</soap:Body>\n"
                   "</soap:Envelope>\n");
}


/* Send a SOAP error response */
static int SendErrorResponse(HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx, const char* pszPrefix,
                             int fClient, const char* pszText, int fNoHeaders)
{
    unsigned int cbContent = 0;
    unsigned int cbResponse = 0;

    HDK_SRV_LOGFMT2((fClient ? HDK_LOG_Level_Warning : HDK_LOG_Level_Error), "%s SOAP fault \"%s\"\n", fClient ? "Client" : "Server", pszText);

    return
        /* Compute the content length */
        (fNoHeaders ||
         SendErrorResponseContent(&cbContent, HDK_XML_OutputStream_Null, 0, fClient, pszText)) &&

        /* Send the response prefix */
        (!pszPrefix ||
         StreamText(&cbResponse, pfnStream, pStreamCtx, pszPrefix)) &&

        /* Send the response headers */
        (fNoHeaders ||
         (StreamText(&cbResponse, pfnStream, pStreamCtx,
                     "500 Internal Server Error\r\n"
                     "Content-Type: text/xml; charset=utf-8\r\n"
                     "Connection: close\r\n"
                     "Content-Length: ") &&
          StreamInt(&cbResponse, pfnStream, pStreamCtx, cbContent) &&
          StreamText(&cbResponse, pfnStream, pStreamCtx,
                     "\r\n\r\n"))) &&

        /* Send the content */
        SendErrorResponseContent(&cbResponse, pfnStream, pStreamCtx, fClient, pszText);
}


/* Helper function for SendAuthResponse */
static int SendAuthResponseContent(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx)
{
    return
        StreamText(pcbStream, pfnStream, pStreamCtx,
                   "<html>\n"
                   "<head>\n"
                   "<title>401 Authorization Required</title>\n"
                   "</head>\n"
                   "<body>\n"
                   "401 Authorization Required\n"
                   "</body>\n"
                   "</html>\n");
}


/* Send a SOAP error response */
static int SendAuthResponse(HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx, const char* pszPrefix)
{
    unsigned int cbContent = 0;
    unsigned int cbResponse = 0;

    return
        /* Compute the content length */
        SendAuthResponseContent(&cbContent, HDK_XML_OutputStream_Null, 0) &&

        /* Send the response prefix */
        (!pszPrefix || StreamText(&cbResponse, pfnStream, pStreamCtx, pszPrefix)) &&

        /* Send the response headers */
        StreamText(&cbResponse, pfnStream, pStreamCtx,
                   "401 Authorization Required\r\n"
                   "WWW-Authenticate: Basic realm=\"HNAP1\"\r\n"
                   "Content-Type: text/html\r\n"
                   "Connection: close\r\n"
                   "Content-Length: ") &&
        StreamInt(&cbResponse, pfnStream, pStreamCtx, cbContent) &&
        StreamText(&cbResponse, pfnStream, pStreamCtx,
                   "\r\n\r\n") &&

        /* Send the content */
        SendAuthResponseContent(&cbResponse, pfnStream, pStreamCtx);
}


/* Send a SOAP response */
static int SendResponse(HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx, const char* pszPrefix,
                        const HDK_XML_Schema* pSchemaOutput, HDK_XML_Struct* pOutput,
                        int fMethodError, int fNoHeaders)
{
    unsigned int cbContent = 0;
    unsigned int cbResponse = 0;

    /* Method error? */
    int serializeOptions = 0;
    if (fMethodError)
    {
        serializeOptions += HDK_XML_SerializeOption_ErrorOutput;
    }

    return
        /* Compute the content length */
        (fNoHeaders ||
         HDK_XML_Serialize(&cbContent, HDK_XML_OutputStream_Null, 0, pSchemaOutput, pOutput, serializeOptions)) &&

        /* Send the response prefix */
        (!pszPrefix ||
         StreamText(&cbResponse, pfnStream, pStreamCtx, pszPrefix)) &&

        /* Send the response headers */
        (fNoHeaders ||
         (StreamText(&cbResponse, pfnStream, pStreamCtx,
                     "200 OK\r\n"
                     "Content-Type: text/xml; charset=utf-8\r\n"
                     "Connection: close\r\n"
                     "Content-Length: ") &&
          StreamInt(&cbResponse, pfnStream, pStreamCtx, cbContent) &&
          StreamText(&cbResponse, pfnStream, pStreamCtx,
                     "\r\n\r\n"))) &&

        /* Send the content */
        HDK_XML_Serialize(&cbResponse, pfnStream, pStreamCtx, pSchemaOutput, pOutput, serializeOptions);
}


/* Decode the HTTP Authorization header */
static int HTTP_AuthDecode(const char* pszHTTPAuth, char* pszAuthBuf, unsigned int cbAuthBuf,
                           const char** ppszUsername, const char** ppszPassword)
{
    int fResult = 0;

    /* Locate the start of the authorization data */
    const char* pszBasicAuth = pszHTTPAuth;
    for ( ; *pszBasicAuth == ' ' || *pszBasicAuth == '\t'; ++pszBasicAuth) {}
    if ((*pszBasicAuth == 'B' || *pszBasicAuth == 'b') &&
        (*(pszBasicAuth + 1) == 'A' || *(pszBasicAuth + 1) == 'a') &&
        (*(pszBasicAuth + 2) == 'S' || *(pszBasicAuth + 2) == 's') &&
        (*(pszBasicAuth + 3) == 'I' || *(pszBasicAuth + 3) == 'i') &&
        (*(pszBasicAuth + 4) == 'C' || *(pszBasicAuth + 4) == 'c') &&
        (*(pszBasicAuth + 5) == ' ' || *(pszBasicAuth + 5) == '\t'))
    {
        unsigned int cbBasicAuth;
        HDK_XML_OutputStream_BufferContext bufferCtx;
        HDK_XML_OutputStream_EncodeBase64_Context base64Ctx;
        unsigned int cbDecoded;

        /* Move beyond 'Basic ' */
        pszBasicAuth += sizeof("Basic ") - 1;
        cbBasicAuth = (unsigned int)strlen(pszBasicAuth);

        /* Decode the the basic authorization data */
        memset(&bufferCtx, 0, sizeof(bufferCtx));
        bufferCtx.pBuf = pszAuthBuf;
        bufferCtx.cbBuf = cbAuthBuf;

        memset(&base64Ctx, 0, sizeof(base64Ctx));
        base64Ctx.pfnStream = HDK_XML_OutputStream_Buffer;
        base64Ctx.pStreamCtx = &bufferCtx;
        if (HDK_XML_OutputStream_DecodeBase64(&cbDecoded, &base64Ctx, pszBasicAuth, cbBasicAuth) &&
            HDK_XML_OutputStream_DecodeBase64Done(&base64Ctx) &&
            cbDecoded < cbAuthBuf)
        {
            char* pszColon;

            /* Null-terminate the decoded output */
            pszAuthBuf[cbDecoded] = 0;

            /* Split into user and password */
            pszColon = strchr(pszAuthBuf, ':');
            if (pszColon)
            {
                *pszColon = 0;
                *ppszUsername = pszAuthBuf;
                *ppszPassword = pszColon + 1;
                fResult = 1;
            }
        }
    }

    return fResult;
}

/* Locate the method node */
static const char* FindMethod(
    int* pfClientError,
    const HDK_MOD_Method** ppMethod,
    HDK_SRV_ModuleContext** ppModuleCtx,
    HDK_SRV_ModuleContext** ppModuleCtxs,
    const HDK_XML_UUID* pNetworkObjectID,
    const char* pszHTTPMethod,
    const char* pszHTTPLocation,
    const char* pszSOAPAction)
{
    const char* pszClientServerError = 0;
    int fClientError = 0;
    unsigned int lenSOAPAction = 0;
    const HDK_MOD_Method* pMethod = 0;
    HDK_SRV_ModuleContext* pModuleCtx = 0;
    unsigned int lenHTTPLocation = (pszHTTPLocation ? (unsigned int)strlen(pszHTTPLocation) : 0);
    HDK_SRV_ModuleContext** ppModuleCtxCur;
    int fLocationMatch = 0;

    /* Trim the SOAPAction header */
    if (pszSOAPAction)
    {
        const char* pszSOAPActionEnd;
        for ( ; strchr(" \t\"", *pszSOAPAction); pszSOAPAction++) {}
        for (pszSOAPActionEnd = pszSOAPAction + strlen(pszSOAPAction) - 1;
             pszSOAPActionEnd >= pszSOAPAction && strchr(" \t\"\r\n", *pszSOAPActionEnd);
             pszSOAPActionEnd--) {}
        pszSOAPActionEnd++;
        lenSOAPAction = (unsigned int)(pszSOAPActionEnd - pszSOAPAction);
    }

    /* Search for the method node */
    for (ppModuleCtxCur = ppModuleCtxs; !pMethod && *ppModuleCtxCur; ++ppModuleCtxCur)
    {
        const HDK_MOD_Module* pModule = (*ppModuleCtxCur)->pModule;

        /* Network Obejct ID match? */
        if ((!pNetworkObjectID && !pModule->pNOID) ||
            (pNetworkObjectID && pModule->pNOID &&
             HDK_XML_IsEqual_UUID(pModule->pNOID, pNetworkObjectID)))
        {
            const HDK_MOD_Method* pMethodCur;
            for (pMethodCur = (*ppModuleCtxCur)->pModule->pMethods; pMethodCur->pszHTTPMethod; ++pMethodCur)
            {
                /* Location match? */
                unsigned int lenHTTPLocationCur = (unsigned int)strlen(pMethodCur->pszHTTPLocation);
                int fLocationMatchCur =
                  (pszHTTPLocation && lenHTTPLocationCur == lenHTTPLocation &&
                   strcmp(pMethodCur->pszHTTPLocation, pszHTTPLocation) == 0) ||
                  (!(pMethodCur->options & HDK_MOD_MethodOption_NoLocationSlash) &&
                   lenHTTPLocationCur == lenHTTPLocation - 1 &&
                   pszHTTPLocation && pszHTTPLocation[lenHTTPLocationCur] == '/' &&
                   strncmp(pMethodCur->pszHTTPLocation, pszHTTPLocation, lenHTTPLocationCur) == 0);
                fLocationMatch |= fLocationMatchCur;

                /* Method match? */
                if (fLocationMatchCur &&
                    pszHTTPMethod && strcmp(pMethodCur->pszHTTPMethod, pszHTTPMethod) == 0 &&
                    ((!pMethodCur->pszSOAPAction && !pszSOAPAction) ||
                     (pMethodCur->pszSOAPAction && pszSOAPAction &&
                      strlen(pMethodCur->pszSOAPAction) == lenSOAPAction &&
                      strncmp(pMethodCur->pszSOAPAction, pszSOAPAction, lenSOAPAction) == 0)))
                {
                    pModuleCtx = *ppModuleCtxCur;
                    pMethod = pMethodCur;
                    break;
                }
            }
        }
    }

    /* No matching method found and location match?  If so, its an error... */
    if (!pMethod && fLocationMatch)
    {
        pszClientServerError = "Unknown SOAP action";
        fClientError = 1;
    }

    /* Return the result */
    *pfClientError = fClientError;
    *ppMethod = pMethod;
    *ppModuleCtx = pModuleCtx;
    return pszClientServerError;
}


/* Parse the input struct */
static const char* ParseInputStruct(
    int* pfClientError,
    HDK_XML_Struct* pStructInput,
    const HDK_XML_Schema* pSchemaInput,
    unsigned int cbContentLength,
    HDK_XML_InputStreamFn pfnInput,
    void* pInputCtx,
    unsigned int cbMaxAlloc)
{
    HDK_XML_ParseError parseError = HDK_XML_ParseEx(pfnInput, pInputCtx, pSchemaInput, pStructInput, cbContentLength,
                                                    0 /* options */, 0 /* ixSchemaNode */, cbMaxAlloc);
    switch (parseError)
    {
        case HDK_XML_ParseError_OK:
            *pfClientError = 0;
            return 0;

        case HDK_XML_ParseError_OutOfMemory:
            *pfClientError = 0;
            return "Out of memory";

        case HDK_XML_ParseError_XMLInvalid:
            *pfClientError = 1;
            return "Invalid XML";

        case HDK_XML_ParseError_XMLUnexpectedElement:
            *pfClientError = 1;
            return "Unexpected XML element";

        case HDK_XML_ParseError_XMLInvalidValue:
            *pfClientError = 1;
            return "Invalid XML value text";

        case HDK_XML_ParseError_IOError:
            *pfClientError = 0;
            return "I/O error";

        case HDK_XML_ParseError_BadContentLength:
            *pfClientError = 1;
            return "Bad content length";

        default:
            *pfClientError = 0;
            return "Unexpected error";
    }
}


/* Call a server method and get output as a structure - returns non-zero on success */
HDK_SRV_EXPORT int HDK_SRV_CallMethodEx
(
    const char** ppszClientServerError,     /* May be 0 - returns non-zero for client/server error description string */
    int* pfClientError,                     /* May be 0 - returns non-zero if client/server error is client error */
    HDK_XML_Struct* psOutput,               /* The output structure */
    HDK_XML_Struct** ppOutput,              /* May be 0 - the output structure passed to the method */
    int method,                             /* The method index */
    HDK_SRV_ModuleContext* pModuleCtx,      /* The method's module context */
    HDK_SRV_ModuleContext** ppModuleCtxs,   /* The list of server module contexts */
    unsigned int cbContentLength,           /* May be 0 - the request content length */
    HDK_XML_InputStreamFn pfnInput,         /* The request input stream */
    void* pInputCtx,                        /* The request input stream context */
    unsigned int cbMaxAlloc                 /* May be 0 - maximum memory allocation allowed by parsing */
)
{
    const char* pszClientServerError = 0;
    int fClientError = 0;
    int fMethodError = 0;
    HDK_XML_Struct* pOutput = 0;
    HDK_MOD_MethodContext methodCtx;
    HDK_XML_Struct sInput;
    HDK_XML_Struct* pInput;

    /* Get the method node */
    const HDK_MOD_Method* pMethod = HDK_MOD_GetMethod(pModuleCtx->pModule, method);

    /* Initialize the input struct */
    HDK_XML_Struct_Init(&sInput);

    /* Parse the input struct */
    if (!(pMethod->options & HDK_MOD_MethodOption_NoInputStruct))
    {
        pszClientServerError = ParseInputStruct(&fClientError, &sInput, pMethod->pSchemaInput,
                                                cbContentLength, pfnInput, pInputCtx, cbMaxAlloc);
        if (pszClientServerError)
        {
            goto cleanup;
        }
    }
    else
    {
        /* Prepare the input struct */
        sInput.node.element = pMethod->pSchemaInput->pSchemaNodes->element;
    }

    /* Validate the input struct */
    if (!(pMethod->options & HDK_MOD_MethodOption_NoInputStruct) &&
        !HDK_XML_Validate(pMethod->pSchemaInput, &sInput, 0))
    {
        pszClientServerError = "Invalid request XML";
        fClientError = 1;
        goto cleanup;
    }

    /* Get the input struct */
    pInput = &sInput;
    if (pMethod->pInputElementPath)
    {
        const HDK_XML_Element* pElement;
        for (pElement = pMethod->pInputElementPath; *pElement != HDK_XML_BuiltinElement_Unknown; ++pElement)
        {
            pInput = HDK_XML_Get_Struct(pInput, *pElement);
            if (!pInput)
            {
                pszClientServerError = "Invalid request XML";
                fClientError = 1;
                goto cleanup;
            }
        }
    }

    /* Prepare the output struct */
    psOutput->node.element = pMethod->pSchemaOutput->pSchemaNodes->element;
    pOutput = psOutput;
    if (pMethod->pOutputElementPath)
    {
        const HDK_XML_Element* pElement;
        for (pElement = pMethod->pOutputElementPath; *pElement != HDK_XML_BuiltinElement_Unknown; ++pElement)
        {
            pOutput = HDK_XML_Set_Struct(pOutput, *pElement);
            if (!pOutput)
            {
                pszClientServerError = "Out of memory";
                fClientError = 0;
                goto cleanup;
            }
        }
    }

    /* Add the HNAP result element */
    if (pMethod->hnapResultElement != HDK_XML_BuiltinElement_Unknown)
    {
        HDK_XML_Set_Enum(pOutput, pMethod->hnapResultElement, pMethod->hnapResultEnumType, pMethod->hnapResultOK);
    }

    /* Call the method */
    memset(&methodCtx, 0, sizeof(methodCtx));
    methodCtx.pModuleCtx = pModuleCtx;
    methodCtx.ppModuleCtxs = ppModuleCtxs;
    pMethod->pfnMethod(&methodCtx, pInput, pOutput);

    /* HNAP error? */
    if (pMethod->hnapResultElement != HDK_XML_BuiltinElement_Unknown)
    {
        int result = HDK_XML_GetEx_Enum(pOutput, pMethod->hnapResultElement, pMethod->hnapResultEnumType,
                                        pMethod->hnapResultOK);
        fMethodError = (result != pMethod->hnapResultOK && result != pMethod->hnapResultREBOOT);
    }

    /* Validate the output struct */
    if (!HDK_XML_Validate(pMethod->pSchemaOutput, psOutput, fMethodError ? HDK_XML_ValidateOption_ErrorOutput : 0))
    {
        pszClientServerError = "Invalid response XML";
        fClientError = 0;
        goto cleanup;
    }

cleanup:
    /* Free the input struct */
    HDK_XML_Struct_Free(&sInput);

    /* Client/server error implies method error */
    if (pszClientServerError)
    {
        fMethodError = 1;
    }

    /* Return the result */
    if (ppszClientServerError)
    {
        *ppszClientServerError = pszClientServerError;
    }
    if (pfClientError)
    {
        *pfClientError = fClientError;
    }
    if (ppOutput)
    {
        *ppOutput = pOutput;
    }
    return !fMethodError;
}


/* Call a server method and stream response - returns non-zero on success */
HDK_SRV_EXPORT int HDK_SRV_CallMethod
(
    void* pServerCtx,                       /* The server context - ignored if pfnHNAPResult is 0 */
    int method,                             /* The method index */
    HDK_SRV_ModuleContext* pModuleCtx,      /* The method's module context */
    HDK_SRV_ModuleContext** ppModuleCtxs,   /* The list of server module contexts */
    unsigned int cbContentLength,           /* May be 0 - the request content length */
    HDK_XML_InputStreamFn pfnInput,         /* The request input stream */
    void* pInputCtx,                        /* The request input stream context */
    HDK_XML_OutputStreamFn pfnOutput,       /* The response output stream */
    void* pOutputCtx,                       /* The response output stream context */
    const char* pszOutputPrefix,            /* May be 0 - the response prefix (e.g. "HTTP 1.1 ") */
    HDK_SRV_HNAPResultFn pfnHNAPResult,     /* May be 0 - the HNAP result translation callback */
    unsigned int cbMaxAlloc,                /* May be 0 - maximum memory allocation allowed by parsing */
    int fNoHeaders                          /* Non-zero does not include HTTP headers in response */
)
{
    int fMethodError;
    int fClientError;
    const char* pszClientServerError;
    HDK_XML_Struct sOutput;
    HDK_XML_Struct* pOutput;

    /* Get the method node */
    const HDK_MOD_Method* pMethod = HDK_MOD_GetMethod(pModuleCtx->pModule, method);

    /* Initialize the output struct */
    HDK_XML_Struct_Init(&sOutput);

    /* Initialize the output struct */
    fMethodError = !HDK_SRV_CallMethodEx(&pszClientServerError, &fClientError, &sOutput, &pOutput,
                                         method, pModuleCtx, ppModuleCtxs,
                                         cbContentLength, pfnInput, pInputCtx, cbMaxAlloc);
    if (fMethodError && pszClientServerError)
    {
        SendErrorResponse(pfnOutput, pOutputCtx, pszOutputPrefix, fClientError, pszClientServerError, fNoHeaders);
    }
    else
    {
        /* Translate the HNAP result */
        if (pfnHNAPResult && pMethod->hnapResultElement != HDK_XML_BuiltinElement_Unknown)
        {
            pfnHNAPResult(pServerCtx, pOutput, pMethod->hnapResultElement, pMethod->hnapResultEnumType,
                          pMethod->hnapResultOK, pMethod->hnapResultREBOOT);
        }

        /* Send the response */
        SendResponse(pfnOutput, pOutputCtx, pszOutputPrefix, pMethod->pSchemaOutput, &sOutput, fMethodError, fNoHeaders);
    }

    /* Free the output struct */
    HDK_XML_Struct_Free(&sOutput);

    return !fMethodError;
}


HDK_SRV_EXPORT int HDK_SRV_HandleRequest
(
    int* pfError,                         /* May be 0 - returns non-zero if an error occurred */
    void* pServerCtx,                     /* The server context */
    HDK_SRV_ModuleContext** ppModuleCtxs, /* The list of server module contexts */
    const char* pszNetworkObjectID,       /* May be 0 */
    const char* pszHTTPMethod,            /* May be 0 */
    const char* pszHTTPLocation,          /* May be 0 */
    const char* pszSOAPAction,            /* May be 0 */
    const char* pszHTTPAuthorization,     /* May be 0 */
    unsigned int cbContentLength,         /* May be 0 - the request content length */
    HDK_XML_InputStreamFn pfnInput,       /* The request input stream */
    void* pInputCtx,                      /* The request input stream context */
    HDK_XML_OutputStreamFn pfnOutput,     /* The request output stream */
    void* pOutputCtx,                     /* The request output stream context */
    const char* pszOutputPrefix,          /* May be 0 - the response prefix (e.g. "HTTP 1.1 ") */
    HDK_SRV_AuthFn pfnAuth,               /* May be 0 - the auth callback */
    HDK_SRV_HNAPResultFn pfnHNAPResult,   /* May be 0 - the HNAP result translation callback */
    unsigned int cbMaxAlloc               /* May be 0 - maximum memory allocation allowed by parsing */
)
{
    int fHandled = 0;
    int fMethodError = 0;
    HDK_XML_UUID uuidNOID;
    const HDK_XML_UUID* pNetworkObjectID = 0;
    const char* pszClientServerError;
    int fClientError;
    const HDK_MOD_Method* pMethod;
    HDK_SRV_ModuleContext* pModuleCtx;

    HDK_SRV_LOGFMT3(HDK_LOG_Level_Info, "Received HTTP %s request for '%s', SOAPAction: \"%s\"...\n",
                    pszHTTPMethod, pszHTTPLocation, pszSOAPAction);

    /* Parse the network object ID, if any */
    if (pszNetworkObjectID && HDK_XML_Parse_UUID(&uuidNOID, pszNetworkObjectID))
    {
        pNetworkObjectID = &uuidNOID;
    }

    /* Find the method node */
    pszClientServerError = FindMethod(&fClientError, &pMethod, &pModuleCtx, ppModuleCtxs,
                                      pNetworkObjectID, pszHTTPMethod, pszHTTPLocation, pszSOAPAction);
    if (pszClientServerError)
    {
        fHandled = 1;
        SendErrorResponse(pfnOutput, pOutputCtx, pszOutputPrefix, fClientError, pszClientServerError, 0);
    }
    if (pMethod)
    {
        /* Basic authentication */
        int fAuthOK = 1;
        if (!(pMethod->options & HDK_MOD_MethodOption_NoBasicAuth))
        {
            char szBuf[256];
            const char* pszUsername = 0;
            const char* pszPassword = 0;
            if (!pszHTTPAuthorization ||
                !HTTP_AuthDecode(pszHTTPAuthorization, szBuf, sizeof(szBuf), &pszUsername, &pszPassword) ||
                (pfnAuth && !pfnAuth(pServerCtx, pszUsername, pszPassword)))
            {
                HDK_SRV_LOGFMT3(HDK_LOG_Level_Warning, "Invalid HTTP Auth credentials: %s (%s:%s)\n", pszHTTPAuthorization, pszUsername, pszPassword);

                fAuthOK = 0;
                fHandled = 1;
                SendAuthResponse(pfnOutput, pOutputCtx, pszOutputPrefix);
            }
        }

        /* Authenticated? */
        if (fAuthOK)
        {
            /* Call the method */
            fHandled = 1;
            fMethodError = !HDK_SRV_CallMethod(pServerCtx,
                                               (int)(pMethod - pModuleCtx->pModule->pMethods), pModuleCtx, ppModuleCtxs,
                                               cbContentLength, pfnInput, pInputCtx, pfnOutput, pOutputCtx,
                                               pszOutputPrefix, pfnHNAPResult, cbMaxAlloc, 0);
        }
    }

    /* Return the result */
    if (pfError)
    {
        *pfError = fMethodError;
    }

    return fHandled;
}


/* Dynamically load server module */
HDK_SRV_MODULE_HANDLE HDK_SRV_OpenModule(HDK_MOD_ModuleFn* ppfnModule, const char* pszModule)
{
    HDK_SRV_MODULE_HANDLE hModule;

#ifdef _MSC_VER
    hModule = LoadLibrary(pszModule);
#else
    hModule = dlopen(pszModule, RTLD_LAZY | RTLD_LOCAL);
#endif
    if (hModule)
    {
#ifdef _MSC_VER
        FARPROC pfn = GetProcAddress(hModule, HDK_MOD_ModuleExport);
#else
        void* pfn = dlsym(hModule, HDK_MOD_ModuleExport);
#endif
        if (pfn)
        {
            void* p = (void*)ppfnModule;
            memcpy(p, &pfn, sizeof(*ppfnModule));
        }
        else
        {
            HDK_SRV_CloseModule(hModule);
            hModule = 0;
        }
    }

    return hModule;
}


/* Dynamically unload server module */
void HDK_SRV_CloseModule(HDK_SRV_MODULE_HANDLE hModule)
{
#ifdef _MSC_VER
    (void)FreeLibrary(hModule);
#else
    dlclose(hModule);
#endif
}
