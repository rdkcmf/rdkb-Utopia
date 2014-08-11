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

#ifndef __HDK_XML_PARSE_H__
#define __HDK_XML_PARSE_H__

#include "hdk_xml.h"
#include "hdk_xml_schema.h"

/* Parser struct state */
typedef enum _HDK_XML_ParseStructState
{
    HDK_XML_ParseStructState_Init,
    HDK_XML_ParseStructState_InitMember,
    HDK_XML_ParseStructState_Populating,
    HDK_XML_ParseStructState_Done
} HDK_XML_ParseStructState;

/* XML deserialization (parse) context */
typedef struct _HDK_XML_ParseContext
{
    void* pXMLParser;
    HDK_XML_SchemaInternal schema;
    HDK_XML_ParseError parseError;      /* Parser error code */
    HDK_XML_ParseStructState structState; /* Non-zero => at top level struct/schema; 0 otherwise */
    HDK_XML_Struct* pStack[16];         /* Struct stack */
    unsigned int ixStack;               /* Current struct stack index */
    unsigned int ixSchemaNode;          /* Current element schema node index */
    HDK_XML_OutputStream_BufferContext bufferCtx; /* Accumulated value text */
    unsigned int cAnyElement;           /* Depth of "any" elements */
    unsigned int cElements;             /* Total element count */
    unsigned int cbTotalAlloc;          /* Total parse memory allocation size */
    unsigned int cbMaxAlloc;            /* Maximum total parse memory allocation size */
    unsigned int options;               /* Parsing options */
} HDK_XML_ParseContext;

#endif /* __HDK_XML_PARSE_H__ */
