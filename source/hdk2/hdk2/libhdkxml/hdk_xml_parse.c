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

#include "hdk_xml.h"
#include "hdk_xml_parse.h"
#include "hdk_xml_schema.h"
#include "hdk_xml_type.h"
#include "hdk_xml_log.h"

#ifdef HDK_XML_LIBXML2
#  include <libxml/parser.h>
#else
#  include <expat.h>
#endif

#include <memory.h>
#include <string.h>

#ifdef HDK_LOGGING
#  ifdef _MSC_VER
#    include <malloc.h>
/* Disable 'warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead' */
#    pragma warning (disable: 6255)
#    define alloca _alloca
#  endif
#endif


/*
 * Helper function to deserialize a member
 */
static HDK_XML_ParseError HDK_XML_DeserializeMember(HDK_XML_SchemaInternal* pSchema, HDK_XML_Type type,
                                                    HDK_XML_Struct* pStruct, HDK_XML_Element element,
                                                    const char* pszValue)
{
    HDK_XML_ParseError result = HDK_XML_ParseError_OK;

    /* Create the new member */
    HDK_XML_Member* pMember = HDK_XML_CallTypeFn_New(type);
    if (pMember)
    {
        /* Deserialize the member */
        if (HDK_XML_CallTypeFn_Deserialize(type, pSchema->pEnumTypes, pMember, (pszValue ? pszValue : "")))
        {
            HDK_XML_Struct_AddMember(pStruct, pMember, element, type);
        }
        else
        {
            HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Failed to deserialize value into element <%s xmlns=\"%s\">\n",
                            HDK_XML_Schema_ElementName(pSchema, element),
                            HDK_XML_Schema_ElementNamespace(pSchema, element));

            HDK_XML_CallTypeFn_Free(type, pMember);
            result = HDK_XML_ParseError_XMLInvalidValue;
        }
    }
    else
    {
        result = HDK_XML_ParseError_OutOfMemory;
    }

    return result;
}


/*
 * Helper function to check max alloc
 */
static int CheckMaxAlloc(HDK_XML_ParseContext* pParseCtx, unsigned int cbAlloc)
{
    /* Have we exceeded the maximum memory allocation size? */
    pParseCtx->cbTotalAlloc += cbAlloc;
    if (pParseCtx->cbMaxAlloc && pParseCtx->cbTotalAlloc > pParseCtx->cbMaxAlloc)
    {
        pParseCtx->parseError = HDK_XML_ParseError_XMLInvalid;
        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Maximum parsing allocation (%u bytes) exeeded\n", pParseCtx->cbMaxAlloc);
        return 0;
    }

    return 1;
}


/*
 * Helper to retrieve the next item in a csv list
 */
static const char* CSV_NextListItem(const char* psz)
{
    int fEscaped = 0;
    for (; *psz; ++psz)
    {
        switch (*psz)
        {
            case '\\':
            {
                fEscaped = !fEscaped;
                break;
            }
            case ',':
            {
                if (!fEscaped)
                {
                    return psz + 1;
                }

                /* Intentional fallthrough */
            }
            default:
            {
                fEscaped = 0;
                break;
            }
        }
    }

    return 0;
}

/*
 * Helper to unescape a csv list item
 * Note: Caller MUST free returned string
 */
static char* CSV_DecodeListItem(const char* psz, unsigned int cb, HDK_XML_ParseError* pParseError)
{
    HDK_XML_OutputStream_BufferContext bufferCtx;
    bufferCtx.cbBuf = cb;
    bufferCtx.ixCur = 0;
    bufferCtx.pBuf = (char*)malloc(cb + 1 /* for '\0' */);
    if (bufferCtx.pBuf)
    {
        HDK_XML_OutputStream_DecodeCSV_Context decodeCtx;
        unsigned int cbStream;

        decodeCtx.pfnStream = HDK_XML_OutputStream_Buffer;
        decodeCtx.pStreamCtx = &bufferCtx;

        if (HDK_XML_OutputStream_DecodeCSV(&cbStream, &decodeCtx, psz, cb))
        {
            bufferCtx.pBuf[bufferCtx.ixCur] = '\0';
            *pParseError = HDK_XML_ParseError_OK;
        }
        else
        {
            *pParseError = HDK_XML_ParseError_XMLInvalidValue;
            free(bufferCtx.pBuf);
            bufferCtx.pBuf = 0;
        }
    }
    else
    {
        *pParseError = HDK_XML_ParseError_OutOfMemory;
    }

    return bufferCtx.pBuf;
}

/*
 * Helper function to deserialize a CSV (comma seperated value) list
 */
