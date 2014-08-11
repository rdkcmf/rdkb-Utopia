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

#include "hdk_cli.h"
#include "hdk_mod.h"
#include "hdk_cli_log.h"

#include "hdk_cli_http.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef _MSC_VER
#  define snprintf _snprintf
#endif /* def _MSC_VER */

#define HDK_CLI_SUCCEEDED(error) (HDK_CLI_Error_OK == error)
#define HDK_CLI_FAILED(error) (!HDK_CLI_SUCCEEDED(error))

/* HDK_CLI_EXPORT */ int HDK_CLI_Init(void)
{
    /* Initialize the HTTP layer. */
    return HDK_CLI_Http_Init();
}

/* HDK_CLI_EXPORT */ void HDK_CLI_Cleanup(void)
{
    /* Clean up the HTTP layer. */
    HDK_CLI_Http_Cleanup();
}


#ifdef HDK_LOGGING

typedef struct _LoggingContext
{
    char* pData;
    size_t cbAllocated;
    size_t cbData;
} LoggingContext;

static int LoggingContext_AppendData(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    LoggingContext* pCtx = (LoggingContext*)pStreamCtx;

    /* Grow the buffer as needed... */
    size_t cbDataRequired = pCtx->cbData + (size_t)cbBuf + 1 /* for '\0' */;
    if (cbDataRequired > pCtx->cbAllocated)
    {
        size_t cbNewAlloc = 2 * pCtx->cbAllocated;
        char* pData;
        if (cbDataRequired > cbNewAlloc)
        {
            cbNewAlloc = cbDataRequired;
        }
        pData = (char*)realloc(pCtx->pData, cbDataRequired);
        if (!pData)
        {
            free(pCtx->pData);
        }
        pCtx->pData = pData;
        if (pCtx->pData)
        {
            pCtx->cbAllocated = cbDataRequired;
        }
    }
    if (pCtx->pData)
    {
        memcpy(&pCtx->pData[pCtx->cbData], pBuf, cbBuf);
        pCtx->cbData += cbBuf;

        pCtx->pData[pCtx->cbData] = '\0';
    }

    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }

    return (0 != pCtx->pData);
}

static void LoggingContext_Init(LoggingContext* pCtx)
{
    memset(pCtx, 0, sizeof(*pCtx));
}

static void LoggingContext_Cleanup(LoggingContext* pCtx)
{
    free(pCtx->pData);
}

#endif /* def HDK_LOGGING */


#define DestroyRequest(pRequest) HDK_CLI_Http_RequestDestroy(pRequest)

