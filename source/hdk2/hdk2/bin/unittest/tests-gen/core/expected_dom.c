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

#include "actual_dom.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAP/",
    /* 1 */ "http://cisco.com/HNAP/Sub/",
    /* 2 */ "http://cisco.com/HNAPExt/",
    /* 3 */ "http://cisco.com/HNAPExt/Sub/",
    /* 4 */ "http://purenetworks.com/HNAP/",
    /* 5 */ "http://purenetworks.com/HNAP/Sub/",
    /* 6 */ "http://purenetworks.com/HNAP1/",
    /* 7 */ "http://purenetworks.com/HNAP1/Sub/",
    /* 8 */ "http://purenetworks.com/HNAPExt/",
    /* 9 */ "http://purenetworks.com/HNAPExt/Sub/",
    /* 10 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_DOM_Element_Action = 0 */ { 0, "Action" },
    /* ACTUAL_DOM_Element_ActionResponse = 1 */ { 0, "ActionResponse" },
    /* ACTUAL_DOM_Element_ActionResult = 2 */ { 0, "ActionResult" },
    /* ACTUAL_DOM_Element_Member = 3 */ { 0, "Member" },
    /* ACTUAL_DOM_Element_Sub_ActionSub = 4 */ { 1, "ActionSub" },
    /* ACTUAL_DOM_Element_Sub_ActionSubResponse = 5 */ { 1, "ActionSubResponse" },
    /* ACTUAL_DOM_Element_Sub_ActionSubResult = 6 */ { 1, "ActionSubResult" },
    /* ACTUAL_DOM_Element_Sub_MemberSub = 7 */ { 1, "MemberSub" },
    /* ACTUAL_DOM_Element_Cisco_ActionExt = 8 */ { 2, "ActionExt" },
    /* ACTUAL_DOM_Element_Cisco_ActionExtResponse = 9 */ { 2, "ActionExtResponse" },
    /* ACTUAL_DOM_Element_Cisco_ActionExtResult = 10 */ { 2, "ActionExtResult" },
    /* ACTUAL_DOM_Element_Cisco_MemberExt = 11 */ { 2, "MemberExt" },
    /* ACTUAL_DOM_Element_Cisco_Sub_ActionExtSub = 12 */ { 3, "ActionExtSub" },
    /* ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResponse = 13 */ { 3, "ActionExtSubResponse" },
    /* ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResult = 14 */ { 3, "ActionExtSubResult" },
    /* ACTUAL_DOM_Element_Cisco_Sub_MemberExtSub = 15 */ { 3, "MemberExtSub" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2 = 16 */ { 4, "ActionLegacy2" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Response = 17 */ { 4, "ActionLegacy2Response" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Result = 18 */ { 4, "ActionLegacy2Result" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP_MemberLegacy2 = 19 */ { 4, "MemberLegacy2" },
    /* ACTUAL_DOM_Element_PN_Sub_ActionLegacy2Sub = 20 */ { 5, "ActionLegacy2Sub" },
    /* ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResponse = 21 */ { 5, "ActionLegacy2SubResponse" },
    /* ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResult = 22 */ { 5, "ActionLegacy2SubResult" },
    /* ACTUAL_DOM_Element_PN_Sub_MemberLegacy2Sub = 23 */ { 5, "MemberLegacy2Sub" },
    /* ACTUAL_DOM_Element_PN_ActionLegacy = 24 */ { 6, "ActionLegacy" },
    /* ACTUAL_DOM_Element_PN_ActionLegacyResponse = 25 */ { 6, "ActionLegacyResponse" },
    /* ACTUAL_DOM_Element_PN_ActionLegacyResult = 26 */ { 6, "ActionLegacyResult" },
    /* ACTUAL_DOM_Element_PN_MemberLegacy = 27 */ { 6, "MemberLegacy" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacy = 28 */ { 7, "ActionSubLegacy" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResponse = 29 */ { 7, "ActionSubLegacyResponse" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResult = 30 */ { 7, "ActionSubLegacyResult" },
    /* ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_MemberLegacySub = 31 */ { 7, "MemberLegacySub" },
    /* ACTUAL_DOM_Element_Purenetworks_ActionLegacyExt = 32 */ { 8, "ActionLegacyExt" },
    /* ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResponse = 33 */ { 8, "ActionLegacyExtResponse" },
    /* ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResult = 34 */ { 8, "ActionLegacyExtResult" },
    /* ACTUAL_DOM_Element_Purenetworks_MemberLegacyExt = 35 */ { 8, "MemberLegacyExt" },
    /* ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSub = 36 */ { 9, "ActionLegacyExtSub" },
    /* ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResponse = 37 */ { 9, "ActionLegacyExtSubResponse" },
    /* ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResult = 38 */ { 9, "ActionLegacyExtSubResult" },
    /* ACTUAL_DOM_Element_Purenetworks_Sub_MemberLegacyExtSub = 39 */ { 9, "MemberLegacyExtSub" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 40 */ { 10, "Body" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 41 */ { 10, "Envelope" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 42 */ { 10, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration http://cisco.com/HNAP/ActionResult
 */

static const HDK_XML_EnumValue s_enum_ActionResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAP/Sub/ActionSubResult
 */

static const HDK_XML_EnumValue s_enum_Sub_ActionSubResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/ActionExtResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_ActionExtResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAPExt/Sub/ActionExtSubResult
 */

static const HDK_XML_EnumValue s_enum_Cisco_Sub_ActionExtSubResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP/ActionLegacy2Result
 */

static const HDK_XML_EnumValue s_enum_Purenetworks_HNAP_ActionLegacy2Result[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP/Sub/ActionLegacy2SubResult
 */

static const HDK_XML_EnumValue s_enum_PN_Sub_ActionLegacy2SubResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/ActionLegacyResult
 */

static const HDK_XML_EnumValue s_enum_PN_ActionLegacyResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/Sub/ActionSubLegacyResult
 */

static const HDK_XML_EnumValue s_enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAPExt/ActionLegacyExtResult
 */

static const HDK_XML_EnumValue s_enum_Purenetworks_ActionLegacyExtResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAPExt/Sub/ActionLegacyExtSubResult
 */

static const HDK_XML_EnumValue s_enum_Purenetworks_Sub_ActionLegacyExtSubResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_ActionResult,
    s_enum_Sub_ActionSubResult,
    s_enum_Cisco_ActionExtResult,
    s_enum_Cisco_Sub_ActionExtSubResult,
    s_enum_Purenetworks_HNAP_ActionLegacy2Result,
    s_enum_PN_Sub_ActionLegacy2SubResult,
    s_enum_PN_ActionLegacyResult,
    s_enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult,
    s_enum_Purenetworks_ActionLegacyExtResult,
    s_enum_Purenetworks_Sub_ActionLegacyExtSubResult
};


/*
 * Method http://cisco.com/HNAP/Action
 */

static const HDK_XML_SchemaNode s_schemaNodes_Action_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Action, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Member, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Action_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Action,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Action_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Action_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Action_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_ActionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_ActionResult, ACTUAL_DOM_EnumType_ActionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Action_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_ActionResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Action_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Action_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAP/Sub/ActionSub
 */

static const HDK_XML_SchemaNode s_schemaNodes_Sub_ActionSub_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Sub_ActionSub, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Sub_MemberSub, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Sub_ActionSub_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Sub_ActionSub,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Sub_ActionSub_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Sub_ActionSub_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Sub_ActionSub_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Sub_ActionSubResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Sub_ActionSubResult, ACTUAL_DOM_EnumType_Sub_ActionSubResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Sub_ActionSub_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Sub_ActionSubResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Sub_ActionSub_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Sub_ActionSub_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAPExt/ActionExt
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_ActionExt_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_ActionExt, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_MemberExt, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_ActionExt_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_ActionExt,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_ActionExt_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_ActionExt_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_ActionExt_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_ActionExtResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_ActionExtResult, ACTUAL_DOM_EnumType_Cisco_ActionExtResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_ActionExt_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_ActionExtResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_ActionExt_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_ActionExt_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAPExt/Sub/ActionExtSub
 */

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Sub_ActionExtSub_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_Sub_ActionExtSub, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_Sub_MemberExtSub, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Sub_ActionExtSub_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_Sub_ActionExtSub,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Sub_ActionExtSub_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Sub_ActionExtSub_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Cisco_Sub_ActionExtSub_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResult, ACTUAL_DOM_EnumType_Cisco_Sub_ActionExtSubResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Cisco_Sub_ActionExtSub_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Cisco_Sub_ActionExtSub_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Cisco_Sub_ActionExtSub_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP/ActionLegacy2
 */

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_HNAP_ActionLegacy2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_HNAP_MemberLegacy2, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_HNAP_ActionLegacy2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_HNAP_ActionLegacy2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_HNAP_ActionLegacy2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_HNAP_ActionLegacy2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Result, ACTUAL_DOM_EnumType_Purenetworks_HNAP_ActionLegacy2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_HNAP_ActionLegacy2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_HNAP_ActionLegacy2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_HNAP_ActionLegacy2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP/Sub/ActionLegacy2Sub
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_Sub_ActionLegacy2Sub_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_Sub_ActionLegacy2Sub, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Sub_MemberLegacy2Sub, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_Sub_ActionLegacy2Sub_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_Sub_ActionLegacy2Sub,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_Sub_ActionLegacy2Sub_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_Sub_ActionLegacy2Sub_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_Sub_ActionLegacy2Sub_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResult, ACTUAL_DOM_EnumType_PN_Sub_ActionLegacy2SubResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_Sub_ActionLegacy2Sub_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_Sub_ActionLegacy2Sub_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_Sub_ActionLegacy2Sub_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/ActionLegacy
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_ActionLegacy_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_ActionLegacy, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_MemberLegacy, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_ActionLegacy_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_ActionLegacy,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_ActionLegacy_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_ActionLegacy_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_ActionLegacy_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_ActionLegacyResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_ActionLegacyResult, ACTUAL_DOM_EnumType_PN_ActionLegacyResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_ActionLegacy_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_ActionLegacyResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_ActionLegacy_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_ActionLegacy_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/Sub/ActionSubLegacy
 */

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacy, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_MemberLegacySub, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacy,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResult, ACTUAL_DOM_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAPExt/ActionLegacyExt
 */

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_ActionLegacyExt_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_ActionLegacyExt, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_MemberLegacyExt, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_ActionLegacyExt_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_ActionLegacyExt,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_ActionLegacyExt_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_ActionLegacyExt_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_ActionLegacyExt_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResult, ACTUAL_DOM_EnumType_Purenetworks_ActionLegacyExtResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_ActionLegacyExt_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_ActionLegacyExt_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_ActionLegacyExt_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAPExt/Sub/ActionLegacyExtSub
 */

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_Sub_ActionLegacyExtSub_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSub, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_Sub_MemberLegacyExtSub, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_Sub_ActionLegacyExtSub_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSub,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_Sub_ActionLegacyExtSub_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_Sub_ActionLegacyExtSub_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_Purenetworks_Sub_ActionLegacyExtSub_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResult, ACTUAL_DOM_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_Purenetworks_Sub_ActionLegacyExtSub_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_Purenetworks_Sub_ActionLegacyExtSub_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_Purenetworks_Sub_ActionLegacyExtSub_Output,
    s_enumTypes
};


/*
 * Methods
 */

static const HDK_MOD_Method s_methods[] =
{
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP/Action",
        ACTUAL_DOM_Method_Action,
        &s_schema_Action_Input,
        &s_schema_Action_Output,
        s_elementPath_Action_Input,
        s_elementPath_Action_Output,
        0,
        ACTUAL_DOM_Element_ActionResult,
        ACTUAL_DOM_EnumType_ActionResult,
        ACTUAL_DOM_Enum_ActionResult_OK,
        ACTUAL_DOM_Enum_ActionResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP/Sub/ActionSub",
        ACTUAL_DOM_Method_Sub_ActionSub,
        &s_schema_Sub_ActionSub_Input,
        &s_schema_Sub_ActionSub_Output,
        s_elementPath_Sub_ActionSub_Input,
        s_elementPath_Sub_ActionSub_Output,
        0,
        ACTUAL_DOM_Element_Sub_ActionSubResult,
        ACTUAL_DOM_EnumType_Sub_ActionSubResult,
        ACTUAL_DOM_Enum_Sub_ActionSubResult_OK,
        ACTUAL_DOM_Enum_Sub_ActionSubResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/ActionExt",
        ACTUAL_DOM_Method_Cisco_ActionExt,
        &s_schema_Cisco_ActionExt_Input,
        &s_schema_Cisco_ActionExt_Output,
        s_elementPath_Cisco_ActionExt_Input,
        s_elementPath_Cisco_ActionExt_Output,
        0,
        ACTUAL_DOM_Element_Cisco_ActionExtResult,
        ACTUAL_DOM_EnumType_Cisco_ActionExtResult,
        ACTUAL_DOM_Enum_Cisco_ActionExtResult_OK,
        ACTUAL_DOM_Enum_Cisco_ActionExtResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAPExt/Sub/ActionExtSub",
        ACTUAL_DOM_Method_Cisco_Sub_ActionExtSub,
        &s_schema_Cisco_Sub_ActionExtSub_Input,
        &s_schema_Cisco_Sub_ActionExtSub_Output,
        s_elementPath_Cisco_Sub_ActionExtSub_Input,
        s_elementPath_Cisco_Sub_ActionExtSub_Output,
        0,
        ACTUAL_DOM_Element_Cisco_Sub_ActionExtSubResult,
        ACTUAL_DOM_EnumType_Cisco_Sub_ActionExtSubResult,
        ACTUAL_DOM_Enum_Cisco_Sub_ActionExtSubResult_OK,
        ACTUAL_DOM_Enum_Cisco_Sub_ActionExtSubResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP/ActionLegacy2",
        ACTUAL_DOM_Method_Purenetworks_HNAP_ActionLegacy2,
        &s_schema_Purenetworks_HNAP_ActionLegacy2_Input,
        &s_schema_Purenetworks_HNAP_ActionLegacy2_Output,
        s_elementPath_Purenetworks_HNAP_ActionLegacy2_Input,
        s_elementPath_Purenetworks_HNAP_ActionLegacy2_Output,
        0,
        ACTUAL_DOM_Element_Purenetworks_HNAP_ActionLegacy2Result,
        ACTUAL_DOM_EnumType_Purenetworks_HNAP_ActionLegacy2Result,
        ACTUAL_DOM_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK,
        ACTUAL_DOM_Enum_Purenetworks_HNAP_ActionLegacy2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP/Sub/ActionLegacy2Sub",
        ACTUAL_DOM_Method_PN_Sub_ActionLegacy2Sub,
        &s_schema_PN_Sub_ActionLegacy2Sub_Input,
        &s_schema_PN_Sub_ActionLegacy2Sub_Output,
        s_elementPath_PN_Sub_ActionLegacy2Sub_Input,
        s_elementPath_PN_Sub_ActionLegacy2Sub_Output,
        0,
        ACTUAL_DOM_Element_PN_Sub_ActionLegacy2SubResult,
        ACTUAL_DOM_EnumType_PN_Sub_ActionLegacy2SubResult,
        ACTUAL_DOM_Enum_PN_Sub_ActionLegacy2SubResult_OK,
        ACTUAL_DOM_Enum_PN_Sub_ActionLegacy2SubResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/ActionLegacy",
        ACTUAL_DOM_Method_PN_ActionLegacy,
        &s_schema_PN_ActionLegacy_Input,
        &s_schema_PN_ActionLegacy_Output,
        s_elementPath_PN_ActionLegacy_Input,
        s_elementPath_PN_ActionLegacy_Output,
        0,
        ACTUAL_DOM_Element_PN_ActionLegacyResult,
        ACTUAL_DOM_EnumType_PN_ActionLegacyResult,
        ACTUAL_DOM_Enum_PN_ActionLegacyResult_OK,
        ACTUAL_DOM_Enum_PN_ActionLegacyResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/Sub/ActionSubLegacy",
        ACTUAL_DOM_Method_Purenetworks_HNAP1_Sub_ActionSubLegacy,
        &s_schema_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input,
        &s_schema_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output,
        s_elementPath_Purenetworks_HNAP1_Sub_ActionSubLegacy_Input,
        s_elementPath_Purenetworks_HNAP1_Sub_ActionSubLegacy_Output,
        0,
        ACTUAL_DOM_Element_Purenetworks_HNAP1_Sub_ActionSubLegacyResult,
        ACTUAL_DOM_EnumType_Purenetworks_HNAP1_Sub_ActionSubLegacyResult,
        ACTUAL_DOM_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK,
        ACTUAL_DOM_Enum_Purenetworks_HNAP1_Sub_ActionSubLegacyResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAPExt/ActionLegacyExt",
        ACTUAL_DOM_Method_Purenetworks_ActionLegacyExt,
        &s_schema_Purenetworks_ActionLegacyExt_Input,
        &s_schema_Purenetworks_ActionLegacyExt_Output,
        s_elementPath_Purenetworks_ActionLegacyExt_Input,
        s_elementPath_Purenetworks_ActionLegacyExt_Output,
        0,
        ACTUAL_DOM_Element_Purenetworks_ActionLegacyExtResult,
        ACTUAL_DOM_EnumType_Purenetworks_ActionLegacyExtResult,
        ACTUAL_DOM_Enum_Purenetworks_ActionLegacyExtResult_OK,
        ACTUAL_DOM_Enum_Purenetworks_ActionLegacyExtResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAPExt/Sub/ActionLegacyExtSub",
        ACTUAL_DOM_Method_Purenetworks_Sub_ActionLegacyExtSub,
        &s_schema_Purenetworks_Sub_ActionLegacyExtSub_Input,
        &s_schema_Purenetworks_Sub_ActionLegacyExtSub_Output,
        s_elementPath_Purenetworks_Sub_ActionLegacyExtSub_Input,
        s_elementPath_Purenetworks_Sub_ActionLegacyExtSub_Output,
        0,
        ACTUAL_DOM_Element_Purenetworks_Sub_ActionLegacyExtSubResult,
        ACTUAL_DOM_EnumType_Purenetworks_Sub_ActionLegacyExtSubResult,
        ACTUAL_DOM_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK,
        ACTUAL_DOM_Enum_Purenetworks_Sub_ActionLegacyExtSubResult_OK
    },
    HDK_MOD_MethodsEnd
};


/*
 * Module
 */

/* 9fb9fd20-25cc-4f1f-9f17-50eb1c3ecf0d */
static const HDK_XML_UUID s_uuid_NOID =
{
    { 0x9f, 0xb9, 0xfd, 0x20, 0x25, 0xcc, 0x4f, 0x1f, 0x9f, 0x17, 0x50, 0xeb, 0x1c, 0x3e, 0xcf, 0x0d }
};

static const HDK_MOD_Module s_module =
{
    &s_uuid_NOID,
    0,
    0,
    s_methods,
    0,
    0
};

const HDK_MOD_Module* ACTUAL_DOM_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
