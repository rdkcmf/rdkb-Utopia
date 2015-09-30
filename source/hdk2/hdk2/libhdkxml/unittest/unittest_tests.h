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

#ifndef __UNITTEST_TESTS_H__
#define __UNITTEST_TESTS_H__

/* Unit tests */
extern void Test_InputStreamBuffer(void);
extern void Test_OutputStreamBuffer(void);
extern void Test_ParseBlob(void);
extern void Test_ParseBool(void);
extern void Test_ParseCSV(void);
extern void Test_ParseDateTime(void);
extern void Test_ParseEx(void);
extern void Test_ParseInt(void);
extern void Test_ParseInvalidTypes(void);
extern void Test_ParseInvalidXML(void);
extern void Test_ParseIPAddress(void);
extern void Test_ParseLong(void);
extern void Test_ParseMACAddress(void);
extern void Test_ParseMaxAlloc(void);
extern void Test_ParseProperties(void);
extern void Test_ParseSimple(void);
extern void Test_ParseUUID(void);
extern void Test_ParseValidTypes(void);
extern void Test_ParseValidLocalDateTime(void);
extern void Test_SerializeBlob(void);
extern void Test_SerializeBool(void);
extern void Test_SerializeCSV(void);
extern void Test_SerializeDateTime(void);
extern void Test_SerializeErrorOutput(void);
extern void Test_SerializeInt(void);
extern void Test_SerializeIPAddress(void);
extern void Test_SerializeLong(void);
extern void Test_SerializeMACAddress(void);
extern void Test_SerializeString(void);
extern void Test_SerializeUUID(void);
extern void Test_Struct(void);
extern void Test_StructBlank(void);
extern void Test_StructBlob(void);
extern void Test_StructBool(void);
extern void Test_StructDateTime(void);
extern void Test_StructEnum(void);
extern void Test_StructInt(void);
extern void Test_StructIPAddress(void);
extern void Test_StructLong(void);
extern void Test_StructMACAddress(void);
extern void Test_StructMember(void);
extern void Test_StructString(void);
extern void Test_TypeInt(void);
extern void Test_TypeLong(void);

#endif /* __UNITTEST_TESTS_H__ */