static HDK_XML_ParseError CSV_DeserializeListHelper(HDK_XML_SchemaInternal* pSchema, unsigned int ixNode,
                                                    HDK_XML_Struct* pStruct, const char* pszValue,
                                                    const char** ppszNextItem)
{
    HDK_XML_ParseError result = HDK_XML_ParseError_OK;

    unsigned int ixChildBegin;
    unsigned int ixChildEnd;

    *ppszNextItem = 0;

    if (HDK_XML_Schema_GetChildNodes(&ixChildBegin, &ixChildEnd, pSchema, ixNode))
    {
        unsigned int ixChild = ixChildBegin;

        if (pszValue && *pszValue)
        {
            const char* pszItem = pszValue;
            while (pszItem)
            {
                if (ixChild < ixChildEnd)
                {
                    const HDK_XML_SchemaNode* pChildNode = HDK_XML_Schema_GetNode(pSchema, ixChild);

                    unsigned int cbItem;
                    char* pszItemDecoded;

                    *ppszNextItem = CSV_NextListItem(pszItem);

                    cbItem = (*ppszNextItem) ? (unsigned int)((*ppszNextItem) - pszItem - 1 /* for ',' */) : (unsigned int)strlen(pszItem);
                    pszItemDecoded = CSV_DecodeListItem(pszItem, cbItem, &result);
                    if (!pszItemDecoded)
                    {
                        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to decode csv list element \"%s\"\n", pszItem);
                        break;
                    }

                    if (pChildNode->type == HDK_XML_BuiltinType_Struct)
                    {
                        /* Tuple array (e.g. CSV({w,x,y}). */
                        HDK_XML_Struct* pTuple = HDK_XML_Append_Struct(pStruct, pChildNode->element);
                        if (pTuple)
                        {
                            result = CSV_DeserializeListHelper(pSchema, ixChild, pTuple, pszItem, ppszNextItem);
                        }
                        else
                        {
                            result = HDK_XML_ParseError_OutOfMemory;
                        }
                    }
                    else
                    {
                        /* Deserialize the child member. */
                        result =
                          HDK_XML_DeserializeMember(pSchema, pChildNode->type, pStruct, pChildNode->element, pszItemDecoded);

                        /* If the child member is unbounded, then do not advance to the next member as this is a homogenouse csv list. */
                        if (!(pChildNode->prop & HDK_XML_SchemaNodeProperty_Unbounded))
                        {
                            ixChild++;
                        }
                    }

                    free(pszItemDecoded);

                    if (HDK_XML_ParseError_OK != result)
                    {
                        break;
                    }

                    pszItem = *ppszNextItem;
                }
                else
                {
                    /* Populated all known child members.*/
                    break;
                }
            }
        }
    }

    return result;
}

static HDK_XML_ParseError CSV_DeserializeList(HDK_XML_SchemaInternal* pSchema, unsigned int ixNode,
                                              HDK_XML_Struct* pStruct, const char* pszValue)
{
    const char* pszLastItem;
    HDK_XML_ParseError result = CSV_DeserializeListHelper(pSchema, ixNode, pStruct, pszValue, &pszLastItem);
    if (HDK_XML_ParseError_OK == result)
    {
        if (pszLastItem)
        {

#ifdef HDK_LOGGING
            const HDK_XML_SchemaNode* pSchemaNode = HDK_XML_Schema_GetNode(pSchema, ixNode);
            HDK_XML_LOGFMT3(HDK_LOG_Level_Error, "Extraneous csv list element(s) \"%s\" in element <%s xmlns=\"%s\">\n",
                            pszLastItem,
                            HDK_XML_Schema_ElementName(pSchema, pSchemaNode->element),
                            HDK_XML_Schema_ElementNamespace(pSchema, pSchemaNode->element));
#endif
            result = HDK_XML_ParseError_XMLInvalidValue;
        }
    }

    return result;
}

#ifdef HDK_LOGGING
/* Number of spaces for the indentation of a child element relative to its parent. */
#  define HDK_XML_LOGGING_ELEMENT_SCOPE_INDENT 2
#endif


/*
 * Member element callback functions
 */

static void elementOpen(HDK_XML_ParseContext* pParseCtx, const char* pszNamespace, const char* pszNamespaceEnd,
                        const char* pszElement, const char* pszElementEnd)
{
    int fFoundElement;
    HDK_XML_Element element;
    const HDK_XML_SchemaNode* pSchemaNode = 0;

    /* Already an error? */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
        return;
    }

    /* Already populated? */
    if (pParseCtx->structState == HDK_XML_ParseStructState_Done)
    {
#ifdef HDK_LOGGING
        char* pszNamespaceNullTerm = (char*)pszNamespace;
        char* pszElementNullTerm = (char*)pszElement;
        if (pszNamespaceEnd)
        {
            size_t cch = pszNamespaceEnd - pszNamespace;
            pszNamespaceNullTerm = (char*)alloca((cch + 1 /* for '\0' */) * sizeof(char));
            memcpy(pszNamespaceNullTerm, pszNamespace, cch * sizeof(char));
            pszNamespaceNullTerm[cch] = '\0';
        }
        if (pszElementEnd)
        {
            size_t cch = pszElementEnd - pszElement;
            pszElementNullTerm = (char*)alloca((cch + 1 /* for '\0' */) * sizeof(char));
            memcpy(pszElementNullTerm, pszElement, cch * sizeof(char));
            pszElementNullTerm[cch] = '\0';
        }
        HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Encountered element <%s xmlns=\"%s\"> after output struct already populated\n",
                        pszElementNullTerm,
                        pszNamespaceNullTerm);
