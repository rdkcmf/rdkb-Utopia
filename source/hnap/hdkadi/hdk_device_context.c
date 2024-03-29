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

#include "hdk_device.h"

#include <utctx/utctx_api.h>


#ifdef HDK_LOGGING

/* Logging callback */
static void LoggingCallback(HDK_LOG_Level level, void* pCtx, const char* psz)
{
    static const char* s_ppszLogLevel[] =
    {
        "Error",
        "Warning",
        "Info",
        "Verbose"
    };

    fprintf((FILE*)pCtx, "[%s]: %s", s_ppszLogLevel[level], psz);
}

/* Initialize logging */
static void LoggingInit(HDK_SRV_DeviceContext* pDeviceCtx)
{
#ifdef HNAP_DEBUG
    pDeviceCtx->fhLogging = stdout;
#else
    pDeviceCtx->fhLogging = fopen("/dev/console", "w");
#endif
    if (pDeviceCtx->fhLogging)
    {
        /* Register the hdk srv & xml logging callback */
        HDK_SRV_RegisterLoggingCallback(LoggingCallback, pDeviceCtx->fhLogging);
        HDK_XML_RegisterLoggingCallback(LoggingCallback, pDeviceCtx->fhLogging);
    }
}

/* Free logging */
static void LoggingFree(HDK_SRV_DeviceContext* pDeviceCtx)
{
#ifndef HNAP_DEBUG
    if (pDeviceCtx->fhLogging)
    {
        /* Close the logging handle */
        fclose(pDeviceCtx->fhLogging);
    }
#else
    (void)pDeviceCtx;
#endif
}

#endif /* HDK_LOGGING */

/* Initialize device context */
void HDK_SRV_Device_Init(HDK_SRV_DeviceContext* pDeviceCtx)
{
    if (pDeviceCtx)
    {
#ifdef HNAP_DEBUG
        /* Zero out the debug request handle */
        pDeviceCtx->fhRequest = 0;
#endif

#ifdef HDK_LOGGING
        /* Initialize logging */
        LoggingInit(pDeviceCtx);
#endif

        /* Now initialize the utopia context */
        Utopia_Init(&pDeviceCtx->utopiaCtx);
    }
}

#ifdef HDK_CGI_FCGI
#include <fcgi_stdio.h>
#endif

/* Free device context */
void HDK_SRV_Device_Free(HDK_SRV_DeviceContext* pDeviceCtx, int fCommit)
{
    /* If the LAN/WLAN or device is rebooting, we need to flush before we free the context */
    if ((pDeviceCtx->utopiaCtx.bfEvents & Utopia_Event_LAN_Restart) == Utopia_Event_LAN_Restart ||
        (pDeviceCtx->utopiaCtx.bfEvents & Utopia_Event_WLAN_Restart) == Utopia_Event_WLAN_Restart ||
        (pDeviceCtx->utopiaCtx.bfEvents & Utopia_Event_Reboot) == Utopia_Event_Reboot)
    {
        /* Flush out the response */
        fflush(stdout);
    }

#ifdef HDK_LOGGING
    /* Free logging */
    LoggingFree(pDeviceCtx);
#endif

    if (pDeviceCtx)
    {
        /* Free the utopia context */
        Utopia_Free(&pDeviceCtx->utopiaCtx, fCommit);
    }
}
