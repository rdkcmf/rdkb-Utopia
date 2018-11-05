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

#include "hdk_srv.h"

#include <stdlib.h>
#include <string.h>


/* Get ADI value from a namespace and name */
int HDK_SRV_ADIGetValue(HDK_SRV_ADIValue* pValue, const HDK_MOD_Module* pModule,
                        const char* pszNamespace, const char* pszName)
{
    int fResult = 0;

    /* Find the ADI schema node */
    const HDK_XML_Schema* pSchemaADI = pModule->pSchemaADI;
    const HDK_XML_SchemaNode* pSchemaNode;
    for (pSchemaNode = pSchemaADI->pSchemaNodes + 1;
         pSchemaNode->ixParent == 0 && pSchemaNode->element != HDK_XML_BuiltinElement_Unknown;
         ++pSchemaNode)
    {
        const char* pszNodeNamespace = HDK_XML_Schema_ElementNamespace(pSchemaADI, pSchemaNode->element);
        const char* pszNodeName = HDK_XML_Schema_ElementName(pSchemaADI, pSchemaNode->element);
        if (*pszName == *pszNodeName && strcmp(pszName, pszNodeName) == 0 &&
            strcmp(pszNamespace, pszNodeNamespace) == 0)
        {
            fResult = 1;
            *pValue = (HDK_SRV_ADIValue)(pSchemaNode - pSchemaADI->pSchemaNodes);
            break;
        }
    }

    return fResult;
}


/* Deserialize an ADI value */
HDK_XML_Member* HDK_SRV_ADIDeserialize(const HDK_MOD_Module* pModule, HDK_SRV_ADIValue value,
                                       HDK_XML_Struct* pStruct, HDK_XML_Element element,
                                       const char* pszValue)
{
    HDK_XML_Member* pMember = 0;

    /* Get the module's ADI schema */
    const HDK_XML_Schema* pSchemaADI = pModule->pSchemaADI;
    if (pSchemaADI)
    {
        HDK_XML_Struct sTemp;
        HDK_XML_InputStream_BufferContext bufCtx;
        HDK_XML_ParseError parseError;

        /* Initialize the temporary struct */
        HDK_XML_Struct_Init(&sTemp);

        /* Deserialize the ADI value */
        memset(&bufCtx, 0, sizeof(bufCtx));
        bufCtx.pBuf = pszValue;
        bufCtx.cbBuf = (unsigned int)strlen(pszValue);
        parseError = HDK_XML_ParseEx(HDK_XML_InputStream_Buffer, &bufCtx, pSchemaADI, &sTemp, 0,
                                     HDK_XML_ParseOption_NoXML, value, 0);
        if (parseError == HDK_XML_ParseError_OK)
        {
            /* Validate the deserialized ADI value */
            HDK_XML_Member* pMemberTemp = sTemp.pHead;
            if (HDK_XML_ValidateEx(pSchemaADI, pMemberTemp, 0, value))
            {
                /* Copy the ADI value member */
                pMember = HDK_XML_Set_Member(pStruct, element, pMemberTemp);
            }
        }

        /* Free the temporary struct */
        HDK_XML_Struct_Free(&sTemp);
    }

    return pMember;
}


/* Serialize an ADI value - free with HDK_SRV_ADISerializeFree */
const char* HDK_SRV_ADISerialize(const HDK_MOD_Module* pModule, HDK_SRV_ADIValue value,
                                 HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    char* pszValue = 0;

    /* Get the module's ADI schema */
    const HDK_XML_Schema* pSchemaADI = pModule->pSchemaADI;
    if (pSchemaADI)
    {
        /* Get the ADI value */
        const HDK_XML_SchemaNode* pSchemaNode = HDK_MOD_ADISchemaNode(pModule, value);
        HDK_XML_Member* pMember = HDK_XML_Get_Member(pStruct, element, pSchemaNode->type);
        if (pMember)
        {
            HDK_XML_Struct sTemp;
            HDK_XML_Member* pMemberTemp;

            /* Initialize the temporary struct */
            HDK_XML_Struct_Init(&sTemp);

            /* Copy the value to the schema element, if necessary */
            if (pMember->element != pSchemaNode->element)
            {
                pMemberTemp = HDK_XML_Set_Member(&sTemp, pSchemaNode->element, pMember);
            }
            else
            {
                pMemberTemp = pMember;
            }
            if (pMemberTemp)
            {
                /* Validate the ADI value */
                if (HDK_XML_ValidateEx(pSchemaADI, pMemberTemp, 0, value))
                {
                    /* Determine the size of the serialized value */
                    unsigned int cbValue;
                    if (HDK_XML_SerializeEx(&cbValue, HDK_XML_OutputStream_Null, 0, pSchemaADI, pMemberTemp,
                                            HDK_XML_SerializeOption_NoNewlines | HDK_XML_SerializeOption_NoXML,
                                            value))
                    {
                        /* Allocate the value buffer */
                        pszValue = (char*)malloc(cbValue + 1);
                        if (pszValue)
                        {
                            /* Serialize the value */
                            HDK_XML_OutputStream_BufferContext bufCtx;
                            memset(&bufCtx, 0, sizeof(bufCtx));
                            bufCtx.pBuf = pszValue;
                            bufCtx.cbBuf = cbValue;
                            if (HDK_XML_SerializeEx(&cbValue, HDK_XML_OutputStream_Buffer, &bufCtx, pSchemaADI, pMemberTemp,
                                                    HDK_XML_SerializeOption_NoNewlines | HDK_XML_SerializeOption_NoXML,
                                                    value))
                            {
                                /* Null-terminate the value */
                                pszValue[cbValue] = 0;
                            }
                            else
                            {
                                /* Free the value */
                                free(pszValue);
                                pszValue = 0;
                            }
                        }
                    }
                }
            }

            /* Free the temporary struct */
            HDK_XML_Struct_Free(&sTemp);
        }
    }

    return pszValue;
}