#endif /*  HDK_LOGGING */

        pParseCtx->parseError = HDK_XML_ParseError_XMLInvalid;
        return;
    }

    /* Get the element */
    fFoundElement =
        HDK_XML_Schema_GetElementIndex(&element, &pParseCtx->schema,
                                       pszNamespace, pszNamespaceEnd, pszElement, pszElementEnd);
    if (fFoundElement)
    {
        if (pParseCtx->structState == HDK_XML_ParseStructState_Init ||
            pParseCtx->structState == HDK_XML_ParseStructState_InitMember)
        {
            pSchemaNode = HDK_XML_Schema_GetNode(&pParseCtx->schema, pParseCtx->ixSchemaNode);
            if (pSchemaNode->element != element)
            {
                pParseCtx->parseError = HDK_XML_ParseError_XMLUnexpectedElement;
                HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Encountered unexpected initial element <%s xmlns=\"%s\">\n",
                                HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element),
                                HDK_XML_Schema_ElementNamespace(&pParseCtx->schema, pSchemaNode->element));
                return;
            }
        }
        else
        {
            pSchemaNode = HDK_XML_Schema_GetChildNode(&pParseCtx->ixSchemaNode, &pParseCtx->schema,
                                                      pParseCtx->ixSchemaNode, element);
        }
    }

    /* Unknown or extra-schema element? */
    if (!pSchemaNode)
    {
        /* Get the parent schema node */
        const HDK_XML_SchemaNode* pSchemaNodeParent = 0;
        if (pParseCtx->structState == HDK_XML_ParseStructState_Populating)
        {
            pSchemaNodeParent = HDK_XML_Schema_GetNode(&pParseCtx->schema, pParseCtx->ixSchemaNode);
        }

        /* Is any element allowed here? */
        if (pSchemaNodeParent && (pSchemaNodeParent->prop & HDK_XML_SchemaNodeProperty_AnyElement))
        {
            pParseCtx->cAnyElement++;
        }
        else
        {
#ifdef HDK_LOGGING
            char* pszNamespaceNullTerm = (char*)pszNamespace;
            char* pszElementNullTerm = (char*)pszElement;
            if (pszNamespaceEnd)
            {
                size_t cch = pszNamespaceEnd - pszNamespace;
                pszNamespaceNullTerm = (char*)alloca((cch + 1 /* for '\0' */) * sizeof(char));
                memcpy(pszNamespaceNullTerm, pszNamespace, cch * sizeof(char));
                pszNamespaceNullTerm[cch] = '\0';
            }
            if (pszElementEnd)
            {
                size_t cch = pszElementEnd - pszElement;
                pszElementNullTerm = (char*)alloca(cch * sizeof(char));
                memcpy(pszElementNullTerm, pszElement, cch * sizeof(char));
                pszElementNullTerm[cch] = '\0';
            }
            HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Encountered unexpected element <%s xmlns=\"%s\">\n", pszElementNullTerm, pszNamespaceNullTerm);
#endif /* HDK_LOGGING */

            pParseCtx->parseError = HDK_XML_ParseError_XMLUnexpectedElement;
            return;
        }
    }
    /* Is this schema node a struct? */
    else if (pSchemaNode->type == HDK_XML_BuiltinType_Struct)
    {
        if (pParseCtx->structState == HDK_XML_ParseStructState_Init)
        {
            /* Require struct! */
            if (pSchemaNode->type != HDK_XML_BuiltinType_Struct)
            {
                HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Encountered unexpected non-struct type element <%s xmlns=\"%s\">\n",
                                HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element),
                                HDK_XML_Schema_ElementNamespace(&pParseCtx->schema, pSchemaNode->element));

                pParseCtx->parseError = HDK_XML_ParseError_XMLUnexpectedElement;
                return;
            }

            /* Initialize the top-level struct */
            pParseCtx->pStack[0]->node.element = pSchemaNode->element;
            HDK_XML_Struct_Free(pParseCtx->pStack[0]);
        }
        else
        {
            /* Too deep? */
            if (pParseCtx->ixStack < sizeof(pParseCtx->pStack) / sizeof(*pParseCtx->pStack))
            {
                HDK_XML_Struct* pStructCur;
                HDK_XML_Struct* pStructNew;

                /* Have we exceeded the maximum memory allocation size? */
                if (!CheckMaxAlloc(pParseCtx, sizeof(HDK_XML_Struct)))
                {
                    return;
                }

                /* Create the new struct */
                pStructCur = pParseCtx->pStack[pParseCtx->ixStack];
                pStructNew = HDK_XML_Append_Struct(pStructCur, element);
                if (pStructNew)
                {
                    pParseCtx->ixStack++;
                    pParseCtx->pStack[pParseCtx->ixStack] = pStructNew;
                }
                else
                {
                    pParseCtx->parseError = HDK_XML_ParseError_OutOfMemory;
                    return;
                }
            }
            else
            {
                pParseCtx->parseError = HDK_XML_ParseError_OutOfMemory;
                return;
            }
        }

