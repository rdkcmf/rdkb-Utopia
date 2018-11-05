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

#include "unittest_util.h"

#include <stdio.h>
#include <string.h>

/* HDK_XML_InputStreamFn callback. */
static int ReadFromFileStream(unsigned int* pcbRead, void* pStreamCtx, char* pBuf, unsigned int cbBuf)
{
    *pcbRead = (unsigned int)fread(pBuf, sizeof(*pBuf), cbBuf, (FILE*)pStreamCtx);

    return !ferror((FILE*)pStreamCtx);
}

/* extern */
int DeserializeFileToStruct(const char* pszXmlFile, const HDK_XML_Schema* pSchema, HDK_XML_Struct* pStruct)
{
    int fSuccess = 0;
    FILE* fhXmlFile = fopen(pszXmlFile, "r");
    if (fhXmlFile)
    {
        fSuccess = HDK_XML_ParseError_OK == HDK_XML_Parse(ReadFromFileStream, fhXmlFile, pSchema, pStruct, 0);

        fclose(fhXmlFile);
    }

    return fSuccess;
}

/* extern */
int SerializeStructToFile(const HDK_XML_Struct* pStruct, const HDK_XML_Schema* pSchema, FILE* fh)
{
    unsigned int cbSerialized = 0;
    return HDK_XML_Serialize(&cbSerialized, HDK_XML_OutputStream_File, fh, pSchema, pStruct, 0);
}