/* Create a request for the given input. */
static HDK_CLI_Error CreateRequest(void** ppRequest,
                                   const HDK_MOD_Method* pMethod,
                                   const HDK_XML_UUID* pNetworkObjectID,
                                   const char* pszHttpHost,
                                   unsigned int timeoutSecs,
                                   const char* pszUsername,
                                   const char* pszPassword)
{
    HDK_CLI_Error error = HDK_CLI_Error_Unknown;
    void* pHttpRequest = 0;

    int fGet = (0 == strcmp("GET", pMethod->pszHTTPMethod));

    size_t cchURL = strlen(pszHttpHost) + strlen(pMethod->pszHTTPLocation);
    char* pszURL = (char*)malloc(cchURL + 1 /* '\0' */);

    char* pszSOAPActionHeader = 0;

    if (pszURL)
    {
        snprintf(pszURL, cchURL + 1, "%s%s", pszHttpHost, pMethod->pszHTTPLocation);

        do
        {
            pHttpRequest = HDK_CLI_Http_RequestCreate(pszURL, fGet);
            if (!pHttpRequest)
            {
                break;
            }

            /* Set the Network Object ID custom header. */
            if (pNetworkObjectID)
            {
                char szNetworkObjectIDHeader[64];

                int ix = sprintf(szNetworkObjectIDHeader, "X-NetworkObjectID: ");

                HDK_XML_OutputStream_BufferContext bufferCtx;
                memset(&bufferCtx, 0, sizeof(bufferCtx));
                bufferCtx.pBuf = &szNetworkObjectIDHeader[ix];
                bufferCtx.cbBuf = sizeof(szNetworkObjectIDHeader) - ix;
                bufferCtx.ixCur = 0;
                if (!HDK_XML_Serialize_UUID(0, HDK_XML_OutputStream_Buffer, &bufferCtx, pNetworkObjectID, 1))
                {
                    break;
                }
                if (!HDK_CLI_Http_RequestAddHeader(pHttpRequest, szNetworkObjectIDHeader))
                {
                    error = HDK_CLI_Error_OutOfMemory;
                    break;
                }
            }

            /* Set the SOAP action header, if one is present. */
            if (pMethod->pszSOAPAction)
            {
                size_t cchSOAPActionHeader = strlen(pMethod->pszSOAPAction) + sizeof("SOAPAction: \"\"") - 1;
                pszSOAPActionHeader = (char*)malloc(cchSOAPActionHeader + 1 /* for '\0' */);

                if (!pszSOAPActionHeader)
                {
                    error = HDK_CLI_Error_OutOfMemory;
                    break;
                }

                snprintf(pszSOAPActionHeader, cchSOAPActionHeader + 1, "SOAPAction: \"%s\"", pMethod->pszSOAPAction);

                /* Set the SOAP action header */
                if (!HDK_CLI_Http_RequestAddHeader(pHttpRequest, pszSOAPActionHeader))
                {
                    break;
                }
            }

            /* Set the Content-Type header, if there is an input struct. */
            if (!(HDK_MOD_MethodOption_NoInputStruct & pMethod->options))
            {
                if (!HDK_CLI_Http_RequestAddHeader(pHttpRequest, "Content-Type: text/xml; charset=\"utf-8\""))
                {
                    break;
                }
            }

            /*
                Don't use Keep-Alive connections.
                WinHTTP will capitalize the 'c' in "Close" when sending this header (can't override this
                behavior), so for consistency this will always be capitalized.
            */
            if (!HDK_CLI_Http_RequestAddHeader(pHttpRequest, "Connection: Close"))
            {
                break;
            }

            /* Set the HTTP Basic Auth, if required .*/
            if (!(HDK_MOD_MethodOption_NoBasicAuth & pMethod->options))
            {
                if (!HDK_CLI_Http_RequestSetBasicAuth(pHttpRequest, pszUsername, pszPassword))
                {
                    break;
                }
            }

            /* Set the HTTP timeout value. */
            if (!HDK_CLI_Http_RequestSetTimeoutSecs(pHttpRequest, timeoutSecs))
            {
                break;
            }

            error = HDK_CLI_Error_OK;
        }
        while (0);

        if (HDK_CLI_FAILED(error))
        {
            DestroyRequest(pHttpRequest);
            pHttpRequest = 0;
        }
    }


    *ppRequest = pHttpRequest;

    free(pszSOAPActionHeader);
    free(pszURL);

    return error;
}

static int AppendDataToStream(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf)
{
    if (pcbStream)
    {
        *pcbStream = cbBuf;
    }

    return HDK_CLI_Http_RequestAppendData(pStreamCtx, pBuf, cbBuf);
}

/* Helper function for serializing an HDK_XML_Struct to a HDK client request. */
static HDK_CLI_Error SerializeStructToRequest(void* pRequest, const HDK_MOD_Method* pMethod, const HDK_XML_Struct* pInput)
{
    /* Serialize the struct. */
    unsigned int cbSerialized = 0;
#ifdef HDK_LOGGING
    LoggingContext loggingCtx;
    LoggingContext_Init(&loggingCtx);

    (void)HDK_XML_Serialize(&cbSerialized, LoggingContext_AppendData, &loggingCtx, pMethod->pSchemaInput, pInput, 0);

    HDK_CLI_LOGFMT5(HDK_LOG_Level_Verbose, "%s %s SOAPAction: \"%s\" request data (%u bytes):\n%s\n", pMethod->pszHTTPMethod, pMethod->pszHTTPLocation, pMethod->pszSOAPAction, cbSerialized, loggingCtx.pData);
    LoggingContext_Cleanup(&loggingCtx);
#endif /* def HDK_LOGGING */

    return HDK_XML_Serialize(&cbSerialized, AppendDataToStream, pRequest, pMethod->pSchemaInput, pInput, 0) ?
      HDK_CLI_Error_OK : HDK_CLI_Error_OutOfMemory;
}

