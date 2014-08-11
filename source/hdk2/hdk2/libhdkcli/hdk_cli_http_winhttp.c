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
 * hdk_cli_http_winhttp.c
 *
 *    WinHTTP-based implementation of hdk_cli_http.h
 */
#include "hdk_cli_http.h"
#include "hdk_xml.h"

#include "hdk_cli_log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <windows.h>
#include <winhttp.h>


typedef struct _WinHttpRequestContext
{
    HINTERNET hSession;
    HINTERNET hConnection;
    HINTERNET hRequest;
    int fGet;
    char* pData;
    size_t cbData;
    size_t cbAllocated;

    HDK_CLI_Http_ReadHeaderFn pfnReadHeader;
    void* pReadHeaderCtx;
    HDK_CLI_Http_ReadFn pfnRead;
    void* pReadCtx;

    char readBuffer[1024];
    DWORD httpResponseCode;
    DWORD cbContentLength;
    DWORD cbContentRead;

    DWORD error;
    HANDLE hCompleteEvent;

    LPWSTR pwszHttpBasicAuthHeader;
} WinHttpRequestContext;

static char s_szHeaderSeperator[] = "\r\n";

/*
 * hdk_cli_http_winhttp.h
 */

/* Caller must free returned string. */
static char* WideCharToUTF8(LPCWSTR lpwsz)
{
    int cchRequired = 0;
    char* psz = 0;

    int fAllocate;
    for (fAllocate = 1; fAllocate >= 0; --fAllocate)
    {
        cchRequired = WideCharToMultiByte(CP_UTF8, 0, lpwsz, -1 /* convert up to and including '\0' */, psz, cchRequired, NULL, NULL);
        if (fAllocate)
        {
            psz = (char*)malloc(cchRequired * sizeof(char));
        }
    }

    if (!cchRequired)
    {
        free(psz);
        psz = NULL;
    }

    return psz;
}

/* Caller MUST free returned string. */
static LPWSTR UTF8ToWideChar(const char* psz)
{
    int cchRequired = 0;
    LPWSTR lpwsz = 0;

    int fAllocate;
    for (fAllocate = 1; fAllocate >= 0; --fAllocate)
    {
        cchRequired = MultiByteToWideChar(CP_UTF8, 0, psz, -1 /* convert up to and including '\0' */, lpwsz, cchRequired);
        if (fAllocate)
        {
            lpwsz = (LPWSTR)malloc(cchRequired * sizeof(WCHAR));
        }
    }

    if (!cchRequired)
    {
        free(lpwsz);
        lpwsz = NULL;
    }

    return lpwsz;
}

/* Caller MUST free returned string. */
static char* Base64Encode(char* pszToEncode)
{
    unsigned int cbEncoded1;
    unsigned int cbEncoded2;
    HDK_XML_OutputStream_BufferContext bufferCtx;
    HDK_XML_OutputStream_EncodeBase64_Context encodeCtx;

    memset(&bufferCtx, 0, sizeof(bufferCtx));

    memset(&encodeCtx, 0, sizeof(encodeCtx));
    encodeCtx.pfnStream = HDK_XML_OutputStream_GrowBuffer;
    encodeCtx.pStreamCtx = &bufferCtx;

    if (HDK_XML_OutputStream_EncodeBase64(&cbEncoded1, &encodeCtx, pszToEncode, (unsigned int)strlen(pszToEncode)) &&
        HDK_XML_OutputStream_EncodeBase64Done(&cbEncoded2, &encodeCtx))
    {
        bufferCtx.pBuf[cbEncoded1 + cbEncoded2] = '\0';
        return bufferCtx.pBuf;
    }

    free(bufferCtx.pBuf);

    return 0;
}

