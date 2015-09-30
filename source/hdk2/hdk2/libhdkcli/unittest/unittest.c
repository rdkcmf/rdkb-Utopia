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

#include "hdk_cli.h"
#include "hdk_xml.h"
#include "hdk_mod.h"

#include "unittest_util.h"

#include "unittest_schema.h"

#include "malloc_interposer.h"

#ifdef _MSC_VER
#  include <fcntl.h>
#  include <io.h>
#  include <crtdbg.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>


/*
    unittest - Unittest harness for HDK client library (libhdkcli).
*/

static const char* s_pszNetworkObjectIDToken = "-o";
static const char* s_pszHttpUsernameToken = "-u";
static const char* s_pszHttpPasswordToken = "-p";
static const char* s_pszHttpTimeoutToken = "-t";
static const char* s_pszLogFileToken = "-l";

static void PrintUsage()
{
    printf("\n");
    printf("Usage: unittest soap_action http_host [%sobject_id] [%shttp_username] [%shttp_password] [%shttp_timeout] [%slog_file] [hnap_input]\n", s_pszNetworkObjectIDToken, s_pszHttpUsernameToken, s_pszHttpPasswordToken, s_pszHttpTimeoutToken, s_pszLogFileToken);
    printf("\n");
    printf("    soap_action - The SOAP action for the HNAP method.\n");
    printf("    http_host - The host+scheme to send the HNAP requests. E.g. http://127.0.0.1:8080\n");
    printf("    [optional] object_id - The Network Object ID to include in the request\n");
    printf("    [optional] http_username - HTTP Basic Auth username\n");
    printf("    [optional] http_password - HTTP Basic Auth password\n");
    printf("    [optional] http_timeout - HTTP timeout in seconds\n");
    printf("    [optional] log_file - file to write logging output\n");
    printf("    [optional] hnap_input - XML file path containing HNAP request parameters\n");
    printf("\n");
    printf("Examples:\n");
    printf("    unittest http://purenetworks.com/HNAP1/GetDeviceSettings http://192.168.1.1\n");
    printf("    unittest http://purenetworks.com/HNAP1/GetDeviceSettings2 http://127.0.0.1:8081 -uadmin -ppassword -t1000 GetDeviceSettings2.request\n");
}

static const char* ParseArgs(int argc, char* argv[],
                             char** ppszSoapAction,
                             char** ppszHttpHost,
                             char** ppszNetworkObjectID,
                             char** ppszHttpUsername, char** ppszHttpPassword,
                             unsigned int* pTimeoutSecs,
                             char** ppszLogFile,
                             char** ppszHnapInputFile)
{
    int i = 0;

    /* First argument is the SOAP action. */
    if (i >= argc)
    {
        return "SOAP action not specified";
    }

    *ppszSoapAction = argv[i++];

    /* Second argument is the HTTP host. */
    if (i >= argc)
    {
        return "HTTP host not specified";
    }

    *ppszHttpHost = argv[i++];

    /* The remaining arguments are optional. */

    *ppszNetworkObjectID = NULL;
    *ppszHttpUsername = NULL;
    *ppszHttpPassword = NULL;
    *pTimeoutSecs = 0;
    *ppszLogFile = NULL;
    *ppszHnapInputFile = NULL;

    while (i < argc)
    {
        if (strstr(argv[i], s_pszNetworkObjectIDToken) == argv[i])
        {
            if (*ppszNetworkObjectID)
            {
                return "Network Object ID specified multiple times";
            }
            else
            {
                *ppszNetworkObjectID = argv[i++] + strlen(s_pszNetworkObjectIDToken);
            }
        }

        else if (strstr(argv[i], s_pszHttpUsernameToken) == argv[i])
        {
            if (*ppszHttpUsername)
            {
                return "HTTP Basic Auth username specified multiple times";
            }
            else
            {
                *ppszHttpUsername = argv[i++] + strlen(s_pszHttpUsernameToken);
            }
        }
        else if (strstr(argv[i], s_pszHttpPasswordToken) == argv[i])
        {
            if (*ppszHttpPassword)
            {
                return "HTTP Basic Auth password specified multiple times";
            }
            else
            {
                *ppszHttpPassword = argv[i++] + strlen(s_pszHttpPasswordToken);
            }
        }
        else if (strstr(argv[i], s_pszHttpTimeoutToken) == argv[i])
        {
            if (*pTimeoutSecs)
            {
                return "HTTP timeout value specified multiple times";
            }
            else
            {
                const char* pszTimeout = argv[i++] + strlen(s_pszHttpTimeoutToken);
                *pTimeoutSecs = strtoul(pszTimeout, 0, 10);

                if ((0 == *pTimeoutSecs) || (UINT_MAX == *pTimeoutSecs))
                {
                    return "Invalid HTTP timeout value";
                }
            }
        }
        else if (strstr(argv[i], s_pszLogFileToken) == argv[i])
        {
            if (*ppszLogFile)
            {
                return "Output log file specified multiple times";
            }
            else
            {
                *ppszLogFile = argv[i++] + strlen(s_pszLogFileToken);
            }
        }
        else
        {
            /* The HNAP input file. */
            if (*ppszHnapInputFile)
            {
                return "HNAP input XML file specified multiple times";
            }
            else
            {
                *ppszHnapInputFile = argv[i++];
            }
        }
    }

    return 0;
}

