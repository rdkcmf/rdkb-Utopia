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

#include "hdk_srv.h"

#include <stdlib.h>
#include <string.h>


#ifndef HDK_SRV_NO_SIMULATOR

/* Simulator schema namespaces*/
static HDK_XML_Namespace HDK_SRV_Simulator_Namespaces[] =
{
    /* 0 */ "http://cisco.com/",
    HDK_XML_Schema_NamespacesEnd
};

/* Simulator schema elements enum */
typedef enum _HDK_SRV_Simulator_ElementEnum
{
    HDK_SRV_Simulator_Element_Name = 0,
    HDK_SRV_Simulator_Element_Namespace = 1,
    HDK_SRV_Simulator_Element_Simulator = 2,
    HDK_SRV_Simulator_Element_State = 3,
    HDK_SRV_Simulator_Element_Value = 4
} HDK_SRV_Simulator_ElementEnum;

/* Simulator schema elements */
HDK_XML_ElementNode HDK_SRV_Simulator_Elements[] =
{
    /* HDK_SRV_Simulator_Element_Name */ { 0, "Name" },
    /* HDK_SRV_Simulator_Element_Namespace */ { 0, "Namespace" },
    /* HDK_SRV_Simulator_Element_Simulator */ { 0, "Simulator" },
    /* HDK_SRV_Simulator_Element_State */ { 0, "State" },
    /* HDK_SRV_Simulator_Element_Value */ { 0, "Value" },
    HDK_XML_Schema_ElementsEnd
};

/* Simulator schema nodes */
HDK_XML_SchemaNode HDK_SRV_Simulator_SchemaNodes[] =
{
        { /* 0 */ 0, HDK_SRV_Simulator_Element_Simulator, HDK_XML_BuiltinType_Struct, 0 },
        { /* 1 */ 0, HDK_SRV_Simulator_Element_State, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
        { /* 2 */ 1, HDK_SRV_Simulator_Element_Namespace, HDK_XML_BuiltinType_String, 0 },
        { /* 3 */ 1, HDK_SRV_Simulator_Element_Name, HDK_XML_BuiltinType_String, 0 },
        { /* 4 */ 1, HDK_SRV_Simulator_Element_Value, HDK_XML_BuiltinType_String, 0 },
        HDK_XML_Schema_SchemaNodesEnd
};

/* Simulator schema */
static const HDK_XML_Schema HDK_SRV_Simulator_Schema =
{
    HDK_SRV_Simulator_Namespaces,
    HDK_SRV_Simulator_Elements,
    HDK_SRV_Simulator_SchemaNodes,
    0
};


/* Helper function to add a simulator node */
static int HDK_SRV_Simulator_AddNode(HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx,
                                     const char* pszNamespace, const char* pszName, const char* pszValue)
{
    int fResult = 0;

    /* Create a new node */
    HDK_SRV_SimulatorNode* pNodeNew = (HDK_SRV_SimulatorNode*)malloc(sizeof(HDK_SRV_SimulatorNode));
    const char* pszNamespaceCopy = HDK_SRV_ADISerializeCopy(pszNamespace);
    const char* pszNameCopy = HDK_SRV_ADISerializeCopy(pszName);
    const char* pszValueCopy = HDK_SRV_ADISerializeCopy(pszValue);
    if (pNodeNew && pszNamespaceCopy && pszNameCopy && pszValueCopy)
    {
        fResult = 1;

        /* Set the new node members */
        pNodeNew->pszNamespace = pszNamespaceCopy;
        pNodeNew->pszName = pszNameCopy;
        pNodeNew->pszValue = pszValueCopy;
        pNodeNew->pNext = 0;

        /* Insert the new node */
        if (!pSimulatorCtx->pHead)
        {
            pSimulatorCtx->pHead = pNodeNew;
        }
        else
        {
            pSimulatorCtx->pTail->pNext = pNodeNew;
        }
        pSimulatorCtx->pTail = pNodeNew;

    }
    else
    {
        free((void*)pNodeNew);
        free((void*)pszNamespaceCopy);
        free((void*)pszNameCopy);
        free((void*)pszValueCopy);
    }

    return fResult;
}


/* Initialize simulator module context */
void HDK_SRV_Simulator_Init(HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx)
{
    memset(pSimulatorCtx, 0, sizeof(*pSimulatorCtx));
}


/* Free simulator module context */
void HDK_SRV_Simulator_Free(HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx)
{
    /* Free the simulator state nodes */
    HDK_SRV_SimulatorNode* pNode = pSimulatorCtx->pHead;
    while (pNode)
    {
        HDK_SRV_SimulatorNode* pNodeNext = pNode->pNext;

        HDK_SRV_ADISerializeFree(pNode->pszNamespace);
        HDK_SRV_ADISerializeFree(pNode->pszName);
        HDK_SRV_ADISerializeFree(pNode->pszValue);
        free((void*)pNode);

        pNode = pNodeNext;
    }

    /* Return context in valid state */
    HDK_SRV_Simulator_Init(pSimulatorCtx);
}


/* Read simulator state */
int HDK_SRV_Simulator_Read(HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx,
                           HDK_XML_InputStreamFn pfnStream, void* pStreamCtx)
{
    int fResult;

    /* Deserialize the simulator struct */
    HDK_XML_Struct sTemp;
    HDK_XML_Struct_Init(&sTemp);
    fResult = (HDK_XML_Parse(pfnStream, pStreamCtx, &HDK_SRV_Simulator_Schema, &sTemp, 0) == HDK_XML_ParseError_OK &&
               HDK_XML_Validate(&HDK_SRV_Simulator_Schema, &sTemp, 0));
    if (fResult)
    {
        /* Iterate the state structs */
        HDK_XML_Member* pState;
        for (pState = sTemp.pHead; pState; pState = pState->pNext)
        {
            /* Add the state node */
            if (!HDK_SRV_Simulator_AddNode(
                    pSimulatorCtx,
                    HDK_XML_Get_String((HDK_XML_Struct*)pState, HDK_SRV_Simulator_Element_Namespace),
                    HDK_XML_Get_String((HDK_XML_Struct*)pState, HDK_SRV_Simulator_Element_Name),
                    HDK_XML_Get_String((HDK_XML_Struct*)pState, HDK_SRV_Simulator_Element_Value)))
            {
                fResult = 0;
                break;
            }
        }
    }

    /* Free the simulator struct */
    HDK_XML_Struct_Free(&sTemp);

    return fResult;
}


/* Write simulator state */
int HDK_SRV_Simulator_Write(HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx,
                            HDK_XML_OutputStreamFn pfnStream, void* pStreamCtx)
{
    int fResult = 1;

    /* Create the simulator struct */
    HDK_SRV_SimulatorNode* pNode;
    HDK_XML_Struct sTemp;
    HDK_XML_Struct_Init(&sTemp);
    sTemp.node.element = HDK_SRV_Simulator_Element_Simulator;
    for (pNode = pSimulatorCtx->pHead; pNode; pNode = pNode->pNext)
    {
        HDK_XML_Struct* pState = HDK_XML_Append_Struct(&sTemp, HDK_SRV_Simulator_Element_State);
        if (!pState ||
            !HDK_XML_Set_String(pState, HDK_SRV_Simulator_Element_Namespace, pNode->pszNamespace) ||
            !HDK_XML_Set_String(pState, HDK_SRV_Simulator_Element_Name, pNode->pszName) ||
            !HDK_XML_Set_String(pState, HDK_SRV_Simulator_Element_Value, pNode->pszValue))
        {
            fResult = 0;
            break;
        }
    }

    /* Serialize the state struct */
    if (fResult)
    {
        unsigned int cbStream;
        fResult = HDK_XML_Serialize(&cbStream, pfnStream, pStreamCtx,
                                    &HDK_SRV_Simulator_Schema, &sTemp, 0);
    }

    /* Free the simulator struct */
    HDK_XML_Struct_Free(&sTemp);

    return fResult;
}


/* Simulator ADIGet implementation */
const char* HDK_SRV_Simulator_ADIGet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                                     const char* pszName)
{
    const char* pszValue = 0;
    HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx = (HDK_SRV_Simulator_ModuleCtx*)pMethodCtx->pModuleCtx->pModuleCtx;

    /* Search for the value node */
    const HDK_SRV_SimulatorNode* pNode;
    for (pNode = pSimulatorCtx->pHead; pNode; pNode = pNode->pNext)
    {
        if (*pszName == *pNode->pszName && strcmp(pszName, pNode->pszName) == 0 &&
            strcmp(pszNamespace, pNode->pszNamespace) == 0)
        {
            break;
        }
    }
    if (pNode)
    {
        pszValue = HDK_SRV_ADISerializeCopy(pNode->pszValue);
    }

    return pszValue;
}


