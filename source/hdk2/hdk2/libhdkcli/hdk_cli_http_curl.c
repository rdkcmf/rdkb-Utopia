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


/*
 * hdk_client_http_curl.c
 *
 *    libcURL-based implementation of hdk_cli_http.h
 */
#include "hdk_cli_http.h"

#include "hdk_cli_log.h"

#ifdef HDK_LOGGING
#include <alloca.h>
#endif /* def HDK_LOGGING */

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>


#define CURL_SUCCEEDED(_code) (CURLE_OK == _code)
#define CURL_FAILED(_code) (!CURL_SUCCEEDED(_code))

typedef struct _CURLRequestContext
{
    CURL* pCURL;
    char* pszURL; /* Prior to libCURL version 7.17, string options must be kept alive until the request is complete (including UTRL) */
    struct curl_slist* pHeaderList;
    char* pszBasicAuthString; /* HTTP Basic Auth string, in form [username]:[password] */
    int fGet;
    char* pData;
    size_t cbData;
    size_t cbAllocated;
} CURLRequestContext;


/*
 * hdk_client_http_interface.h
 */

int HDK_CLI_Http_Init(void)
{
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);

    if (CURL_FAILED(code))
    {
        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "curl_global_init() failed with error %d ('%s')\n", code, curl_easy_strerror(code));
    }

    return CURL_SUCCEEDED(code);
}

void HDK_CLI_Http_Cleanup(void)
{
    curl_global_cleanup();
}

void* HDK_CLI_Http_RequestCreate(const char* pszURL, int fGet)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)malloc(sizeof(CURLRequestContext));
    if (pCtx)
    {
        memset(pCtx, 0, sizeof(*pCtx));

        pCtx->fGet = !!fGet;

        pCtx->pCURL = curl_easy_init();
        if (pCtx->pCURL)
        {
            do
            {
                CURLcode code;

                /* Copy the URL */
                if (pszURL)
                {
                    pCtx->pszURL = (char*)malloc(strlen(pszURL) + 1 /* for '\0' */);
                    if (!pCtx->pszURL)
                    {
                        HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "failed to allocate memory to copy URL '%s'\n", pszURL);
                        break;
                    }
                    strcpy(pCtx->pszURL, pszURL);
                }

                /* Set the request URL and HTTP action */
                code = curl_easy_setopt(pCtx->pCURL, CURLOPT_URL, pCtx->pszURL);
                if (CURL_FAILED(code))
                {
                    HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_URL, %s) failed with error %d ('%s')\n", pCtx->pCURL, pCtx->pszURL, code, curl_easy_strerror(code));
                    break;
                }

                code = curl_easy_setopt(pCtx->pCURL, CURLOPT_POST, !(pCtx->fGet));
                if (CURL_FAILED(code))
                {
                    HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_POST, %u) failed with error %d ('%s')\n", pCtx->pCURL, !(pCtx->fGet), code, curl_easy_strerror(code));
                    break;
                }

                code = curl_easy_setopt(pCtx->pCURL, CURLOPT_HTTPGET, pCtx->fGet);
                if (CURL_FAILED(code))
                {
                    HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_HTTPGET, %u) failed with error %d ('%s')\n", pCtx->pCURL, pCtx->fGet, code, curl_easy_strerror(code));
                    break;
                }

                code = curl_easy_setopt(pCtx->pCURL, CURLOPT_IGNORE_CONTENT_LENGTH, 1);
                if (CURL_FAILED(code))
                {
                    HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_IGNORE_CONTENT_LENGTH) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
                    break;
                }

                code = curl_easy_setopt(pCtx->pCURL, CURLOPT_FOLLOWLOCATION, 1);
                if (CURL_FAILED(code))
                {
                    HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_FOLLOWLOCATION) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
                    break;
                }

                HDK_CLI_LOGFMT2(HDK_LOG_Level_Verbose, "Created request %p for URL '%s'\n", pCtx->pCURL, pCtx->pszURL);

                return pCtx;
            }
            while (0);
        }
        else
        {
            HDK_CLI_LOG(HDK_LOG_Level_Error, "curl_easy_init() failed\n");
        }
    }

    HDK_CLI_Http_RequestDestroy(pCtx);

    return 0;
}

void HDK_CLI_Http_RequestDestroy(void* pRequestCtx)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;
    if (NULL != pCtx)
    {
        HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Destroying request %p\n", pCtx->pCURL);

        free(pCtx->pszURL);

        curl_slist_free_all(pCtx->pHeaderList);
        curl_easy_cleanup(pCtx->pCURL);

        free(pCtx->pszBasicAuthString);
        free(pCtx->pData);
    }

    free(pCtx);
}