static const char* FindFileName(const char* pszPath)
{
#ifdef _MSC_VER
#  define PATH_SEP '\\'
#else /* ndef _MSC_VER */
#  define PATH_SEP '/'
#endif /* def _MSC_VER */
    if (pszPath)
    {
        const char* pszInputFileName = strrchr(pszPath, PATH_SEP);
        if (pszInputFileName)
        {
            /* move past the '/' */
            return (++pszInputFileName);
        }
        else
        {
            return pszPath;
        }
    }

    return pszPath;
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
    char* pszSoapAction = NULL;
    char* pszHttpHost = NULL;
    char* pszNetworkObjectID = NULL;
    char* pszHttpUsername = NULL;
    char* pszHttpPassword = NULL;
    unsigned int timeoutSecs = 0;
    char* pszLogFile = NULL;
    char* pszHnapInputFile = NULL;

    HDK_XML_UUID noid;

    HDK_XML_Struct sSchemaInput;
    HDK_XML_Struct sSchemaOutput;

    /*
        As this program is intended to unittest the HDK client library, it should only
        return a non-zero result if something unrelated to the HDK client library fails.
        I.e. all failures are considered failures unless they come from the HDK client library itself.
    */
    int iError = 0;

    const char* pszParseArgsError = NULL;

    FILE* pfhLogFile = NULL;

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

    /* Enable memory leak detection on exit */
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif

    /* Parse arguments (minus the exectuable). */
    pszParseArgsError = ParseArgs(argc - 1, argv + 1,
                                  &pszSoapAction,
                                  &pszHttpHost,
                                  &pszNetworkObjectID,
                                  &pszHttpUsername, &pszHttpPassword,
                                  &timeoutSecs,
                                  &pszLogFile,
                                  &pszHnapInputFile);

    if (pszParseArgsError)
    {
        fprintf(stderr, "%s", pszParseArgsError);
        PrintUsage();

        exit(-1);
    }

    if (pszNetworkObjectID)
    {
        if (!HDK_XML_Parse_UUID(&noid, pszNetworkObjectID))
        {
            fprintf(stderr, "Invalid Network object ID value '%s'\n", pszNetworkObjectID);
            exit(-1);
        }
    }

    if (pszLogFile)
    {
        pfhLogFile = fopen(pszLogFile, "wb");
        if (!pfhLogFile)
        {
            fprintf(stderr, "Failed to open log file '%s' with error %d\n", pszLogFile, errno);
            exit(-1);
        }
    }

    {
        /* Only print out the file name since we don't want paths in the test output. */
        const char* pszInputFileName = FindFileName(pszHnapInputFile);

        /* Dump out the arguments getting passed to the hdkcli lib. */
        printf("\n");
        printf("      SOAP action: %s\n", pszSoapAction);
        printf("        HTTP host: %s\n", pszHttpHost);
        printf("Network Object ID: %s\n", pszNetworkObjectID);
        printf("    HTTP username: %s\n", pszHttpUsername);
        printf("    HTTP password: %s\n", pszHttpPassword);
        printf("          timeout: %d\n", timeoutSecs);
        printf("       HNAP input: %s\n", pszInputFileName);
        printf("\n");
    }

    HDK_XML_Struct_Init(&sSchemaInput);
    HDK_XML_Struct_Init(&sSchemaOutput);

    if (HDK_CLI_Init())
    {
        if (pfhLogFile)
        {
            /* Register the logging callback. */
            HDK_CLI_RegisterLoggingCallback(LoggingCallback, pfhLogFile);
        }

        do
        {
            const HDK_MOD_Module* pModule = UNITTEST_SCHEMA_Module();
            int method = 0;
            HDK_CLI_Error cliError = HDK_CLI_Error_OK;

            HDK_XML_Struct* pInput = NULL;
            HDK_XML_Struct* pOutput = NULL;

            /* Find the method enumeration value */
            const HDK_MOD_Method* pMethod;
            for (pMethod = pModule->pMethods; pMethod->pszHTTPMethod; ++pMethod)
            {
                if (pMethod->pszSOAPAction && strcmp(pMethod->pszSOAPAction, pszSoapAction) == 0)
                {
                    break;
                }
                /* Explicit empty SOAP action indicates NULL SOAP action method. */
                if (*pszSoapAction == '\0' && NULL == pMethod->pszSOAPAction)
                {
                    break;
                }
                ++method;
            }
            if (!pMethod->pszHTTPMethod)
            {
                fprintf(stderr, "Unknown SOAPAction '%s'\n", pszSoapAction);
                iError = -2;
                break;
            }

            /* Load the input file, if provided. */
            if (pszHnapInputFile)
            {
                if (!pMethod->pSchemaInput)
                {
                    fprintf(stderr, "Specified an input file '%s' for method '%s' with no input schema.\n", pszHnapInputFile, pszSoapAction);
                    iError = -2;
                    break;
                }
                if (!DeserializeFileToStruct(pszHnapInputFile, pMethod->pSchemaInput, &sSchemaInput))
                {
                    fprintf(stderr, "Failed to deserialize HNAP input file '%s'\n", pszHnapInputFile);
                    iError = -2;
                    break;
                }

                /* Find the input struct using the input element path. */
                pInput = &sSchemaInput;
                if (pMethod->pInputElementPath)
                {
                    const HDK_XML_Element* pElement;
                    for (pElement = pMethod->pInputElementPath; HDK_MOD_ElementPathEnd != *pElement; ++pElement)
                    {
                        pInput = HDK_XML_Get_Struct(pInput, *pElement);
                        if (!pInput)
                        {
                            break;
                        }
                    }

                    if (!pInput)
                    {
                        break;
                    }
                }
            }

            /* Wrap the output struct to match the schema. */
            pOutput = &sSchemaOutput;
            pOutput->node.element = pMethod->pSchemaOutput->pSchemaNodes->element;
            if (pMethod->pOutputElementPath)
            {
                const HDK_XML_Element* pElement;
                for (pElement = pMethod->pOutputElementPath; HDK_MOD_ElementPathEnd != *pElement; ++pElement)
                {
                    pOutput = HDK_XML_Set_Struct(pOutput, *pElement);
                    if (!pOutput)
                    {
                        break;
                    }
                }

                if (!pOutput)
                {
                    fprintf(stderr, "Failed to wrap output struct\n");
                    break;
                }
            }

            cliError = HDK_CLI_Request(pszNetworkObjectID ? &noid : 0,
                                       pszHttpHost,
                                       pszHttpUsername, pszHttpPassword,
                                       timeoutSecs,
                                       pModule, method,
                                       pInput, pOutput);
            if (HDK_CLI_Error_OK == cliError)
            {
                /* Serailize the response to stdout. */
                if (!SerializeStructToFile(&sSchemaOutput, pMethod->pSchemaOutput, stdout))
                {
                    fprintf(stderr, "Failed to serialize output\n");
                    iError = -2;
                }
            }
            else
            {
                const char* pszError = NULL;
                switch (cliError)
                {
                    case HDK_CLI_Error_OK:
                    {
                        pszError = "OK";
                        break;
                    }
                    case HDK_CLI_Error_RequestInvalid:
                    {
                        pszError = "RequestInvalid";
                        break;
                    }
                    case HDK_CLI_Error_ResponseInvalid:
                    {
                        pszError = "ResponseInvalid";
                        break;
                    }
                    case HDK_CLI_Error_SoapFault:
                    {
                        pszError = "SoapFault";
                        break;
                    }
                    case HDK_CLI_Error_XmlParse:
                    {
                        pszError = "XmlParse";
                        break;
                    }
                    case HDK_CLI_Error_HttpAuth:
                    {
                        pszError = "HttpAuth";
                        break;
                    }
                    case HDK_CLI_Error_HttpUnknown:
                    {
                        pszError = "HttpUnknown";
                        break;
                    }
                    case HDK_CLI_Error_Connection:
                    {
                        pszError = "Connection";
                        break;
                    }
                    case HDK_CLI_Error_OutOfMemory:
                    {
                        pszError = "OutOfMemory";
                        break;
                    }
                    case HDK_CLI_Error_InvalidArg:
                    {
                        pszError = "InvalidArg";
                        break;
                    }
                    case HDK_CLI_Error_Unknown:
                    {
                        pszError = "Unknown";
                        break;
                    }
                }
                printf("HNAP call failed with error %s (%d)\n", pszError, cliError);
            }

        } while (0);

        HDK_CLI_Cleanup();
    }
    else
    {
        fprintf(stderr, "Failed to initialize HDK client library.\n");
        iError = -2;
    }

    HDK_XML_Struct_Free(&sSchemaInput);
    HDK_XML_Struct_Free(&sSchemaOutput);

    if (pfhLogFile)
    {
        fclose(pfhLogFile);
    }

    /*
        exit() MUST be called in order to report c-runtime memory usage statistics.
    */
    exit(iError);
}