static BOOL ReceiveResponse(WinHttpRequestContext* pCtx)
{
    if (!WinHttpReceiveResponse(pCtx->hRequest, NULL /* Reserved.  Must be set to NULL. */))
    {
        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpReceiveResponse(%p) failed with error %u\n", pCtx->hRequest, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static BOOL ReadData(WinHttpRequestContext* pCtx)
{
    if (!WinHttpReadData(pCtx->hRequest, pCtx->readBuffer, sizeof(pCtx->readBuffer), NULL))
    {
        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpReadData(%p) failed with error %u\n", pCtx->hRequest, GetLastError());
        return FALSE;
    }

    return TRUE;
}

/* WinHTTP status callback */
static void CALLBACK WinHttp_Status_Callback(HINTERNET hInternet, DWORD_PTR dwContext,
                                             DWORD dwInternetStatus,
                                             LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)dwContext;

    BOOL fSuccess = TRUE;

    (void)hInternet;
    assert(hInternet == pCtx->hRequest);

    HDK_CLI_LOGFMT5(HDK_LOG_Level_Verbose, "Callback %p %08x 0x%08x %p %u\n", hInternet, dwContext, dwInternetStatus, lpvStatusInformation, dwStatusInformationLength);

    switch (dwInternetStatus)
    {
        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            assert(*((HINTERNET*)lpvStatusInformation) == pCtx->hRequest);
            pCtx->hRequest = 0;

            /* If this fails, we will hang indefinitely. */
            SetEvent(pCtx->hCompleteEvent);
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            /* Pass the headers to the callback. */
            BOOL fContinue = TRUE;

            DWORD cbHeaders = 0;

            void* pvBuffer = 0;

            /* First pass is to determine the size of the buffer needed and allocate, second pass to retrieve data. */
            int fAllocate;
            for (fAllocate = 1; fAllocate >= 0; fAllocate--)
            {
                fSuccess = WinHttpQueryHeaders(pCtx->hRequest,
                                               WINHTTP_QUERY_RAW_HEADERS,
                                               WINHTTP_HEADER_NAME_BY_INDEX,
                                               (fAllocate) ? WINHTTP_NO_OUTPUT_BUFFER : pvBuffer,
                                               &cbHeaders,
                                               WINHTTP_NO_HEADER_INDEX);

                if (fAllocate)
                {
                    pvBuffer = malloc(cbHeaders);
                    if (!pvBuffer)
                    {
                        fSuccess = FALSE;
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        break;
                    }
                }
            }

            if (fSuccess)
            {
                WCHAR* pwszHeader = (WCHAR*)pvBuffer;
                while (0 != *pwszHeader)
                {
                    /* Covnert to UTF8. */
                    char* pszHeader = WideCharToUTF8(pwszHeader);

                    HDK_CLI_LOGFMT2(HDK_LOG_Level_Verbose, "Request %p received header '%s'\n", hInternet, pszHeader);

                    if (pszHeader)
                    {
                        unsigned int cbHeader = (unsigned int)strlen(pszHeader);

                        /* Pass to header callback. */
                        fContinue = (cbHeader == pCtx->pfnReadHeader(pCtx->pReadHeaderCtx, pszHeader, cbHeader));

                        free(pszHeader);

                        if (!fContinue)
                        {
                            break;
                        }
                    }
                    else
                    {
                        fSuccess = FALSE;
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        break;
                    }

                    pwszHeader += wcslen(pwszHeader) + 1; /* for terminating \0 */
                }
            }

            free(pvBuffer);

            if (fSuccess)
            {
                /* Get the HTTP response code. */
                DWORD dwSize = sizeof(pCtx->httpResponseCode);
                fSuccess = WinHttpQueryHeaders(pCtx->hRequest,
                                               WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                               WINHTTP_HEADER_NAME_BY_INDEX,
                                               &pCtx->httpResponseCode,
                                               &dwSize,
                                               WINHTTP_NO_HEADER_INDEX);
            }

            if (fSuccess)
            {
                /* Get the Content-Length header value. */
                DWORD cbHeaderLength = 0;
                LPWSTR pwszHeaderValue = WINHTTP_NO_OUTPUT_BUFFER;

                int fAlloc;
                for (fAlloc = 1; fAlloc >= 0; fAlloc--)
                {
                    fSuccess = WinHttpQueryHeaders(pCtx->hRequest,
                                                   WINHTTP_QUERY_CONTENT_LENGTH,
                                                   WINHTTP_HEADER_NAME_BY_INDEX,
                                                   pwszHeaderValue,
                                                   &cbHeaderLength,
                                                   WINHTTP_NO_HEADER_INDEX);

                    if (fAlloc)
                    {
                        pwszHeaderValue = (LPWSTR)malloc(cbHeaderLength);
                        if (NULL == pwszHeaderValue)
                        {
                            fSuccess = FALSE;
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                            break;
                        }
                    }
                }

                if (fSuccess)
                {
                    /* Parse the Content-Length value. An invalid value will return 0 and the Content-Length check will be skipped. */
                    LPWSTR pwszEnd = 0;
                    long contentLength = wcstol(pwszHeaderValue, &pwszEnd, 10);
                    if (pwszHeaderValue == pwszEnd || contentLength < 0)
                    {
                        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "Request %p encountered invalid Content-Length header value '%ws'\n", pCtx->hRequest, pwszHeaderValue);
                        fSuccess = FALSE;
                        SetLastError(ERROR_WINHTTP_INVALID_SERVER_RESPONSE);
                    }
                    else
                    {
                        pCtx->cbContentLength = (DWORD)contentLength;
                    }
                }
                else
                {
                    /* If no Content-Length header was provided, continue on. */
                    fSuccess = (ERROR_WINHTTP_HEADER_NOT_FOUND == GetLastError());
                }

                free(pwszHeaderValue);
            }

            if (fSuccess)
            {
                if (fContinue)
                {
                    fSuccess = ReadData(pCtx);
                }
                else
                {
                    /* Client aborted reading of response. */
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Warning, "Client aborted request %p during response headers\n", hInternet);

                    WinHttpCloseHandle(pCtx->hRequest);
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            assert(lpvStatusInformation == pCtx->readBuffer);
            if (dwStatusInformationLength)
            {
                HDK_CLI_LOGFMT2(HDK_LOG_Level_Verbose, "Read %u bytes of data from request %p\n", dwStatusInformationLength, hInternet);

                pCtx->cbContentRead += dwStatusInformationLength;
                if ((unsigned int)dwStatusInformationLength == pCtx->pfnRead(pCtx->pReadCtx, (char*)lpvStatusInformation, (unsigned int)dwStatusInformationLength))
                {
                    /* Read the next block of data. */
                    fSuccess = ReadData(pCtx);
                }
                else
                {
                    /* Client aborted reading of response. */
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Warning, "Client aborted request %p during response data\n", hInternet);

                    WinHttpCloseHandle(pCtx->hRequest);
                }
            }
            else
            {
                HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Completed read of request %p\n", hInternet);

                /* Done reading data. */
                WinHttpCloseHandle(pCtx->hRequest);

                /* If the provided Content-Length header value does not match the actual received byte count, log a warning. */
                if ((0 != pCtx->cbContentLength) && (pCtx->cbContentRead != pCtx->cbContentLength))
                {
                    HDK_CLI_LOGFMT3(HDK_LOG_Level_Warning, "Request %p read an unexpected number of bytes [expected: %u, actual: %u]\n", hInternet, pCtx->cbContentLength, pCtx->cbContentRead);
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_REDIRECT:
        {
            HDK_CLI_LOGFMT2(HDK_LOG_Level_Verbose, "Redirecting request %p to %ws\n", pCtx->hRequest, (LPWSTR)lpvStatusInformation);

            /* WinHTTP will not authentication headers in redirected requests, therefore it must be added explicitly. */
            fSuccess = WinHttpAddRequestHeaders(pCtx->hRequest, pCtx->pwszHttpBasicAuthHeader, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
            if (!fSuccess)
            {
                HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "Failed to set Basic Auth header on redirected request %p with error %d\n", pCtx->hRequest, GetLastError());
            }
            break;
        }
        case WINHTTP_CALLBACK_FLAG_REQUEST_ERROR:
        {
            WINHTTP_ASYNC_RESULT* pAsyncResult = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;

            HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttp request error %u in API %u\n", pAsyncResult->dwError, pAsyncResult->dwResult);

            /* End the request on any error. */
            pCtx->error = pAsyncResult->dwError;
            WinHttpCloseHandle(pCtx->hRequest);
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            fSuccess = ReceiveResponse(pCtx);
            break;
        }
        default:
        {
            break;
        }
    }

    if (!fSuccess)
    {
        pCtx->error = GetLastError();
        WinHttpCloseHandle(pCtx->hRequest);
    }
}

int HDK_CLI_Http_Init(void)
{
    return 1;
}

void HDK_CLI_Http_Cleanup(void)
{
}

void* HDK_CLI_Http_RequestCreate(const char* pszURL, int fGet)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)malloc(sizeof(WinHttpRequestContext));
    if (pCtx)
    {
        LPWSTR lpwszURL = UTF8ToWideChar(pszURL);

        memset(pCtx, 0, sizeof(*pCtx));

        pCtx->fGet = !!fGet;

        if (lpwszURL)
        {
            do
            {
                WCHAR wcTmp;
                DWORD dwFlags = 0;

                URL_COMPONENTS urlComponents;

                memset(&urlComponents, 0, sizeof(urlComponents));
                urlComponents.dwStructSize = sizeof(urlComponents);
                urlComponents.dwHostNameLength = (DWORD)-1;
                urlComponents.dwUrlPathLength = (DWORD)-1;

                if (!WinHttpCrackUrl(lpwszURL, 0, 0, &urlComponents))
                {
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "WinHttpCrackUrl failed with error %d\n", GetLastError());
                    break;
                }

                /* NULL-terminate the URL string to pass as the hostname string. */
                wcTmp = urlComponents.lpszHostName[urlComponents.dwHostNameLength];
                urlComponents.lpszHostName[urlComponents.dwHostNameLength] = L'\0';

                pCtx->hSession = WinHttpOpen(0, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
                if (!pCtx->hSession)
                {
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "WinHttpOpen failed with error %d", GetLastError());
                    break;
                }

                pCtx->hConnection = WinHttpConnect(pCtx->hSession, urlComponents.lpszHostName, urlComponents.nPort, 0 /* Reserved.  Must be zero. */);
                if (!pCtx->hConnection)
                {
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "WinHttpConnect failed with error %d", GetLastError());
                    break;
                }

                /* Restore the URL string */
                urlComponents.lpszHostName[urlComponents.dwHostNameLength] = wcTmp;

                dwFlags = WINHTTP_FLAG_REFRESH /* Pragma: no-cache */ |
                  WINHTTP_FLAG_NULL_CODEPAGE /* URL object is only valid ANSI characters  */;
                if (INTERNET_DEFAULT_HTTPS_PORT == urlComponents.nPort)
                {
                    dwFlags |= WINHTTP_FLAG_SECURE;
                }

                pCtx->hRequest = WinHttpOpenRequest(pCtx->hConnection,
                                                    (pCtx->fGet) ? L"GET" : L"POST",
                                                    urlComponents.lpszUrlPath,
                                                    0,
                                                    WINHTTP_NO_REFERER,
                                                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    dwFlags);
                if (!pCtx->hRequest)
                {
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "WinHttpOpenRequest failed with error %d\n", GetLastError());
                    break;
                }

                /*
                    Set the request context value before setting the callback.  Once the callback is set, our context must be present to
                    handle any callbacks that may occur.
                */
                if (!WinHttpSetOption(pCtx->hRequest, WINHTTP_OPTION_CONTEXT_VALUE, &pCtx, sizeof(DWORD_PTR)))
                {
                    HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpSetOption(%p, WINHTTP_OPTION_CONTEXT_VALUE) failed with error %d\n", pCtx->hRequest, GetLastError());
                    break;
                }

#ifdef _M_X64
#  undef WINHTTP_INVALID_STATUS_CALLBACK
#  define WINHTTP_INVALID_STATUS_CALLBACK ((WINHTTP_STATUS_CALLBACK)(-1LL))
#endif
                if (WINHTTP_INVALID_STATUS_CALLBACK == WinHttpSetStatusCallback(pCtx->hRequest, WinHttp_Status_Callback,
                                                                                WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                                                                                0 /* Reserved. Must be NULL. */))
                {
                    HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpSetStatusCallback(%p) failed with error %d\n", pCtx->hRequest, GetLastError());
                    break;
                }

                dwFlags = WINHTTP_DISABLE_KEEP_ALIVE;
                if (!WinHttpSetOption(pCtx->hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &dwFlags, sizeof(dwFlags)))
                {
                    HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpSetOption(%p, WINHTTP_DISABLE_KEEP_ALIVE) failed with error %d\n", pCtx->hRequest, GetLastError());
                    break;
                }

#ifndef VALIDATE_CERTIFCATES
                /* Disable ceriticate validation. */
                dwFlags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                        SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

                if (!WinHttpSetOption(pCtx->hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags)))
                {
                    HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpSetOption(%p, WINHTTP_OPTION_SECURITY_FLAGS) failed with error %d\n", pCtx->hRequest, GetLastError());
                    break;
                }
