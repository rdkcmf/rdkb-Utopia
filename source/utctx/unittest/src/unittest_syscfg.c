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

#include "unittest_syscfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* unittest syscfg node */
typedef struct _SysCfg_Node
{
    char* pszKey;
    char* pszNamespace;
    char* pszValue;
    struct _SysCfg_Node* pNext;
} SysCfg_Node;

static SysCfg_Node* g_pHead;


/*
 * unittest_syscfg.h
 */

void SysCfg_Commit(void)
{
    FILE* fhState;

    if ((fhState = fopen(UTCTX_STATE_FILE, "w")) != 0)
    {
        SysCfg_Node* pNode = g_pHead;
        SysCfg_Node* pNext;

        while (pNode)
        {
            if (pNode->pszNamespace)
            {
                fprintf(fhState, "%s::%s=%s\n", pNode->pszNamespace,
                        pNode->pszKey, (pNode->pszValue ? pNode->pszValue : ""));
            }
            else
            {
                fprintf(fhState, "%s=%s\n", pNode->pszKey, (pNode->pszValue ? pNode->pszValue : ""));
            }

            /* Free node */
            pNext = pNode->pNext;
            if (pNode->pszValue)
            {
                free(pNode->pszValue);
            }
            free(pNode->pszKey);
            if (pNode->pszNamespace)
            {
                free(pNode->pszNamespace);
            }
            free(pNode);
            pNode = pNext;
        }
        fclose(fhState);
    }
}

int SysCfg_Init(void)
{
    char pszLine[2048] = {'\0'};
    char* pszEnd;
    char* pszKey;
    char* pszNamespace;
    char* pszValue;
    FILE* fhState;

    /* Zero out the syscfg head pointer */
    g_pHead = 0;

    if ((fhState = fopen(UTCTX_STATE_FILE, "r")) == 0)
    {
        return 0;
    }

    while (fgets(pszLine, sizeof(pszLine) - 1, fhState))
    {
        /* Find the delimeter '=' */
        if ((pszValue = strchr(pszLine, '=')) == 0)
        {
            return 0;
        }

        *pszValue = '\0';
        ++pszValue;

        /* Look for '::' indicating namespace */
        if ((pszKey = strstr(pszLine, "::")) != 0)
        {
            *pszKey = '\0';
            pszKey += 2;
            pszNamespace = pszLine;
        }
        else
        {
            pszKey = pszLine;
            pszNamespace = 0;
        }

        /* Trim the value */
        for (; strchr(" \t", *pszValue); pszValue++);
        for (pszEnd = pszValue + strlen(pszValue) - 1; pszEnd >= pszValue && strchr(" \t\n", *pszEnd); pszEnd--);

        pszEnd++;
        *pszEnd = '\0';

        if (SysCfg_Set(pszNamespace, pszKey, pszValue) == 0)
        {
            return 0;
        }
    }

    fclose(fhState);
    return 1;
}

int SysCfg_Get(char* pszNamespace, char* pszKey, char* pszValue, int cbBuf)
{
    SysCfg_Node* pNode = g_pHead;

    if (!pszKey || !pszValue)
    {
        return 0;
    }

    while(pNode)
    {
        if (pszNamespace)
        {
            if (pNode->pszNamespace && !strcmp(pNode->pszNamespace, pszNamespace) && !strcmp(pNode->pszKey, pszKey))
            {
                strncpy(pszValue, pNode->pszValue, cbBuf - 1);
                return 1;
            }
        }
        else if (!strcmp(pNode->pszKey, pszKey))
        {
            strncpy(pszValue, pNode->pszValue, cbBuf - 1);
            return 1;
        }

        pNode = pNode->pNext;
    }

    return 0;
}

int SysCfg_GetAll(char* pBuffer, int ccbBuf, int* pccbBuf)
{
    char* pPtr = pBuffer;
    SysCfg_Node* pNode = g_pHead;

    if (!pBuffer || !pccbBuf)
    {
        return 0;
    }

    /* Zero out the buffer and buf length pointer */
    memset(pBuffer, 0, ccbBuf);
    *pccbBuf = 0;

    while(pNode)
    {
        /* Length of the namespace string plus the '::' delimeter, or 0 if namespace is null */
        int iLength = (pNode->pszNamespace == 0 ? 0 : strlen(pNode->pszNamespace) + 2);
        /* Length of the key string plus the '=' delimeter */
        iLength += strlen(pNode->pszKey) + 1;
        /* Length of the value string plus the '\0' delimeter */
        iLength += (pNode->pszValue == 0 ? 0 : strlen(pNode->pszValue)) + 1;

        /* Fail if we don't have room */
        if ((*pccbBuf + iLength) > ccbBuf)
        {
            return 0;
        }

        if (pNode->pszNamespace)
        {
            sprintf(pPtr, "%s::%s=%s", pNode->pszNamespace, pNode->pszKey, pNode->pszValue);
        }
        else
        {
            sprintf(pPtr, "%s=%s", pNode->pszKey, pNode->pszValue);
        }

        *pccbBuf += iLength;
        pPtr += iLength;
        pNode = pNode->pNext;
    }

    return 1;
}

