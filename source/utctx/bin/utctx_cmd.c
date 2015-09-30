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
 * Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
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

#include "utctx.h"
#include "utctx_api.h"

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
