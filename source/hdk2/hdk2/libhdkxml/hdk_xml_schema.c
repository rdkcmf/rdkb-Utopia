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

#include "hdk_xml_schema.h"

#include <string.h>


/* Initialize the internal schema struct */
void HDK_XML_Schema_Init(HDK_XML_SchemaInternal* pSchema, const HDK_XML_Schema* pSchemaExternal)
{
    const HDK_XML_ElementNode* pElement;
    const HDK_XML_SchemaNode* pSchemaNode;

    /* Copy the external schema members */
    pSchema->ppszNamespaces = pSchemaExternal->ppszNamespaces;
    pSchema->pElements = pSchemaExternal->pElements;
    pSchema->pSchemaNodes = pSchemaExternal->pSchemaNodes;
    pSchema->pEnumTypes = pSchemaExternal->pEnumTypes;

    /* Count the elements */
    for (pElement = pSchemaExternal->pElements; pElement->pszElement; ++pElement) {}
    pSchema->cElements = (unsigned int)(pElement - pSchemaExternal->pElements);

    /* Count the schema nodes */
    for (pSchemaNode = pSchemaExternal->pSchemaNodes; pSchemaNode->element != HDK_XML_BuiltinElement_Unknown; ++pSchemaNode) {}
    pSchema->cSchemaNodes = (unsigned int)(pSchemaNode - pSchemaExternal->pSchemaNodes);
}


/* Get a namespace index from a namespace string */
int HDK_XML_Schema_GetNamespaceIndex(unsigned int* pIXNamespace, const HDK_XML_SchemaInternal* pSchema,
                                     const char* pszNamespace, const char* pszNamespaceEnd)
{
    unsigned int cchNamespace = (pszNamespaceEnd ? (unsigned int)(pszNamespaceEnd - pszNamespace) : (unsigned int)strlen(pszNamespace));

    /* Find the namespace */
    const HDK_XML_Namespace* ppszNamespace;
    for (ppszNamespace = pSchema->ppszNamespaces; *ppszNamespace; ++ppszNamespace)
    {
        unsigned int cch = (unsigned int)strlen(*ppszNamespace);
        if (cch == cchNamespace && strncmp(*ppszNamespace, pszNamespace, cchNamespace) == 0)
        {
            *pIXNamespace = (unsigned int)(ppszNamespace - pSchema->ppszNamespaces);
            return 1;
        }
    }
    return 0;
}


/* Get an element index from a namespace string and name string */
int HDK_XML_Schema_GetElementIndex(HDK_XML_Element* pElement, const HDK_XML_SchemaInternal* pSchema,
                                   const char* pszNamespace, const char* pszNamespaceEnd,
                                   const char* pszElement, const char* pszElementEnd)
{
    unsigned int ixNamespace;
    unsigned int ix1 = 0;
    unsigned int ix2 = pSchema->cElements - 1;

    /* Find the namespace */
    if (!HDK_XML_Schema_GetNamespaceIndex(&ixNamespace, pSchema, pszNamespace, pszNamespaceEnd))
    {
        return 0;
    }

    /* Binary search for the element */
    while (ix1 <= ix2)
    {
        unsigned int ixElement = (ix1 + ix2) / 2;
        const HDK_XML_ElementNode* pElemNode = &pSchema->pElements[ixElement];
        int result = (ixNamespace < pElemNode->ixNamespace ? -1 :
                      (ixNamespace > pElemNode->ixNamespace ? 1 : 0));
        if (result == 0)
        {
            if (pszElementEnd)
            {
                unsigned int cchElement = (unsigned int)(pszElementEnd - pszElement);
                result = strncmp(pszElement, pElemNode->pszElement, cchElement);
                if (result == 0 && *(pElemNode->pszElement + cchElement))
                {
                    result = -1;
                }
            }
            else
            {
                result = strcmp(pszElement, pElemNode->pszElement);
            }
        }

        if (result == 0)
        {
            *pElement = ixElement;
            return 1;
        }
        else if (result < 0)
        {
            if (ixElement == 0)
            {
                break;
            }
            ix2 = ixElement - 1;
        }
        else
        {
            ix1 = ixElement + 1;
        }
    }

    return 0;
}


