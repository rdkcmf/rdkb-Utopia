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

#ifndef __HDK_XML_H__
#define __HDK_XML_H__

#include "hdk_log.h"

#include <time.h>

#ifndef _MSC_VER
#  include <stdint.h>
#endif


/* Macro to control public exports */
#ifdef __cplusplus
#  define HDK_XML_EXTERN_PREFIX extern "C"
#else
#  define HDK_XML_EXTERN_PREFIX extern
#endif

#ifdef HDK_XML_STATIC
#  define HDK_XML_EXPORT HDK_XML_EXTERN_PREFIX
#else
#  ifdef _MSC_VER
#    ifdef HDK_XML_BUILD
#      define HDK_XML_EXPORT HDK_XML_EXTERN_PREFIX __declspec(dllexport)
#    else
#      define HDK_XML_EXPORT HDK_XML_EXTERN_PREFIX __declspec(dllimport)
#    endif
#  else
#    ifdef HDK_XML_BUILD
#      define HDK_XML_EXPORT HDK_XML_EXTERN_PREFIX __attribute__ ((visibility("default")))
#    else
#      define HDK_XML_EXPORT HDK_XML_EXTERN_PREFIX
#    endif
#  endif
#endif


/*
 * Structure member definition
 */

/* Member node element */
typedef int HDK_XML_Element;

/* Built-in element enumeration */
typedef enum _HDK_XML_BuiltinElement
{
    HDK_XML_BuiltinElement_Unknown = -1
} HDK_XML_BuiltinElement;

/* Member node type (>= 0 => built-in type, < 0 => user type) */
typedef int HDK_XML_Type;

/* Built-in type enumeration */
typedef enum _HDK_XML_BuiltinType
{
    HDK_XML_BuiltinType_Struct = 0,
    HDK_XML_BuiltinType_Blank,
    HDK_XML_BuiltinType_Blob,
    HDK_XML_BuiltinType_Bool,
    HDK_XML_BuiltinType_DateTime,
    HDK_XML_BuiltinType_Int,
    HDK_XML_BuiltinType_Long,
    HDK_XML_BuiltinType_String,
    HDK_XML_BuiltinType_IPAddress,
    HDK_XML_BuiltinType_MACAddress,
    HDK_XML_BuiltinType_UUID
} HDK_XML_BuiltinType;

/* Structure member node */
typedef struct _HDK_XML_Member
{
    HDK_XML_Element element;
    HDK_XML_Type type;
    struct _HDK_XML_Member* pNext;
} HDK_XML_Member;

/* Struct member */
typedef struct _HDK_XML_Struct
{
    HDK_XML_Member node;
    HDK_XML_Member* pHead;
    HDK_XML_Member* pTail;
} HDK_XML_Struct;

/* Struct initialization/free */
HDK_XML_EXPORT void HDK_XML_Struct_Init(HDK_XML_Struct* pStruct);
HDK_XML_EXPORT void HDK_XML_Struct_Free(HDK_XML_Struct* pStruct);


/*
 * Member accessors
 */

/* Generic member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_Member* pMember);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_SetEx_Member(HDK_XML_Member* pMemberDst, const HDK_XML_Member* pMember);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_Member* pMember);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Get_Member(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type);

/* Unknown enumeration value */
#define HDK_XML_Enum_Unknown -1