/* Simulator ADISet implementation */
int HDK_SRV_Simulator_ADISet(HDK_MOD_MethodContext* pMethodCtx, const char* pszNamespace,
                             const char* pszName, const char* pszValue)
{
    int fResult = 0;
    HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx = (HDK_SRV_Simulator_ModuleCtx*)pMethodCtx->pModuleCtx->pModuleCtx;

    /* Search for the value node */
    HDK_SRV_SimulatorNode* pNode;
    for (pNode = pSimulatorCtx->pHead; pNode; pNode = pNode->pNext)
    {
        if (*pszName == *pNode->pszName && strcmp(pszName, pNode->pszName) == 0 &&
            strcmp(pszNamespace, pNode->pszNamespace) == 0)
        {
            break;
        }
    }
    if (pNode)
    {
        /* Unchanged value? */
        if (strcmp(pNode->pszValue, pszValue) == 0)
        {
            fResult = 1;
        }
        else
        {
            /* Replace the node value */
            const char* pszValueCopy = HDK_SRV_ADISerializeCopy(pszValue);
            if (pszValueCopy)
            {
                HDK_SRV_ADISerializeFree(pNode->pszValue);
                pNode->pszValue = pszValueCopy;
                fResult = 1;
            }
        }
    }
    else
    {
        /* Add a new node */
        fResult = HDK_SRV_Simulator_AddNode(pSimulatorCtx, pszNamespace, pszName, pszValue);
    }

    return fResult;
}

#endif /* #ifndef HDK_SRV_NO_SIMULATOR */