/* Copy an ADI value string constant - free with HDK_SRV_ADISerializeFree */
const char* HDK_SRV_ADISerializeCopy(const char* pszValue)
{
    unsigned int cbValue = (unsigned int)strlen(pszValue);
    char* pszValueCopy = (char*)malloc(cbValue + 1);
    if (pszValueCopy)
    {
        strcpy(pszValueCopy, pszValue);
    }
    return pszValueCopy;
}


/* Free a serialized ADI value */
void HDK_SRV_ADISerializeFree(const char* pszValue)
{
    free((void*)pszValue);
}


/* ADI value get */
HDK_XML_Member* HDK_SRV_ADIGet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                               HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    HDK_XML_Member* pMember = 0;

    /* Get the module's ADI set function */
    HDK_SRV_ADIGetFn pfnADIGet = pMethodCtx->pModuleCtx->pfnADIGet;
    if (pfnADIGet)
    {
        /* Get the ADI value */
        const HDK_MOD_Module* pModule = HDK_SRV_ADIModule(pMethodCtx);
        const char* pszNamespace = HDK_MOD_ADIValueNamespace(pModule, value);
        const char* pszName = HDK_MOD_ADIValueName(pModule, value);
        const char* pszValue = pfnADIGet(pMethodCtx, pszNamespace, pszName);
        if (pszValue)
        {
            /* Deserialize the ADI value */
            pMember = HDK_SRV_ADIDeserialize(pModule, value, pStruct, element, pszValue);

            /* Free the value */
            HDK_SRV_ADISerializeFree(pszValue);
        }
    }

    return pMember;
}


/* ADI value set */
int HDK_SRV_ADISet(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value,
                   HDK_XML_Struct* pStruct, HDK_XML_Element element)
{
    int fResult = 0;

    /* Get the module's ADI set function */
    HDK_SRV_ADISetFn pfnADISet = pMethodCtx->pModuleCtx->pfnADISet;
    if (pfnADISet)
    {
        /* Serialize the ADI value */
        const HDK_MOD_Module* pModule = HDK_SRV_ADIModule(pMethodCtx);
        const char* pszValue = HDK_SRV_ADISerialize(pModule, value, pStruct, element);
        if (pszValue)
        {
            /* Set the ADI value */
            const char* pszNamespace = HDK_MOD_ADIValueNamespace(pModule, value);
            const char* pszName = HDK_MOD_ADIValueName(pModule, value);
            fResult = pfnADISet(pMethodCtx, pszNamespace, pszName, pszValue);

            /* Free the serialized ADI value */
            HDK_SRV_ADISerializeFree(pszValue);
        }
    }

    return fResult;
}

/* ADI bool value get */
int HDK_SRV_ADIGetBool(HDK_MOD_MethodContext* pMethodCtx, HDK_SRV_ADIValue value)
{
    HDK_XML_Struct sTemp;
    int* piReturn;
    int iReturn;

    /* Initialize temprary struct */
    HDK_XML_Struct_Init(&sTemp);

    piReturn = HDK_XML_GetMember_Bool(HDK_SRV_ADIGet(pMethodCtx, value, &sTemp, HDK_XML_BuiltinElement_Unknown));

    iReturn = piReturn ? *piReturn : 0;

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);

    return iReturn;
}

