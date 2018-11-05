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

#include "hdk_xml.h"
#include "hdk_xml_schema.h"
#include "hdk_xml_log.h"


/* Helper function for HDK_XML_Validate */
static int HDK_XML_Validate_Helper(const HDK_XML_SchemaInternal* pSchema, const HDK_XML_Struct* pStruct, int options,
                                   unsigned int ixSchemaNode)
{
    /* Get the struct's child tree nodes */
    unsigned int ixChildBegin;
    unsigned int ixChildEnd;
    if (HDK_XML_Schema_GetChildNodes(&ixChildBegin, &ixChildEnd, pSchema, ixSchemaNode))
    {
        unsigned int ixChild;
        HDK_XML_Member* pMember;

        /* Iterate the child tree nodes */
        for (ixChild = ixChildBegin; ixChild < ixChildEnd; ++ixChild)
        {
            const HDK_XML_SchemaNode* pChildNode = HDK_XML_Schema_GetNode(pSchema, ixChild);

            /* Count the matching members */
            unsigned int nChild = 0;
            for (pMember = pStruct->pHead; pMember; pMember = pMember->pNext)
            {
                if (pMember->element == pChildNode->element &&
                    (pMember->type == pChildNode->type || pMember->type == HDK_XML_BuiltinType_Blank))
                {
                    nChild++;
                }
            }

            /* Does the count match the element tree? */
            if ((nChild == 0 && !((pChildNode->prop & HDK_XML_SchemaNodeProperty_Optional) ||
                                  ((options & HDK_XML_ValidateOption_ErrorOutput) &&
                                   !(pChildNode->prop & HDK_XML_SchemaNodeProperty_ErrorOutput)))) ||
                (nChild > 1 && !(pChildNode->prop & HDK_XML_SchemaNodeProperty_Unbounded)))
            {
                HDK_XML_LOGFMT5(HDK_LOG_Level_Error, "Validation of element <%s xmlns=\"%s\"> failed due to unexpected count (%u) of child element <%s xmlns=\"%s\">\n",
                                HDK_XML_Schema_ElementName(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element),
                                HDK_XML_Schema_ElementNamespace(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element),
                                nChild,
                                HDK_XML_Schema_ElementName(pSchema, pChildNode->element),
                                HDK_XML_Schema_ElementNamespace(pSchema, pChildNode->element));
                return 0;
            }
        }

        /* Iterate the members */
        for (pMember = pStruct->pHead; pMember; pMember = pMember->pNext)
        {
            /* Ensure the member is allowed in this struct */
            int fAllowed = 0;
            for (ixChild = ixChildBegin; ixChild < ixChildEnd; ixChild++)
            {
                const HDK_XML_SchemaNode* pChildNode = HDK_XML_Schema_GetNode(pSchema, ixChild);
                if (pMember->element == pChildNode->element &&
                    (pMember->type == pChildNode->type || pMember->type == HDK_XML_BuiltinType_Blank))
                {
                    HDK_XML_Struct* pChildStruct;

                    /* This member is allowed by the schema */
                    fAllowed = 1;

                    /* Validate struct members */
                    pChildStruct = HDK_XML_GetMember_Struct(pMember);
                    if (pChildStruct)
                    {
                        if (!HDK_XML_Validate_Helper(pSchema, pChildStruct, options, ixChild))
                        {
                            return 0;
                        }
                    }

                    break;
                }
            }
            if (!fAllowed)
            {
                HDK_XML_LOGFMT4(HDK_LOG_Level_Error, "Validation of element <%s xmlns=\"%s\"> failed due to disallowed child element <%s xmlns=\"%s\">\n",
                                HDK_XML_Schema_ElementName(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element),
                                HDK_XML_Schema_ElementNamespace(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element),
                                HDK_XML_Schema_ElementName(pSchema, pMember->element),
                                HDK_XML_Schema_ElementNamespace(pSchema, pMember->element));
                return 0;
            }
        }
    }
    else
    {
        /* No child tree nodes - ensure there are no members */
        if (pStruct->pHead)
        {
            HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Validation failed due to child element in child-less element <%s xmlns=\"%s\">\n",
                            HDK_XML_Schema_ElementName(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element),
                            HDK_XML_Schema_ElementNamespace(pSchema, HDK_XML_Schema_GetNode(pSchema, ixSchemaNode)->element));

            return 0;
        }
    }

    return 1;
}


/* Validate a struct to a schema */
int HDK_XML_Validate(const HDK_XML_Schema* pSchema, const HDK_XML_Struct* pStruct, int options)
{
    return HDK_XML_ValidateEx(pSchema, (const HDK_XML_Member*)pStruct, options, 0);
}


/* Validate a struct to a schema (specify schema starting location) */
int HDK_XML_ValidateEx(const HDK_XML_Schema* pSchema, const HDK_XML_Member* pMember, int options,
                       unsigned int ixSchemaNode)
{
    const HDK_XML_SchemaNode* pSchemaNode = HDK_XML_Schema_GetNode(pSchema, ixSchemaNode);
    int fResult = (pMember->element == pSchemaNode->element &&
                   (pMember->type == pSchemaNode->type || pMember->type == HDK_XML_BuiltinType_Blank));

    if (fResult && pMember->type == HDK_XML_BuiltinType_Struct)
    {
        HDK_XML_SchemaInternal schema;
        HDK_XML_Schema_Init(&schema, pSchema);
        fResult = HDK_XML_Validate_Helper(&schema, (const HDK_XML_Struct*)pMember, options, ixSchemaNode);
    }

    return fResult;
}
