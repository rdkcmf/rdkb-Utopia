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

#ifndef __HDK_SRV_H__
#define __HDK_SRV_H__

#include "hdk_mod.h"
#include "hdk_xml.h"
#include "hdk_log.h"

#ifdef _MSC_VER
#  include <Windows.h>
#endif


/* Macro to control public exports */
#ifdef __cplusplus
#  define HDK_SRV_EXTERN_PREFIX extern "C"
#else
#  define HDK_SRV_EXTERN_PREFIX extern
#endif

#ifdef HDK_SRV_STATIC
#  define HDK_SRV_EXPORT HDK_SRV_EXTERN_PREFIX
#else
#  ifdef _MSC_VER
#    ifdef HDK_SRV_BUILD
#      define HDK_SRV_EXPORT HDK_SRV_EXTERN_PREFIX __declspec(dllexport)
#    else
#      define HDK_SRV_EXPORT HDK_SRV_EXTERN_PREFIX __declspec(dllimport)
#    endif
#  else
#    ifdef HDK_SRV_BUILD
#      define HDK_SRV_EXPORT HDK_SRV_EXTERN_PREFIX __attribute__ ((visibility("default")))
#    else
#      define HDK_SRV_EXPORT HDK_SRV_EXTERN_PREFIX
#    endif
#  endif
#endif


/*
 * HDK Abstract Device Interface (ADI) function types
 */

/* ADI value type */
typedef int HDK_SRV_ADIValue;

/* ADI value get function type */
typedef const char* (*HDK_SRV_ADIGetFn)(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                        const char* pszName);

/* ADI value set function type - return non-zero for success, zero otherwise */
typedef int (*HDK_SRV_ADISetFn)(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                const char* pszName, const char* pszValue);

/* ADI value get */
HDK_SRV_EXPORT HDK_XML_Member* HDK_SRV_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                              HDK_XML_Struct* pStruct, HDK_XML_Element element);

/* ADI value set */
HDK_SRV_EXPORT int HDK_SRV_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                                  HDK_XML_Struct* pStruct, HDK_XML_Element element);

/* ADI method context module */
#define HDK_SRV_ADIModule(pMethodCtx) \
    ((pMethodCtx)->pModuleCtx->pModule)

/* Get ADI value enumeration from a namespace and name */
HDK_SRV_EXPORT int HDK_SRV_ADIGetValue(HDK_SRV_ADIValue* pValue, const HDK_MOD_Module* pModule,
                                       const char* pszNamespace, const char* pszName);

/* ADI value deserialization */
HDK_SRV_EXPORT HDK_XML_Member* HDK_SRV_ADIDeserialize(const HDK_MOD_Module* pModule, HDK_SRV_ADIValue value,
                                                      HDK_XML_Struct* pStruct, HDK_XML_Element element,
                                                      const char* pszValue);

