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
#include "unittest_tests.h"

#include "hdk_xml.h"

#include <stdlib.h>
#include <string.h>


static void StreamBufferHelper(const char* pszTitle,
                               HDK_XML_OutputStreamFn pfnBuf, char* pBuf, unsigned int cbBuf,
                               const HDK_XML_Schema* pSchema, const HDK_XML_Struct* pStruct)
{
    HDK_XML_OutputStream_BufferContext bufferCtx;
    unsigned int cbContent;
    int result;

    UnittestLog1("\n%s:\n\n", pszTitle);

    /* Serialize the buffer */
    memset(&bufferCtx, 0, sizeof(bufferCtx));
    bufferCtx.pBuf = pBuf;
    bufferCtx.cbBuf = cbBuf;
    result = HDK_XML_Serialize(&cbContent, pfnBuf, &bufferCtx, pSchema, pStruct, 0);

    /* Display the result */
    if (result)
    {
        bufferCtx.pBuf[cbContent] = 0;
        UnittestLog3("Content Length = %d, %d\n\n%s", bufferCtx.ixCur, cbContent, bufferCtx.pBuf);
    }
    else
    {
        UnittestLog("Struct serialization failed!\n");
    }

    /* Free the buffer, if necessary */
    if (pfnBuf == HDK_XML_OutputStream_GrowBuffer)
    {
        free(bufferCtx.pBuf);
    }
}


void Test_OutputStreamBuffer()
{
    HDK_XML_Struct sTemp;
    unsigned int cbContent;
    char szBuf[1024];

    /* Schema table */
    HDK_XML_SchemaNode schemaNodes[] = {
        { /* 0 */ 0, Element_a, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, Element_b, HDK_XML_BuiltinType_Int, 0 },
        { /* 2 */ 0, Element_c, HDK_XML_BuiltinType_String, 0 },
        { 0, HDK_XML_BuiltinElement_Unknown, 0, 0 }
    };

    /* Schema struct */
    HDK_XML_Schema schema;
    memset(&schema, 0, sizeof(schema));
    schema.ppszNamespaces = g_schema_namespaces;
    schema.pElements = g_schema_elements;
    schema.pSchemaNodes = schemaNodes;

    /* Initialize the struct */
    HDK_XML_Struct_Init(&sTemp);
    sTemp.node.element = Element_a;
    HDK_XML_Set_Int(&sTemp, Element_b, 3);
    HDK_XML_Set_String(&sTemp, Element_c, "Hello");

    /* Compute the content length */
    if (HDK_XML_Serialize(&cbContent, HDK_XML_OutputStream_Null, 0, &schema, &sTemp, 0))
    {
        UnittestLog1("Content Length (Computed)= %d\n", cbContent);

        /* Serialize the struct to fixed-size buffers of various sizes */
        StreamBufferHelper("Serialize to a big fixed-size buffer",
                           HDK_XML_OutputStream_Buffer, szBuf, sizeof(szBuf) - 1, &schema, &sTemp);
        StreamBufferHelper("Serialize to an exact fixed-size buffer",
                           HDK_XML_OutputStream_Buffer, szBuf, cbContent, &schema, &sTemp);
        StreamBufferHelper("Serialize to a too-small fixed-size buffer",
                           HDK_XML_OutputStream_Buffer, szBuf, cbContent - 1, &schema, &sTemp);
        StreamBufferHelper("Serialize to a one-byte fixed-size buffer",
                           HDK_XML_OutputStream_Buffer, szBuf, 1, &schema, &sTemp);
        StreamBufferHelper("Serialize to a zero fixed-size buffer",
                           HDK_XML_OutputStream_Buffer, szBuf, 0, &schema, &sTemp);

        /* Serialize the struct to grow buffers */
        StreamBufferHelper("Serialize to a grow buffer (big initial size)",
                           HDK_XML_OutputStream_GrowBuffer, 0, 1024, &schema, &sTemp);
        StreamBufferHelper("Serialize to a grow buffer (small initial size)",
                           HDK_XML_OutputStream_GrowBuffer, 0, 8, &schema, &sTemp);
    }
    else
    {
        UnittestLog("Null struct serialization failed!\n");
    }

    /* Free the struct */
    HDK_XML_Struct_Free(&sTemp);
}


void Test_InputStreamBuffer()
{
    HDK_XML_InputStream_BufferContext bufferCtx;
    const char szBufInput[] = "123456789";
    char szBuf[10];
    unsigned int cbRead;
    int fResult;
    int i;

    /* Tests... */

    UnittestLog("Reading in 5-byte chunks:\n");
    memset(&bufferCtx, 0, sizeof(bufferCtx));
    bufferCtx.pBuf = szBufInput;
    bufferCtx.cbBuf = sizeof(szBufInput);
    for (i = 0; i < 4; ++i)
    {
        memset(szBuf, 0, sizeof(szBuf));
        fResult = HDK_XML_InputStream_Buffer(&cbRead, &bufferCtx, szBuf, 5);
        UnittestLog3("  result = %d, read = %d, \"%s\"\n", fResult, cbRead, szBuf);
    }

    UnittestLog("\nReading in 4-byte chunks:\n");
    memset(&bufferCtx, 0, sizeof(bufferCtx));
    bufferCtx.pBuf = szBufInput;
    bufferCtx.cbBuf = sizeof(szBufInput);
    for (i = 0; i < 4; ++i)
    {
        memset(szBuf, 0, sizeof(szBuf));
        fResult = HDK_XML_InputStream_Buffer(&cbRead, &bufferCtx, szBuf, 4);
        UnittestLog3("  result = %d, read = %d, \"%s\"\n", fResult, cbRead, szBuf);
    }
}
