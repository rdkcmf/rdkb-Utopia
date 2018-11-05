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

#include "hdk_cgi_context.h"

#include "hdk_srv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* For opendir/_findfirst */
#ifdef _MSC_VER
#  include <io.h>
#else
#  include <dirent.h>
#endif


/* Simulator state file */
#ifndef HDK_CGI_SIMULATOR_STATE_FILE
#define HDK_CGI_SIMULATOR_STATE_FILE "simulator.ds"
#endif


/* Server device context module node */
typedef struct _HDK_CGI_ServerContext_Module
{
    HDK_SRV_MODULE_HANDLE hModule;
    HDK_SRV_ModuleContext moduleCtx;
    struct _HDK_CGI_ServerContext_Module* pNext;
} HDK_CGI_ServerContext_Module;


/* Server (and module) device context */
typedef struct _HDK_CGI_ServerContext
{
    HDK_SRV_Simulator_ModuleCtx simulatorCtx;
    HDK_CGI_ServerContext_Module* pModulesHead;
} HDK_CGI_ServerContext;


/* Initialize the server device context.  Returns non-zero for success, zero otherwise. */
void* HDK_CGI_ServerContext_Init()
{
    /* Malloc an simulator context */
    HDK_CGI_ServerContext* pServerCtx = (HDK_CGI_ServerContext*)malloc(sizeof(HDK_CGI_ServerContext));
    if (pServerCtx)
    {
        FILE* pfh;

        /* Clear the server context */
        memset(pServerCtx, 0, sizeof(*pServerCtx));

        /* Initialize the simulator context */
        HDK_SRV_Simulator_Init(&pServerCtx->simulatorCtx);

        /* Read the simulator state file */
        pfh = fopen(HDK_CGI_SIMULATOR_STATE_FILE, "rb");
        if (!pfh || !HDK_SRV_Simulator_Read(&pServerCtx->simulatorCtx, HDK_XML_InputStream_File, pfh))
        {
            /* Free the simulator context */
            HDK_SRV_Simulator_Free(&pServerCtx->simulatorCtx);
            free(pServerCtx);
            pServerCtx = 0;
        }
        if (pfh)
        {
            fclose(pfh);
        }
    }

    return pServerCtx;
}


/* Free the server device context */
void HDK_CGI_ServerContext_Free(void* pServerCtx, int fCommit)
{
    HDK_CGI_ServerContext_Module* pModule;

    /* Commit simulator state changes? */
    if (fCommit)
    {
        /* Open the simulator state file */
        FILE* pfh = fopen(HDK_CGI_SIMULATOR_STATE_FILE, "wb");
        if (pfh)
        {
            /* Write the simulator context */
            HDK_SRV_Simulator_Write(&((HDK_CGI_ServerContext*)pServerCtx)->simulatorCtx, HDK_XML_OutputStream_File, pfh);

            /* Close the simulator state file */
            fclose(pfh);
        }
    }

    /* Free the simulator context */
    HDK_SRV_Simulator_Free(&((HDK_CGI_ServerContext*)pServerCtx)->simulatorCtx);

    /* Unload server modules */
    for (pModule = ((HDK_CGI_ServerContext*)pServerCtx)->pModulesHead; pModule; )
    {
        HDK_CGI_ServerContext_Module* pNext = pModule->pNext;

        /* Close the module */
        HDK_SRV_CloseModule(pModule->hModule);

        /* Free the module node */
        free(pModule);
        pModule = pNext;
    }

    /* Free the server context */
    free(pServerCtx);
}


