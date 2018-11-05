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

#ifndef __HDK_MOD_H__
#define __HDK_MOD_H__

#include "hdk_xml.h"


/*
 * HDK server method callback
 */

/* Server method context */
typedef struct _HDK_MOD_MethodContext
{
    struct _HDK_SRV_ModuleContext* pModuleCtx;
    struct _HDK_SRV_ModuleContext** ppModuleCtxs;
} HDK_MOD_MethodContext;

/* Server method callback function */
typedef void (*HDK_MOD_MethodFn)(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * HDK method definition
 */

/* Method options */
typedef enum _HDK_MOD_MethodOptions
{
    HDK_MOD_MethodOption_NoLocationSlash = 0x01,
    HDK_MOD_MethodOption_NoBasicAuth = 0x02,
    HDK_MOD_MethodOption_NoInputStruct = 0x04
} HDK_MOD_MethodOptions;

/* Method struct */
typedef struct _HDK_MOD_Method
{
    const char* pszHTTPMethod;
    const char* pszHTTPLocation;
    const char* pszSOAPAction;          /* May be 0 */
    HDK_MOD_MethodFn pfnMethod;
    const HDK_XML_Schema* pSchemaInput;
    const HDK_XML_Schema* pSchemaOutput;
    const HDK_XML_Element* pInputElementPath;  /* May be 0 */
    const HDK_XML_Element* pOutputElementPath; /* May be 0 */
    int options;
    HDK_XML_Element hnapResultElement;  /* HDK_XML_BuiltinElement_Unknown if none */
    HDK_XML_Type hnapResultEnumType;
    int hnapResultOK;
    int hnapResultREBOOT;
} HDK_MOD_Method;

/* Structure element path terminator */
#define HDK_MOD_ElementPathEnd HDK_XML_BuiltinElement_Unknown

/* Method list terminator */
#define HDK_MOD_MethodsEnd { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }


/*
 * HDK event definition
 */

/* Event struct */
typedef struct _HDK_MOD_Event
{
    const char* pszEventURI;            /* May be 0 */
    const HDK_XML_Schema* pSchema;
} HDK_MOD_Event;

/* Event list terminator */
#define HDK_MOD_EventsEnd { 0, 0 }


/*
 * HDK service definition
 */

/* Service struct */
typedef struct _HDK_MOD_Service
{
    const char* pszURI;
    const HDK_MOD_Method** pMethods;
    const HDK_MOD_Event** pEvents;
} HDK_MOD_Service;

/* Service list terminator */
#define HDK_MOD_ServicesEnd { 0, 0, 0 }


/*
 * HDK module
 */

/* Module struct */
typedef struct _HDK_MOD_Module
{
    const HDK_XML_UUID* pNOID;
    const char* pszFriendlyName;
    const HDK_MOD_Service* pServices;
    const HDK_MOD_Method* pMethods;
    const HDK_MOD_Event* pEvents;
    const HDK_XML_Schema* pSchemaADI;
} HDK_MOD_Module;

/* Get a method by method enumeration value */
#define HDK_MOD_GetMethod(pModule, method) (&(pModule)->pMethods[method])

/* Get an event by event enumeration value */
#define HDK_MOD_GetEvent(pModule, event) (&(pModule)->pEvents[event])

/* Module accessor function type */
typedef const HDK_MOD_Module* (*HDK_MOD_ModuleFn)(void);

/* Dynamic module export name */
#define HDK_MOD_ModuleExport "HDK_SRV_Module"


/*
 * Module ADI accessors
 */

/* ADI value schema node accessor */
#define HDK_MOD_ADISchemaNode(pModule, value) \
    (&(pModule)->pSchemaADI->pSchemaNodes[value])

/* ADI value element namespace */
#define HDK_MOD_ADIValueNamespace(pModule, value) \
    HDK_XML_Schema_ElementNamespace((pModule)->pSchemaADI, HDK_MOD_ADISchemaNode(pModule, value)->element)

/* ADI value element name */
#define HDK_MOD_ADIValueName(pModule, value) \
    HDK_XML_Schema_ElementName((pModule)->pSchemaADI, HDK_MOD_ADISchemaNode(pModule, value)->element)

#endif /* __HDK_MOD_H__ */