int HDK_CLI_Http_RequestAddHeader(void* pRequestCtx, const char* pszHeader)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;
    pCtx->pHeaderList = curl_slist_append(pCtx->pHeaderList, pszHeader);

    return (NULL != pCtx->pHeaderList);
}

int HDK_CLI_Http_RequestSetTimeoutSecs(void* pRequestCtx, unsigned int uiTimeoutSecs)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;

    CURLcode code = curl_easy_setopt(pCtx->pCURL, CURLOPT_TIMEOUT, uiTimeoutSecs);

    if (CURL_FAILED(code))
    {
        HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_TIMEOUT, %u) failed with error %d ('%s')\n", pCtx->pCURL, uiTimeoutSecs, code, curl_easy_strerror(code));
    }
    return CURL_SUCCEEDED(code);
}

int HDK_CLI_Http_RequestSetBasicAuth(void* pRequestCtx,
                                     const char* pszUsername, const char* pszPassword)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;

    CURLcode code;
    char* pszUserPwd = NULL;
    do
    {
        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        if (CURL_FAILED(code))
        {
            break;
        }

        pszUserPwd = (char*)malloc(((pszUsername) ? strlen(pszUsername) : 0) +
                                   1 /* ':' */ +
                                   ((pszPassword) ? strlen(pszPassword) : 0) +
                                   1 /* '\0' */);

        if (!pszUserPwd)
        {
            code = CURLE_OUT_OF_MEMORY;
            break;
        }
        sprintf(pszUserPwd, "%s:%s",
                (pszUsername) ? pszUsername : "",
                (pszPassword) ? pszPassword : "");

        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_USERPWD, pszUserPwd);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_USERPWD, %s) failed with error %d ('%s')\n", pCtx->pCURL, pszUserPwd, code, curl_easy_strerror(code));
            break;
        }
    }
    while (0);

    if (CURL_SUCCEEDED(code))
    {
        /* If the set succeeded, retain the current auth string (libcurl expects it will be valid) */
        free(pCtx->pszBasicAuthString);
        pCtx->pszBasicAuthString = pszUserPwd;
    }
    else
    {
        free(pszUserPwd);
    }

    return CURL_SUCCEEDED(code);
}

int HDK_CLI_Http_RequestAppendData(void* pRequestCtx, const char* pData, unsigned int cbData)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;
    size_t cbDataNew = pCtx->cbData + (size_t)cbData;

    /* Grow the buffer as needed... */
    if (cbDataNew > pCtx->cbAllocated)
    {
        size_t cbNewAlloc = 2 * pCtx->cbAllocated;
        if (cbDataNew > cbNewAlloc)
        {
            cbNewAlloc = cbDataNew;
        }
        pCtx->pData = (char*)realloc(pCtx->pData, cbNewAlloc);
        if (pCtx->pData)
        {
            pCtx->cbAllocated = cbNewAlloc;
        }
    }
    if (pCtx->pData)
    {
        memcpy(&pCtx->pData[pCtx->cbData], pData, cbData);
        pCtx->cbData += cbData;
    }

    return (pCtx->pData || !pCtx->cbData) ? cbData : 0;
}

typedef struct _ReadHeaderContext
{
    HDK_CLI_Http_ReadHeaderFn pfnReadHeader;
    void* pReadHeaderCtx;
} ReadHeaderContext;

typedef struct _ReadContext
{
    HDK_CLI_Http_ReadFn pfnRead;
    void* pReadCtx;
} ReadContext;

static size_t WriteHeader_Callback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    ReadHeaderContext* pCtx = (ReadHeaderContext*)stream;

#ifdef HDK_LOGGING
    size_t cchHeader = ((size * nmemb) / sizeof(char)) - (sizeof("\r\n") - 1);
    if (cchHeader)
    {
        char* pszHeader = (char*)alloca((cchHeader + 1) * sizeof(char));
        memcpy(pszHeader, ptr, cchHeader * sizeof(char));
        pszHeader[cchHeader] = '\0';
        HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Received header '%s'\n", (char*)pszHeader);
    }
#endif /* def HDK_LOGGING */

    return (size_t)pCtx->pfnReadHeader(pCtx->pReadHeaderCtx, (char*)ptr, size * nmemb);
}

