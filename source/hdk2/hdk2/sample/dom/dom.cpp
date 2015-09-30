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

#include "hdk_dom.h"

#ifdef _MSC_VER
#  include <fcntl.h>
#  include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>


static const char* s_pszDescriptionToken = "--description";
static const char* s_pszMACToken = "--mac";

enum Action
{
    Action_Add,
    Action_Edit,
    Action_Remove,
    Action_Print
};

static const char* s_pszAddAction = "add";
static const char* s_pszEditAction = "edit";
static const char* s_pszRemoveAction = "remove";
static const char* s_pszPrintAction = "print";

static const char* ParseArgs(int argc, char* argv[],
                             Action& action,
                             char*& pszSerialNumber,
                             char*& pszDescription,
                             HDK::MACAddress& mac)
{
    int i = 0;

    pszDescription = NULL;
    mac = HDK::MACAddress::Blank();

    /* First argument is the action. */
    if (i >= argc)
    {
        return "action not specified";
    }

    static struct ActionMap
    {
        Action m_action;
        const char* m_pszAction;
    } s_actions[] =
        {
            { Action_Add, s_pszAddAction },
            { Action_Edit, s_pszEditAction },
            { Action_Remove, s_pszRemoveAction },
            { Action_Print, s_pszPrintAction },
        };

    bool fValidAction = false;
    for (size_t ix = 0; ix < sizeof(s_actions) / sizeof(*s_actions); ix++)
    {
        if (0 == strcmp(argv[i], s_actions[ix].m_pszAction))
        {
            action = s_actions[ix].m_action;
            fValidAction = true;
            break;
        }
    }

    if (!fValidAction)
    {
        return "invalid action";
    }

    i++;

    /* Second argument is the serial number. */
    if (i >= argc)
    {
        return "no serial number specified";
    }

    pszSerialNumber = argv[i++];

    while (i < argc)
    {
        if (0 == strcmp(argv[i], s_pszDescriptionToken))
        {
            i++;
            if (i >= argc)
            {
                return "no description";
            }
            pszDescription = argv[i++];
        }
        else if (0 == strcmp(argv[i], s_pszMACToken))
        {
            i++;
            if (i >= argc)
            {
                return "no MAC address";
            }
            const char* pszMAC = argv[i++];
            if (!mac.FromString(pszMAC))
            {
                return "invalid MAC address value";
            }
        }
    }

    return 0;
}

static void PrintUsage()
{
    printf("\n");
    printf("Usage: dom %s|%s|%s|%s SERIAL_NUMBER [%sDESC] [%sMAC]\n",
           s_pszAddAction, s_pszEditAction, s_pszRemoveAction, s_pszPrintAction,
           s_pszDescriptionToken, s_pszMACToken);
    printf("\n");
}

static HDK::Uri_cisco_com_sample::DeviceStruct FindDevice(const HDK::Uri_cisco_com_sample::NetworkStruct& network,
                                                          const char* pszSerialNumber)
{
    for (HDK::Uri_cisco_com_sample::DeviceArrayIter iter = network.get_Devices().begin();
         iter != network.get_Devices().end();
         iter++)
    {
        if (0 == strcmp(pszSerialNumber, (*iter).get_SerialNumber()))
        {
            return *iter;
        }
    }

    return HDK::Uri_cisco_com_sample::DeviceStruct(0);
}

static bool AddDevice(HDK::Uri_cisco_com_sample::NetworkStruct& network,
                      const char* pszSerialNumber, const char* pszDescription,
                      const HDK::MACAddress & mac)
{
    if (!FindDevice(network, pszSerialNumber).IsNull())
    {
        fprintf(stderr, "Device '%s' already exists\n", pszSerialNumber);
        return false;
    }

    HDK::Uri_cisco_com_sample::DeviceStruct deviceToAdd;

    deviceToAdd.set_SerialNumber(pszSerialNumber);

    if (pszDescription)
    {
        deviceToAdd.set_Description(pszDescription);
    }

    if (!mac.IsBlank())
    {
        deviceToAdd.set_MAC(mac);
    }

    HDK::Uri_cisco_com_sample::DeviceArray devices = network.get_Devices();
    if (devices.IsNull())
    {
        HDK::Uri_cisco_com_sample::DeviceArray newDevices;
        newDevices.append(deviceToAdd);
        network.set_Devices(newDevices);
    }
    else
    {
        devices.append(deviceToAdd);
    }

    return true;
}

