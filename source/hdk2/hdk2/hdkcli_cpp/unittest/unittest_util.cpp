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

#include "unittest.h"
#include "unittest_schema.h"
#include "unittest_util.h"

#include "hdk_xml.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


const char* DateTimeToString(time_t t, int fUTC)
{
    static char s_szBuf[32];
    struct tm tm;
    if (fUTC)
    {
        HDK_XML_gmtime(t, &tm);
    }
    else
    {
        HDK_XML_localtime(t, &tm);
    }
    sprintf(s_szBuf, (fUTC ? "%04d-%02d-%02dT%02d:%02d:%02dZ" : "%04d-%02d-%02dT%02d:%02d:%02d"),
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_min);
    return s_szBuf;
}

const char* EnumToString(HDK_XML_Type type, int value)
{
    if (HDK_XML_Enum_Unknown == value)
    {
        return "__UNKNOWN__";
    }
    else
    {
        return s_enumTypes[-type - 1][value];
    }
}

static const char* ParseErrorToString(HDK_XML_ParseError parseError)
{
    const char* parseErrors[] =
    {
        "HDK_XML_ParseError_OK",
        "HDK_XML_ParseError_OutOfMemory",
        "HDK_XML_ParseError_XMLInvalid",
        "HDK_XML_ParseError_XMLUnexpectedElement",
        "HDK_XML_ParseError_XMLInvalidValue"
    };
    return parseErrors[parseError];
}

void ParseTestHelper(HDK_XML_Struct* pStruct, const HDK_XML_SchemaNode* pSchemaNodes, ...)
{
    va_list args;
    HDK_XML_Schema schema;
    char* pszXMLChunk;
    char* pszXML;
    unsigned int cbXML = 0;
    HDK_XML_InputStream_BufferContext bufferCtx;

    /* Variable arguments init */
    va_start(args, pSchemaNodes);

    /* Schema */
    memset(&schema, 0, sizeof(schema));
    schema.ppszNamespaces = s_namespaces;
    schema.pElements = s_elements;
    schema.pSchemaNodes = pSchemaNodes;
    schema.pEnumTypes = s_enumTypes;

    /* Create one XML buffer */
    for (pszXMLChunk = va_arg(args, char*); pszXMLChunk; pszXMLChunk = va_arg(args, char*))
    {
        cbXML += (unsigned int)strlen(pszXMLChunk);
    }
    pszXML = (char*)malloc(cbXML + 1);
    if (pszXML)
    {
        HDK_XML_ParseError parseError;

        *pszXML = 0;
        va_start(args, pSchemaNodes);
        for (pszXMLChunk = va_arg(args, char*); pszXMLChunk; pszXMLChunk = va_arg(args, char*))
        {
            strcat(pszXML, pszXMLChunk);
        }

        /* Parse the XML */
        memset(&bufferCtx, 0, sizeof(bufferCtx));
        bufferCtx.pBuf = pszXML;
        bufferCtx.cbBuf = cbXML;

        parseError = HDK_XML_Parse(HDK_XML_InputStream_Buffer, &bufferCtx, &schema, pStruct, cbXML);
        if (parseError != HDK_XML_ParseError_OK)
        {
            UnittestLog1("Error: HDK_XML_Parse failed with parse error %s\n",
                         ParseErrorToString(parseError));
        }

        /* Free the XML buffer */
        free(pszXML);
    }

    /* Variable arguments free */
    va_end(args);
}

void SerializeTestHelper(const HDK_XML_Struct* pStruct, const HDK_XML_SchemaNode* pSchemaNodes)
{
    HDK_XML_Schema schema;
    unsigned int cbStream = 0;

    /* Schema */
    memset(&schema, 0, sizeof(schema));
    schema.ppszNamespaces = s_namespaces;
    schema.pElements = s_elements;
    schema.pSchemaNodes = pSchemaNodes;
    schema.pEnumTypes = s_enumTypes;

    if (!HDK_XML_Serialize(&cbStream, HDK_XML_OutputStream_File, stdout, &schema, pStruct, 0))
    {
        UnittestLog("Error: HDK_XML_Serialize failed\n");
    }
}