#endif /* ndef VALIDATE_CERTIFCATES */

                pCtx->hCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (!pCtx->hCompleteEvent)
                {
                    HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "CreateEvent failed with error %d\n", GetLastError());
                    break;
                }

                free(lpwszURL);

                HDK_CLI_LOGFMT2(HDK_LOG_Level_Verbose, "Created request %p for URL '%s'\n", pCtx->hRequest, pszURL);

                return pCtx;
            }
            while (0);

            free(lpwszURL);
        }
    }

    HDK_CLI_Http_RequestDestroy(pCtx);

    return 0;
}

void HDK_CLI_Http_RequestDestroy(void* pRequestCtx)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;
    if (pCtx)
    {
        if (pCtx->hRequest)
        {
            HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Destroyed request %p\n", pCtx->hRequest);

            (void)WinHttpCloseHandle(pCtx->hRequest);
            pCtx->hRequest = 0;
        }
        if (pCtx->hConnection)
        {
            WinHttpCloseHandle(pCtx->hConnection);
            pCtx->hConnection = 0;
        }
        if (pCtx->hSession)
        {
            WinHttpCloseHandle(pCtx->hSession);
            pCtx->hSession = 0;
        }

        if (pCtx->hCompleteEvent)
        {
            CloseHandle(pCtx->hCompleteEvent);
            pCtx->hCompleteEvent = 0;
        }

        free(pCtx->pData);
    }

    free(pCtx);
}