int SysCfg_Set(char* pszNamespace, char* pszKey, char* pszValue)
{
    SysCfg_Node* pNode = g_pHead;
    SysCfg_Node* pPrevious = g_pHead;

    if (!pszKey || !pszValue)
    {
        return 0;
    }

    /* See if the syscfg value exists */
    while(pNode)
    {
        if (pszNamespace)
        {
            if (pNode->pszNamespace && !strcmp(pNode->pszNamespace, pszNamespace) && !strcmp(pNode->pszKey, pszKey))
            {
                break;
            }
        }
        else if (!strcmp(pNode->pszKey, pszKey))
        {
            break;
        }
        pPrevious = pNode;
        pNode = pNode->pNext;
    }

    if (!pNode)
    {
        /* Allocate memory */
        if ((pNode = (SysCfg_Node*)malloc(sizeof(SysCfg_Node))) == 0)
        {
            return 0;
        }
        if ((pNode->pszKey = (char*)malloc(sizeof(char*) * (strlen(pszKey) + 1))) == 0)
        {
            free(pNode);
            return 0;
        }
        if ((pNode->pszValue = (char*)malloc(sizeof(char*) * (strlen(pszValue) + 1))) == 0)
        {
            free(pNode->pszKey);
            free(pNode);
            return 0;
        }
        if (pszNamespace &&
            (pNode->pszNamespace = (char*)malloc(sizeof(char*) * (strlen(pszNamespace) + 1))) == 0)
        {
            free(pNode->pszKey);
            free(pNode->pszValue);
            free(pNode);
            return 0;
        }

        /* Set the key */
        strcpy(pNode->pszKey, pszKey);

        /* Set the namespace */
        if (pszNamespace)
        {
            strcpy(pNode->pszNamespace, pszNamespace);
        }
        else
        {
            pNode->pszNamespace = 0;
        }
        pNode->pNext = 0;

        /* Add the node to the end of the linked list */
        if (!pPrevious)
        {
            g_pHead = pNode;
        }
        else
        {
            pPrevious->pNext = pNode;
        }
    }
    else
    {
        /* Free the old value */
        if (pNode->pszValue)
        {
            free(pNode->pszValue);
        }

        /* Allocate memory for the new value */
        if ((pNode->pszValue = (char*)malloc(sizeof(char*) * (strlen(pszValue) + 1))) == 0)
        {
            return 0;
        }
    }

    /* Set the value */
    strcpy(pNode->pszValue, pszValue);
    return 1;
}

int SysCfg_Unset(char* pszNamespace, char* pszKey)
{
    SysCfg_Node* pNode = g_pHead;
    SysCfg_Node* pPrevious = g_pHead;

    if (!pszKey)
    {
        return 0;
    }

    /* See if the syscfg value exists and return error if not */
    while(pNode)
    {
        if (pszNamespace)
        {
            if (pNode->pszNamespace && !strcmp(pNode->pszNamespace, pszNamespace) && !strcmp(pNode->pszKey, pszKey))
            {
                break;
            }
        }
        else if (!strcmp(pNode->pszKey, pszKey))
        {
            break;
        }
        pPrevious = pNode;
        pNode = pNode->pNext;
    }
    if (pNode == 0)
    {
        return 0;
    }

    /* Set the previous node's next to next node */
    if (pNode == g_pHead)
    {
        g_pHead = pNode->pNext;
    }
    else
    {
        pPrevious->pNext = pNode->pNext;
    }

    /* Free up this node */
    free(pNode->pszKey);
    if (pNode->pszNamespace)
    {
        free(pNode->pszNamespace);
    }
    if (pNode->pszValue)
    {
        free(pNode->pszValue);
    }
    free(pNode);

    return 1;
}
