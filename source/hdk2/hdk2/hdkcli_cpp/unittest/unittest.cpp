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

#include "unittest.h"
#include "malloc_interposer.h"

#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>

static const char* s_pszLogFileToken = "-l";

static int ParseArgs(int argc, char* argv[], char** ppszTestName, char** ppszLogFile)
{
    int i = 0;

    *ppszTestName = 0;
    *ppszLogFile = 0;

    while (i < argc)
    {
        if (strstr(argv[i], s_pszLogFileToken) == argv[i])
        {
            if (*ppszLogFile)
            {
                /* "Log file specified multiple times" */
                return 0;
            }
            else
            {
                *ppszLogFile = argv[i++] + strlen(s_pszLogFileToken);
            }
        }
        else
        {
            /* Assume test name. */
            if (*ppszTestName)
            {
                /* "Test name specified multiple times" */
                return 0;
            }
            *ppszTestName = argv[i++];
        }
    }

    /* Must have a provided test name. */
    return (0 != *ppszTestName);
}


/* Main entry point */
int main(int argc, char* argv[])
{
    int result = 0;

    char* pszTestName = 0;
    char* pszLogFile = 0;

    /*
        Clear all malloc interposer stats up to this point, as some library initializers may allocate data
        which would be erroneously reported as a memory leak (since memory leaks are reported prior the libraries
        being unloaded and their destructors called.)
    */
    clear_interposer_stats();

#ifdef _MSC_VER
    /* Ensure stdin and stdout are in binary mode */
    (void)setmode(fileno(stdin), _O_BINARY);
    (void)setmode(fileno(stdout), _O_BINARY);
#endif

    /* Usage */
    if (!ParseArgs(argc - 1, argv + 1, &pszTestName, &pszLogFile))
    {
        /* Output the available test names */
        UnittestNode* pTest;
        for (pTest = g_Unittests; pTest->pszTestName; ++pTest)
        {
            UnittestLog1("%s\n", pTest->pszTestName);
        }
    }
    else
    {
        /* Find the unit test node */
        UnittestNode* pTest;
        for (pTest = g_Unittests; pTest->pszTestName; ++pTest)
        {
            if (strcmp(pTest->pszTestName, pszTestName) == 0)
            {
                break;
            }
        }
        if (!pTest->pszTestName)
        {
            UnittestLog1("Error: Unknown test \"%s\"\n", pszTestName);
            result = -1;
        }
        else
        {
            pTest->pfnTest();
        }
    }

    exit(result);
}