#ifdef HDK_LOGGING
        {
            /* Log the struct open element. */
            const char* pszElementNamespace = 0;
            size_t cchIndent = pParseCtx->ixStack * HDK_XML_LOGGING_ELEMENT_SCOPE_INDENT;
            char* pszIndent = (char*)alloca(cchIndent + 1 /* for '\0' */);

            const HDK_XML_SchemaNode* pParentNode = HDK_XML_Schema_GetNode(&pParseCtx->schema, pSchemaNode->ixParent);

            /* Include the namespace if this is the top-level element or it differs from its parent's namespace. */
            if ((HDK_XML_ParseStructState_Init == pParseCtx->structState) || (HDK_XML_ParseStructState_InitMember == pParseCtx->structState) ||
                (HDK_XML_Schema_GetElementNode(&pParseCtx->schema, pSchemaNode->element)->ixNamespace !=
                 HDK_XML_Schema_GetElementNode(&pParseCtx->schema, pParentNode->element)->ixNamespace))
            {
                pszElementNamespace = HDK_XML_Schema_ElementNamespace(&pParseCtx->schema, pSchemaNode->element);
            }

            memset(pszIndent, ' ', cchIndent);
            pszIndent[cchIndent] = '\0';

            HDK_XML_LOGFMT5(HDK_LOG_Level_Verbose, "%s<%s%s%s%s>\n",
                            pszIndent,
                            HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element),
                            (pszElementNamespace) ? " xmlns=\"" : "", (pszElementNamespace) ? pszElementNamespace : "", (pszElementNamespace) ? "\"" : "");
        }
#endif /* def HDK_LOGGING */
    }
    /* Initial non-struct can't be non-member option */
    else if (pParseCtx->structState == HDK_XML_ParseStructState_Init)
    {
        pParseCtx->parseError = HDK_XML_ParseError_XMLInvalid;
        return;
    }

    /* Starting population? */
    if (pParseCtx->structState == HDK_XML_ParseStructState_Init ||
        pParseCtx->structState == HDK_XML_ParseStructState_InitMember)
    {
        pParseCtx->structState = HDK_XML_ParseStructState_Populating;
    }

    /* Clear the element value */
    if (pParseCtx->bufferCtx.pBuf)
    {
        pParseCtx->bufferCtx.pBuf[0] = '\0';
    }
    pParseCtx->bufferCtx.ixCur = 0;
}

static void elementClose(HDK_XML_ParseContext* pParseCtx)
{
    const HDK_XML_SchemaNode* pSchemaNode;

    /* Already an error? */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
        return;
    }

    /* Are we in an unknown element scope? */
    if (pParseCtx->cAnyElement > 0)
    {
        pParseCtx->cAnyElement--;
        return;
    }

    /* Deserialize the type */
    pSchemaNode = HDK_XML_Schema_GetNode(&pParseCtx->schema, pParseCtx->ixSchemaNode);
    if (pSchemaNode->type == HDK_XML_BuiltinType_Struct)
    {
        if ((pSchemaNode->prop & HDK_XML_SchemaNodeProperty_CSV) ||
            (pParseCtx->options & HDK_XML_ParseOption_CSV))
        {
#ifdef HDK_LOGGING
            /* Log the csv list.*/
            size_t cchIndent = (pParseCtx->ixStack + 1) * HDK_XML_LOGGING_ELEMENT_SCOPE_INDENT;
            char* pszIndent = (char*)alloca(cchIndent + 1 /* for '\0' */);

            memset(pszIndent, ' ', cchIndent);
            pszIndent[cchIndent] = '\0';

            HDK_XML_LOGFMT2(HDK_LOG_Level_Verbose, "%s%s\n", pszIndent, pParseCtx->bufferCtx.pBuf);
#endif /* def HDK_LOGGING */

            pParseCtx->parseError =
              CSV_DeserializeList(&pParseCtx->schema, pParseCtx->ixSchemaNode,
                                  pParseCtx->pStack[pParseCtx->ixStack], pParseCtx->bufferCtx.pBuf);
        }

#ifdef HDK_LOGGING
        {
            /* Log the struct element close.*/
            size_t cchIndent = pParseCtx->ixStack * HDK_XML_LOGGING_ELEMENT_SCOPE_INDENT;
            char* pszIndent = (char*)alloca(cchIndent + 1 /* for '\0' */);

            memset(pszIndent, ' ', cchIndent);
            pszIndent[cchIndent] = '\0';

            HDK_XML_LOGFMT2(HDK_LOG_Level_Verbose, "%s</%s>\n", pszIndent, HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element));
        }
#endif /* def HDK_LOGGING */

        /* Pop the struct population stack */
        if (pParseCtx->ixStack > 0)
        {
            pParseCtx->ixStack--;
        }
        else
        {
            pParseCtx->structState = HDK_XML_ParseStructState_Done;
        }
    }
    else
    {
#ifdef HDK_LOGGING
        /* Log the element and value. */
        const char* pszNamespace = 0;
        size_t cchIndent = (pParseCtx->ixStack + 1 /* additional scope for child member elements */) * HDK_XML_LOGGING_ELEMENT_SCOPE_INDENT;
        char* pszIndent = (char*)alloca(cchIndent + 1 /* for '\0' */);

        const HDK_XML_SchemaNode* pParentNode = HDK_XML_Schema_GetNode(&pParseCtx->schema, pSchemaNode->ixParent);

        /* Include the namespace if it differs from its parent's namespace. */
        if (HDK_XML_Schema_GetElementNode(&pParseCtx->schema, pSchemaNode->element)->ixNamespace !=
            HDK_XML_Schema_GetElementNode(&pParseCtx->schema, pParentNode->element)->ixNamespace)
        {
             pszNamespace = HDK_XML_Schema_ElementNamespace(&pParseCtx->schema, pSchemaNode->element);
        }

        memset(pszIndent, ' ', cchIndent);
        pszIndent[cchIndent] = '\0';

        HDK_XML_LOGFMT7(HDK_LOG_Level_Verbose, "%s<%s%s%s%s>%s</%s>\n",
                        pszIndent,
                        HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element),
                        (pszNamespace) ? " xmlns=\"" : "", (pszNamespace) ? pszNamespace : "", (pszNamespace) ? "\"" : "",
                        pParseCtx->bufferCtx.pBuf,
                        HDK_XML_Schema_ElementName(&pParseCtx->schema, pSchemaNode->element));