/* ADI string indexed get */
HDK_XML_Member* HDK_SRV_ADIGetValue_ByString
(
    HDK_MOD_MethodContext* pMethodCtx,
    HDK_SRV_ADIValue adiValue,
    HDK_XML_Struct* pStruct,
    HDK_XML_Struct* pKeyStruct,
    HDK_XML_Element keyElement,
    HDK_XML_Element valueElement,
    HDK_XML_Type valueType
)
{
    char* pszKey;
    HDK_XML_Member* pmReturn = 0;
    HDK_XML_Struct* psStruct;

    if ((pszKey = HDK_XML_Get_String(pKeyStruct, keyElement)) != 0)
    {
        HDK_XML_Struct sTemp;

        /* Initialize the temp struct */
        HDK_XML_Struct_Init(&sTemp);

        /* Get the ADI key/value array */
        psStruct = HDK_XML_GetMember_Struct(HDK_SRV_ADIGet(pMethodCtx, adiValue, &sTemp, HDK_XML_BuiltinElement_Unknown));
        if (psStruct != 0)
        {
            HDK_XML_Member* pmTemp = 0;

            /* Iterate over the array looking for a matching key */
            for (pmTemp = psStruct->pHead; pmTemp; pmTemp = pmTemp->pNext)
            {
                HDK_XML_Struct* psTemp = HDK_XML_GetMember_Struct(pmTemp);
                const char* pszTemp = psTemp ? HDK_XML_Get_String(psTemp, keyElement) : 0;
                if (pszTemp && strcmp(pszTemp, pszKey) == 0)
                {
                    /* Found it, copy the key/value */
                    if ((pmReturn = HDK_XML_Get_Member(psTemp, valueElement, valueType)) != 0)
                    {
                        pmReturn = HDK_XML_Set_Member(pStruct, valueElement, pmReturn);
                    }
                    break;
                }
            }
        }

        /* Free the temp struct */
        HDK_XML_Struct_Free(&sTemp);
    }

    return pmReturn;
}

/* ADI string indexed set */
int HDK_SRV_ADISetValue_ByString
(
    HDK_MOD_MethodContext* pMethodCtx,
    HDK_SRV_ADIValue adiValue,
    HDK_XML_Struct* pStruct,
    HDK_XML_Element infoElement,
    HDK_XML_Element keyElement,
    HDK_XML_Element valueElement,
    HDK_XML_Type valueType
)
{
    char* pszKey;
    HDK_XML_Member* pmReturn = 0;
    HDK_XML_Member* pValue = 0;
    HDK_XML_Struct* psStruct;
    int fSuccess = 0;

    /* Don't do anything if the key/value elements are not in the input struct */
    if ((pValue = HDK_XML_Get_Member(pStruct, valueElement, valueType)) != 0 &&
        (pszKey = HDK_XML_Get_String(pStruct, keyElement)) != 0)
    {
        HDK_XML_Struct sTemp;

        /* Initialize the temp struct */
        HDK_XML_Struct_Init(&sTemp);

        /* Get the ADI key/value array */
        psStruct = HDK_XML_GetMember_Struct(HDK_SRV_ADIGet(pMethodCtx, adiValue, &sTemp, valueElement));
        if (psStruct != 0)
        {
            HDK_XML_Member* pmTemp = 0;

            /* Iterate over the array looking for a matching key */
            for (pmTemp = psStruct->pHead; pmTemp; pmTemp = pmTemp->pNext)
            {
                HDK_XML_Struct* psTemp = HDK_XML_GetMember_Struct(pmTemp);
                const char* pszTemp = HDK_XML_Get_String(psTemp, keyElement);
                if (pszTemp && strcmp(pszTemp, pszKey) == 0)
                {
                    /* Found it, set the value */
                    pmReturn = HDK_XML_Set_Member(psTemp, valueElement, pValue);
                    break;
                }
            }
            if (pmTemp == 0)
            {
                /* If we didn't find a matching key, then append the key/value */
                if ((psStruct = HDK_XML_Append_Struct(psStruct, infoElement)) != 0 &&
                    HDK_XML_Set_String(psStruct, keyElement, pszKey))
                {
                    pmReturn = HDK_XML_Set_Member(psStruct, valueElement, pValue);
                }
            }
        }
        else
        {
            /* If no array exists at, create it and append the key/value */
            if ((psStruct = HDK_XML_Set_Struct(&sTemp, valueElement)) != 0 &&
                (psStruct = HDK_XML_Set_Struct(psStruct, infoElement)) != 0 &&
                HDK_XML_Set_String(psStruct, keyElement, pszKey))
            {
                pmReturn = HDK_XML_Set_Member(psStruct, valueElement, pValue);
            }
        }

        /* If we we're able to successfully set the value, then set it in the ADI */
        fSuccess = pmReturn && HDK_SRV_ADISet(pMethodCtx, adiValue, &sTemp, valueElement);

        /* Free the temp struct */
        HDK_XML_Struct_Free(&sTemp);
    }

    return fSuccess;
}