/* ADI value serialization */
HDK_SRV_EXPORT const char* HDK_SRV_ADISerialize(const HDK_MOD_Module* pModule, HDK_SRV_ADIValue value,
                                                HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_SRV_EXPORT const char* HDK_SRV_ADISerializeCopy(const char* pszValue);
HDK_SRV_EXPORT void HDK_SRV_ADISerializeFree(const char* pszValue);


/*
 * HDK ADI helper functions
 */

/* ADI bool value get */
HDK_SRV_EXPORT int HDK_SRV_ADIGetBool(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value);

/*
 * ADI string-indexed get
 *
 * This is the temp struct that comes back from the ADIGet call:
 *
 * <*>
 *   <key/value structure>
 *     <keyElement>KeyString</keyElement>
 *     <valueElement>Value</valueElement>
 *   </key/value structure>
 *   ...
 * <*>
 *
 * key/value structure elements are iterated and the keyElement is string
 * compared to the value passed in through pKeyStruct's keyElement.  If the key
 * strings match, then the valueElement member, which is of type valueType, is
 * copied into pStruct.
 *
 * The return struct pStruct:
 *
 * <*>
 *   <valueElement>Value</valueElement>
 * <*>
 *
 * Where keyElement is type HDK_XML_BuiltinType_String, and valueElement is type
 * valueType.
 */
HDK_SRV_EXPORT HDK_XML_Member* HDK_SRV_ADIGetValue_ByString
(
    HDK_MOD_MethodContext* pMethodCtx,
    HDK_SRV_ADIValue adiValue,
    HDK_XML_Struct* pStruct,
    HDK_XML_Struct* pKeyStruct,
    HDK_XML_Element keyElement,
    HDK_XML_Element valueElement,
    HDK_XML_Type valueType
);

/*
 * ADI string-indexed set
 *
 * The input struct pStruct:
 *
 * <*>
 *   <keyElement>KeyString</keyElement>
 *   <valueElement>Value</valueElement>
 * <*>
 *
 * Where keyElement is type HDK_XML_BuiltinType_String, and valueElement is type
 * valueType.
 *
 * An ADIGet call is made, populating a temp struct. The key/value structure
 * infoElements are iterated and the keyElement is string compared to the value
 * passed through the pStruct's keyElement.  If a matching keyElement is found,
 * the valueElement is updated, otherwise a new infoElement struct with
 * pStruct's keyElement/valueElement is appended to the temp struct.  The temp
 * struct is then passed into the ADISet call.
 *
 * This is the temp struct that is set in the ADISet call:
 *
 * <*>
 *   <infoElement>
 *     <keyElement>KeyString</keyElement>
 *     <valueElement>Value</valueElement>
 *   </infoElement>
 *   ...
 * <*>
 */
HDK_SRV_EXPORT int HDK_SRV_ADISetValue_ByString
(
    HDK_MOD_MethodContext* pMethodCtx,
    HDK_SRV_ADIValue adiValue,
    HDK_XML_Struct* pStruct,
    HDK_XML_Element infoElement,
    HDK_XML_Element keyElement,
    HDK_XML_Element valueElement,
    HDK_XML_Type valueType
);


#ifndef HDK_SRV_NO_SIMULATOR

/*
 * HDK server simulator ADI implementation
 */

typedef struct _HDK_SRV_SimulatorNode
{
    const char* pszNamespace;
    const char* pszName;
    const char* pszValue;
    struct _HDK_SRV_SimulatorNode* pNext;
} HDK_SRV_SimulatorNode;

typedef struct _HDK_SRV_Simulator_ModuleCtx
{
    HDK_SRV_SimulatorNode* pHead;
    HDK_SRV_SimulatorNode* pTail;
} HDK_SRV_Simulator_ModuleCtx;

/* Initialize simulator module context */
HDK_SRV_EXPORT void HDK_SRV_Simulator_Init(HDK_SRV_Simulator_ModuleCtx* pModuleCtx);

/* Free simulator module context */
HDK_SRV_EXPORT void HDK_SRV_Simulator_Free(HDK_SRV_Simulator_ModuleCtx* pModuleCtx);

/* Read simulator state */
HDK_SRV_EXPORT int HDK_SRV_Simulator_Read(HDK_SRV_Simulator_ModuleCtx* pModuleCtx,
                                          HDK_XML_InputStreamFn pfnStream, void* pStreamCtx);

/* Write simulator state */
HDK_SRV_EXPORT int HDK_SRV_Simulator_Write(HDK_SRV_Simulator_ModuleCtx* pModuleCtx,
                                           HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx);

/* Simulator ADIGet implementation */
HDK_SRV_EXPORT const char* HDK_SRV_Simulator_ADIGet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                                    const char* pszName);

/* Simulator ADISet implementation */
HDK_SRV_EXPORT int HDK_SRV_Simulator_ADISet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                            const char* pszName, const char* pszValue);

#endif /* #ifndef HDK_SRV_NO_SIMULATOR */


/*
 * HDK server module context
 */

/* Server module context struct */
typedef struct _HDK_SRV_ModuleContext
{
    const HDK_MOD_Module* pModule;
    void* pModuleCtx;
    HDK_SRV_ADIGetFn pfnADIGet;
    HDK_SRV_ADISetFn pfnADISet;
} HDK_SRV_ModuleContext;

/* Module context list terminator */
#define HDK_SRV_ModuleContextEnd (HDK_SRV_ModuleContext*)0


/*
 * HDK server request handling
 */

/* Authorization callback function */
typedef int (*HDK_SRV_AuthFn)(void* pServerCtx, const char* pszUser, const char* pszPassword);

/* HNAP result callback function definition */
typedef void (*HDK_SRV_HNAPResultFn)(void* pServerCtx, HDK_XML_Struct* pOutput,
                                     HDK_XML_Element resultElement, HDK_XML_Type resultType,
                                     int resultOK, int resultReboot);

/* Handle a request - return non-zero if handled, zero otherwise */
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
);


/*
 * Call HDK server methods directly
 */

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
);

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
);


/*
 * Dynamic server module support
 */

/* Dynamic server module handle */
#ifdef _MSC_VER
#  define HDK_SRV_MODULE_HANDLE HMODULE
#else
#  define HDK_SRV_MODULE_HANDLE void*
#endif

/* Load/unload module */
HDK_SRV_EXPORT HDK_SRV_MODULE_HANDLE HDK_SRV_OpenModule(HDK_MOD_ModuleFn* ppfnModule, const char* pszModule);
HDK_SRV_EXPORT void HDK_SRV_CloseModule(HDK_SRV_MODULE_HANDLE hModule);


/*
 * Logging
 */

HDK_SRV_EXPORT void HDK_SRV_RegisterLoggingCallback(HDK_LOG_CallbackFn pfnCallback, void* pCtx);

#endif /* __HDK_SRV_H__ */