#endif /* def HDK_LOGGING */

        /* Have we exceeded the maximum memory allocation size? */
        if (CheckMaxAlloc(pParseCtx, sizeof(HDK_XML_Member)))
        {
            /* Deserialize the member */
            pParseCtx->parseError =
                HDK_XML_DeserializeMember(&pParseCtx->schema, pSchemaNode->type, pParseCtx->pStack[pParseCtx->ixStack],
                                          pSchemaNode->element, pParseCtx->bufferCtx.pBuf);
        }
    }

    /* Update the current element index */
    pParseCtx->ixSchemaNode = pSchemaNode->ixParent;
}

static void elementValue(HDK_XML_ParseContext* pParseCtx, const char* pszValue, int cbValue)
{
    const HDK_XML_SchemaNode* pSchemaNode;

    /* Already an error? */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
        return;
    }

    /* Does this element have a value? */
    pSchemaNode = HDK_XML_Schema_GetNode(&pParseCtx->schema, pParseCtx->ixSchemaNode);
    if (pParseCtx->cAnyElement > 0 ||
        (pSchemaNode->type == HDK_XML_BuiltinType_Struct &&
         !(pParseCtx->options & HDK_XML_ParseOption_CSV) &&
         !(pSchemaNode->prop & HDK_XML_SchemaNodeProperty_CSV)))
    {
        return;
    }

    /* Have we exceeded the maximum memory allocation size? */
    if (!CheckMaxAlloc(pParseCtx, cbValue))
    {
        return;
    }

    /* Append to the value buffer */
    if (!HDK_XML_OutputStream_GrowBuffer(0, &pParseCtx->bufferCtx, pszValue, cbValue))
    {
        pParseCtx->parseError = HDK_XML_ParseError_OutOfMemory;
        return;
    }
    else
    {
        pParseCtx->bufferCtx.pBuf[pParseCtx->bufferCtx.ixCur] = '\0';
    }
}


/*
 * XML parser element callback functions
 */

static void elementStartHandler(
#ifdef HDK_XML_LIBXML2
    void* _pParseCtx,
    const xmlChar* pszElement,
    const xmlChar* pszPrefix,
    const xmlChar* pszNamespace,
    int nNamespaces,
    const xmlChar** ppszNamespaces,
    int nAttributes,
    int nDefaulted,
    const xmlChar** ppszAttributes
#else
    void* _pParseCtx,
    const char* pszElement,
    const char** ppszAttributes
#endif
    )
{
    HDK_XML_ParseContext* pParseCtx = (HDK_XML_ParseContext*)_pParseCtx;
    const char* pszNamespaceEnd = 0;

#ifdef HDK_XML_LIBXML2
    /* Unused parameters */
    (void)pszPrefix;
    (void)nNamespaces;
    (void)ppszNamespaces;
    (void)nAttributes;
    (void)nDefaulted;
    (void)ppszAttributes;

    /* No namespace? */
    if (!pszNamespace)
    {
        pszNamespace = (xmlChar*)"";
    }
#else
    const char* pszNamespace;

    /* Unused parameters */
    (void)ppszAttributes;

    /* Locate the end of the namespace */
    pszNamespace = pszElement;
    for (pszNamespaceEnd = pszElement; *pszNamespaceEnd && *pszNamespaceEnd != '!'; ++pszNamespaceEnd) {}
    if (*pszNamespaceEnd)
    {
        pszElement = pszNamespaceEnd + 1;
    }
    else
    {
        /* No namespace */
        pszNamespace = "";
        pszNamespaceEnd = 0;
    }
#endif

    /* Handle the element open */
    elementOpen(pParseCtx, (const char*)pszNamespace, pszNamespaceEnd, (const char*)pszElement, 0);

    /* Stop the parser on error */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
#ifdef HDK_XML_LIBXML2
        xmlStopParser((xmlParserCtxt*)pParseCtx->pXMLParser);
#else
        XML_StopParser((struct XML_ParserStruct*)pParseCtx->pXMLParser, XML_FALSE);
#endif
    }
}

static void elementCloseHandler(
#ifdef HDK_XML_LIBXML2
    void* _pParseCtx,
    const xmlChar* pszElement,
    const xmlChar* pszPrefix,
    const xmlChar* pszNamespace
#else
    void* _pParseCtx,
    const char* pszElement
#endif
    )
{
    HDK_XML_ParseContext* pParseCtx = (HDK_XML_ParseContext*)_pParseCtx;

#ifdef HDK_XML_LIBXML2
    /* Unused parameters */
    (void)pszElement;
    (void)pszPrefix;
    (void)pszNamespace;
#else
    /* Unused parameters */
    (void)pszElement;
#endif

    /* Handle the element close */
    elementClose(pParseCtx);

    /* Stop the parser on error */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
#ifdef HDK_XML_LIBXML2
        xmlStopParser((xmlParserCtxt*)pParseCtx->pXMLParser);
#else
        XML_StopParser((struct XML_ParserStruct*)pParseCtx->pXMLParser, XML_FALSE);
#endif
    }
}

