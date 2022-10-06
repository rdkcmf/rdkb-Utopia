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


typedef struct _LocalHostList
{
    char* pszListName;
    char* ppszIPs[3];
    char* ppszIPRanges[3];
    char* ppszMACs[3];
} LocalHostList;

typedef struct _InternetAccessPolicy
{
    char* pszPolicyName;
    char* pszEnabled;
    char* pszEnforcementSchedule;
    char* pszAccess;
    LocalHostList localHostList;
    char* ppszBlockURLs[3];
    char* ppszBlockKeywords[3];
    char* ppszBlockApplications[3];
} InternetAccessPolicy;

static InternetAccessPolicy g_IAPs[3] =
{
    {
        "My Deny IAP",    /* pszPolicyName */
        "1",              /* pszEnabled */
        "11 16:00 23:59", /* pszEnforcementSchedule */
        "allow",          /* pszAccess */
        {                 /* localHostList */
            "Your local host list",                       /* pszListName */
            {"10", "20", 0},                              /* ppszIPs */
            {"90 99", "180 199", 0},                      /* ppszIPRanges */
            {"55:44:33:22:11:00", "AA:BB:CC:DD:EE:FF", 0} /* ppszMACs */
        },
        {"www.msn.com", "www.microsoft.com", 0}, /* ppszBlockURLs */
        {"vista", "windows", 0},                 /* ppszBlockKeywords */
        {"Office", "Messenger", 0}               /* ppszBlockApplications */
    },
    {
        "My Allow IAP", /* pszPolicyName */
        "0",            /* pszEnabled */
        "1111111",      /* pszEnforcementSchedule */
        "allow",        /* pszAccess */
        {               /* localHostList */
            "My local host list",                         /* pszListName */
            {"1", "2", 0},                                /* ppszIPs */
            {"100 149", "200 249", 0},                    /* ppszIPRanges */
            {"00:11:22:33:44:55", "FF:EE:DD:CC:BB:AA", 0} /* ppszMACs */
        },
        {"www.google.com", 0, 0}, /* ppszBlockURLs */
        {0, 0, 0},                /* ppszBlockKeywords */
        {"Safari", "iChat", 0}    /* ppszBlockApplications */
    },
    {
        "none", /* pszPolicyName */
        "",     /* pszEnabled */
        "",     /* pszEnforcementSchedule */
        "",     /* pszAccess */
        {       /* localHostList */
            "",        /* pszListName */
            {0, 0, 0}, /* ppszIPs */
            {0, 0, 0}, /* ppszIPRanges */
            {0, 0, 0}  /* ppszMACs */
        },
        {0, 0, 0}, /* ppszBlockURLs */
        {0, 0, 0}, /* ppszBlockKeywords */
        {0, 0, 0}  /* ppszBlockApplications */
    }
};


/*
 * sample_utctx.c - Sample UTCTX stand-alone app to set and get InternetAccessPolicies
 */

