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

enum
{
    UTCTX_Test__UNKNOWN__ = 0,
    UTCTX_Test_Get,
    UTCTX_Test_Set,
    UTCTX_Test_Unset,
    UTCTX_Test_Event,
    UTCTX_Test_UtopiaValues
};

static void s_RunGets(void)
{
    char psBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char psKeyBuffer[UTOPIA_KEY_NS_SIZE] = {'\0'};
    int i;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing get... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetKey(&ctx, (UtopiaValue)i, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_Get(&ctx, (UtopiaValue)i, psBuffer, sizeof(psBuffer));
        if (psBuffer[0] != '\0')
        {
            printf("Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
        }
    }

    /* Testing get indexed... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetIndexedKey(&ctx, (UtopiaValue)i, 1, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetIndexed(&ctx, (UtopiaValue)i, 1, psBuffer, sizeof(psBuffer));
        if (psBuffer[0] != '\0')
        {
            printf("Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
        }
    }

    /* Testing get indexed2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetIndexed2Key(&ctx, (UtopiaValue)i, 1, 1, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetIndexed2(&ctx, (UtopiaValue)i, 1, 1, psBuffer, sizeof(psBuffer));
        if (psBuffer[0] != '\0')
        {
            printf("Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
        }
    }

    /* Testing get named... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetNamedKey(&ctx, (UtopiaValue)i, "pfx", psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetNamed(&ctx, (UtopiaValue)i, "pfx", psBuffer, sizeof(psBuffer));
        if (psBuffer[0] != '\0')
        {
            printf("Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
        }
    }

    /* Testing get named 2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetNamed2Key(&ctx, (UtopiaValue)i, "pfx", "pfx", psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetNamed2(&ctx, (UtopiaValue)i, "pfx", "pfx", psBuffer, sizeof(psBuffer));
        if (psBuffer[0] != '\0')
        {
            printf("Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
        }
    }

    /* Free the context */
    Utopia_Free(&ctx, 1);
}

static void s_RunSets(int fCommit)
{
    char pszValue[20];
    int i;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing set... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        sprintf(pszValue, "SetValue-%d", i);
        Utopia_Set(&ctx, (UtopiaValue)i, pszValue);
    }

    /* Testing set indexed... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        sprintf(pszValue, "SetValue-%d", i);
        Utopia_SetIndexed(&ctx, (UtopiaValue)i, 1, pszValue);
    }

    /* Testing set indexed2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        sprintf(pszValue, "SetValue-%d", i);
        Utopia_SetIndexed2(&ctx, (UtopiaValue)i, 1, 1, pszValue);
    }

    /* Testing set named... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        sprintf(pszValue, "SetValue-%d", i);
        Utopia_SetNamed(&ctx, (UtopiaValue)i, "pfx", pszValue);
    }

    /* Testing set named 2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        sprintf(pszValue, "SetValue-%d", i);
        Utopia_SetNamed2(&ctx, (UtopiaValue)i, "pfx", "pfx", pszValue);
    }

    /* Free the context */
    Utopia_Free(&ctx, fCommit);
}

static void s_RunUnsets(int fCommit)
{
    int i;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing unset... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        Utopia_Unset(&ctx, (UtopiaValue)i);
    }

    /* Testing unset indexed... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        Utopia_UnsetIndexed(&ctx, (UtopiaValue)i, 1);
    }

    /* Testing unset indexed2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        Utopia_UnsetIndexed2(&ctx, (UtopiaValue)i, 1, 1);
    }

    /* Testing unset named... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        Utopia_UnsetNamed(&ctx, (UtopiaValue)i, "pfx");
    }

    /* Testing unset named 2... */
    for (i = UtopiaValue__TEST_BEGIN__; i < UtopiaValue__TEST_LAST__; ++i)
    {
        Utopia_UnsetNamed2(&ctx, (UtopiaValue)i, "pfx", "pfx");
    }

    /* Free the context */
    Utopia_Free(&ctx, fCommit);
}

static void s_RunEvent(int fCommit)
{
    int i;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing event set (no reboot, no lan restart) */
    for (i = 1; i < Utopia_Event__LAST__; i = i << 1)
    {
        /*
         * Skip Reboot since it overrides all others and
         * WLan/LanRestart since it overrides DHCPServerRestart
         * for now.
         */
        if ((Utopia_Event)i != Utopia_Event_Reboot &&
            (Utopia_Event)i != Utopia_Event_LAN_Restart &&
            (Utopia_Event)i != Utopia_Event_WLAN_Restart)
        {
            Utopia_SetEvent(&ctx, (Utopia_Event)i);
        }
    }

    /* Free the context */
    Utopia_Free(&ctx, fCommit);

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing event set (no reboot) */
    for (i = 1; i < Utopia_Event__LAST__; i = i << 1)
    {
        /*
         * Skip Reboot since it overrides all others
         */
        if ((Utopia_Event)i != Utopia_Event_Reboot)
        {
            Utopia_SetEvent(&ctx, (Utopia_Event)i);
        }
    }

    /* Free the context */
    Utopia_Free(&ctx, fCommit);

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    /* Testing event set (w/ reboot) */
    for (i = 1; i < Utopia_Event__LAST__; i = i << 1)
    {
        Utopia_SetEvent(&ctx, (Utopia_Event)i);
    }

    /* Free the context */
    Utopia_Free(&ctx, fCommit);
}