static void elementValueHandler(
#ifdef HDK_XML_LIBXML2
    void* _pParseCtx,
    const xmlChar* pValue,
    int cbValue
#else
    void* _pParseCtx,
    const char* pValue,
    int cbValue
#endif
    )
{
    HDK_XML_ParseContext* pParseCtx = (HDK_XML_ParseContext*)_pParseCtx;

    /* Handle the element value (part) */
    elementValue(pParseCtx, (const char*)pValue, cbValue);

    /* Stop the parser on error */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
#ifdef HDK_XML_LIBXML2
        xmlStopParser((xmlParserCtxt*)pParseCtx->pXMLParser);
#else
        XML_StopParser((struct XML_ParserStruct*)pParseCtx->pXMLParser, XML_FALSE);
#endif
    }
}


/*
 * XML de-serialization
 */

/* Initialize XML parse context */
HDK_XML_ParseContextPtr HDK_XML_Parse_New(const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct)
{
    return HDK_XML_Parse_NewEx(pSchema, pStruct, 0, 0, 0);
}

/* Initialize XML parse context (specify schema starting location) */
HDK_XML_ParseContextPtr HDK_XML_Parse_NewEx(const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                            int options, unsigned int ixSchemaNode,
                                            unsigned int cbMaxAlloc)
{
    /* Create the parse context */
    HDK_XML_ParseContext* pParseCtx = (HDK_XML_ParseContext*)malloc(sizeof(HDK_XML_ParseContext));
    if (pParseCtx)
    {
#ifdef HDK_XML_LIBXML2
        xmlSAXHandler saxHandler;
#endif

        /* Init the parse context struct */
        memset(pParseCtx, 0, sizeof(*pParseCtx));
        HDK_XML_Schema_Init(&pParseCtx->schema, pSchema);
        pParseCtx->structState = ((options & HDK_XML_ParseOption_Member) ?
                                  HDK_XML_ParseStructState_InitMember : HDK_XML_ParseStructState_Init);
        pParseCtx->pStack[0] = pStruct;
        pParseCtx->ixSchemaNode = ixSchemaNode;
        pParseCtx->cbMaxAlloc = cbMaxAlloc;
        pParseCtx->options = options;

        /* Create the XML parser */
#ifdef HDK_XML_LIBXML2
        memset(&saxHandler, 0, sizeof(saxHandler));
        saxHandler.initialized = XML_SAX2_MAGIC;
        saxHandler.startElementNs = elementStartHandler;
        saxHandler.endElementNs = elementCloseHandler;
        saxHandler.characters = elementValueHandler;
        pParseCtx->pXMLParser = xmlCreatePushParserCtxt(&saxHandler, pParseCtx, 0, 0, 0);
#else
        pParseCtx->pXMLParser = XML_ParserCreateNS(NULL, '!');
#endif
        if (pParseCtx->pXMLParser)
        {
#ifndef HDK_XML_LIBXML2
            XML_SetUserData((struct XML_ParserStruct*)pParseCtx->pXMLParser, pParseCtx);
            XML_SetElementHandler((struct XML_ParserStruct*)pParseCtx->pXMLParser, elementStartHandler, elementCloseHandler);
            XML_SetCharacterDataHandler((struct XML_ParserStruct*)pParseCtx->pXMLParser, elementValueHandler);
#endif

            HDK_XML_LOGFMT1(HDK_LOG_Level_Verbose, "Created XML parser %p\n", pParseCtx->pXMLParser);
        }
        else
        {
            HDK_XML_LOG(HDK_LOG_Level_Error, "Failed to create XML parser\n");

            HDK_XML_Parse_Free(pParseCtx);
            pParseCtx = 0;
        }
    }

    return pParseCtx;
}

/* Parse XML data helper */
static HDK_XML_ParseError HDK_XML_Parse_Data_Helper(HDK_XML_ParseContextPtr pParseCtx, const char* pBuf, unsigned int cbBuf, int fDone)
{
#ifdef HDK_XML_LIBXML2
    xmlParserErrors xmlErrorCode;
#endif

    /* Do nothing if the parser is in an error state */
    if (pParseCtx->parseError != HDK_XML_ParseError_OK)
    {
        return pParseCtx->parseError;
    }

    /* Parse the XML in the buffer */
#ifdef HDK_XML_LIBXML2
    if ((xmlErrorCode = (xmlParserErrors)xmlParseChunk((xmlParserCtxt*)pParseCtx->pXMLParser, pBuf, cbBuf, fDone)) != XML_ERR_OK)
#else
    if (!XML_Parse((struct XML_ParserStruct*)pParseCtx->pXMLParser, pBuf, cbBuf, fDone))
#endif
    {
#ifndef HDK_XML_LIBXML2
        enum XML_Error xmlErrorCode = XML_GetErrorCode((struct XML_ParserStruct*)pParseCtx->pXMLParser);
#endif

        HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "XML data parsing failed with parser error %d\n", xmlErrorCode);

        /* Error - XML error */
        switch (xmlErrorCode)
        {
#ifdef HDK_XML_LIBXML2
            case XML_ERR_NO_MEMORY:
#else
            case XML_ERROR_NO_MEMORY:
#endif
                pParseCtx->parseError = HDK_XML_ParseError_OutOfMemory;
                break;

            default:
                if (pParseCtx->parseError == HDK_XML_ParseError_OK)
                {
                    pParseCtx->parseError = HDK_XML_ParseError_XMLInvalid;
                }
                break;
        }
    }

    return pParseCtx->parseError;
}