/* Get an element index from an element URI string */
int HDK_XML_Schema_GetElementURIIndex(HDK_XML_Element* pElement, const HDK_XML_SchemaInternal* pSchema,
                                      const char* pszElementFQ, const char* pszElementFQEnd)
{
    const char* pszNamespace;
    const char* pszNamespaceEnd;
    const char* pszElement = 0;

    /* Find the element */
    if (pszElementFQEnd)
    {
        const char* p;
        for (p = pszElementFQEnd - 1; p != pszElementFQ; --p)
        {
            if (*p == '/')
            {
                pszElement = p + 1;
                break;
            }
        }
    }
    else
    {
        const char* p;
        for (p = pszElementFQ; *p; ++p)
        {
            if (*p == '/')
            {
                pszElement = p + 1;
            }
        }
    }
    if (!pszElement)
    {
        return 0;
    }

    /* Search for the element */
    pszNamespace = pszElementFQ;
    pszNamespaceEnd = pszElement;
    if (HDK_XML_Schema_GetElementIndex(pElement, pSchema, pszNamespace, pszNamespaceEnd, pszElement, pszElementFQEnd))
    {
        return 1;
    }

    /* Namespace does not end with a '/'? */
    pszNamespaceEnd -= 1;
    if (pszNamespaceEnd > pszNamespace)
    {
        if (HDK_XML_Schema_GetElementIndex(pElement, pSchema, pszNamespace, pszNamespaceEnd, pszElement, pszElementFQEnd))
        {
            return 1;
        }
    }

    return 0;
}


/* Get a child schema node from an element index */
const HDK_XML_SchemaNode* HDK_XML_Schema_GetChildNode(unsigned int* pixChild, const HDK_XML_SchemaInternal* pSchema,
                                                      unsigned int ixParent, HDK_XML_Element element)
{
    unsigned int ix1 = ixParent + 1;
    unsigned int ix2 = pSchema->cSchemaNodes - 1;
    while (ix1 <= ix2)
    {
        unsigned int ix = (ix1 + ix2) / 2;
        const HDK_XML_SchemaNode* pNode = &pSchema->pSchemaNodes[ix];
        if (ixParent == pNode->ixParent)
        {
            const HDK_XML_SchemaNode* pSchemaNodesEnd = pSchema->pSchemaNodes + pSchema->cSchemaNodes;
            const HDK_XML_SchemaNode* pNode2;
            for (pNode2 = pNode; pNode2 >= pSchema->pSchemaNodes && pNode2->ixParent == ixParent; pNode2--)
            {
                if ((HDK_XML_Element)pNode2->element == element)
                {
                    *pixChild = (unsigned int)(pNode2 - pSchema->pSchemaNodes);
                    return pNode2;
                }
            }
            for (pNode2 = pNode + 1; pNode2 < pSchemaNodesEnd && pNode2->ixParent == ixParent; pNode2++)
            {
                if ((HDK_XML_Element)pNode2->element == element)
                {
                    *pixChild = (unsigned int)(pNode2 - pSchema->pSchemaNodes);
                    return pNode2;
                }
            }
            break;
        }
        else if (ixParent < pNode->ixParent)
        {
            if (ix == 0)
            {
                break;
            }
            ix2 = ix - 1;
        }
        else
        {
            ix1 = ix + 1;
        }
    }

    return 0;
}


/* Get a parent's child schema nodes from the parent's schema node index */
const HDK_XML_SchemaNode* HDK_XML_Schema_GetChildNodes(unsigned int* pixChildBegin, unsigned int* pixChildEnd,
                                                       const HDK_XML_SchemaInternal* pSchema, unsigned int ixParent)
{
    unsigned int ix1 = ixParent + 1;
    unsigned int ix2 = pSchema->cSchemaNodes - 1;
    while (ix1 <= ix2)
    {
        unsigned int ix = (ix1 + ix2) / 2;
        const HDK_XML_SchemaNode* pNode = &pSchema->pSchemaNodes[ix];
        if (ixParent == pNode->ixParent)
        {
            unsigned int ixChildBegin;
            unsigned int ixChildEnd;

            for (ixChildBegin = ix;
                 ixChildBegin > ixParent && pSchema->pSchemaNodes[ixChildBegin].ixParent == ixParent;
                 --ixChildBegin) {}

            for (ixChildEnd = ix;
                 ixChildEnd < pSchema->cSchemaNodes && pSchema->pSchemaNodes[ixChildEnd].ixParent == ixParent;
                 ++ixChildEnd) {}

            *pixChildBegin = ixChildBegin + 1;
            *pixChildEnd = ixChildEnd;
            return &pSchema->pSchemaNodes[ixChildBegin];
        }
        else if (ixParent < pNode->ixParent)
        {
            if (ix == 0)
            {
                break;
            }
            ix2 = ix - 1;
        }
        else
        {
            ix1 = ix + 1;
        }
    }
    return 0;
}