int HDK_CLI_Http_RequestAddHeader(void* pRequestCtx, const char* pszHeader)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;

    int iSuccess = 0;

    /* Must append the header seperators before pushing to WinHTTP. */
    size_t cchHeaderWithSeperator = strlen(pszHeader)+ sizeof(s_szHeaderSeperator) /* this will include space for '\0' */;
    char* pszHeaderWithSeperator = (char*)malloc(cchHeaderWithSeperator);
    if (pszHeaderWithSeperator)
    {
        LPWSTR lpwszHeader = 0;

        strcpy(pszHeaderWithSeperator, pszHeader);
        strcat(pszHeaderWithSeperator, s_szHeaderSeperator);

        lpwszHeader = UTF8ToWideChar(pszHeaderWithSeperator);
        if (lpwszHeader)
        {
            iSuccess = WinHttpAddRequestHeaders(pCtx->hRequest, lpwszHeader, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
            if (iSuccess)
            {
                HDK_CLI_LOGFMT1(HDK_LOG_Level_Verbose, "Including header '%s'\n", pszHeader);
            }
            else
            {
                HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpAddRequestHeaders(%p) failed with error %d\n", pCtx->hRequest, GetLastError());
            }
            free(lpwszHeader);
        }

        free(pszHeaderWithSeperator);
    }

    return iSuccess;
}