/* Parse XML data */
HDK_XML_ParseError HDK_XML_Parse_Data(HDK_XML_ParseContextPtr pParseCtx, const char* pBuf, unsigned int cbBuf)
{
    return HDK_XML_Parse_Data_Helper(pParseCtx, pBuf, cbBuf, 0);
}

/* Free XML parse context */
HDK_XML_ParseError HDK_XML_Parse_Free(HDK_XML_ParseContextPtr pParseCtx)
{
    HDK_XML_ParseError parseError;

    /* Finish parsing */
    HDK_XML_Parse_Data_Helper(pParseCtx, 0, 0, 1);
    parseError = pParseCtx->parseError;

    /* Free the XML parser */
    if (pParseCtx->pXMLParser)
    {
        HDK_XML_LOGFMT1(HDK_LOG_Level_Verbose, "Destroying XML parser %p\n", pParseCtx->pXMLParser);
#ifdef HDK_XML_LIBXML2
        xmlFreeParserCtxt((xmlParserCtxt*)pParseCtx->pXMLParser);
#else
        XML_ParserFree((struct XML_ParserStruct*)pParseCtx->pXMLParser);
#endif
    }

    /* Free allocated memory */
    free(pParseCtx->bufferCtx.pBuf);
    free(pParseCtx);

    return parseError;
}

/* Get the parser error */
HDK_XML_ParseError HDK_XML_Parse_GetError(HDK_XML_ParseContextPtr pParseCtx)
{
    return pParseCtx->parseError;
}

/* XML parse from input stream */
HDK_XML_ParseError HDK_XML_Parse(HDK_XML_InputStreamFn pfnStream, void* pStreamCtx,
                                 const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                 unsigned int cbContentLength)
{
    return HDK_XML_ParseEx(pfnStream, pStreamCtx, pSchema, pStruct, cbContentLength, 0, 0, 0);
}

/* XML parse from input stream (specify schema starting location) */
HDK_XML_ParseError HDK_XML_ParseEx(HDK_XML_InputStreamFn pfnStream, void* pStreamCtx,
                                   const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                   unsigned int cbContentLength,
                                   int options, unsigned int ixSchemaNode,
                                   unsigned int cbMaxAlloc)
{
    HDK_XML_ParseError parseError = HDK_XML_ParseError_OK;
    const HDK_XML_SchemaNode* pSchemaNode;
    HDK_XML_ParseContextPtr pParseCtx = 0;
    HDK_XML_OutputStream_BufferContext bufferCtx;
    char szBuf[1024];
    unsigned int cbTotal = 0;

    /* Non-XML parse requires member option */
    if (options & HDK_XML_ParseOption_NoXML)
    {
        options |= HDK_XML_ParseOption_Member;
    }

    /* XML parse? */
    pSchemaNode = HDK_XML_Schema_GetNode(pSchema, ixSchemaNode);
    if (!(options & HDK_XML_ParseOption_NoXML) ||
        pSchemaNode->type == HDK_XML_BuiltinType_Struct)
    {
        /* Create the parse context */
        pParseCtx = HDK_XML_Parse_NewEx(pSchema, pStruct, options, ixSchemaNode, cbMaxAlloc);
        if (!pParseCtx)
        {
            return HDK_XML_ParseError_OutOfMemory;
        }
    }

    /* Initialize the value buffer */
    memset(&bufferCtx, 0, sizeof(bufferCtx));

    /* Parse the input one chunk at a time */
    while (!cbContentLength || cbTotal < cbContentLength)
    {
        unsigned int cbRead;

        /* Determine the input chunk size */
        unsigned int cbChunk = (cbContentLength ? cbContentLength - cbTotal : sizeof(szBuf));
        if (cbChunk > sizeof(szBuf))
        {
            cbChunk = sizeof(szBuf);
        }

        /* Read the input chunk */
        if (!pfnStream(&cbRead, pStreamCtx, szBuf, cbChunk))
        {
            HDK_XML_LOGFMT1(HDK_LOG_Level_Error, "Failed to read %u bytes from parse input stream\n", cbChunk);
            parseError = HDK_XML_ParseError_IOError;
            break;
        }

        /* Handle the input chunk */
        if (pParseCtx)
        {
            /* XML-parse the input chunk */
            parseError = HDK_XML_Parse_Data(pParseCtx, szBuf, cbRead);
        }
        else
        {
            /* Append to the value buffer */
            if (!HDK_XML_OutputStream_GrowBuffer(0, &bufferCtx, szBuf, cbRead))
            {
                parseError = HDK_XML_ParseError_OutOfMemory;
            }
            else if (cbMaxAlloc && bufferCtx.ixCur > cbMaxAlloc)
            {
                parseError = HDK_XML_ParseError_XMLInvalid;
            }
            else
            {
                bufferCtx.pBuf[bufferCtx.ixCur] = '\0';
            }
        }
        if (parseError != HDK_XML_ParseError_OK)
        {
            break;
        }

        /* Update the total */
        cbTotal += cbRead;

        /* Done? */
        if (cbRead != cbChunk)
        {
            break;
        }
    }

    /* Free the parse context */
    if (pParseCtx)
    {
        HDK_XML_ParseError parseErrorFree = HDK_XML_Parse_Free(pParseCtx);
        if (parseError == HDK_XML_ParseError_OK)
        {
            parseError = parseErrorFree;
        }
    }
    else
    {
        /* Deserialize the value buffer */
        HDK_XML_SchemaInternal schema;
        HDK_XML_Schema_Init(&schema, pSchema);
        parseError = HDK_XML_DeserializeMember(&schema, pSchemaNode->type, pStruct,
                                               pSchemaNode->element, bufferCtx.pBuf);
    }

    /* Free the value buffer */
    free(bufferCtx.pBuf);

    /* Bad content length? */
    if (parseError == HDK_XML_ParseError_OK && cbContentLength && cbTotal != cbContentLength)
    {
        parseError = HDK_XML_ParseError_BadContentLength;
        HDK_XML_LOGFMT2(HDK_LOG_Level_Error, "Content length %u differs from expected %u\n", cbContentLength, cbTotal);
    }

    /* Return the result */
    return parseError;
}


