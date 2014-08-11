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

#include "hdk_client.h"

#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

static const char* s_pszPortToken = "-q";
static const char* s_pszHttpUsernameToken = "-u";
static const char* s_pszHttpPasswordToken = "-p";
static const char* s_pszHttpTimeoutToken = "-t";

void PrintGetDeviceSettingsResponse(const HDK::Purenetworks_HNAP1::GetDeviceSettingsResponseStruct & response)
{
    printf("Result: %u\n", response.get_GetDeviceSettingsResult());
    printf("Type: %u\n", response.get_Type());
    printf("DeviceName: %s\n", response.get_DeviceName());
    printf("VendorName: %s\n", response.get_VendorName());
    printf("ModelDescription: %s\n", response.get_ModelDescription());
    printf("ModelName: %s\n", response.get_ModelName());
    printf("FirmwareVersion: %s\n", response.get_FirmwareVersion());
    printf("PresentationURL: %s\n", response.get_PresentationURL());

    printf("SOAPActions:\n");
    {
        HDK::Purenetworks_HNAP1::StringArray arrstr = response.get_SOAPActions();
        for (HDK::Purenetworks_HNAP1::StringArray::StringArrayIter iter = arrstr.begin(); iter != arrstr.end(); iter++)
        {
            printf("    %s\n", iter.value());
        }
    }

    printf("SubDeviceURLs:\n");
    {
        HDK::Purenetworks_HNAP1::StringArray arrstr = response.get_SubDeviceURLs();
        for (HDK::Purenetworks_HNAP1::StringArray::StringArrayIter iter = arrstr.begin(); iter != arrstr.end(); iter++)
        {
            printf("    %s\n", iter.value());
        }
    }


    printf("Tasks:\n");
    {
        HDK::Purenetworks_HNAP1::TaskExtensionArray arrte = response.get_Tasks();
        for (HDK::Purenetworks_HNAP1::TaskExtensionArrayIter iter = arrte.begin(); iter != arrte.end(); iter++)
        {
            printf("   Name: %s\n", (*iter).get_Name());
            printf("   URL: %s\n", (*iter).get_URL());
            printf("   Type: %u\n", (*iter).get_Type());
        }
    }
}

void PrintGetDeviceSettings2Response(const HDK::Purenetworks_HNAP1::GetDeviceSettings2ResponseStruct & response)
{
    printf("SerialNumber: %s\n", response.get_SerialNumber());
    printf("TimeZone: %s\n", response.get_TimeZone());
    printf("AutoAdjustDST: %s\n", response.get_AutoAdjustDST() ? "true" : "false");
    printf("Locale: %s\n", response.get_Locale());

    printf("SupportedLocales:\n");
    {
        HDK::Purenetworks_HNAP1::StringArray arrstr = response.get_SupportedLocales();
        for (HDK::Purenetworks_HNAP1::StringArrayIter iter = arrstr.begin(); iter != arrstr.end(); iter++)
        {
            const char* pszLocale = *iter;
            printf("    %s\n", pszLocale);
        }
    }

    printf("SSL: %s\n", response.get_SSL() ? "true" : "false");
}

static void PrintUsage()
{
    printf("\n");
    printf("Usage: client host [%sport] [%shttp_username] [%shttp_password] [%shttp_timeout] [hnap_input]\n", s_pszPortToken, s_pszHttpUsernameToken, s_pszHttpPasswordToken, s_pszHttpTimeoutToken);
    printf("\n");
    printf("    host - The host to send the HNAP requests. E.g. 127.0.0.1, localhost or WRT610N\n");
    printf("    [optional] port - The optional port to send the HNAP requests.  The default is 80.\n");
    printf("    [optional] http_username - HTTP Basic Auth username\n");
    printf("    [optional] http_password - HTTP Basic Auth password\n");
    printf("    [optional] http_timeout - HTTP timeout in seconds\n");
    printf("    [optional] hnap_input - XML file path containing HNAP request parameters\n");
    printf("\n");
    printf("Examples:\n");
    printf("    client 192.168.1.1\n");
    printf("    client WRT610N %s443 %spassword\n", s_pszPortToken, s_pszHttpPasswordToken);
}