int main(void)
{
    int iSucceeded = 1;
    UtopiaContext ctx;

    /* Initialize the context */
    if (Utopia_Init(&ctx) == 0)
    {
        printf("Error initializing context\n");
        iSucceeded = 0;
    }
    else
    {
        unsigned int i, j;

        /* Set all of the InternetAccessPolicies */
        for (i = 0; iSucceeded && (i < sizeof(g_IAPs) / sizeof(InternetAccessPolicy)); ++i)
        {
            /* NOTE: Need to pass i + 1 as index because g_IAPs' index starts at 0 and InternetAccessPolicies' starts at 1 */
            if (Utopia_SetIndexed(&ctx, UtopiaValue_InternetAccessPolicy, i + 1, g_IAPs[i].pszPolicyName) == 0 ||
                Utopia_SetIndexed(&ctx, UtopiaValue_IAP_Enabled, i + 1, g_IAPs[i].pszEnabled) == 0 ||
                Utopia_SetIndexed(&ctx, UtopiaValue_IAP_EnforcementSchedule, i + 1, g_IAPs[i].pszEnforcementSchedule) == 0 ||
                Utopia_SetIndexed(&ctx, UtopiaValue_IAP_Access, i + 1, g_IAPs[i].pszAccess) == 0 ||
                Utopia_SetIndexed(&ctx, UtopiaValue_IAP_LocalHostList, i + 1, g_IAPs[i].localHostList.pszListName) == 0)
            {
                iSucceeded = 0;
                break;
            }
            for (j = 0; iSucceeded && g_IAPs[i].localHostList.ppszIPs[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_IP, i + 1, j + 1, g_IAPs[i].localHostList.ppszIPs[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
            for (j = 0; iSucceeded && g_IAPs[i].localHostList.ppszIPRanges[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_IPRange, i + 1, j + 1, g_IAPs[i].localHostList.ppszIPRanges[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
            for (j = 0; iSucceeded && g_IAPs[i].localHostList.ppszMACs[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_MAC, i + 1, j + 1, g_IAPs[i].localHostList.ppszMACs[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
            for (j = 0; iSucceeded && g_IAPs[i].ppszBlockURLs[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_BlockURL, i + 1, j + 1, g_IAPs[i].ppszBlockURLs[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
            for (j = 0; iSucceeded && g_IAPs[i].ppszBlockKeywords[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_BlockKeyword, i + 1, j + 1, g_IAPs[i].ppszBlockKeywords[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
            for (j = 0; iSucceeded && g_IAPs[i].ppszBlockApplications[j]; ++j)
            {
                if (Utopia_SetIndexed2(&ctx, UtopiaValue_IAP_BlockApplication, i + 1, j + 1, g_IAPs[i].ppszBlockApplications[j]) == 0)
                {
                    iSucceeded = 0;
                    break;
                }
            }
        }

        /* Now, get the values we just set, printing them out */
        if (iSucceeded)
        {
            char pszBuffer[1000] = {'\0'};
            int i, j;

            /* Loop over IACs, printing the values out, until we no longer get one back */
            for (i = 1; Utopia_GetIndexed(&ctx, UtopiaValue_InternetAccessPolicy, i, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++i)
            {
                printf("\nInternetAccessPolicy_%d: %s\n", i, pszBuffer);

                pszBuffer[0] = '\0';
                Utopia_GetIndexed(&ctx, UtopiaValue_IAP_Enabled, i, pszBuffer, sizeof(pszBuffer));
                printf("\tEnabled: %s\n", pszBuffer);

                pszBuffer[0] = '\0';
                Utopia_GetIndexed(&ctx, UtopiaValue_IAP_EnforcementSchedule, i, pszBuffer, sizeof(pszBuffer));
                printf("\tEnforcementSchedule: %s\n", pszBuffer);

                pszBuffer[0] = '\0';
                Utopia_GetIndexed(&ctx, UtopiaValue_IAP_Access, i, pszBuffer, sizeof(pszBuffer));
                printf("\tAccess: %s\n", pszBuffer);

                pszBuffer[0] = '\0';
                Utopia_GetIndexed(&ctx, UtopiaValue_IAP_LocalHostList, i, pszBuffer, sizeof(pszBuffer));
                printf("\tLocalHostList: %s\n", pszBuffer);

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_IP, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\t\tIP_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_IPRange, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\t\tIPRange_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_MAC, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\t\tMAC_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_BlockURL, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\tBlockURL_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_BlockKeyword, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\tBlockKeyword_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }

                pszBuffer[0] = '\0';
                for (j = 1; Utopia_GetIndexed2(&ctx, UtopiaValue_IAP_BlockApplication, i, j, pszBuffer, sizeof(pszBuffer)) && (strcmp(pszBuffer, "none") != 0); ++j)
                {
                    printf("\tBlockApplication_%d: %s\n", j, pszBuffer);
                    pszBuffer[0] = '\0';
                }
                pszBuffer[0] = '\0';
            }
        }
        printf("\n");
    }

    /* Free the context */
    Utopia_Free(&ctx, iSucceeded);
    exit(0);
}