int HDK_CLI_Http_RequestSetTimeoutSecs(void* pRequestCtx, unsigned int uiTimeoutSecs)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;

    /* WinHTTP treats 0 as "no timeout", so only set non-zero values. */
    if (0 != uiTimeoutSecs)
    {
        /*
         * Set each timeout value to the caller-specified value.  While this may not be the intention,
         * WinHTTP does not expose a way to set a "general timeout" for HTTP requests, so this is the
         * compromise chosen.
         */
        int iTimeoutMsecs = (int)uiTimeoutSecs * 1000;
        return WinHttpSetTimeouts(pCtx->hRequest, iTimeoutMsecs, iTimeoutMsecs, iTimeoutMsecs, iTimeoutMsecs);
    }
    else
    {
/* WinHTTP default timeouts (taken from MSDN documentation */
#define DEFAULT_WINHTTP_RESOLVE_TIMEOUT_MSECS (0 * 1000)
#define DEFAULT_WINHTTP_CONNECT_TIMEOUT_MSECS (60 * 1000)
#define DEFAULT_WINHTTP_SEND_TIMEOUT_MSECS (30 * 1000)
#define DEFAULT_WINHTTP_RECEIVE_TIMEOUT_MSECS (30 * 1000)

        return WinHttpSetTimeouts(pCtx->hRequest,
                                  DEFAULT_WINHTTP_RESOLVE_TIMEOUT_MSECS,
                                  DEFAULT_WINHTTP_CONNECT_TIMEOUT_MSECS,
                                  DEFAULT_WINHTTP_SEND_TIMEOUT_MSECS,
                                  DEFAULT_WINHTTP_RECEIVE_TIMEOUT_MSECS);
    }
}

