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

#ifndef __UNITTEST_MODULE_H__
#define __UNITTEST_MODULE_H__

#include "hdk_srv.h"


/*
 * Elements
 */

typedef enum _Elements
{
    Element_A = 0,
    Element_ADI,
    Element_ADIGet,
    Element_ADIGetResponse,
    Element_ADISet,
    Element_ADISetResponse,
    Element_Bar,
    Element_BarResponse,
    Element_BarResult,
    Element_Foo,
    Element_FooResponse,
    Element_FooResult,
    Element_Int,
    Element_Struct,
    HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Envelope,
    HNAP12_Element_Schemas_xmlsoap_org_soap_envelope_Header
} Elements;


/*
 * Enumeration BarResult
 */

typedef enum _BarResult
{
    BarResult_OK = 0,
    BarResult_REBOOT,
    BarResult_ERROR
} BarResult;

extern HDK_XML_Member* Set_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value);
extern HDK_XML_Member* Append_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value);
extern BarResult* Get_BarResult(HDK_XML_Struct* pStruct, HDK_XML_Element element);
extern BarResult Get_BarResultEx(HDK_XML_Struct* pStruct, HDK_XML_Element element, BarResult value);
extern BarResult* Get_BarResultMember(HDK_XML_Member* pMember);


/*
 * Enumeration FooResult
 */

typedef enum _FooResult
{
    FooResult_OK = 0,
    FooResult_REBOOT,
    FooResult_ERROR
} FooResult;

extern HDK_XML_Member* Set_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value);
extern HDK_XML_Member* Append_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value);
extern FooResult* Get_FooResult(HDK_XML_Struct* pStruct, HDK_XML_Element element);
extern FooResult Get_FooResultEx(HDK_XML_Struct* pStruct, HDK_XML_Element element, FooResult value);
extern FooResult* Get_FooResultMember(HDK_XML_Member* pMember);


/*
 * Methods
 */

extern void Method_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void Method_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void Method_Bar(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void Method_Foo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * ADI
 */

typedef enum _ADIValues
{
    ADI_Int = 1,
    ADI_Struct
} ADIValues;


/*
 * Module entity accessors
 */

extern const HDK_MOD_Module* GetModule(void);
extern const HDK_MOD_Module* GetModule2(void);

#endif /* __UNITTEST_MODULE_H__ */
