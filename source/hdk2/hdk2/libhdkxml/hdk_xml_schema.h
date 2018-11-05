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

#ifndef __HDK_XML_SCHEMA_H__
#define __HDK_XML_SCHEMA_H__

#include "hdk_xml.h"

/* Internal XML schema struct */
typedef struct _HDK_XML_SchemaInternal
{
    const HDK_XML_Namespace* ppszNamespaces;
    const HDK_XML_ElementNode* pElements;
    const HDK_XML_SchemaNode* pSchemaNodes;
    const HDK_XML_EnumType* pEnumTypes;
    unsigned int cElements;
    unsigned int cSchemaNodes;
} HDK_XML_SchemaInternal;

/* Initialize the internal schema struct */
extern void HDK_XML_Schema_Init(HDK_XML_SchemaInternal* pSchema, const HDK_XML_Schema* pSchemaExternal);

/* Get a namespace string from a namespace index */
#define HDK_XML_Schema_GetNamespace(pSchema, ixNamespace) \
    (pSchema->ppszNamespaces[ixNamespace])

/* Get a namespace index from a namespace string */
extern int HDK_XML_Schema_GetNamespaceIndex(unsigned int* pIXNamespace, const HDK_XML_SchemaInternal* pSchema,
                                            const char* pszNamespace, const char* pszNamespaceEnd);

/* Get an element name string from an element index */
#define HDK_XML_Schema_GetElementNode(pSchema, element) \
    (&(pSchema)->pElements[element])

/* Get an element index from a namespace string and name string */
extern int HDK_XML_Schema_GetElementIndex(HDK_XML_Element* pElement, const HDK_XML_SchemaInternal* pSchema,
                                          const char* pszNamespace, const char* pszNamespaceEnd,
                                          const char* pszElement, const char* pszElementEnd);

/* Get an element index from an element URI string */
extern int HDK_XML_Schema_GetElementURIIndex(HDK_XML_Element* pElement, const HDK_XML_SchemaInternal* pSchema,
                                             const char* pszElementFQ, const char* pszElementFQEnd);

/* Get a schema node from a schema node index */
#define HDK_XML_Schema_GetNode(pSchema, ixNode) \
    (&(pSchema)->pSchemaNodes[ixNode])

/* Get a child schema node from an element index */
extern const HDK_XML_SchemaNode* HDK_XML_Schema_GetChildNode(unsigned int* pixChild, const HDK_XML_SchemaInternal* pSchema,
                                                             unsigned int ixParent, HDK_XML_Element element);

/* Get a parent's child schema nodes from the parent's schema node index */
extern const HDK_XML_SchemaNode* HDK_XML_Schema_GetChildNodes(unsigned int* pixChildEnd, unsigned int* pixChildBegin,
                                                              const HDK_XML_SchemaInternal* pSchema, unsigned int ixParent);

#endif /* __HDK_XML_SCHEMA_H__ */