static unsigned int ReceiveHeader_Callback(void* pCtx, char* pData, unsigned int cbData)
{
    /* Unused parameters. */
    (void)pCtx;
    (void)pData;

    return cbData;
}

typedef struct _ReceiveContext
{
    HDK_XML_ParseContextPtr xmlParseContext;
    HDK_XML_ParseError xmlParseError;
} ReceiveContext;

static unsigned int Receive_Callback(void* pReceiveCtx, char* pBuf, unsigned int cbBuf)
{
    ReceiveContext* pCtx = (ReceiveContext*)pReceiveCtx;

    pCtx->xmlParseError = HDK_XML_Parse_Data(pCtx->xmlParseContext, pBuf, cbBuf);

    return (HDK_XML_ParseError_OK == pCtx->xmlParseError) ? cbBuf : 0;
}

/* Helper to map an HTTP resonse code to an HDK_CLI_Error. */
static HDK_CLI_Error MapHttpCodeToClientError(int httpCode)
{
    switch (httpCode)
    {
        case 200: /* HTTP 200 OK */
        {
            return HDK_CLI_Error_OK;
        }
        case 401: /* HTTP 401 Unauthorized */
        {
            return HDK_CLI_Error_HttpAuth;
        }
        case 500: /* HTTP 500 Server Fault */
        {
            return HDK_CLI_Error_SoapFault;
        }
        default:
        {
            return HDK_CLI_Error_HttpUnknown;
        }
    }
}

/* Helper to map an XML parse error to an HDK_CLI_Error. */
static HDK_CLI_Error MapXmlParseErrorToClientError(HDK_XML_ParseError xmlParseError)
{
    switch (xmlParseError)
    {
        case HDK_XML_ParseError_OK:
        {
            return HDK_CLI_Error_OK;
        }
        case HDK_XML_ParseError_XMLUnexpectedElement:
        {
            return HDK_CLI_Error_ResponseInvalid;
        }
        case HDK_XML_ParseError_XMLInvalid:
        case HDK_XML_ParseError_XMLInvalidValue:
        {
            return HDK_CLI_Error_XmlParse;
        }
        case HDK_XML_ParseError_OutOfMemory:
        {
            return HDK_CLI_Error_OutOfMemory;
        }
        case HDK_XML_ParseError_IOError:
        case HDK_XML_ParseError_BadContentLength:
        {
            return HDK_CLI_Error_Unknown;
        }
    }

    return HDK_CLI_Error_Unknown;
}

/* Helper function to send a request and receive/parse the response. */
static HDK_CLI_Error SendRequestAndReceiveResponse
(
    void* pRequest,
    const HDK_XML_Schema* pSchemaOutput,
    HDK_XML_Struct* pOutput
)
{
    HDK_CLI_Error error = HDK_CLI_Error_Unknown;

    ReceiveContext receiveCtx;
    receiveCtx.xmlParseError = HDK_XML_ParseError_OK;
    receiveCtx.xmlParseContext = HDK_XML_Parse_New(pSchemaOutput, pOutput);
    if (receiveCtx.xmlParseContext)
    {
        /*
            Send the request and parse the response.
            Any value > 0 is the HTTP status code of the response and anything else
            indicates an error.
        */
        int requestResult = HDK_CLI_Http_RequestSend(pRequest, ReceiveHeader_Callback, NULL, Receive_Callback, &receiveCtx);
        if (0 < requestResult)
        {
            /* Clean up the XML parser (this includes completion of the XML parsing). */
            HDK_XML_ParseError xmlParseErrorFinal = HDK_XML_Parse_Free(receiveCtx.xmlParseContext);

            error = MapHttpCodeToClientError(requestResult);
            if (HDK_CLI_SUCCEEDED(error))
            {
                /* Return the final parse error if there wasn't a previous one. */
                error = MapXmlParseErrorToClientError((HDK_XML_ParseError_OK == receiveCtx.xmlParseError) ?
                                                      xmlParseErrorFinal : receiveCtx.xmlParseError);
            }
        }
        else
        {
            /* Clean up the XML parser (ignoring the result due to failure to receive a valid response). */
            (void)HDK_XML_Parse_Free(receiveCtx.xmlParseContext);

            error = HDK_CLI_Error_Connection;
        }
    }
    else
    {
        /* Failed to create an XML parser. */
        error = HDK_CLI_Error_OutOfMemory;
    }

    return error;
}

