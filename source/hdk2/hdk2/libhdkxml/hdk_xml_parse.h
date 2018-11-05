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
