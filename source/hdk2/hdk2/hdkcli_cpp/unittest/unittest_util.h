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

#pragma once

#include "hdk_xml.h"

/* Convert HDK types to string - NOT thread-safe! */
extern const char* DateTimeToString(time_t t, int fUTC);

/* Convert an enum value to its serialized representation (string). */
extern const char* EnumToString(HDK_XML_Type type, int value);

/* Helper function for parse/serialize/validate tests */
extern void ParseTestHelper(HDK_XML_Struct* pStruct, const HDK_XML_SchemaNode* pSchemaNodes, ... /* XML chunks */);

extern void SerializeTestHelper(const HDK_XML_Struct* pStruct, const HDK_XML_SchemaNode* pSchemaNodes);

/* ParseTestHelper XML chunks terminator */
#define ParseTestHelperEnd (const char*)0
