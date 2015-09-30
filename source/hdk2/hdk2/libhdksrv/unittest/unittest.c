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

#include "unittest.h"
#include "malloc_interposer.h"

#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "hdk_srv.h"

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
            FILE* pfhLogFile = 0;

            /* Open an output logfile if one was specified. */
            if (pszLogFile)
            {
                pfhLogFile = fopen(pszLogFile, "wb");
                if (pfhLogFile)
                {
                    HDK_SRV_RegisterLoggingCallback(LoggingCallback, pfhLogFile);
                }
                else
                {
                    UnittestLog2("Warning: Failed to open log file '%s' with error %d\n", pszLogFile, errno);
                }
            }

            pTest->pfnTest();

            if (pfhLogFile)
            {
                fclose(pfhLogFile);
            }
        }
    }

    exit(result);
}
