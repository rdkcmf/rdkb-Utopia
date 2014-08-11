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

#ifndef __HDK_CLI_H__
#define __HDK_CLI_H__

#include "hdk_mod.h"
#include "hdk_xml.h"
#include "hdk_log.h"


/* Macro to control public exports */
#ifdef __cplusplus
#  define HDK_CLI_EXTERN_PREFIX extern "C"
#else
#  define HDK_CLI_EXTERN_PREFIX extern
#endif

#ifdef HDK_CLI_STATIC
#  define HDK_CLI_EXPORT HDK_CLI_EXTERN_PREFIX
#else
#  ifdef _MSC_VER
#    ifdef HDK_CLI_BUILD
#      define HDK_CLI_EXPORT HDK_CLI_EXTERN_PREFIX __declspec(dllexport)
#    else
#      define HDK_CLI_EXPORT HDK_CLI_EXTERN_PREFIX __declspec(dllimport)
#    endif
#  else
#    ifdef HDK_CLI_BUILD
#      define HDK_CLI_EXPORT HDK_CLI_EXTERN_PREFIX __attribute__ ((visibility("default")))
#    else
#      define HDK_CLI_EXPORT HDK_CLI_EXTERN_PREFIX
#    endif
#  endif
#endif

/*
 * Global initialization / cleanup
 */

/* Must be called prior to making any other HDK client library call.  Returns non-0 on success. */
HDK_CLI_EXPORT int HDK_CLI_Init(void);

/* Must be called once for each successful call to HDK_CLI_Init(). */
HDK_CLI_EXPORT void HDK_CLI_Cleanup(void);


/*
 * HDK client method call
 */

/* HDK client error code enumeration */
typedef enum _HDK_CLI_Error
{
    HDK_CLI_Error_OK = 0,               /* No error */
    HDK_CLI_Error_RequestInvalid,       /* Error validating the request. */
    HDK_CLI_Error_ResponseInvalid,      /* Error validating the response. */
    HDK_CLI_Error_SoapFault,            /* SOAP fault returned by server. */
    HDK_CLI_Error_XmlParse,             /* Error parsing the XML SOAP response. */
    HDK_CLI_Error_HttpAuth,             /* Invalid HTTP Basic Authentication credentials. */
    HDK_CLI_Error_HttpUnknown,          /* Unknown/Unexpected HTTP response code. */
    HDK_CLI_Error_Connection,           /* Transport-layer connection error. */
    HDK_CLI_Error_OutOfMemory,          /* Memory allocation failed. */
    HDK_CLI_Error_InvalidArg,           /* An argument is invalid. */
    HDK_CLI_Error_Unknown               /* Unknown error */
} HDK_CLI_Error;

/* Perform an HDK method call */
HDK_CLI_EXPORT HDK_CLI_Error HDK_CLI_Request
(
    const HDK_XML_UUID* pNetworkObjectID, /* May be 0 - The network object ID of target of this method call */
    const char* pszHttpHost,              /* The host and protocol scheme for the request.
                                             E.g. "https://192.168.1.1" or http://localhost:8001 */
    const char* pszUsername,              /* The HTTP Basic Auth username. */
    const char* pszPassword,              /* The HTTP Basic Auth password. */
    unsigned int timeoutSecs,             /* The request timeout, in seconds. 0 indicates the system default. */
    const HDK_MOD_Module* pModule,        /* The HDK module pointer */
    int method,                           /* The method enumeration value */
    const HDK_XML_Struct* pInput,         /* The input parameters for the HNAP call. */
    HDK_XML_Struct* pOutput               /* The output parameters from the HNAP call. */
);

/*
 * Logging
 */
HDK_CLI_EXPORT void HDK_CLI_RegisterLoggingCallback(HDK_LOG_CallbackFn pfn, void* pCtx);

#endif /* __HDK_CLI_H__ */