/* Helper function to validate and populate the request input it. */
static HDK_CLI_Error ValidateAndPopulateRequestInput
(
    void* pRequest,
    const HDK_MOD_Method* pMethod,
    const HDK_XML_Struct* pInput
)
{
    HDK_CLI_Error error = HDK_CLI_Error_Unknown;

    HDK_XML_Struct sSchemaInput;
    HDK_XML_Struct* pWrappedInput = &sSchemaInput;

    HDK_XML_Struct_Init(&sSchemaInput);

    do
    {
        /* Only allow input data if the method expects it. */
        if (!(HDK_MOD_MethodOption_NoInputStruct & pMethod->options))
        {
            if (!pInput)
            {
                /* Fail if required input is not provided. */
                error = HDK_CLI_Error_RequestInvalid;
                HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "Input struct not provided for request %p\n", pRequest);
                break;
            }

            /* Prepare the input struct, wrapping if necessary (e.g. in a SOAP envelope). */
            pWrappedInput->node.element = pMethod->pSchemaInput->pSchemaNodes->element;
            if (pMethod->pInputElementPath)
            {
                const HDK_XML_Element* pElement;
                for (pElement = pMethod->pInputElementPath; HDK_MOD_ElementPathEnd != *pElement; ++pElement)
                {
                    pWrappedInput = HDK_XML_Set_Struct(pWrappedInput, *pElement);
                    if (!pWrappedInput)
                    {
                        break;
                    }
                }

                /* The element path should always end at the caller's input struct (unless there was a failure.) */
                assert(!pWrappedInput || (pInput->node.element == pWrappedInput->node.element));
            }

            if (!pWrappedInput)
            {
                /* Failed to allocate space for the wrapping element(s). */
                error = HDK_CLI_Error_OutOfMemory;
                break;
            }

            /*
             * Insert the caller's input struct into the schema input struct (a.k.a. sSchemaInput).
             * This is done to avoid copying the entire input struct into the schema input struct.
             */
            pWrappedInput->pHead = pInput->pHead;
            pWrappedInput->pTail = pInput->pTail;

            /* Validate the input request. */
            if (!HDK_XML_Validate(pMethod->pSchemaInput, &sSchemaInput, 0))
            {
                error = HDK_CLI_Error_RequestInvalid;
                HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "Failed to validate the input structure for request %p\n", pRequest);
                break;
            }

            /* Serialize the request. */
            error = SerializeStructToRequest(pRequest, pMethod, &sSchemaInput);
        }
        else
        {
            /* No input struct. */
            error = HDK_CLI_Error_OK;
        }
    }
    while (0);

    if (pWrappedInput)
    {
        /* Remove the input struct from the sInputSchema struct so it is not deallocated here. */
        pWrappedInput->pHead = pWrappedInput->pTail = NULL;
    }

    /* Cleanup the schema input struct. */
    HDK_XML_Struct_Free(&sSchemaInput);

    return error;
}

