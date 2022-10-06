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

#include <utctx/utctx.h>
#include <utctx/utctx_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void usage()
{
    printf("Usage: \n"
           "\tutctx_cmd get [ns1::]name1 [ns2::]name2 ... \n"
           "\tutctx_cmd set [ns1::]name1=value1 [ns2::]name2=value2 ...\n"
           );
}

static int s_UTCTX_BatchGet(UtopiaContext* pCtx, char** ppszArgs, int iArgCount)
{
    int i;

    for (i = 0; i < iArgCount; ++i)
    {
        char* pszName;
        char* pszNamespace = 0;
        char pszBuf[1024] = {'\0'};

        if ((pszName = strstr(ppszArgs[i], "::")) != 0)
        {
            *pszName = '\0';
            pszName += 2;
            pszNamespace = ppszArgs[i];
        }
        else
        {
            pszName = ppszArgs[i];
        }

        /* Attempt to get the value, ignoring failures */
        Utopia_RawGet(pCtx, pszNamespace, pszName, pszBuf, sizeof(pszBuf));

        if (pszNamespace)
        {
            printf("SYSCFG_%s_%s='%s'\n", pszNamespace, pszName, pszBuf);
        }
        else
        {
            printf("SYSCFG_%s='%s'\n", pszName, pszBuf);
        }
    }

    return 0;
}

static int s_UTCTX_BatchSet(UtopiaContext* pCtx, char** ppszArgs, int iArgCount)
{
    int i;

    for (i = 0; i < iArgCount; ++i)
    {
        char* pszName;
        char* pszNamespace = 0;
        char* pszValue;

        if ((pszValue = strchr(ppszArgs[i], '=')) != 0)
        {
            *pszValue = '\0';
            pszValue++;
        }
        else
        {
            return 1;
        }

        if ((pszName = strstr(ppszArgs[i], "::")) != 0)
        {
            *pszName = '\0';
            pszName += 2;
            pszNamespace = ppszArgs[i];
        }
        else
        {
            pszName = ppszArgs[i];
        }

        if (!Utopia_RawSet(pCtx, pszNamespace, pszName, pszValue))
        {
            return 1;
        }
    }

    return 0;
}

/*
 * utctx_cmd.c - UTCTX stand-alone batch get/set application
 */

int main(int argc, char** argv)
{
    int iFailure = 0;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        iFailure = 1;
    }
    else
    {
        if (argc < 3)
        {
            usage();
            iFailure = 1;
        }
        else if (strcmp(argv[1], "get") == 0)
        {
            iFailure = s_UTCTX_BatchGet(&ctx, argv + 2, argc - 2);
        }
        else if (strcmp(argv[1], "set") == 0)
        {
            iFailure = s_UTCTX_BatchSet(&ctx, argv + 2, argc - 2);
        }
        else
        {
            usage();
            iFailure = 1;
        }
    }

    if (iFailure)
    {
        printf("SYSCFG_FAILED='true'\n");
    }

    /* Free the context */
    Utopia_Free(&ctx, !iFailure);

    exit(iFailure);
}