static void s_RunUtopiaValues(void)
{
    char psBuffer[UTOPIA_BUF_SIZE] = {'\0'};
    char psKeyBuffer[UTOPIA_KEY_NS_SIZE] = {'\0'};
    int i;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        exit(1);
    }

    for (i = UtopiaValue__TEST_LAST__ + 1; i < UtopiaValue__LAST__; ++i)
    {
        psBuffer[0] = '\0';
        psKeyBuffer[0] = '\0';
        Utopia_GetKey(&ctx, (UtopiaValue)i, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_Get(&ctx, (UtopiaValue)i, psBuffer, sizeof(psBuffer));
        if (psKeyBuffer[0] != '\0' || psBuffer[0] != '\0')
        {
            printf("Get: Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
            continue;
        }
        Utopia_GetIndexedKey(&ctx, (UtopiaValue)i, 1, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetIndexed(&ctx, (UtopiaValue)i, 1, psBuffer, sizeof(psBuffer));
        if (psKeyBuffer[0] != '\0' || psBuffer[0] != '\0')
        {
            printf("GetIndexed: Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
            continue;
        }
        Utopia_GetIndexed2Key(&ctx, (UtopiaValue)i, 1, 1, psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetIndexed2(&ctx, (UtopiaValue)i, 1, 1, psBuffer, sizeof(psBuffer));
        if (psKeyBuffer[0] != '\0' || psBuffer[0] != '\0')
        {
            printf("GetIndexed2: Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
            continue;
        }
        Utopia_GetNamedKey(&ctx, (UtopiaValue)i, "wl0", psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetNamed(&ctx, (UtopiaValue)i, "wl0", psBuffer, sizeof(psBuffer));
        if (psKeyBuffer[0] != '\0' || psBuffer[0] != '\0')
        {
            printf("GetNamed: Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
            continue;
        }
        Utopia_GetNamed2Key(&ctx, (UtopiaValue)i, "pfx", "pfx", psKeyBuffer, sizeof(psKeyBuffer));
        Utopia_GetNamed2(&ctx, (UtopiaValue)i, "pfx", "pfx", psBuffer, sizeof(psBuffer));
        if (psKeyBuffer[0] != '\0' || psBuffer[0] != '\0')
        {
            printf("GetNamed2: Key=%s, Value=%s\n", psKeyBuffer, psBuffer);
            continue;
        }
        printf("Failed to get key and/or value for index %d\n", i);
    }

    /* Free the context */
    Utopia_Free(&ctx, 0);
}


/*
 * unittest.c - UtCtx unittest application
 */

int main(int argc, char* argv[])
{
    int eTest = UTCTX_Test__UNKNOWN__;
    int fCommit = 0;

    if (argc >= 2)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            eTest = UTCTX_Test_Get;
        }
        else if (strcmp(argv[1], "set") == 0)
        {
            eTest = UTCTX_Test_Set;
        }
        else if (strcmp(argv[1], "unset") == 0)
        {
            eTest = UTCTX_Test_Unset;
        }
        else if (strcmp(argv[1], "event") == 0)
        {
            eTest = UTCTX_Test_Event;
        }
        else if (strcmp(argv[1], "utopia-values") == 0)
        {
            eTest = UTCTX_Test_UtopiaValues;
        }
    }

    if (argc == 3)
    {
        if (strcmp(argv[2], "commit") == 0)
        {
            fCommit = 1;
        }
    }

    switch(eTest)
    {
        case UTCTX_Test_Get:
            {
                /* Test the get api */
                printf("Testing UTCTX Get API...\n");
                s_RunGets();
            }
            break;

        case UTCTX_Test_Set:
            {
                /* Test the set api */
                printf("Testing UTCTX Set API...\n");
                s_RunSets(fCommit);
                s_RunGets();
            }
            break;

        case UTCTX_Test_Unset:
            {
                /* Test the unset api */
                printf("Testing UTCTX Unset API...\n");
                s_RunUnsets(fCommit);
                s_RunGets();
            }
            break;

        case UTCTX_Test_Event:
            {
                /* Test the event api */
                printf("Testing the UTCTX Event API...\n");
                s_RunEvent(fCommit);
            }
            break;

        case UTCTX_Test_UtopiaValues:
            {
                /* Test the event api */
                printf("Testing the UTCTX UtopiaValues...\n");
                s_RunUtopiaValues();
            }
            break;

        default:
            {
                printf("Usage: %s <get|set|unset|event|utopia-values> [commit]\n", argv[0]);
                exit(1);
            }
    }

    exit(0);
}