int HDK_CLI_Http_RequestSetBasicAuth(void* pRequestCtx,
                                     const char* pszUsername, const char* pszPassword)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;

    int iSuccess = 0;

    /* With WinHTTP, the HTTP basic auth header must be constructed manually because WinHTTPSetCredentials won't accept "" as a username. */

    size_t cchUsername = (pszUsername) ? strlen(pszUsername) : 0;
    size_t cchPassword = (pszPassword) ? strlen(pszPassword) : 0;

    char* pszCredentials = 0;
    char* pszCredentialsBase64Encoded = 0;

    LPWSTR lpwszHttpBasicAuthHeader = 0;
    do
    {
        size_t cchHeader = 0;

        /* Construct the USERNAME:PASSWORD string which is then base-64 encoded. */
        pszCredentials = (char*)malloc(cchUsername + 1 /* for ':' */ + cchPassword + 1 /* for '\0' */);
        if (!pszCredentials)
        {
            break;
        }
        *pszCredentials = 0;

        sprintf(pszCredentials, "%s:%s", (pszUsername) ? pszUsername : "", (pszPassword) ? pszPassword : "");

        /* Base-64 encode the credentials. */
        pszCredentialsBase64Encoded = Base64Encode(pszCredentials);
        if (!pszCredentialsBase64Encoded)
        {
            HDK_CLI_LOGFMT1(HDK_LOG_Level_Error, "Failed to base64 encode string '%s'\n", pszCredentials);
            break;
        }

        /* Construct the HTTP basic auth header. */
        cchHeader = strlen(pszCredentialsBase64Encoded) + 21 /* for "Authorization: Basic " */ + sizeof(s_szHeaderSeperator);
        lpwszHttpBasicAuthHeader = (LPWSTR)malloc((cchHeader + 1 /* for '\0' */) * sizeof(WCHAR));
        if (!lpwszHttpBasicAuthHeader)
        {
            break;
        }

        swprintf(lpwszHttpBasicAuthHeader, cchHeader, L"Authorization: Basic %S%S", pszCredentialsBase64Encoded, s_szHeaderSeperator);

        iSuccess = WinHttpAddRequestHeaders(pCtx->hRequest, lpwszHttpBasicAuthHeader, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

        if (!iSuccess)
        {
            HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "Failed to set Basic Auth header on request %p with error %d\n", pCtx->hRequest, GetLastError());
        }
    }
    while (0);

    free(pszCredentials);
    free(pszCredentialsBase64Encoded);

    if (iSuccess)
    {
        /* Cache the Basic Auth header as it may be needed again. */
        free(pCtx->pwszHttpBasicAuthHeader);
        pCtx->pwszHttpBasicAuthHeader = lpwszHttpBasicAuthHeader;
    }
    else
    {
        free(lpwszHttpBasicAuthHeader);
    }

    return iSuccess;
}