static size_t Write_Callback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    ReadContext* pCtx = (ReadContext*)stream;

    HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Received %u bytes of data\n", (unsigned int)(size * nmemb));
    return (size_t)pCtx->pfnRead(pCtx->pReadCtx, (char*)ptr, size * nmemb);
}

int HDK_CLI_Http_RequestSend(void* pRequestCtx,
                             HDK_CLI_Http_ReadHeaderFn pfnReadHeader, void* pReadHeaderCtx,
                             HDK_CLI_Http_ReadFn pfnRead, void* pReadCtx)
{
    CURLRequestContext* pCtx = (CURLRequestContext*)pRequestCtx;

    long httpResponseCode = -1;
    do
    {
        CURLcode code;
        ReadHeaderContext readHeaderCtx;
        ReadContext readCtx;

        readHeaderCtx.pfnReadHeader = pfnReadHeader;
        readHeaderCtx.pReadHeaderCtx = pReadHeaderCtx;

        readCtx.pfnRead = pfnRead;
        readCtx.pReadCtx = pReadCtx;

        /* Add the POST data (if POSTing) */
        if (!pCtx->fGet)
        {
            code = curl_easy_setopt(pCtx->pCURL, CURLOPT_POSTFIELDSIZE, pCtx->cbData);
            if (CURL_FAILED(code))
            {
                HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_POSTFIELDSIZE, %u) failed with error %d ('%s')\n", pCtx->pCURL, (unsigned int)pCtx->cbData, code, curl_easy_strerror(code));
                break;
            }

            code = curl_easy_setopt(pCtx->pCURL, CURLOPT_POSTFIELDS, pCtx->pData);
            if (CURL_FAILED(code))
            {
                HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_POSTFIELDS, 0) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
                break;
            }
        }

        /* Set the read headers callback. */
        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_WRITEHEADER, &readHeaderCtx);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_WRITEHEADER) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
            break;
        }

        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_HEADERFUNCTION, WriteHeader_Callback);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_HEADERFUNCTION) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
            break;
        }

        /* Set the read callback. */
        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_WRITEDATA, &readCtx);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_WRITEDATA) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
            break;
        }

        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_WRITEFUNCTION, Write_Callback);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_WRITEFUNCTION) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
            break;
        }

        code = curl_easy_setopt(pCtx->pCURL, CURLOPT_HTTPHEADER, pCtx->pHeaderList);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_setopt(%p, CURLOPT_HTTPHEADER, %p) failed with error %d ('%s')\n", pCtx->pCURL, (void*)pCtx->pHeaderList, code, curl_easy_strerror(code));
            break;
        }

        /*
         * If the client-supplied callbacks aborted the operation (and we get CURLE_WRITE_ERROR),
         * continue on so we can still return the correct HTTP response code.
         */
        code = curl_easy_perform(pCtx->pCURL);
        if (CURL_FAILED(code) && (CURLE_WRITE_ERROR != code))
        {

#ifdef HDK_LOGGING
            long error = 0;
            (void)curl_easy_getinfo(pCtx->pCURL, CURLINFO_OS_ERRNO, &error);
            HDK_CLI_LOGFMT4(HDK_LOG_Level_Error, "curl_easy_perform(%p) failed with CURL error %d ('%s') and OS error %ld\n", pCtx->pCURL, code, curl_easy_strerror(code), error);
#endif /* def HDK_LOGGING */

            break;
        }

        code = curl_easy_getinfo(pCtx->pCURL, CURLINFO_RESPONSE_CODE, &httpResponseCode);
        if (CURL_FAILED(code))
        {
            HDK_CLI_LOGFMT3(HDK_LOG_Level_Error, "curl_easy_getinfo(%p, CURLINFO_RESPONSE_CODE) failed with error %d ('%s')\n", pCtx->pCURL, code, curl_easy_strerror(code));
            break;
        }

#ifdef HDK_LOGGING
        {
            double totalTime = 0.0;
            char* pszURL = NULL;

            (void)curl_easy_getinfo(pCtx->pCURL, CURLINFO_TOTAL_TIME, &totalTime);
            (void)curl_easy_getinfo(pCtx->pCURL, CURLINFO_EFFECTIVE_URL, &pszURL);

            HDK_CLI_LOGFMT4(HDK_LOG_Level_Info, "HTTP request %p for '%s' completed in %f seconds with STATUS %ld\n", pCtx->pCURL, pszURL, totalTime, httpResponseCode);
        }
#endif /* def HDK_LOGGING */

    }
    while (0);

    return (int)httpResponseCode;
}