/* Enumeration member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iValue);
HDK_XML_EXPORT int* HDK_XML_Get_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type);
HDK_XML_EXPORT int HDK_XML_GetEx_Enum(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Type type, int iDefault);
HDK_XML_EXPORT int* HDK_XML_GetMember_Enum(HDK_XML_Member* pMember, HDK_XML_Type type);

/* Struct member accessors */
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_Set_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_SetEx_Struct(HDK_XML_Struct* pStructDst, HDK_XML_Element element, const HDK_XML_Struct* pStruct);
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_Append_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_AppendEx_Struct(HDK_XML_Struct* pStructDst, HDK_XML_Element element, const HDK_XML_Struct* pStruct);
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_Get_Struct(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Struct* HDK_XML_GetMember_Struct(HDK_XML_Member* pMember);

/* Blank member accessors (use sparingly) */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Blank(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Blank(HDK_XML_Struct* pStruct, HDK_XML_Element element);

/* Bool member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fValue);
HDK_XML_EXPORT int* HDK_XML_Get_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT int HDK_XML_GetEx_Bool(HDK_XML_Struct* pStruct, HDK_XML_Element element, int fDefault);
HDK_XML_EXPORT int* HDK_XML_GetMember_Bool(HDK_XML_Member* pMember);

/* Int (signed 32 bit) type */
#ifdef _MSC_VER
typedef __int32 HDK_XML_Int;
#else
typedef int32_t HDK_XML_Int;
#endif

/* Int member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iValue);
HDK_XML_EXPORT HDK_XML_Int* HDK_XML_Get_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Int HDK_XML_GetEx_Int(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Int iDefault);
HDK_XML_EXPORT HDK_XML_Int* HDK_XML_GetMember_Int(HDK_XML_Member* pMember);

/* Long (signed 64 bit) type */
#ifdef _MSC_VER
typedef __int64 HDK_XML_Long;
#else
typedef int64_t HDK_XML_Long;
#endif

/* Long member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llValue);
HDK_XML_EXPORT HDK_XML_Long* HDK_XML_Get_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT HDK_XML_Long HDK_XML_GetEx_Long(HDK_XML_Struct* pStruct, HDK_XML_Element element, HDK_XML_Long llDefault);
HDK_XML_EXPORT HDK_XML_Long* HDK_XML_GetMember_Long(HDK_XML_Member* pMember);

/* String member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszValue);
HDK_XML_EXPORT char* HDK_XML_Get_String(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT const char* HDK_XML_GetEx_String(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pszDefault);
HDK_XML_EXPORT char* HDK_XML_GetMember_String(HDK_XML_Member* pMember);

/* DateTime member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tValue);
HDK_XML_EXPORT time_t* HDK_XML_Get_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT time_t HDK_XML_GetEx_DateTime(HDK_XML_Struct* pStruct, HDK_XML_Element element, time_t tDefault);
HDK_XML_EXPORT time_t* HDK_XML_GetMember_DateTime(HDK_XML_Member* pMember);

/* DateTime helper functions */
HDK_XML_EXPORT time_t HDK_XML_mktime(int year, int mon, int mday, int hour, int min, int sec, int fUTC);   /* Returns -1 for failure */
HDK_XML_EXPORT void HDK_XML_localtime(time_t t, struct tm* ptm);
HDK_XML_EXPORT void HDK_XML_gmtime(time_t t, struct tm* ptm);

/* Blob member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pValue, unsigned int cbValue);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, const char* pValue, unsigned int cbValue);
HDK_XML_EXPORT char* HDK_XML_Get_Blob(HDK_XML_Struct* pStruct, HDK_XML_Element element, unsigned int* pcbValue);
HDK_XML_EXPORT char* HDK_XML_GetMember_Blob(HDK_XML_Member* pMember, unsigned int* pcbValue);

/* IPAddress type */
typedef struct _HDK_XML_IPAddress
{
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
} HDK_XML_IPAddress;

/* IPAddress member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pIPAddress);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pIPAddress);
HDK_XML_EXPORT HDK_XML_IPAddress* HDK_XML_Get_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT const HDK_XML_IPAddress* HDK_XML_GetEx_IPAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_IPAddress* pDefault);
HDK_XML_EXPORT HDK_XML_IPAddress* HDK_XML_GetMember_IPAddress(HDK_XML_Member* pMember);

/* IPAddress helpers */
HDK_XML_EXPORT int HDK_XML_IsEqual_IPAddress(const HDK_XML_IPAddress* pIPAddress1, const HDK_XML_IPAddress* pIPAddress2);

/* MACAddress type */
typedef struct _HDK_XML_MACAddress
{
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    unsigned char e;
    unsigned char f;
} HDK_XML_MACAddress;

/* MACAddress member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pMACAddress);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pMACAddress);
HDK_XML_EXPORT HDK_XML_MACAddress* HDK_XML_Get_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT const HDK_XML_MACAddress* HDK_XML_GetEx_MACAddress(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_MACAddress* pDefault);
HDK_XML_EXPORT HDK_XML_MACAddress* HDK_XML_GetMember_MACAddress(HDK_XML_Member* pMember);

/* MACAddress helpers */
HDK_XML_EXPORT int HDK_XML_IsEqual_MACAddress(const HDK_XML_MACAddress* pMACAddress1, const HDK_XML_MACAddress* pMACAddress2);

/* UUID type */
typedef struct _HDK_XML_UUID
{
    unsigned char bytes[16];
} HDK_XML_UUID;

/* UUID member accessors */
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Set_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pUUID);
HDK_XML_EXPORT HDK_XML_Member* HDK_XML_Append_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pUUID);
HDK_XML_EXPORT HDK_XML_UUID* HDK_XML_Get_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element);
HDK_XML_EXPORT const HDK_XML_UUID* HDK_XML_GetEx_UUID(HDK_XML_Struct* pStruct, HDK_XML_Element element, const HDK_XML_UUID* pDefault);
HDK_XML_EXPORT HDK_XML_UUID* HDK_XML_GetMember_UUID(HDK_XML_Member* pMember);