/* Initialize the server module list */
HDK_SRV_ModuleContext** HDK_CGI_ServerModules_Init(void* pServerCtx)
{
    HDK_SRV_ModuleContext** ppModuleCtxs;
    unsigned int nModules = 0;

    /* Load server modules */
#ifdef _MSC_VER
    struct _finddata_t dirEntry;
    intptr_t hDir = _findfirst("modules\\*.*", &dirEntry);
    if (hDir != -1)
#else
    DIR* pdir = opendir("modules");
    if (pdir)
#endif
    {
        /* Load the modules */
#ifdef _MSC_VER
        do
#else
        struct dirent dirEntry;
        struct dirent* pDirEntry;
        while (!readdir_r(pdir, &dirEntry, &pDirEntry) && pDirEntry)
#endif
        {
#ifdef _MSC_VER
            const char pszModules[] = "modules\\";
            const char* pszName = dirEntry.name;
#else
            const char pszModules[] = "modules/";
            const char* pszName = dirEntry.d_name;
#endif
            char szModule[256];
            HDK_SRV_MODULE_HANDLE hModule;
            HDK_MOD_ModuleFn pfnModule;

            /* Special file? */
            if (*pszName == '.')
            {
                continue;
            }

            /* Module path too large for buffer? */
            if (sizeof(pszModules) + strlen(pszName) > sizeof(szModule))
            {
                continue;
            }

            /* Build the module file path */
            strcpy(szModule, pszModules);
            strcat(szModule, pszName);

            /* Load server module */
            hModule = HDK_SRV_OpenModule(&pfnModule, szModule);
            if (hModule)
            {
                /* Allocate the server module node */
                HDK_CGI_ServerContext_Module* pNew = (HDK_CGI_ServerContext_Module*)malloc(sizeof(HDK_CGI_ServerContext_Module));
                if (pNew)
                {
                    nModules++;
                    pNew->hModule = hModule;

                    /* Set the server module context */
                    pNew->moduleCtx.pModule = pfnModule();
                    pNew->moduleCtx.pModuleCtx = pServerCtx;
                    pNew->moduleCtx.pfnADIGet = HDK_SRV_Simulator_ADIGet;
                    pNew->moduleCtx.pfnADISet = HDK_SRV_Simulator_ADISet;

                    /* Add the node to the modules list */
                    pNew->pNext = ((HDK_CGI_ServerContext*)pServerCtx)->pModulesHead;
                    ((HDK_CGI_ServerContext*)pServerCtx)->pModulesHead = pNew;
                }
                else
                {
                    HDK_SRV_CloseModule(hModule);
                }
            }
        }
#ifdef _MSC_VER
        while (_findnext(hDir, &dirEntry) == 0);
#endif

        /* Close the modules directory */
#ifdef _MSC_VER
        _findclose(hDir);
#else
        closedir(pdir);
#endif
    }

    /* Allocate the module context list */
    ppModuleCtxs = (HDK_SRV_ModuleContext**)malloc((nModules + 1) * sizeof(*ppModuleCtxs));
    if (ppModuleCtxs)
    {
        HDK_SRV_ModuleContext** ppModuleCtx = ppModuleCtxs;
        HDK_CGI_ServerContext_Module* pModule;
        for (pModule = ((HDK_CGI_ServerContext*)pServerCtx)->pModulesHead; pModule; pModule = pModule->pNext)
        {
            *ppModuleCtx++ = &pModule->moduleCtx;
        }
        *ppModuleCtx = 0;
    }

    return ppModuleCtxs;
}


/* Free the server module list */
void HDK_CGI_ServerModules_Free(HDK_SRV_ModuleContext** ppModuleContexts)
{
    free(ppModuleContexts);
}


/* Helper function to get simulator values */
static const char* GetSimulatorValue(const HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx,
                                     const char* pszNamespace, const char* pszName)
{
    const HDK_SRV_SimulatorNode* pNode;
    for (pNode = pSimulatorCtx->pHead; pNode; pNode = pNode->pNext)
    {
        if (*pNode->pszName == *pszName && strcmp(pNode->pszName, pszName) == 0 &&
            strcmp(pNode->pszNamespace, pszNamespace) == 0)
        {
            return pNode->pszValue;
        }
    }
    return 0;
}


/* Server authentication callback function */
int HDK_CGI_Authenticate(void* pServerCtx, const char* pszUser, const char* pszPassword)
{
    /* Get the device user and password */
    HDK_SRV_Simulator_ModuleCtx* pSimulatorCtx = &((HDK_CGI_ServerContext*)pServerCtx)->simulatorCtx;
    const char* pszUserSim = GetSimulatorValue(pSimulatorCtx, "http://purenetworks.com/HNAP1/", "Username");
    const char* pszPasswordSim = GetSimulatorValue(pSimulatorCtx, "http://purenetworks.com/HNAP1/", "AdminPassword");
    return pszUserSim && pszPasswordSim &&
        strcmp(pszUserSim, pszUser) == 0 &&
        strcmp(pszPasswordSim, pszPassword) == 0;
}


/* Server HNAP result callback function */
void HDK_CGI_HNAPResult(void* pServerCtx, HDK_XML_Struct* pOutput,
                      HDK_XML_Element resultElement, HDK_XML_Type resultType,
                      int resultOK, int resultReboot)
{
    /* Unused parameters */
    (void)pServerCtx;
    (void)pOutput;
    (void)resultElement;
    (void)resultType;
    (void)resultOK;
    (void)resultReboot;
}
