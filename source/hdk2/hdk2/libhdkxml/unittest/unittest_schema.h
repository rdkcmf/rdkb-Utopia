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

#ifndef __UNITTEST_SCHEMA_H__
#define __UNITTEST_SCHEMA_H__

#include "hdk_xml.h"

/* Element enumeration */
typedef enum _Element
{
    Element_NoNamespace = 0,
    Element_a,
    Element_b,
    Element_c,
    Element_d,
    Element_e,
    Element_f,
    Element_g,
    Element_h,
    Element_i,
    Element_j,
    Element_k,
    Element_Cisco_Bar,
    Element_Cisco_Foo
} Element;

/* Namespace table */
extern HDK_XML_Namespace g_schema_namespaces[];

/* Elements table */
extern HDK_XML_ElementNode g_schema_elements[];

#endif /* __UNITTEST_SCHEMA_H__ */
