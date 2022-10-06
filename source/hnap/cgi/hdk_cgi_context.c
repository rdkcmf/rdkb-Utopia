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

#include "hdk_device.h"
#include "hdk_srv.h"
#include "hnap12.h"
#include "hotspot.h"
#include <utctx/utctx_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* <crypt.h> Function prototype required to compile as ISO C */
extern char* crypt( const char* key, const char* setting );

/* Initialize the server device context.  Returns non-zero for success, zero otherwise. */
void* HDK_CGI_ServerContext_Init()
{
    /* Malloc a device context */
    HDK_SRV_DeviceContext* pDeviceCtx = (HDK_SRV_DeviceContext*)malloc(sizeof(HDK_SRV_DeviceContext));
    if (pDeviceCtx)
    {
        /* Initialize the device context */
        HDK_SRV_Device_Init(pDeviceCtx);
    }

    return pDeviceCtx;
}


/* Free the server device context */
void HDK_CGI_ServerContext_Free(void* pServerCtx, int fCommit)
{
    /* Free the device context */
    HDK_SRV_Device_Free(pServerCtx, fCommit);
    free(pServerCtx);
}


/* Initialize the server module list */
HDK_SRV_ModuleContext** HDK_CGI_ServerModules_Init(void* pServerCtx)
{
    /* Allocate the modules array */
    HDK_SRV_ModuleContext** ppModuleCtxs = (HDK_SRV_ModuleContext**)malloc(3 * sizeof(HDK_SRV_ModuleContext*));
    if (ppModuleCtxs)
    {
        /* HNAP12 module */
        ppModuleCtxs[0] = (HDK_SRV_ModuleContext*)malloc(sizeof(HDK_SRV_ModuleContext));
        if (ppModuleCtxs)
        {
            memset(ppModuleCtxs[0], 0, sizeof(*ppModuleCtxs[0]));
            ppModuleCtxs[0]->pModule = HNAP12_Module();
            ppModuleCtxs[0]->pModuleCtx = pServerCtx;
            ppModuleCtxs[0]->pfnADIGet = HDK_SRV_Device_ADIGet;
            ppModuleCtxs[0]->pfnADISet = HDK_SRV_Device_ADISet;
        }

        /* HOTSPOT module */
        ppModuleCtxs[1] = (HDK_SRV_ModuleContext*)malloc(sizeof(HDK_SRV_ModuleContext));
        if (ppModuleCtxs)
        {
            memset(ppModuleCtxs[1], 0, sizeof(*ppModuleCtxs[1]));
            ppModuleCtxs[1]->pModule = HOTSPOT_Module();
            ppModuleCtxs[1]->pModuleCtx = pServerCtx;
            ppModuleCtxs[1]->pfnADIGet = HDK_SRV_Device_ADIGet;
            ppModuleCtxs[1]->pfnADISet = HDK_SRV_Device_ADISet;
        }

        /* Module list terminator */
        ppModuleCtxs[2] = 0;
    }

    return ppModuleCtxs;
}


/* Free the server module list */
void HDK_CGI_ServerModules_Free(HDK_SRV_ModuleContext** ppModuleContexts)
{
    /* Free the modules */
    HDK_SRV_ModuleContext** ppModuleContext;
    for (ppModuleContext = ppModuleContexts; *ppModuleContext; ++ppModuleContext)
    {
        free(*ppModuleContext);
    }

    /* Free the module array */
    free(ppModuleContexts);
}


/* Server authentication callback function */
int HDK_CGI_Authenticate(void* pServerCtx, const char* pszUser, const char* pszPassword)
{
    char pszDevicePassword[UTOPIA_MAX_PASSWD_LENGTH + 1] = {'\0'};
    char pszDeviceUsername[UTOPIA_MAX_USERNAME_LENGTH + 1] = {'\0'};

    /* Get the username and password stored in the device */
    Utopia_Get(&((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx,
               UtopiaValue_HTTP_AdminPassword, pszDevicePassword, UTOPIA_MAX_PASSWD_LENGTH);
    Utopia_Get(&((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx,
               UtopiaValue_HTTP_AdminUser, pszDeviceUsername, UTOPIA_MAX_USERNAME_LENGTH);

    /* Decrypt and compare username and password */
    return strcmp(crypt(pszPassword, pszDevicePassword), pszDevicePassword) == 0 &&
        strcmp(pszUser, pszDeviceUsername) == 0;
}


/* Server HNAP result callback function */
void HDK_CGI_HNAPResult(void* pServerCtx, HDK_XML_Struct* pOutput,
                        HDK_XML_Element resultElement, HDK_XML_Type resultType,
                        int resultOK, int resultReboot)
{
    int* pResult;

    if ((pResult = HDK_XML_Get_Enum(pOutput, resultElement, resultType)) != 0)
    {
        if (*pResult == resultOK)
        {
            /* Return a 'REBOOT' result for reboot, and lan/wlan restart */
            if (((((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx.bfEvents & Utopia_Event_LAN_Restart) == Utopia_Event_LAN_Restart) ||
                ((((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx.bfEvents & Utopia_Event_WLAN_Restart) == Utopia_Event_WLAN_Restart) ||
                ((((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx.bfEvents & Utopia_Event_Reboot) == Utopia_Event_Reboot))
            {
                HDK_XML_Set_Enum(pOutput, resultElement, resultType, resultReboot);
            }
        }
        else
        {
            /* If there was an error, we need to clear the actions */
            ((HDK_SRV_DeviceContext*)pServerCtx)->utopiaCtx.bfEvents = Utopia_Event__NONE__;
        }
    }
}