/*
 * Type de-serialization
 */

int HDK_XML_Parse_Blob(char* pData, unsigned int* pcbData, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_Blob blobMember;
    blobMember.node.type = HDK_XML_BuiltinType_Blob;
    blobMember.pValue = 0;
    blobMember.cbValue = 0;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_Blob, 0, (HDK_XML_Member*)&blobMember, pszValue);
    if (fSuccess)
    {
        if (pData)
        {
            fSuccess = (blobMember.cbValue <= *pcbData);
            if (fSuccess)
            {
                memcpy(pData, blobMember.pValue, blobMember.cbValue);
            }
        }

        /* Return bytes copied/required */
        *pcbData = blobMember.cbValue;
    }

    free(blobMember.pValue);

    return fSuccess;
}

int HDK_XML_Parse_Bool(int* pfValue, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_Bool boolMember;
    boolMember.node.type = HDK_XML_BuiltinType_Bool;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_Bool, 0, (HDK_XML_Member*)&boolMember, pszValue);
    if (fSuccess)
    {
        *pfValue = boolMember.fValue;
    }

    return fSuccess;
}

int HDK_XML_Parse_DateTime(time_t* ptime, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_DateTime datetimeMember;
    datetimeMember.node.type = HDK_XML_BuiltinType_DateTime;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_DateTime, 0, (HDK_XML_Member*)&datetimeMember, pszValue);
    if (fSuccess)
    {
        *ptime = datetimeMember.tValue;
    }

    return fSuccess;
}

int HDK_XML_Parse_Int(HDK_XML_Int* pInt, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_Int intMember;
    intMember.node.type = HDK_XML_BuiltinType_Int;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_Int, 0, (HDK_XML_Member*)&intMember, pszValue);
    if (fSuccess)
    {
        *pInt = intMember.iValue;
    }

    return fSuccess;
}

int HDK_XML_Parse_Long(HDK_XML_Long* pLong, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_Long longMember;
    longMember.node.type = HDK_XML_BuiltinType_Long;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_Long, 0, (HDK_XML_Member*)&longMember, pszValue);
    if (fSuccess)
    {
        *pLong = longMember.llValue;
    }

    return fSuccess;
}

int HDK_XML_Parse_IPAddress(HDK_XML_IPAddress* pIPAddress, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_IPAddress ipMember;
    ipMember.node.type = HDK_XML_BuiltinType_IPAddress;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_IPAddress, 0, (HDK_XML_Member*)&ipMember, pszValue);
    if (fSuccess)
    {
        *pIPAddress = ipMember.ipAddress;
    }

    return fSuccess;
}

int HDK_XML_Parse_MACAddress(HDK_XML_MACAddress* pMACAddress, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_MACAddress macMember;
    macMember.node.type = HDK_XML_BuiltinType_MACAddress;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_MACAddress, 0, (HDK_XML_Member*)&macMember, pszValue);
    if (fSuccess)
    {
        *pMACAddress = macMember.macAddress;
    }

    return fSuccess;
}

int HDK_XML_Parse_UUID(HDK_XML_UUID* pUUID, const char* pszValue)
{
    int fSuccess;

    HDK_XML_Member_UUID uuidMember;
    uuidMember.node.type = HDK_XML_BuiltinType_UUID;

    fSuccess = HDK_XML_CallTypeFn_Deserialize(HDK_XML_BuiltinType_UUID, 0, (HDK_XML_Member*)&uuidMember, pszValue);
    if (fSuccess)
    {
        memcpy(pUUID, &uuidMember.uuid, sizeof(*pUUID));
    }

    return fSuccess;
}