int HDK_CLI_Http_RequestAppendData(void* pRequestCtx, const char* pData, unsigned int cbData)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;
    size_t cbDataNew = pCtx->cbData + (size_t)cbData;

    /* Grow the buffer as needed... */
    if (cbDataNew > pCtx->cbAllocated)
    {
        size_t cbNewAlloc = 2 * pCtx->cbAllocated;
        char* pNewData;
        if (cbDataNew > cbNewAlloc)
        {
            cbNewAlloc = cbDataNew;
        }
        pNewData = (char*)realloc(pCtx->pData, cbNewAlloc);
        if (!pNewData)
        {
            free(pCtx->pData);
        }
        pCtx->pData = pNewData;
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

int HDK_CLI_Http_RequestSend(void* pRequestCtx,
                             HDK_CLI_Http_ReadHeaderFn pfnReadHeader, void* pReadHeaderCtx,
                             HDK_CLI_Http_ReadFn pfnRead, void* pReadCtx)
{
    WinHttpRequestContext* pCtx = (WinHttpRequestContext*)pRequestCtx;

    void* pPOSTData = pCtx->pData;
    DWORD cbData = (DWORD)pCtx->cbData;

    if (pCtx->fGet)
    {
#ifdef HDK_LOGGING
        if (cbData)
        {
            HDK_CLI_LOGFMT2(HDK_LOG_Level_Warning, "HTTP GET request %p contains %u bytes of data; data will be ignored.\n", pCtx->hRequest, cbData);
        }
#endif /*def HDK_LOGGING */

        pPOSTData = NULL;
        cbData = 0;
    }

    pCtx->pfnReadHeader = pfnReadHeader;
    pCtx->pReadHeaderCtx = pReadHeaderCtx;
    pCtx->pfnRead = pfnRead;
    pCtx->pReadCtx = pReadCtx;

    if (WinHttpSendRequest(pCtx->hRequest,
                           WINHTTP_NO_ADDITIONAL_HEADERS,
                           0,
                           pPOSTData,
                           cbData,
                           cbData,
                           (DWORD_PTR)pCtx))
    {
#ifdef HDK_LOGGING
        HINTERNET hRequest = pCtx->hRequest;
#endif /* def HDK_LOGGING */

        /* Wait for the request to complete. */
        WaitForSingleObject(pCtx->hCompleteEvent, INFINITE);

        if (ERROR_SUCCESS == pCtx->error)
        {
            HDK_CLI_LOGFMT2(HDK_LOG_Level_Info, "HTTP request %p completed with STATUS %d\n", hRequest, pCtx->httpResponseCode);
            return pCtx->httpResponseCode;
        }

        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "HTTP request %p failed with error %d\n", hRequest, pCtx->error);
    }
    else
    {
        HDK_CLI_LOGFMT2(HDK_LOG_Level_Error, "WinHttpSendRequest(%p) failed with error %d\n", pCtx->hRequest, GetLastError());
    }

    return -1;
}
