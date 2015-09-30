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


const char* IPAddressToString(const HDK_XML_IPAddress* pIP)
{
    static char s_szBuf[32];
    sprintf(s_szBuf, "%d.%d.%d.%d", pIP->a, pIP->b, pIP->c, pIP->d);
    return s_szBuf;
}


const char* MACAddressToString(const HDK_XML_MACAddress* pMAC)
{
    static char s_szBuf[32];
    sprintf(s_szBuf, "%02X.%02X.%02X.%02X.%02X.%02X", pMAC->a, pMAC->b, pMAC->c, pMAC->d, pMAC->e, pMAC->f);
    return s_szBuf;
}


const char* UUIDToString(const HDK_XML_UUID* pUUID)
{
    static char s_szBuf[48];
    int i;
    unsigned int val[16];
    for (i = 0; i < 16; ++i)
    {
        val[i] = pUUID->bytes[i];
    }
    sprintf(s_szBuf, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7],
            val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15]);
    return s_szBuf;
}


const char* ParseErrorToString(HDK_XML_ParseError parseError)
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


const char* BuiltinTypeToString(HDK_XML_BuiltinType type)
{
    const char* types[] =
    {
        "Struct",
        "Blank",
        "Blob",
        "Bool",
        "DateTime",
        "Int",
        "Long",
        "String",
        "IPAddress",
        "MACAddress",
        "UUID"
    };
    return types[type];
}