/* Helper function to validate the response output and retrieve it. */
static HDK_CLI_Error ValidateAndRetrieveResponseOutput
(
    const HDK_MOD_Method* pMethod,
    HDK_XML_Struct* pSchemaOutput,
    HDK_XML_Struct* pOutput
)
{
    HDK_CLI_Error error = HDK_CLI_Error_Unknown;
    int validateOptions = 0;

    HDK_XML_Struct* pUnwrappedOutput = pSchemaOutput;

    /* Get the output struct from the element path (unwrap). */
    if (pMethod->pOutputElementPath)
    {
        const HDK_XML_Element* pElement;
        for (pElement = pMethod->pOutputElementPath; HDK_MOD_ElementPathEnd != *pElement; ++pElement)
        {
            pUnwrappedOutput = HDK_XML_Get_Struct(pUnwrappedOutput, *pElement);
            if (!pUnwrappedOutput)
            {
                break;
            }
        }
    }

    /* If the output unwrapping failed, this could be due to either an invalid response or an invalid output
     * element path (or output schema). If the response validates against the provided schema, but the output
     * unwrapping has failed this indicates the output schema and output path are inconsistent. This cannot be
     * determined until after validation (and then only for non-"HNAP error"  responses.)
     */
    if (pUnwrappedOutput)
    {
        /* If the response output has an HNAP result, validation may be affected. */
        if (HDK_XML_BuiltinElement_Unknown != pMethod->hnapResultElement)
        {

            /* An HNAP error response will not include any other elements and this must be flagged for validation. */
            int result = HDK_XML_GetEx_Enum(pUnwrappedOutput,
                                            pMethod->hnapResultElement,
                                            pMethod->hnapResultEnumType,
                                            pMethod->hnapResultOK);

            if ((pMethod->hnapResultOK != result) && (pMethod->hnapResultREBOOT != result))
            {
                validateOptions |= HDK_XML_ValidateOption_ErrorOutput;
            }
        }
    }

    /* Validate the output struct.  Invalid responses are considered errors. */
    if (HDK_XML_Validate(pMethod->pSchemaOutput, pSchemaOutput, validateOptions))
    {
        if (pUnwrappedOutput)
        {
            /* The output path should always lead to a siblingless struct. Other output paths are not supported. */
            assert((NULL == pUnwrappedOutput->node.pNext) && (pUnwrappedOutput->node.type == HDK_XML_BuiltinType_Struct));

            /* pOutput isn't checked until here to allow callers to make calls they expect to fail and not require a valid pOutput value. */
            if (pOutput)
            {
                /* Update the output struct from the unwrapped output schema struct. */
                pOutput->node.element = pUnwrappedOutput->node.element;
                pOutput->node.type = HDK_XML_BuiltinType_Struct;
                pOutput->node.pNext = NULL;

                /* Remove the output from the schema output struct and return it. */
                pOutput->pHead = pUnwrappedOutput->pHead;
                pUnwrappedOutput->pHead = NULL;

                pOutput->pTail = pUnwrappedOutput->pTail;
                pUnwrappedOutput->pTail = NULL;

                error = HDK_CLI_Error_OK;
            }
            else
            {
                error = HDK_CLI_Error_InvalidArg;
            }
        }
        else
        {
            error = HDK_CLI_Error_InvalidArg;
            HDK_CLI_LOG(HDK_LOG_Level_Error, "Failed to validate output due to inconsistent element path and schema\n");
        }
    }
    else
    {
        error = HDK_CLI_Error_ResponseInvalid;
        HDK_CLI_LOG(HDK_LOG_Level_Error, "Failed to validate the output structure\n");
    }

    return error;
}


/* HDK_CLI_EXPORT */ HDK_CLI_Error HDK_CLI_Request
(
    const HDK_XML_UUID* pNetworkObjectID,
    const char* pszHttpHost,
    const char* pszUsername,
    const char* pszPassword,
    unsigned int timeoutSecs,
    const HDK_MOD_Module* pModule,
    int method,
    const HDK_XML_Struct* pInput,
    HDK_XML_Struct* pOutput
)
{
    /* Lookup the method */
    const HDK_MOD_Method* pMethod = HDK_MOD_GetMethod(pModule, method);

    /* Create the request object. */
    void* pRequest;
    HDK_CLI_Error error = CreateRequest(&pRequest,
                                        pMethod, pNetworkObjectID,
                                        pszHttpHost, timeoutSecs, pszUsername, pszPassword);
    if (HDK_CLI_SUCCEEDED(error))
    {
        HDK_XML_Struct sSchemaOutput;
        HDK_XML_Struct_Init(&sSchemaOutput);
        do
        {
            error = ValidateAndPopulateRequestInput(pRequest, pMethod, pInput);
            if (HDK_CLI_FAILED(error))
            {
                break;
            }

            error = SendRequestAndReceiveResponse(pRequest, pMethod->pSchemaOutput, &sSchemaOutput);
            if (HDK_CLI_FAILED(error))
            {
                break;
            }

            error = ValidateAndRetrieveResponseOutput(pMethod, &sSchemaOutput, pOutput);
        }
        while (0);

        HDK_XML_Struct_Free(&sSchemaOutput);

        /* Clean up the request object. */
        DestroyRequest(pRequest);
    }
    else
    {
        /* Failed to create a request object. */
    }

    return error;
}