static const char* ParseArgs(int argc, char* argv[],
                             char** ppszHost,
                             unsigned short* pPort,
                             char** ppszHttpUsername, char** ppszHttpPassword,
                             unsigned int* pTimeoutSecs,
                             char** ppszHnapInputFile)
{
    int i = 0;

    /* First argument is the HTTP host. */
    if (i >= argc)
    {
        return "HTTP host not specified";
    }

    *ppszHost = argv[i++];

    /* The remaining arguments are optional. */

    *pPort = 80;
    *ppszHttpUsername = NULL;
    *ppszHttpPassword = NULL;
    *pTimeoutSecs = 0;
    *ppszHnapInputFile = NULL;

    while (i < argc)
    {
        if (strstr(argv[i], s_pszPortToken) == argv[i])
        {
            const char* pszPort = argv[i++] + strlen(s_pszPortToken);
            unsigned long port = strtoul(pszPort, 0, 10);

            if ((0 == port) || (USHRT_MAX <= port))
            {
                return "Invalid port";
            }

            *pPort = (unsigned short)port;
        }
        else if (strstr(argv[i], s_pszHttpUsernameToken) == argv[i])
        {
            *ppszHttpUsername = argv[i++] + strlen(s_pszHttpUsernameToken);
        }
        else if (strstr(argv[i], s_pszHttpPasswordToken) == argv[i])
        {
            *ppszHttpPassword = argv[i++] + strlen(s_pszHttpPasswordToken);
        }
        else if (strstr(argv[i], s_pszHttpTimeoutToken) == argv[i])
        {
            const char* pszTimeout = argv[i++] + strlen(s_pszHttpTimeoutToken);
            *pTimeoutSecs = strtoul(pszTimeout, 0, 10);

            if ((0 == *pTimeoutSecs) || (UINT_MAX == *pTimeoutSecs))
            {
                return "Invalid HTTP timeout value";
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

/* Main entry point */
int main(int argc, char* argv[])
{
    /*
        As this program is intended to unittest the HDK client library, it should only
        return a non-zero result if something unrelated to the HDK client library fails.
        I.e. all failures are considered failures unless they come from the HDK client library itself.
    */
    int iError = 0;

    /* Parse arguments (minus the exectuable). */
    char* pszHost = NULL;
    unsigned short port = 80;
    char* pszHttpUsername = NULL;
    char* pszHttpPassword = NULL;
    unsigned int timeoutSecs = 0;
    char* pszHnapInputFile = NULL;

    const char* pszParseArgsError = ParseArgs(argc - 1, argv + 1,
                                              &pszHost,
                                              &port,
                                              &pszHttpUsername, &pszHttpPassword,
                                              &timeoutSecs,
                                              &pszHnapInputFile);

#ifdef _MSC_VER
    /* Ensure stdin and stdout are in binary mode */
    (void)setmode(fileno(stdin), _O_BINARY);
    (void)setmode(fileno(stdout), _O_BINARY);
#endif

    if (pszParseArgsError)
    {
        fprintf(stderr, "%s", pszParseArgsError);
        PrintUsage();

        exit(-1);
    }

    if (HDK::InitializeClient())
    {
        HDK::HNAPDevice device(pszHost, port, pszHttpUsername, pszHttpPassword);

        {
            HDK::Purenetworks_HNAP1::GetDeviceSettingsResponseStruct response;
            HDK::ClientError error = HDK::Purenetworks_HNAP1::GetDeviceSettings_GET(&device, response, NULL, timeoutSecs);
            if (HDK::ClientError_OK == error)
            {
                PrintGetDeviceSettingsResponse(response);
            }
            else
            {
                fprintf(stderr, "GetDeviceSettings failed with client error %d\n", error);
            }
        }

        {
            HDK::Purenetworks_HNAP1::GetDeviceSettings2ResponseStruct response;
            HDK::ClientError error = HDK::Purenetworks_HNAP1::GetDeviceSettings2(&device, response, NULL, timeoutSecs);
            if (HDK::ClientError_OK == error)
            {
                PrintGetDeviceSettings2Response(response);
            }
            else
            {
                fprintf(stderr, "GetDeviceSettings2 failed with client error %d\n", error);
            }
        }

        HDK::UninitializeClient();
    }

    exit(iError);
}