void ParseTestHelper(HDK_XML_Struct* pStruct, const HDK_XML_SchemaNode* pSchemaNodes,
                     const HDK_XML_EnumType* pEnumTypes, unsigned int ixSchemaNode,
                     int options, unsigned int cbMaxAlloc, ...)
{
    va_list args;
    HDK_XML_Schema schema;
    char* pszXMLChunk;
    char* pszXML;
    unsigned int cbXML = 0;
    HDK_XML_InputStream_BufferContext bufferCtx;

    /* Variable arguments init */
    va_start(args, cbMaxAlloc);

    /* Schema */
    memset(&schema, 0, sizeof(schema));
    schema.ppszNamespaces = g_schema_namespaces;
    schema.pElements = g_schema_elements;
    schema.pSchemaNodes = pSchemaNodes;
    schema.pEnumTypes = pEnumTypes;

    /* Create one XML buffer */
    for (pszXMLChunk = va_arg(args, char*); pszXMLChunk; pszXMLChunk = va_arg(args, char*))
    {
        cbXML += (unsigned int)strlen(pszXMLChunk);
    }
    pszXML = (char*)malloc(cbXML + 1);
    if (pszXML)
    {
        int parseOptions = 0;
        HDK_XML_ParseError parseError;

        *pszXML = 0;
        va_start(args, cbMaxAlloc);
        for (pszXMLChunk = va_arg(args, char*); pszXMLChunk; pszXMLChunk = va_arg(args, char*))
        {
            strcat(pszXML, pszXMLChunk);
        }

        /* Parse options */
        if (options & ParseTestHelper_Parse_NoXML)
        {
            parseOptions |= HDK_XML_ParseOption_NoXML;
        }
        if (options & ParseTestHelper_Parse_Member)
        {
            parseOptions |= HDK_XML_ParseOption_Member;
        }
        if (options & ParseTestHelper_Parse_CSV)
        {
            parseOptions |= HDK_XML_ParseOption_CSV;
        }

        /* Parse the XML */
        memset(&bufferCtx, 0, sizeof(bufferCtx));
        bufferCtx.pBuf = pszXML;
        bufferCtx.cbBuf = cbXML;
        if (parseOptions || ixSchemaNode || cbMaxAlloc)
        {
            parseError = HDK_XML_ParseEx(HDK_XML_InputStream_Buffer, &bufferCtx, &schema, pStruct, cbXML,
                                         parseOptions, ixSchemaNode, cbMaxAlloc);
        }
        else
        {
            parseError = HDK_XML_Parse(HDK_XML_InputStream_Buffer, &bufferCtx, &schema, pStruct, cbXML);
        }
        if (parseError != HDK_XML_ParseError_OK)
        {
            UnittestLog1("Error: HDK_XML_Parse failed with parse error %s\n",
                         ParseErrorToString(parseError));
        }

        /* Free the XML buffer */
        free(pszXML);
    }

    /* Compute the content length */
    if (!(options & ParseTestHelper_NoSerialize))
    {
        unsigned int cbContent;
        int serializeOptions = 0;
        int fSerializeResult;

        /* Serialize options */
        if (options & ParseTestHelper_ErrorOutput)
        {
            serializeOptions += HDK_XML_SerializeOption_ErrorOutput;
        }
        if (options & ParseTestHelper_Serialize_NoNewlines)
        {
            serializeOptions += HDK_XML_SerializeOption_NoNewlines;
        }
        if (options & ParseTestHelper_Serialize_NoXML)
        {
            serializeOptions += HDK_XML_SerializeOption_NoXML;
        }
        if (options & ParseTestHelper_Serialize_CSV)
        {
            serializeOptions += HDK_XML_SerializeOption_CSV;
        }

        /* Compute the content length */
        if ((options & ParseTestHelper_Parse_Member) || (options & ParseTestHelper_Parse_NoXML))
        {
            fSerializeResult = HDK_XML_SerializeEx(&cbContent, HDK_XML_OutputStream_Null, 0, &schema,
                                                   pStruct->pTail, serializeOptions, ixSchemaNode);
        }
        else if (ixSchemaNode)
        {
            fSerializeResult = HDK_XML_SerializeEx(&cbContent, HDK_XML_OutputStream_Null, 0, &schema,
                                                   (HDK_XML_Member*)pStruct, serializeOptions, ixSchemaNode);
        }
        else
        {
            fSerializeResult = HDK_XML_Serialize(&cbContent, HDK_XML_OutputStream_Null, 0, &schema,
                                                 pStruct, serializeOptions);
        }

        if (fSerializeResult)
        {
            UnittestLog1("Content Length (Computed)= %d\n\n", cbContent);

            /* Serialize */
            if ((options & ParseTestHelper_Parse_Member) || (options & ParseTestHelper_Parse_NoXML))
            {
                fSerializeResult = HDK_XML_SerializeEx(&cbContent, UnittestStream, UnittestStreamCtx, &schema,
                                                       pStruct->pTail, serializeOptions, ixSchemaNode);
            }
            else if (ixSchemaNode)
            {
                fSerializeResult = HDK_XML_SerializeEx(&cbContent, UnittestStream, UnittestStreamCtx, &schema,
                                                       (HDK_XML_Member*)pStruct, serializeOptions, ixSchemaNode);
            }
            else
            {
                fSerializeResult = HDK_XML_Serialize(&cbContent, UnittestStream, UnittestStreamCtx, &schema,
                                                     pStruct, serializeOptions);
            }

            /* Serialize the struct */
            if (fSerializeResult)
            {
                UnittestLog1("\nContent Length (Actual) = %d\n", cbContent);
            }
            else
            {
                UnittestLog("\nStruct serialization failed!\n");
            }
        }
        else
        {
            UnittestLog("Null struct serialization failed!\n");
        }
    }

    /* Validate the struct */
    if (!(options & ParseTestHelper_NoValidate))
    {
        int fValid;
        int validateOptions = 0;

        /* Validate options */
        if (options & ParseTestHelper_ErrorOutput)
        {
            validateOptions += HDK_XML_ValidateOption_ErrorOutput;
        }

        /* Validate */
        if ((options & ParseTestHelper_Parse_Member) || (options & ParseTestHelper_Parse_NoXML))
        {
            fValid = HDK_XML_ValidateEx(&schema, pStruct->pTail, validateOptions, ixSchemaNode);
        }
        else if (ixSchemaNode)
        {
            fValid = HDK_XML_ValidateEx(&schema, (HDK_XML_Member*)pStruct, validateOptions, ixSchemaNode);
        }
        else
        {
            fValid = HDK_XML_Validate(&schema, pStruct, validateOptions);
        }

        UnittestLog1("Struct valid: %d\n", fValid);
    }

    /* Variable arguments free */
    va_end(args);
}