/* UUID helpers */
HDK_XML_EXPORT int HDK_XML_IsEqual_UUID(const HDK_XML_UUID* pUUID1, const HDK_XML_UUID* pUUID2);


/*
 * Output streams
 */

/* Output stream function type - returns non-zero on success, zero otherwise */
typedef int (*HDK_XML_OutputStreamFn)(unsigned int* pcbStream /* optional */, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* C I/O FILE* output stream function (pStreamCtx is a FILE*) */
HDK_XML_EXPORT int HDK_XML_OutputStream_File(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* Buffer output stream function context */
typedef struct _HDK_XML_OutputStream_BufferContext
{
    char* pBuf;
    unsigned int cbBuf;                 /* Buffer size */
    unsigned int ixCur;                 /* Initialize to 0 */
} HDK_XML_OutputStream_BufferContext;

/* Buffer output stream function (pStreamCtx is HDK_XML_OutputStream_BufferContext*) */
HDK_XML_EXPORT int HDK_XML_OutputStream_Buffer(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* Grow-buffer output stream function (pStreamCtx is HDK_XML_OutputStream_BufferContext*)
 * Initialize pStreamCtx.pBuf with zero or malloc; always call free on pStreamCtx.pBuf.
 */
HDK_XML_EXPORT int HDK_XML_OutputStream_GrowBuffer(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* Null output stream function (pStreamCtx is not used) */
HDK_XML_EXPORT int HDK_XML_OutputStream_Null(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);


/*
 * Encoding output streams
 */

/* Output stream wrapping stream context */
typedef struct _HDK_XML_OutputStream_Encode_Context
{
    HDK_XML_OutputStreamFn pfnStream;  /* Wrapped stream */
    void* pStreamCtx;                  /* Wrapped stream context */
} HDK_XML_OutputStream_Encode_Context;

/* XML output stream encoding context */
typedef HDK_XML_OutputStream_Encode_Context HDK_XML_OutputStream_EncodeXML_Context;

/* XML string encoding wrapper */
HDK_XML_EXPORT int HDK_XML_OutputStream_EncodeXML(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* CSV output stream encoding context */
typedef HDK_XML_OutputStream_Encode_Context HDK_XML_OutputStream_EncodeCSV_Context;

/* CSV string encoding wrapper */
HDK_XML_EXPORT int HDK_XML_OutputStream_EncodeCSV(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* CSV output stream decoding context */
typedef HDK_XML_OutputStream_Encode_Context HDK_XML_OutputStream_DecodeCSV_Context;

/* CSV string decoding wrapper */
HDK_XML_EXPORT int HDK_XML_OutputStream_DecodeCSV(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);

/* Base64 output stream encoding context */
typedef struct _HDK_XML_OutputStream_EncodeBase64_Context
{
    HDK_XML_OutputStreamFn pfnStream;  /* Wrapped stream */
    void* pStreamCtx;                  /* Wrapped stream context */
    int state;
    int prev;
} HDK_XML_OutputStream_EncodeBase64_Context;

/* Base64 encoding wrapper */
HDK_XML_EXPORT int HDK_XML_OutputStream_EncodeBase64(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);
HDK_XML_EXPORT int HDK_XML_OutputStream_EncodeBase64Done(unsigned int* pcbStream, HDK_XML_OutputStream_EncodeBase64_Context* pStreamCtx);

/* Base64 output stream decoding context */
typedef HDK_XML_OutputStream_EncodeBase64_Context HDK_XML_OutputStream_DecodeBase64_Context;

/* Base64 decoding wrapper */
HDK_XML_EXPORT int HDK_XML_OutputStream_DecodeBase64(unsigned int* pcbStream, void* pStreamCtx, const char* pBuf, unsigned int cbBuf);
HDK_XML_EXPORT int HDK_XML_OutputStream_DecodeBase64Done(HDK_XML_OutputStream_DecodeBase64_Context* pStreamCtx);


/*
 * Input streams
 */

/* Input stream function type - returns non-zero on success, zero otherwise.  If cbRead != cbBuf then done. */
typedef int (*HDK_XML_InputStreamFn)(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf);

/* C I/O FILE* input stream function (pStreamCtx is a FILE*) */
HDK_XML_EXPORT int HDK_XML_InputStream_File(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf);

/* Buffer input stream function context */
typedef struct _HDK_XML_InputStream_BufferContext
{
    const char* pBuf;
    unsigned int cbBuf;                 /* Buffer size */
    unsigned int ixCur;                 /* Initialize to 0 */
} HDK_XML_InputStream_BufferContext;

/* Buffer input stream function (pStreamCtx is HDK_XML_InputStream_BufferContext*) */
HDK_XML_EXPORT int HDK_XML_InputStream_Buffer(unsigned int* pcbStream, void* pStreamCtx, char* pBuf, unsigned int cbBuf);


/*
 * XML schema
 */

/* Namespaces type definition */
typedef const char* HDK_XML_Namespace;

/* Element node struct */
typedef struct _HDK_XML_ElementNode
{
    unsigned char ixNamespace;
    const char* pszElement;
} HDK_XML_ElementNode;

/* Schema tree node properties */
typedef enum _HDK_XML_SchemaNodeProperties
{
    HDK_XML_SchemaNodeProperty_Optional = 0x01,
    HDK_XML_SchemaNodeProperty_Unbounded = 0x02,
    HDK_XML_SchemaNodeProperty_AnyElement = 0x04,
    HDK_XML_SchemaNodeProperty_ErrorOutput = 0x08,
    HDK_XML_SchemaNodeProperty_CSV = 0x10
} HDK_XML_SchemaNodeProperties;

/* Schema tree node struct */
typedef struct _HDK_XML_SchemaNode
{
    unsigned int ixParent;
    HDK_XML_Element element;
    HDK_XML_Type type;
    char prop;
} HDK_XML_SchemaNode;

/* Enumeration type definition */
typedef const char* HDK_XML_EnumValue;
typedef const HDK_XML_EnumValue* HDK_XML_EnumType;

/* XML schema struct */
typedef struct _HDK_XML_Schema
{
    const HDK_XML_Namespace* ppszNamespaces;
    const HDK_XML_ElementNode* pElements;
    const HDK_XML_SchemaNode* pSchemaNodes;
    const HDK_XML_EnumType* pEnumTypes;
} HDK_XML_Schema;

/* Schema list terminators */
#define HDK_XML_Schema_NamespacesEnd (HDK_XML_Namespace)0
#define HDK_XML_Schema_ElementsEnd { 0, 0 }
#define HDK_XML_Schema_SchemaNodesEnd { 0, HDK_XML_BuiltinElement_Unknown, 0, 0 }
#define HDK_XML_Schema_EnumTypeValuesEnd (HDK_XML_EnumValue)0

/* Get an element's namespace string */
#define HDK_XML_Schema_ElementNamespace(pSchema, element) \
    ((pSchema)->ppszNamespaces[(pSchema)->pElements[element].ixNamespace])

/* Get an element's name string */
#define HDK_XML_Schema_ElementName(pSchema, element) \
    ((pSchema)->pElements[element].pszElement)

/* Get an enumeration value string from an enumeration value */
#define HDK_XML_Schema_EnumValueString(pSchema, enumType, enumValue) \
    ((pSchema)->pEnumTypes[-(enumType) - 1][enumValue])


/*
 * XML parsing
 */

/* XML parser error codes */
typedef enum _HDK_XML_ParseError
{
    HDK_XML_ParseError_OK = 0,
    HDK_XML_ParseError_OutOfMemory,
    HDK_XML_ParseError_XMLInvalid,
    HDK_XML_ParseError_XMLUnexpectedElement,
    HDK_XML_ParseError_XMLInvalidValue,
    HDK_XML_ParseError_IOError,
    HDK_XML_ParseError_BadContentLength
} HDK_XML_ParseError;

/* XML parser options */
typedef enum _HDK_XML_ParseOptions
{
    HDK_XML_ParseOption_NoXML = 0x01,   /* De-serialize top-level, non-struct members as text */
    HDK_XML_ParseOption_Member = 0x02,  /* De-serialize to child member */
    HDK_XML_ParseOption_CSV = 0x04      /* De-serialize members as comma-seperated values */
} HDK_XML_ParseOptions;

/* XML parse functions */
typedef struct _HDK_XML_ParseContext* HDK_XML_ParseContextPtr;
HDK_XML_EXPORT HDK_XML_ParseContextPtr HDK_XML_Parse_New(const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct);
HDK_XML_EXPORT HDK_XML_ParseContextPtr HDK_XML_Parse_NewEx(const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                                           int options, unsigned int ixSchemaNode,
                                                           unsigned int cbMaxAlloc /* 0 for no maximum */);
HDK_XML_EXPORT HDK_XML_ParseError HDK_XML_Parse_Data(HDK_XML_ParseContextPtr pParseCtx, const char* pBuf, unsigned int cbBuf);
HDK_XML_EXPORT HDK_XML_ParseError HDK_XML_Parse_Free(HDK_XML_ParseContextPtr pParseCtx);
HDK_XML_EXPORT HDK_XML_ParseError HDK_XML_Parse_GetError(HDK_XML_ParseContextPtr pParseCtx);

/* XML parse from input stream */
HDK_XML_EXPORT HDK_XML_ParseError HDK_XML_Parse(HDK_XML_InputStreamFn pfnStream, void* pStreamCtx,
                                                const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                                unsigned int cbContentLength /* 0 if unknown */);
HDK_XML_EXPORT HDK_XML_ParseError HDK_XML_ParseEx(HDK_XML_InputStreamFn pfnStream, void* pStreamCtx,
                                                  const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct,
                                                  unsigned int cbContentLength /* 0 if unknown */,
                                                  int options, unsigned int ixSchemaNode,
                                                  unsigned int cbMaxAlloc /* 0 for no maximum */);

/* Type de-serialization */
HDK_XML_EXPORT int HDK_XML_Parse_Blob(char* pData, unsigned int* pcbData, const char* psz);
HDK_XML_EXPORT int HDK_XML_Parse_Bool(int* pfValue, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_DateTime(time_t* ptime, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_Int(HDK_XML_Int* pInt, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_Long(HDK_XML_Long* pLong, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_IPAddress(HDK_XML_IPAddress* pIPAddress, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_MACAddress(HDK_XML_MACAddress* pMACAddress, const char* pszValue);
HDK_XML_EXPORT int HDK_XML_Parse_UUID(HDK_XML_UUID* pUUID, const char* pszValue);


/*
 * XML serialization
 */

/* XML serialization options */
typedef enum _HDK_XML_SerializeOptions
{
    HDK_XML_SerializeOption_ErrorOutput = 0x01, /* Only output schema error nodes */
    HDK_XML_SerializeOption_NoNewlines = 0x02,  /* Don't output newlines */
    HDK_XML_SerializeOption_NoXML = 0x04,       /* Don't output XML header */
    HDK_XML_SerializeOption_CSV = 0x08          /* Serailize members as comma-seperated values */
} HDK_XML_SerializeOptions;

/* Structure XML serialization */
HDK_XML_EXPORT int HDK_XML_Serialize(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                     const HDK_XML_Schema* pSchema, const HDK_XML_Struct* pStruct, int options);
HDK_XML_EXPORT int HDK_XML_SerializeEx(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                       const HDK_XML_Schema* pSchema, const HDK_XML_Member* pMember, int options,
                                       unsigned int ixSchemaNode);

/* Type serialization */
HDK_XML_EXPORT int HDK_XML_Serialize_Blob(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                          const char* pData, unsigned int cbData, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_Bool(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                          int fValue, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_DateTime(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                              time_t tValue, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_Int(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                         HDK_XML_Int iValue, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_Long(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                          HDK_XML_Long llValue, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_String(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                            const char* pszValue, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_IPAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                               const HDK_XML_IPAddress* pIPAddress, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_MACAddress(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                                const HDK_XML_MACAddress* pMACAddress, int fNullTerminate);
HDK_XML_EXPORT int HDK_XML_Serialize_UUID(unsigned int* pcbStream, HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx,
                                          const HDK_XML_UUID* pUUID, int fNullTerminate);


/*
 * Schema validation
 */

/* Schema validation options */
typedef enum _HDK_XML_ValidateOptions
{
    HDK_XML_ValidateOption_ErrorOutput = 0x01  /* Only output schema error nodes */
} HDK_XML_ValidateOptions;

/* Validate a struct to a schema */
HDK_XML_EXPORT int HDK_XML_Validate(const HDK_XML_Schema* pSchema, const HDK_XML_Struct* pStruct, int options);
HDK_XML_EXPORT int HDK_XML_ValidateEx(const HDK_XML_Schema* pSchema, const HDK_XML_Member* pMember, int options,
                                      unsigned int ixSchemaNode);


/*
 * Logging
 */

HDK_XML_EXPORT void HDK_XML_RegisterLoggingCallback(HDK_LOG_CallbackFn pfnCallback, void* pCtx);

#endif /* __HDK_XML_H__ */