static bool EditDevice(HDK::Uri_cisco_com_sample::NetworkStruct& network,
                       const char* pszSerialNumber, const char* pszDescription,
                       const HDK::MACAddress & mac)
{
    HDK::Uri_cisco_com_sample::DeviceStruct deviceToEdit(FindDevice(network, pszSerialNumber));
    if (deviceToEdit.IsNull())
    {
        fprintf(stderr, "Device '%s' does not exist\n", pszSerialNumber);
        return false;
    }

    if (pszDescription)
    {
        deviceToEdit.set_Description(pszDescription);
    }
    if (!mac.IsBlank())
    {
        deviceToEdit.set_MAC(mac);
    }
    return true;
}

static bool RemoveDevice(HDK::Uri_cisco_com_sample::NetworkStruct& network, const char* pszSerialNumber)
{
    HDK::Uri_cisco_com_sample::DeviceArray newDevices;
    for (HDK::Uri_cisco_com_sample::DeviceArrayIter iter = network.get_Devices().begin();
         iter != network.get_Devices().end();
         iter++)
    {
        if (0 != strcmp(pszSerialNumber, (*iter).get_SerialNumber()))
        {
            newDevices.append(*iter);
        }
    }

    network.set_Devices(newDevices);
    return true;
}

static bool PrintDevice(HDK::Uri_cisco_com_sample::NetworkStruct& network, const char* pszSerialNumber)
{
    HDK::Uri_cisco_com_sample::DeviceStruct deviceToPrint(FindDevice(network, pszSerialNumber));
    if (deviceToPrint.IsNull())
    {
        fprintf(stderr, "Device '%s' does not exist\n", pszSerialNumber);
        return false;
    }

    char szMAC[64];
    printf("Device [%s]:\n", deviceToPrint.get_SerialNumber());
    printf("    %s\n", deviceToPrint.get_Description());
    printf("    %s\n", deviceToPrint.get_MAC().ToString(szMAC));
    return true;
}


/* Main entry point */
int main(int argc, char* argv[])
{

    /* Parse arguments (minus the exectuable). */
    Action action;
    char* pszSerialNumber;
    char* pszDescription;
    HDK::MACAddress mac;

    const char* pszParseArgsError = ParseArgs(argc - 1, argv + 1,
                                              action,
                                              pszSerialNumber,
                                              pszDescription,
                                              mac);

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

    bool fSuccess = false;

    HDK::Uri_cisco_com_sample::NetworkStruct network;

    // Load the network from disk.
    if (!network.FromFile("network.xml"))
    {
        printf("failed to load network from file, generating new one...\n");
    }

    switch (action)
    {
        case Action_Add:
        {
            fSuccess = AddDevice(network, pszSerialNumber, pszDescription, mac);
            break;
        }
        case Action_Edit:
        {
            fSuccess = EditDevice(network, pszSerialNumber, pszDescription, mac);
            break;
        }
        case Action_Remove:
        {
            fSuccess = RemoveDevice(network, pszSerialNumber);
            break;
        }
        case Action_Print:
        {
            fSuccess = PrintDevice(network, pszSerialNumber);
            break;
        }
        default:
        {
            break;
        }
    }

    if (fSuccess)
    {
        // Write the network to disk.
        if (!network.ToFile("network.xml"))
        {
            fprintf(stderr, "failed to write network to disk\n");
            return errno;
        }
    }

    return 0;
}
