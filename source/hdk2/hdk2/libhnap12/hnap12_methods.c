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

#include "hnap12.h"

#include "hnap12_util.h"
#include "hdk_srv.h"

#include <stdlib.h>
#include <string.h>


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result)                    \
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)

/* Helper method for doing an ADIGet on RadioID indexed values */
#define RadioIDKeyed_ADIGet(pMemberCtx, adiElement, pInput, pOutput, valueElement, valueType) \
    HDK_SRV_ADIGetValue_ByString(pMethodCtx, HNAP12_ADI_##adiElement, pOutput, pInput, \
                                 HNAP12_Element_PN_RadioID, HNAP12_Element_##valueElement, valueType)

/* Helper method for doing an ADISet on RadioID indexed values */
#define RadioIDKeyed_ADISet(pMethodCtx, adiElement, pInput, valueElement, valueType) \
    HDK_SRV_ADISetValue_ByString(pMethodCtx, HNAP12_ADI_##adiElement, pInput, HNAP12_Element_##adiElement##Info, \
                                 HNAP12_Element_PN_RadioID, HNAP12_Element_##valueElement, valueType)


#if defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY__) || defined(__HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__)

static int s_HNAP12_ValidateEncryptionKey(char* pszKey, HNAP12_Enum_PN_WiFiEncryption eWiFiEncryption)
{
    if ((eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_WEP_64 && strlen(pszKey) == 10) ||
        (eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_WEP_128 && strlen(pszKey) == 26))
    {
        return HNAP12_UTL_WEPKey_IsValid(pszKey);
    }
    else if (eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_AES ||
             eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_TKIP ||
             eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_TKIPORAES)
    {
        return HNAP12_UTL_WPAKey_IsValid(pszKey, strlen(pszKey) == 64);
    }

    return 0;
}

#endif /* defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY__) || defined(__HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__) */


#if defined(__HNAP12_METHOD_PN_GETWLANRADIOSECURITY__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY) || \
    defined(__HNAP12_METHOD_PN_GETWLANRADIOSETTINGS__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSETTINGS) || \
    defined(__HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__) || defined(__HNAP12_METHOD_PN_SETWANSETTINGS__) || \
    defined(__HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__)

/* Helper function to validate that enumValue is in the element of one of the structs in pStruct */
static HDK_XML_Member* s_HNAP12_Value_Exists_InStruct(HDK_XML_Struct* pStruct, HDK_XML_Element element, void* pValue, HDK_XML_Type type)
{
    HDK_XML_Member* pMember;

    if (!pStruct || !pValue)
    {
        return 0;
    }

    /* Iterate over the pStruct array, searching for the enumValue value */
    for (pMember = pStruct->pHead; pMember; pMember = pMember->pNext)
    {
        char* psz = 0;
        int* pi = 0;

        /* Handle user defined types */
        if (type < 0)
        {
            if (element == HDK_XML_BuiltinElement_Unknown)
            {
                pi = HDK_XML_GetMember_Enum(pMember, type);
            }
            else
            {
                pi = HDK_XML_Get_Enum(HDK_XML_GetMember_Struct(pMember), element, type);
            }
        }
        /* Handle builtin int type */
        else if (type == HDK_XML_BuiltinType_Int)
        {
            if (element == HDK_XML_BuiltinElement_Unknown)
            {
                pi = HDK_XML_GetMember_Int(pMember);
            }
            else
            {
                pi = HDK_XML_Get_Int(HDK_XML_GetMember_Struct(pMember), element);
            }
        }
        /* Handle builtin string type */
        else if (type == HDK_XML_BuiltinType_String)
        {
            psz = HDK_XML_Get_String(HDK_XML_GetMember_Struct(pMember), element);
        }

        if (pi && *pi == *(int*)pValue)
        {
            return pMember;
        }
        if (psz && strcmp(psz, (char*)pValue) == 0)
        {
            return pMember;
        }
    }

    return 0;
}

#endif /* defined(__HNAP12_METHOD_PN_GETWLANRADIOSECURITY__) || ... */


#if defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSETTINGS) || \
    defined(__HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__) || defined(__HNAP12_METHOD_PN_SETWANSETTINGS__) || \
    defined(__HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__)

/* Helper function to validate that enumValue is in the element of one of the structs in pStruct */
static HDK_XML_Member* s_HNAP12_Enum_Exists_ArrayOfStruct(HDK_XML_Struct* pStruct, HDK_XML_Element element, int enumValue, HDK_XML_Type enumType)
{
    return s_HNAP12_Value_Exists_InStruct(pStruct, element, &enumValue, enumType);
}

/* Helper function to validate that enumValue is one of the enums in the array of enums pStruct */
static HDK_XML_Member* s_HNAP12_Enum_Exists_ArrayOfEnum(HDK_XML_Struct* pStruct, int enumValue, HDK_XML_Type enumType)
{
    return s_HNAP12_Value_Exists_InStruct(pStruct, HDK_XML_BuiltinElement_Unknown, &enumValue, enumType);
}

#endif /* defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY__) || ... */


#if defined(__HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSETTINGS__)

/* Helper function to validate that iValue is in the element of one of the structs in pStruct */
static HDK_XML_Member* s_HNAP12_Int_Exists_ArrayOfStruct(HDK_XML_Struct* pStruct, HDK_XML_Element element, int iInt)
{
    return s_HNAP12_Value_Exists_InStruct(pStruct, element, &iInt, HDK_XML_BuiltinType_Int);
}

/* Helper function to validate that iValue is one of the ints in the array of ints pStruct */
static HDK_XML_Member* s_HNAP12_Int_Exists_ArrayOfInt(HDK_XML_Struct* pStruct, int iInt)
{
    return s_HNAP12_Value_Exists_InStruct(pStruct, HDK_XML_BuiltinElement_Unknown, &iInt, HDK_XML_BuiltinType_Int);
}

#endif /* defined(__HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSETTINGS) */


#if defined(__HNAP12_METHOD_PN_GETWLANRADIOSECURITY__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSECURITY) || \
    defined(__HNAP12_METHOD_PN_GETWLANRADIOSETTINGS__) || defined(__HNAP12_METHOD_PN_SETWLANRADIOSETTINGS)

/* Helper function to validate that pszValue is in the element of one of the structs in pStruct */
static HDK_XML_Member* s_HNAP12_String_Exists_ArrayOfStruct(HDK_XML_Struct* pStruct, HDK_XML_Element element, char* pszValue)
{
    return s_HNAP12_Value_Exists_InStruct(pStruct, element, pszValue, HDK_XML_BuiltinType_String);
}

#endif /* defined(__HNAP12_METHOD_PN_GETWLANRADIOSECURITY__) || ... */


/*
 * Method http://purenetworks.com/HNAP1/AddPortMapping
 */

#ifdef __HNAP12_METHOD_PN_ADDPORTMAPPING__

void HNAP12_Method_PN_AddPortMapping(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_IPAddress* pIpAddr;
    HNAP12_Enum_PN_IPProtocol* peProtocol = 0;
    HDK_XML_Struct* psPortMappings = 0;
    HDK_XML_Struct* psPortMapping;

    HDK_XML_Struct sTemp;
    HDK_XML_Struct_Init(&sTemp);

    /* Unknown protocol? */
    peProtocol = HNAP12_Get_PN_IPProtocol(pInput, HNAP12_Element_PN_PortMappingProtocol);
    if (*peProtocol == HNAP12_Enum_PN_IPProtocol__UNKNOWN__)
    {
        SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
        goto finish;
    }

    /* Make sure this mapping doesn't already exist */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_PortMappings, &sTemp, HNAP12_Element_PN_PortMappings);
    if ((psPortMappings = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_PortMappings)) != 0)
    {
        int* pi;
        int* piExternalPort;
        HNAP12_Enum_PN_IPProtocol* pe;
        HDK_XML_Member* pmPortMapping;

        piExternalPort = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_ExternalPort);

        /* Loop over the array searching for a duplicate port mapping */
        for (pmPortMapping = psPortMappings->pHead; pmPortMapping; pmPortMapping = pmPortMapping->pNext)
        {
            pe = HNAP12_Get_PN_IPProtocol(HDK_XML_GetMember_Struct(pmPortMapping), HNAP12_Element_PN_PortMappingProtocol);
            pi = HDK_XML_Get_Int(HDK_XML_GetMember_Struct(pmPortMapping), HNAP12_Element_PN_ExternalPort);

            /* If we have a protocol, and it matches, or no protocol, and the ports match, then return an error */
            if (((pe && *peProtocol == *pe) || !pe) &&
                (pi && *piExternalPort == *pi))
            {
                SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
                goto finish;
            }
        }
    }
    else
    {
        /* If we don't have a port mapping struct, then create a new one */
        psPortMappings = HDK_XML_Set_Struct(&sTemp, HNAP12_Element_PN_PortMappings);
    }

    /* Return an error if we don't have and couldn't create a port mapping struct */
    if (!psPortMappings)
    {
        SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
        goto finish;
    }

    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_LanIPAddress, &sTemp, HNAP12_Element_PN_RouterIPAddress);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_LanSubnetMask, &sTemp, HNAP12_Element_PN_RouterSubnetMask);

    /* Validate the IPAddress */
    pIpAddr = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_InternalClient);
    if (!HNAP12_UTL_IPAddress_IsWithinSubnet(HDK_XML_Get_IPAddress(&sTemp, HNAP12_Element_PN_RouterIPAddress),
                                             HDK_XML_Get_IPAddress(&sTemp, HNAP12_Element_PN_RouterSubnetMask),
                                             pIpAddr))
    {
        SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
        goto finish;
    }

    /* Add a new port mapping to the struct */
    psPortMapping = HDK_XML_Append_Struct(psPortMappings, HNAP12_Element_PN_PortMapping);
    if (!psPortMapping)
    {
        SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
        goto finish;
    }

    /* Set the required elements */
    HDK_XML_Set_String(psPortMapping, HNAP12_Element_PN_PortMappingDescription, HDK_XML_Get_String(pInput, HNAP12_Element_PN_PortMappingDescription));
    HDK_XML_Set_IPAddress(psPortMapping, HNAP12_Element_PN_InternalClient, pIpAddr);
    HNAP12_Set_PN_IPProtocol(psPortMapping, HNAP12_Element_PN_PortMappingProtocol, *peProtocol);
    HDK_XML_Set_Int(psPortMapping, HNAP12_Element_PN_ExternalPort, HDK_XML_GetEx_Int(pInput, HNAP12_Element_PN_ExternalPort, 0));
    HDK_XML_Set_Int(psPortMapping, HNAP12_Element_PN_InternalPort, HDK_XML_GetEx_Int(pInput, HNAP12_Element_PN_InternalPort, 0));

    /* Set the PortMappings values. If any fail, return an error. */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_PortMappings, &sTemp, HNAP12_Element_PN_PortMappings))
    {
        SetHNAPResult(pOutput, HNAP12, PN_AddPortMapping, ERROR);
    }

finish:

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_ADDPORTMAPPING__ */


/*
 * Method http://purenetworks.com/HNAP1/DeletePortMapping
 */

#ifdef __HNAP12_METHOD_PN_DELETEPORTMAPPING__

void HNAP12_Method_PN_DeletePortMapping(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Member* pmPortMapping = 0;
    HDK_XML_Struct* psPortMappings;

    HDK_XML_Struct sTemp;
    HDK_XML_Struct_Init(&sTemp);

    /* First off, retrieve the current port mapping array */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_PortMappings, &sTemp, HNAP12_Element_PN_PortMappings);
    if ((psPortMappings = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_PortMappings)) != 0)
    {
        int* pi;
        int* piExternalPort;
        HNAP12_Enum_PN_IPProtocol* pe;
        HNAP12_Enum_PN_IPProtocol* peProtocol;
        HDK_XML_Member* pmPortMappingPrev = 0;

        peProtocol = HNAP12_Get_PN_IPProtocol(pInput, HNAP12_Element_PN_PortMappingProtocol);
        piExternalPort = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_ExternalPort);

        /* Loop over the array searching for the port mapping to delete */
        for (pmPortMapping = psPortMappings->pHead; pmPortMapping; pmPortMapping = pmPortMapping->pNext)
        {
            pe = HNAP12_Get_PN_IPProtocol(HDK_XML_GetMember_Struct(pmPortMapping), HNAP12_Element_PN_PortMappingProtocol);
            pi = HDK_XML_Get_Int(HDK_XML_GetMember_Struct(pmPortMapping), HNAP12_Element_PN_ExternalPort);

            /* If we have a protocol, and it matches, or no protocol, and the ports match, then delete this entry */
            if (((pe && *peProtocol == *pe) || !pe) &&
                (pi && *piExternalPort == *pi))
            {
                if (!pmPortMappingPrev)
                {
                    psPortMappings->pHead = pmPortMapping->pNext;
                }
                else
                {
                    pmPortMappingPrev->pNext = pmPortMapping->pNext;
                }

                /* Since this is a heap member, we also need to free the member */
                HDK_XML_Struct_Free(HDK_XML_GetMember_Struct(pmPortMapping));
                free(pmPortMapping);

                break;
            }

            pmPortMappingPrev = pmPortMapping;
        }
    }

    /* If we didn't find the port mapping member to delete, or validating/setting the new port mapping array fails, return error */
    if (!pmPortMapping ||
        !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_PortMappings, &sTemp, HNAP12_Element_PN_PortMappings))
    {
        SetHNAPResult(pOutput, HNAP12, PN_DeletePortMapping, ERROR);
    }

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_DELETEPORTMAPPING__ */


/*
 * Method http://purenetworks.com/HNAP1/DownloadSpeedTest
 */

#ifdef __HNAP12_METHOD_PN_DOWNLOADSPEEDTEST__

void HNAP12_Method_PN_DownloadSpeedTest(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;
    (void)pOutput;
}

#endif /* __HNAP12_METHOD_PN_DOWNLOADSPEEDTEST__ */


/*
 * Method http://purenetworks.com/HNAP1/GetClientStats
 */

#ifdef __HNAP12_METHOD_PN_GETCLIENTSTATS__

void HNAP12_Method_PN_GetClientStats(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the ClientStats values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ClientStats, pOutput, HNAP12_Element_PN_ClientStats);
}

#endif /* __HNAP12_METHOD_PN_GETCLIENTSTATS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetConnectedDevices
 */

#ifdef __HNAP12_METHOD_PN_GETCONNECTEDDEVICES__

void HNAP12_Method_PN_GetConnectedDevices(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the ConnectedDevices values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ConnectedClients, pOutput, HNAP12_Element_PN_ConnectedClients);
}

#endif /* __HNAP12_METHOD_PN_GETCONNECTEDDEVICES__ */


/*
 * Method http://purenetworks.com/HNAP1/GetDeviceSettings
 */

#ifdef __HNAP12_METHOD_PN_GETDEVICESETTINGS__

static int SOAPActionsCompare(const void* a, const void* b)
{
    return strcmp(*(const char**)a, *(const char**)b);
}

void HNAP12_Method_PN_GetDeviceSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* pSOAPActions;
    HDK_SRV_ModuleContext** ppModuleCtx;
    unsigned int nSOAPActions = 0;
    const char** ppszSOAPActions = 0;

    /* Unused parameters */
    (void)pInput;

    /* Get the DeviceSettings values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DeviceType, pOutput, HNAP12_Element_PN_Type);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DeviceName, pOutput, HNAP12_Element_PN_DeviceName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_VendorName, pOutput, HNAP12_Element_PN_VendorName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ModelDescription, pOutput, HNAP12_Element_PN_ModelDescription);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ModelName, pOutput, HNAP12_Element_PN_ModelName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_FirmwareVersion, pOutput, HNAP12_Element_PN_FirmwareVersion);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_PresentationURL, pOutput, HNAP12_Element_PN_PresentationURL);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_SubDeviceURLs, pOutput, HNAP12_Element_PN_SubDeviceURLs);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_TaskExtensions, pOutput, HNAP12_Element_PN_Tasks);

    /* Count the SOAPActions */
    for (ppModuleCtx = pMethodCtx->ppModuleCtxs; *ppModuleCtx; ++ppModuleCtx)
    {
        const HDK_MOD_Method* pMethod;
        for (pMethod = (*ppModuleCtx)->pModule->pMethods; pMethod->pfnMethod; ++pMethod)
        {
            if (pMethod->pszSOAPAction)
            {
                ++nSOAPActions;
            }
        }
    }

    /* Allocate the SOAPActions array */
    ppszSOAPActions = (nSOAPActions ? (const char**)malloc(nSOAPActions * sizeof(*ppszSOAPActions)) : 0);
    if (ppszSOAPActions)
    {
        /* Populate the SOAPActions array */
        unsigned int ixSOAPAction = 0;
        for (ppModuleCtx = pMethodCtx->ppModuleCtxs; *ppModuleCtx; ++ppModuleCtx)
        {
            const HDK_MOD_Method* pMethod;
            for (pMethod = (*ppModuleCtx)->pModule->pMethods; pMethod->pfnMethod; ++pMethod)
            {
                if (pMethod->pszSOAPAction)
                {
                    ppszSOAPActions[ixSOAPAction++] = pMethod->pszSOAPAction;
                }
            }
        }

        /* Sort the SOAPActions array */
        qsort((void*)ppszSOAPActions, nSOAPActions, sizeof(*ppszSOAPActions), SOAPActionsCompare);
    }

    /* Add the SOAPActions array */
    pSOAPActions = HDK_XML_Set_Struct(pOutput, HNAP12_Element_PN_SOAPActions);
    if (pSOAPActions && nSOAPActions)
    {
        unsigned int ixSOAPAction;
        for (ixSOAPAction = 0; ixSOAPAction < nSOAPActions; ++ixSOAPAction)
        {
            HDK_XML_Append_String(pSOAPActions, HNAP12_Element_PN_string, ppszSOAPActions[ixSOAPAction]);
        }
    }

    /* Free the SOAPActions array */
    free((void*)ppszSOAPActions);
}

#endif /* __HNAP12_METHOD_PN_GETDEVICESETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetDeviceSettings2
 */

#ifdef __HNAP12_METHOD_PN_GETDEVICESETTINGS2__

void HNAP12_Method_PN_GetDeviceSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the DeviceSettings2 values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_SerialNumber, pOutput, HNAP12_Element_PN_SerialNumber);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_TimeZone, pOutput, HNAP12_Element_PN_TimeZone);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_AutoAdjustDST, pOutput, HNAP12_Element_PN_AutoAdjustDST);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_Locale, pOutput, HNAP12_Element_PN_Locale);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_SupportedLocales, pOutput, HNAP12_Element_PN_SupportedLocales);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_SSL, pOutput, HNAP12_Element_PN_SSL);
}

#endif /* __HNAP12_METHOD_PN_GETDEVICESETTINGS2__ */


/*
 * Method http://purenetworks.com/HNAP1/GetFirmwareSettings
 */

#ifdef __HNAP12_METHOD_PN_GETFIRMWARESETTINGS__

void HNAP12_Method_PN_GetFirmwareSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the FirmwareSettings values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_FirmwareVersion, pOutput, HNAP12_Element_PN_FirmwareVersion);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_VendorName, pOutput, HNAP12_Element_PN_VendorName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ModelName, pOutput, HNAP12_Element_PN_ModelName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ModelRevision, pOutput, HNAP12_Element_PN_ModelRevision);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_FirmwareDate, pOutput, HNAP12_Element_PN_FirmwareDate);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_UpdateMethods, pOutput, HNAP12_Element_PN_UpdateMethods);
}

#endif /* __HNAP12_METHOD_PN_GETFIRMWARESETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetMACFilters2
 */

#ifdef __HNAP12_METHOD_PN_GETMACFILTERS2__

void HNAP12_Method_PN_GetMACFilters2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the MACFilters2 values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_MFEnabled, pOutput, HNAP12_Element_PN_Enabled);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_MFIsAllowList, pOutput, HNAP12_Element_PN_IsAllowList);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_MFMACList, pOutput, HNAP12_Element_PN_MACList);
}

#endif /* __HNAP12_METHOD_PN_GETMACFILTERS2__ */


/*
 * Method http://purenetworks.com/HNAP1/GetNetworkStats
 */

#ifdef __HNAP12_METHOD_PN_GETNETWORKSTATS__

void HNAP12_Method_PN_GetNetworkStats(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the NetworkStats values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DeviceNetworkStats, pOutput, HNAP12_Element_PN_Stats);
}

#endif /* __HNAP12_METHOD_PN_GETNETWORKSTATS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetPortMappings
 */

#ifdef __HNAP12_METHOD_PN_GETPORTMAPPINGS__

void HNAP12_Method_PN_GetPortMappings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the PortMappings values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_PortMappings, pOutput, HNAP12_Element_PN_PortMappings);
}

#endif /* __HNAP12_METHOD_PN_GETPORTMAPPINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetRouterLanSettings2
 */

#ifdef __HNAP12_METHOD_PN_GETROUTERLANSETTINGS2__

void HNAP12_Method_PN_GetRouterLanSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the RouterLanSettings2 values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_LanIPAddress, pOutput, HNAP12_Element_PN_RouterIPAddress);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_LanSubnetMask, pOutput, HNAP12_Element_PN_RouterSubnetMask);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DHCPServerEnabled, pOutput, HNAP12_Element_PN_DHCPServerEnabled);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DHCPIPAddressFirst, pOutput, HNAP12_Element_PN_IPAddressFirst);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DHCPIPAddressLast, pOutput, HNAP12_Element_PN_IPAddressLast);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DHCPLeaseTime, pOutput, HNAP12_Element_PN_LeaseTime);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DHCPReservations, pOutput, HNAP12_Element_PN_DHCPReservations);
}

#endif /* __HNAP12_METHOD_PN_GETROUTERLANSETTINGS2__ */


/*
 * Method http://purenetworks.com/HNAP1/GetRouterSettings
 */

#ifdef __HNAP12_METHOD_PN_GETROUTERSETTINGS__

void HNAP12_Method_PN_GetRouterSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the RouterSettings values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ManageRemote, pOutput, HNAP12_Element_PN_ManageRemote);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ManageWireless, pOutput, HNAP12_Element_PN_ManageWireless);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RemotePort, pOutput, HNAP12_Element_PN_RemotePort);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RemoteSSL, pOutput, HNAP12_Element_PN_RemoteSSL);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DomainName, pOutput, HNAP12_Element_PN_DomainName);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiredQoS, pOutput, HNAP12_Element_PN_WiredQoS);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WPSPin, pOutput, HNAP12_Element_PN_WPSPin);
}

#endif /* __HNAP12_METHOD_PN_GETROUTERSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioFrequencies
 */

#ifdef __HNAP12_METHOD_PN_GETWLANRADIOFREQUENCIES__

void HNAP12_Method_PN_GetWLanRadioFrequencies(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WLanRadioFrequencies values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioFrequencyInfos, pOutput, HNAP12_Element_PN_RadioFrequencyInfos);
}

#endif /* __HNAP12_METHOD_PN_GETWLANRADIOFREQUENCIES__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioSecurity
 */

#ifdef __HNAP12_METHOD_PN_GETWLANRADIOSECURITY__

void HNAP12_Method_PN_GetWLanRadioSecurity(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszRadioId = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadioID);
    HDK_XML_Member* pmRadio = 0;
    HDK_XML_Struct* psRadios;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, make sure this is a valid RadioID */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioInfos, &sTemp, HNAP12_Element_PN_RadioInfos);
    psRadios = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_RadioInfos);
    if ((pmRadio = s_HNAP12_String_Exists_ArrayOfStruct(psRadios, HNAP12_Element_PN_RadioID, pszRadioId)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_GetWLanRadioSecurity, ERROR_BAD_RADIOID);
        goto finish;
    }

    /* Get the WLanRadioSecurity values */
    RadioIDKeyed_ADIGet(pMemberCtx, PN_WLanSecurityEnabled, pInput, pOutput, PN_Enabled, HDK_XML_BuiltinType_Bool);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanType, pInput, pOutput, PN_Type, HNAP12_EnumType_PN_WiFiSecurity);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanEncryption, pInput, pOutput, PN_Encryption, HNAP12_EnumType_PN_WiFiEncryption);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanKey, pInput, pOutput, PN_Key, HDK_XML_BuiltinType_String);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanKeyRenewal, pInput, pOutput, PN_KeyRenewal, HDK_XML_BuiltinType_Int);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusIP1, pInput, pOutput, PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusPort1, pInput, pOutput, PN_RadiusPort1, HDK_XML_BuiltinType_Int);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusSecret1, pInput, pOutput, PN_RadiusSecret1, HDK_XML_BuiltinType_String);

    /* Only set the Radius2 values if the device supports it */
    if (HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_Radius2Supported))
    {
        RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusIP2, pInput, pOutput, PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress);
        RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusPort2, pInput, pOutput, PN_RadiusPort2, HDK_XML_BuiltinType_Int);
        RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanRadiusSecret2, pInput, pOutput, PN_RadiusSecret2, HDK_XML_BuiltinType_String);
    }
    else
    {
        HDK_XML_Set_Blank(pOutput, HNAP12_Element_PN_RadiusIP2);
        HDK_XML_Set_Int(pOutput, HNAP12_Element_PN_RadiusPort2, 0);
        HDK_XML_Set_String(pOutput, HNAP12_Element_PN_RadiusSecret2, "");
    }

finish:

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_GETWLANRADIOSECURITY__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioSettings
 */

#ifdef __HNAP12_METHOD_PN_GETWLANRADIOSETTINGS__

void HNAP12_Method_PN_GetWLanRadioSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszRadioId = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadioID);
    HDK_XML_Member* pmRadio = 0;
    HDK_XML_Struct* psRadios;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, make sure this is a valid RadioID */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioInfos, &sTemp, HNAP12_Element_PN_RadioInfos);
    psRadios = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_RadioInfos);
    if ((pmRadio = s_HNAP12_String_Exists_ArrayOfStruct(psRadios, HNAP12_Element_PN_RadioID, pszRadioId)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_GetWLanRadioSettings, ERROR_BAD_RADIOID);
        goto finish;
    }

    /* Get the WLanRadioSettings values */
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanEnabled, pInput, pOutput, PN_Enabled, HDK_XML_BuiltinType_Bool);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanMode, pInput, pOutput, PN_Mode, HNAP12_EnumType_PN_WiFiMode);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanMacAddress, pInput, pOutput, PN_MacAddress, HDK_XML_BuiltinType_MACAddress);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanSSID, pInput, pOutput, PN_SSID, HDK_XML_BuiltinType_String);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanSSIDBroadcast, pInput, pOutput, PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanChannelWidth, pInput, pOutput, PN_ChannelWidth, HDK_XML_BuiltinType_Int);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanChannel, pInput, pOutput, PN_Channel, HDK_XML_BuiltinType_Int);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanSecondaryChannel, pInput, pOutput, PN_SecondaryChannel, HDK_XML_BuiltinType_Int);
    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanQoS, pInput, pOutput, PN_QoS, HDK_XML_BuiltinType_Bool);

finish:

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_GETWLANRADIOSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadios
 */

#ifdef __HNAP12_METHOD_PN_GETWLANRADIOS__

void HNAP12_Method_PN_GetWLanRadios(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WLanRadios values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioInfos, pOutput, HNAP12_Element_PN_RadioInfos);
}

#endif /* __HNAP12_METHOD_PN_GETWLANRADIOS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWanInfo
 */

#ifdef __HNAP12_METHOD_PN_GETWANINFO__

void HNAP12_Method_PN_GetWanInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WanInfo values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanSupportedTypes, pOutput, HNAP12_Element_PN_SupportedTypes);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanAutoDetectType, pOutput, HNAP12_Element_PN_AutoDetectType);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanStatus, pOutput, HNAP12_Element_PN_Status);
}

#endif /* __HNAP12_METHOD_PN_GETWANINFO__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWanSettings
 */

#ifdef __HNAP12_METHOD_PN_GETWANSETTINGS__

void HNAP12_Method_PN_GetWanSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    HNAP12_Enum_PN_WANStatus eWanStatus;
    HNAP12_Enum_PN_WANType eWanType;
    HDK_XML_IPAddress defaultIP = {0,0,0,0};

    /* Unused parameters */
    (void)pInput;

    /* Initialize temprary struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Retrieve the values needed for comparison first */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanType, pOutput, HNAP12_Element_PN_Type);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanStatus, &sTemp, HNAP12_Element_PN_Status);
    eWanType = HNAP12_GetEx_PN_WANType(pOutput, HNAP12_Element_PN_Type, HNAP12_Enum_PN_WANType__UNKNOWN__);
    eWanStatus = HNAP12_GetEx_PN_WANStatus(&sTemp, HNAP12_Element_PN_Status, HNAP12_Enum_PN_WANStatus__UNKNOWN__);

    /*
     * If the WANType is not one of the following, then we need to set the username,
     * password, servicename, maxidletime, & autoreconnect into the output struct
     */
    if (eWanType != HNAP12_Enum_PN_WANType_DHCP && eWanType != HNAP12_Enum_PN_WANType_Static &&
        eWanType != HNAP12_Enum_PN_WANType_BridgedOnly && eWanType != HNAP12_Enum_PN_WANType_Dynamic1483Bridged &&
        eWanType != HNAP12_Enum_PN_WANType_Static1483Bridged && eWanType != HNAP12_Enum_PN_WANType_Static1483Routed &&
        eWanType != HNAP12_Enum_PN_WANType_StaticIPOA)
    {
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanUsername, pOutput, HNAP12_Element_PN_Username);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanPassword, pOutput, HNAP12_Element_PN_Password);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanMaxIdleTime, pOutput, HNAP12_Element_PN_MaxIdleTime);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanAutoReconnect, pOutput, HNAP12_Element_PN_AutoReconnect);

        /* The service name depends upon the WANType */
        if (eWanType == HNAP12_Enum_PN_WANType_BigPond)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanAuthService, pOutput, HNAP12_Element_PN_ServiceName);
        }
        else if (eWanType == HNAP12_Enum_PN_WANType_DHCPPPPoE || eWanType == HNAP12_Enum_PN_WANType_StaticPPPoE ||
                 eWanType == HNAP12_Enum_PN_WANType_StaticPPPOA || eWanType == HNAP12_Enum_PN_WANType_DynamicPPPOA)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanPPPoEService, pOutput, HNAP12_Element_PN_ServiceName);
        }
        else if (eWanType == HNAP12_Enum_PN_WANType_DynamicL2TP || eWanType == HNAP12_Enum_PN_WANType_DynamicPPTP ||
                 eWanType == HNAP12_Enum_PN_WANType_StaticL2TP || eWanType == HNAP12_Enum_PN_WANType_StaticPPTP)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanLoginService, pOutput, HNAP12_Element_PN_ServiceName);
        }
        else
        {
            HDK_XML_Set_String(pOutput, HNAP12_Element_PN_ServiceName, "");
        }
    }
    else
    {
        HDK_XML_Set_String(pOutput, HNAP12_Element_PN_Username, "");
        HDK_XML_Set_String(pOutput, HNAP12_Element_PN_Password, "");
        HDK_XML_Set_String(pOutput, HNAP12_Element_PN_ServiceName, "");
        HDK_XML_Set_Int(pOutput, HNAP12_Element_PN_MaxIdleTime, 0);
        HDK_XML_Set_Bool(pOutput, HNAP12_Element_PN_AutoReconnect, 0);
    }

    if (eWanType == HNAP12_Enum_PN_WANType_DHCP || eWanType == HNAP12_Enum_PN_WANType_DHCPPPPoE ||
        eWanType == HNAP12_Enum_PN_WANType_DynamicL2TP || eWanType == HNAP12_Enum_PN_WANType_DynamicPPTP ||
        eWanType == HNAP12_Enum_PN_WANType_DynamicPPPOA || eWanType == HNAP12_Enum_PN_WANType_Dynamic1483Bridged ||
        eWanType == HNAP12_Enum_PN_WANType_BigPond)
    {
        /* If we're connected, get the values from the device, otherwise return '0.0.0.0' */
        if (eWanStatus == HNAP12_Enum_PN_WANStatus_CONNECTED)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanIPAddress, pOutput, HNAP12_Element_PN_IPAddress);
        }
        else
        {
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_IPAddress, &defaultIP);
        }

        /* SubnetMask and Gateway should only be set for a subset of the above WANTypes */
        if (eWanType != HNAP12_Enum_PN_WANType_DHCPPPPoE && eWanType != HNAP12_Enum_PN_WANType_DynamicPPPOA &&
            eWanType != HNAP12_Enum_PN_WANType_BigPond &&
            /* If we're connected get the values from the device, otherwise return '0.0.0.0' */
            eWanStatus == HNAP12_Enum_PN_WANStatus_CONNECTED)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanSubnetMask, pOutput, HNAP12_Element_PN_SubnetMask);
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanGateway, pOutput, HNAP12_Element_PN_Gateway);
        }
        else
        {
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_SubnetMask, &defaultIP);
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_Gateway, &defaultIP);
        }
    }
    /* If the WANType is bridged-only then leave the IPAddress, SubnetMask, and Gateway blank */
    else if (eWanType != HNAP12_Enum_PN_WANType_BridgedOnly)
    {
        /* The IPAddress should always be set for Static* WANTypes */
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanIPAddress, pOutput, HNAP12_Element_PN_IPAddress);

        /* The SubnetMask and Gateway should be set if not one of the following */
        if (eWanType != HNAP12_Enum_PN_WANType_StaticPPPoE && eWanType != HNAP12_Enum_PN_WANType_StaticPPPOA)
        {
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanSubnetMask, pOutput, HNAP12_Element_PN_SubnetMask);
            HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanGateway, pOutput, HNAP12_Element_PN_Gateway);
        }
        else
        {
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_SubnetMask, &defaultIP);
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_Gateway, &defaultIP);
        }
    }
    else
    {
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_IPAddress, &defaultIP);
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_SubnetMask, &defaultIP);
            HDK_XML_Set_IPAddress(pOutput, HNAP12_Element_PN_Gateway, &defaultIP);
    }

    /* If the WANType is not bridged-only, then set DNSSettings in output struct */
    if (eWanType != HNAP12_Enum_PN_WANType_BridgedOnly)
    {
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanDNS, pOutput, HNAP12_Element_PN_DNS);
    }
    else
    {
        HDK_XML_Struct* pDNS = HDK_XML_Set_Struct(pOutput, HNAP12_Element_PN_DNS);

        if (pDNS)
        {
            HDK_XML_Set_IPAddress(pDNS, HNAP12_Element_PN_Primary, &defaultIP);
            HDK_XML_Set_IPAddress(pDNS, HNAP12_Element_PN_Secondary, &defaultIP);
        }
    }

    /* Set the remainging Wan settings into the output struct */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanMacAddress, pOutput, HNAP12_Element_PN_MacAddress);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanMTU, pOutput, HNAP12_Element_PN_MTU);

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_GETWANSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWirelessClientCapabilities
 */

#ifdef __HNAP12_METHOD_PN_GETWIRELESSCLIENTCAPABILITIES__

void HNAP12_Method_PN_GetWirelessClientCapabilities(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WirelessClientCapabilities values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSupportedNetworkType, pOutput, HNAP12_Element_PN_SupportedNetworkType);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSupportedSecurity, pOutput, HNAP12_Element_PN_SupportedSecurity);
}

#endif /* __HNAP12_METHOD_PN_GETWIRELESSCLIENTCAPABILITIES__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWirelessClientConnectionInfo
 */

#ifdef __HNAP12_METHOD_PN_GETWIRELESSCLIENTCONNECTIONINFO__

void HNAP12_Method_PN_GetWirelessClientConnectionInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WirelessClientConnectionInfo values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSSID, pOutput, HNAP12_Element_PN_SSID);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientFrequency, pOutput, HNAP12_Element_PN_Frequency);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientMode, pOutput, HNAP12_Element_PN_Mode);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientChannelWidth, pOutput, HNAP12_Element_PN_ChannelWidth);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientChannel, pOutput, HNAP12_Element_PN_Channel);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSignalStrength, pOutput, HNAP12_Element_PN_SignalStrength);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientNoise, pOutput, HNAP12_Element_PN_Noise);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientLinkSpeedIn, pOutput, HNAP12_Element_PN_LinkSpeedIn);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientLinkSpeedOut, pOutput, HNAP12_Element_PN_LinkSpeedOut);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientWmmEnabled, pOutput, HNAP12_Element_PN_WmmEnabled);
}

#endif /* __HNAP12_METHOD_PN_GETWIRELESSCLIENTCONNECTIONINFO__ */


/*
 * Method http://purenetworks.com/HNAP1/GetWirelessClientSettings
 */

#ifdef __HNAP12_METHOD_PN_GETWIRELESSCLIENTSETTINGS__

void HNAP12_Method_PN_GetWirelessClientSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WirelessClientSettings values */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSSID, pOutput, HNAP12_Element_PN_SSID);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientNetworkType, pOutput, HNAP12_Element_PN_NetworkType);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSecurityEnabled, pOutput, HNAP12_Element_PN_SecurityEnabled);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSecurityType, pOutput, HNAP12_Element_PN_SecurityType);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientEncryption, pOutput, HNAP12_Element_PN_Encryption);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientKey, pOutput, HNAP12_Element_PN_Key);
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientConnected, pOutput, HNAP12_Element_PN_Connected);
}

#endif /* __HNAP12_METHOD_PN_GETWIRELESSCLIENTSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/IsDeviceReady
 */

#ifdef __HNAP12_METHOD_PN_ISDEVICEREADY__

void HNAP12_Method_PN_IsDeviceReady(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;
    (void)pOutput;

    if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_IsDeviceReady))
    {
        SetHNAPResult(pOutput, HNAP12, PN_IsDeviceReady, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_ISDEVICEREADY__ */


/*
 * Method http://purenetworks.com/HNAP1/Reboot
 */

#ifdef __HNAP12_METHOD_PN_REBOOT__

void HNAP12_Method_PN_Reboot(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;
    (void)pOutput;

    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RebootTrigger, 0, HNAP12_Element_PN_RebootTrigger);
}

#endif /* __HNAP12_METHOD_PN_REBOOT__ */


/*
 * Method http://purenetworks.com/HNAP1/RenewWanConnection
 */

#ifdef __HNAP12_METHOD_PN_RENEWWANCONNECTION__

void HNAP12_Method_PN_RenewWanConnection(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Set the ADI values. If any fail, return an error. */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanRenewTimeout, pInput, HNAP12_Element_PN_RenewTimeout))
    {
        SetHNAPResult(pOutput, HNAP12, PN_RenewWanConnection, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_RENEWWANCONNECTION__ */


/*
 * Method http://purenetworks.com/HNAP1/SetAccessPointMode
 */

#ifdef __HNAP12_METHOD_PN_SETACCESSPOINTMODE__

void HNAP12_Method_PN_SetAccessPointMode(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Set the ADI values. If any fail, return an error. */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_IsAccessPoint, pInput, HNAP12_Element_PN_IsAccessPoint))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetAccessPointMode, ERROR);
    }
    else
    {
        /* Get the new LanIPAddress */
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_LanIPAddress, pOutput, HNAP12_Element_PN_NewIPAddress);
    }
}

#endif /* __HNAP12_METHOD_PN_SETACCESSPOINTMODE__ */


/*
 * Method http://purenetworks.com/HNAP1/SetDeviceSettings
 */

#ifdef __HNAP12_METHOD_PN_SETDEVICESETTINGS__

void HNAP12_Method_PN_SetDeviceSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Set the ADI values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DeviceName, pInput, HNAP12_Element_PN_DeviceName) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_AdminPassword, pInput, HNAP12_Element_PN_AdminPassword)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_SETDEVICESETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/SetDeviceSettings2
 */

#ifdef __HNAP12_METHOD_PN_SETDEVICESETTINGS2__

void HNAP12_Method_PN_SetDeviceSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;

    /* Initialize temprary struct */
    HDK_XML_Struct_Init(&sTemp);

    /*
     * Check if setting the username is supported by the device. If not, then we need to
     * return an error if the client is attempting to set the username to something
     * other than what was used to authenticate the HTTP POST (current device username)
     */
    if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_UsernameSupported))
    {
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_Username, &sTemp, HNAP12_Element_PN_Username);

        if (strcmp(HDK_XML_Get_String(pInput, HNAP12_Element_PN_Username),
                   HDK_XML_GetEx_String(&sTemp, HNAP12_Element_PN_Username, "")))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings2, ERROR_USERNAME_NOT_SUPPORTED);
            goto finish;
        }
    }

    /*
     * Check if setting the timezone is supported by the device. If not, then we need to
     * return an error if the client is attempting to set the timezone to something
     * other than what the current timezone setting is
     */
    if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_TimeZoneSupported))
    {
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_TimeZone, &sTemp, HNAP12_Element_PN_TimeZone);

        if (strcmp(HDK_XML_Get_String(pInput, HNAP12_Element_PN_TimeZone),
                   HDK_XML_GetEx_String(&sTemp, HNAP12_Element_PN_TimeZone, "")))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings2, ERROR_TIMEZONE_NOT_SUPPORTED);
            goto finish;
        }
    }

    /* If we're disabling SSL and RemoteSSL is enabled, return error if RemoteSSLNeedsSSL */
    if (!HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_SSL, 0))
    {
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RemoteSSL, &sTemp, HNAP12_Element_PN_RemoteSSL);

        if (HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_RemoteSSL, 1) &&
            HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_RemoteSSLNeedsSSL))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings2, ERROR_REMOTE_SSL_NEEDS_SSL);
            goto finish;
        }
    }

    /* Attempt to set the timezone, returning the appropriate error if it fails */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_TimeZone, pInput, HNAP12_Element_PN_TimeZone))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings2, ERROR_TIMEZONE_NOT_SUPPORTED);
        goto finish;
    }

    /* Set the ADI values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_Username, pInput, HNAP12_Element_PN_Username) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_AutoAdjustDST, pInput, HNAP12_Element_PN_AutoAdjustDST) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_SSL, pInput, HNAP12_Element_PN_SSL) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_Locale, pInput, HNAP12_Element_PN_Locale)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetDeviceSettings2, ERROR);
    }

finish:

    /* Free the temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETDEVICESETTINGS2__ */


/*
 * Method http://purenetworks.com/HNAP1/SetMACFilters2
 */

#ifdef __HNAP12_METHOD_PN_SETMACFILTERS2__

void HNAP12_Method_PN_SetMACFilters2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_MACAddress* pMac1;
    HDK_XML_MACAddress* pMac2;
    HDK_XML_Member* pmMACInfo1;
    HDK_XML_Member* pmMACInfo2;

    HDK_XML_Struct* psMACList = HDK_XML_Get_Struct(pInput, HNAP12_Element_PN_MACList);

    /* Verify that we don't have any duplicate MACs */
    for (pmMACInfo1 = psMACList->pHead; pmMACInfo1; pmMACInfo1 = pmMACInfo1->pNext)
    {
        pMac1 = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmMACInfo1), HNAP12_Element_PN_MacAddress);

        for (pmMACInfo2 = psMACList->pHead; pmMACInfo2; pmMACInfo2 = pmMACInfo2->pNext)
        {
            pMac2 = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmMACInfo2), HNAP12_Element_PN_MacAddress);

            /* If it's the same MACInfo member continue */
            if (pmMACInfo2 == pmMACInfo1)
            {
                continue;
            }
            if (HDK_XML_IsEqual_MACAddress(pMac1, pMac2))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetMACFilters2, ERROR);
                return;
            }
        }
    }

    /* Validate and set the device values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_MFEnabled, pInput, HNAP12_Element_PN_Enabled) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_MFIsAllowList, pInput, HNAP12_Element_PN_IsAllowList) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_MFMACList, pInput, HNAP12_Element_PN_MACList)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetMACFilters2, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_SETMACFILTERS2__ */


/*
 * Method http://purenetworks.com/HNAP1/SetRouterLanSettings2
 */

#ifdef __HNAP12_METHOD_PN_SETROUTERLANSETTINGS2__

void HNAP12_Method_PN_SetRouterLanSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int* pnLeaseTime;
    HDK_XML_IPAddress* pRouterIP;
    HDK_XML_IPAddress* pRouterSubnet;
    HDK_XML_IPAddress* pIpFirst;
    HDK_XML_IPAddress* pIpLast;

    /* Retrive values needed for validation */
    pRouterIP = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_RouterIPAddress);
    pRouterSubnet = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_RouterSubnetMask);
    pIpFirst = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_IPAddressFirst);
    pIpLast = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_IPAddressLast);

    /* Validate the router subnet mask */
    if (!HNAP12_UTL_IPAddress_IsValidSubnet(pRouterSubnet))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_SUBNET);
        return;
    }

    /* Validate the router IP address */
    if (!HNAP12_UTL_IPAddress_IsValid(pRouterIP, pRouterSubnet))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_IP_ADDRESS);
        return;
    }

    /* Validate the DHCP range, returning corresponding error. */
    if (!HNAP12_UTL_IPAddress_IsValidRange(pRouterIP, pRouterSubnet, pIpFirst, pIpLast))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_IP_RANGE);
        return;
    }

    /* Validate the lease time */
    pnLeaseTime = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_LeaseTime);
    if (*pnLeaseTime <= 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR);
        return;
    }

    /* Retrieve the DHCP reservations for validation */
    {
        HDK_XML_IPAddress* pIp1;
        HDK_XML_IPAddress* pIp2;
        HDK_XML_MACAddress* pMac1;
        HDK_XML_MACAddress* pMac2;
        HDK_XML_Member* pmDHCP1;
        HDK_XML_Member* pmDHCP2;
        HDK_XML_Struct* psDHCPs = HDK_XML_Get_Struct(pInput, HNAP12_Element_PN_DHCPReservations);

        /* Check that the device supports DHCP reservations */
        if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_DHCPReservationsSupported))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_RESERVATIONS_NOT_SUPPORTED);
            return;
        }

        /* Iterate over the DHCP reservations */
        for (pmDHCP1 = psDHCPs->pHead; pmDHCP1; pmDHCP1 = pmDHCP1->pNext)
        {
            /*
             * Validate the IP address of the DHCP reservation is in the device subnet.
             */
            pIp1 = HDK_XML_Get_IPAddress(HDK_XML_GetMember_Struct(pmDHCP1), HNAP12_Element_PN_IPAddress);
            if (!HNAP12_UTL_IPAddress_IsWithinSubnet(pRouterIP, pRouterSubnet, pIp1) ||
                HDK_XML_IsEqual_IPAddress(pRouterIP, pIp1))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_RESERVATION);
                return;
            }

            /* Go through the DHCP reservations and validate that there are no duplicate IPs or MACs */
            pMac1 = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmDHCP1), HNAP12_Element_PN_MacAddress);
            for (pmDHCP2 = psDHCPs->pHead; pmDHCP2; pmDHCP2 = pmDHCP2->pNext)
            {
                /* If it's the same DHCP member continue */
                if (pmDHCP2 == pmDHCP1)
                {
                    continue;
                }

                pIp2 = HDK_XML_Get_IPAddress(HDK_XML_GetMember_Struct(pmDHCP2), HNAP12_Element_PN_IPAddress);
                pMac2 = HDK_XML_Get_MACAddress(HDK_XML_GetMember_Struct(pmDHCP2), HNAP12_Element_PN_MacAddress);
                if (HDK_XML_IsEqual_IPAddress(pIp1, pIp2) || HDK_XML_IsEqual_MACAddress(pMac1, pMac2))
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_RESERVATION);
                    return;
                }
            }
        }
    }

    /* Set the router ip and return correspond error if device rejects */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_LanIPAddress, pInput, HNAP12_Element_PN_RouterIPAddress))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_IP_ADDRESS);
        return;
    }

    /* Set the router subnet mask and return corresponding error if device rejects */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_LanSubnetMask, pInput, HNAP12_Element_PN_RouterSubnetMask))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR_BAD_SUBNET);
        return;
    }

    /* Set the remaining device values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DHCPServerEnabled, pInput, HNAP12_Element_PN_DHCPServerEnabled) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DHCPIPAddressFirst, pInput, HNAP12_Element_PN_IPAddressFirst) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DHCPIPAddressLast, pInput, HNAP12_Element_PN_IPAddressLast) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DHCPLeaseTime, pInput, HNAP12_Element_PN_LeaseTime) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DHCPReservations, pInput, HNAP12_Element_PN_DHCPReservations)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterLanSettings2, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_SETROUTERLANSETTINGS2__ */


/*
 * Method http://purenetworks.com/HNAP1/SetRouterSettings
 */

#ifdef __HNAP12_METHOD_PN_SETROUTERSETTINGS__

void HNAP12_Method_PN_SetRouterSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;

    /* Initialize temprary struct */
    HDK_XML_Struct_Init(&sTemp);

    /*
     * If the device doesn't support remote management, we need to return an error if the client
     * is trying to set ManageRemote, ManageWireless, RemotePort, or RemoteSSL to non-
     * default settings.
     */
    if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_RemoteManagementSupported))
    {
        /* Retrieve the remote management defaults from device */
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ManageRemote, &sTemp, HNAP12_Element_PN_ManageRemote);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ManageWireless, &sTemp, HNAP12_Element_PN_ManageWireless);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RemotePort, &sTemp, HNAP12_Element_PN_RemotePort);
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_RemoteSSL, &sTemp, HNAP12_Element_PN_RemoteSSL);

        /* Now compare them to what the client is attempting to set */
        if (HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_ManageRemote, 0) != HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_ManageRemote, 0) ||
            HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_ManageWireless, 0) != HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_ManageWireless, 0) ||
            HDK_XML_GetEx_Int(pInput, HNAP12_Element_PN_RemotePort, 0) != HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_RemotePort, 0) ||
            HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_RemoteSSL, 0) != HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_RemoteSSL, 0))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_REMOTE_MANAGE_NOT_SUPPORTED);
            goto finish;
        }
    }
    else
    {
        /* If ManageRemote is enabled, then make sure password is not default, and valid */
        if (HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_ManageRemote, 0))
        {
            if (HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_IsAdminPasswordDefault))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_REMOTE_MANAGE_DEFAULT_PASSWORD);
                goto finish;
            }

            /* If enabling RemoteSSL, we need to do a couple of checks */
            if (HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_RemoteSSL, 0))
            {
                /* Make sure remote manage via SSL is supported, if not, return appropriate error */
                if (!HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_ManageViaSSLSupported))
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_REMOTE_SSL_NOT_SUPPORTED);
                    goto finish;
                }

                /* If RemoteSSL requires SSL, make sure SSL is enabled */
                if (HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_RemoteSSLNeedsSSL))
                {
                    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_SSL, &sTemp, HNAP12_Element_PN_SSL);

                    if (!HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_SSL, 0))
                    {
                        SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_REMOTE_SSL_NEEDS_SSL);
                        goto finish;
                    }
                }
            }
            else
            {
                /* If disabling RemoteSSL, make sure ManageOnlyViaSSL is not true */
                if (HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_ManageOnlyViaSSL))
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_REMOTE_MANAGE_MUST_BE_SSL);
                    goto finish;
                }
            }
        }
    }

    /* Check that the client is allowed to change the domain name if they are attempting to and validate */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_DomainName, &sTemp, HNAP12_Element_PN_DomainName);
    if ((strcmp(HDK_XML_Get_String(pInput, HNAP12_Element_PN_DomainName),
                HDK_XML_GetEx_String(&sTemp, HNAP12_Element_PN_DomainName, "")) &&
         !HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_DomainNameChangeSupported)) ||
        !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_DomainName, pInput, HNAP12_Element_PN_DomainName))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_DOMAIN_NOT_SUPPORTED);
        goto finish;
    }

    /* Check that wired qos is allowed if the client is attempting to enable it and validate */
    if ((HDK_XML_GetEx_Bool(pInput, HNAP12_Element_PN_WiredQoS, 0) &&
         !HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_WiredQoSSupported)) ||
        !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiredQoS, pInput, HNAP12_Element_PN_WiredQoS))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR_QOS_NOT_SUPPORTED);
        goto finish;
    }

    /* Set the remaining device values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_ManageRemote, pInput, HNAP12_Element_PN_ManageRemote) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_ManageWireless, pInput, HNAP12_Element_PN_ManageWireless) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_RemotePort, pInput, HNAP12_Element_PN_RemotePort) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_RemoteSSL, pInput, HNAP12_Element_PN_RemoteSSL)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetRouterSettings, ERROR);
    }

finish:

    /* Free the temprorary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETROUTERSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioFrequency
 */

#ifdef __HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__

void HNAP12_Method_PN_SetWLanRadioFrequency(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszRadioId = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadioID);
    int* piFrequency;
    HDK_XML_Member* pmFrequency = 0;
    HDK_XML_Member* pmFrequencyInfo = 0;
    HDK_XML_Struct* psFrequencies;
    HDK_XML_Struct* psFrequencyInfos;

    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, make sure this is a valid RadioID */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioFrequencyInfos, &sTemp, HNAP12_Element_PN_RadioFrequencyInfos);
    psFrequencyInfos = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_RadioFrequencyInfos);
    if ((pmFrequencyInfo = s_HNAP12_String_Exists_ArrayOfStruct(psFrequencyInfos, HNAP12_Element_PN_RadioID, pszRadioId)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioFrequency, ERROR_BAD_RADIOID);
        goto finish;
    }

    /* Validate and set the frequency in the device */
    psFrequencies = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmFrequencyInfo), HNAP12_Element_PN_Frequencies);
    piFrequency = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_Frequency);
    if (!piFrequency ||
        ((pmFrequency = s_HNAP12_Int_Exists_ArrayOfInt(psFrequencies, *piFrequency)) == 0) ||
        !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanFrequency, pInput, PN_Frequency, HDK_XML_BuiltinType_Int))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioFrequency, ERROR);
    }

finish:

    /* Free the temprorary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETWLANRADIOFREQUENCY__ */


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioSecurity
 */

#ifdef __HNAP12_METHOD_PN_SETWLANRADIOSECURITY__

void HNAP12_Method_PN_SetWLanRadioSecurity(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszRadioId = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadioID);
    int* pfEnabled;
    HNAP12_Enum_PN_WiFiSecurity eWiFiSecurity;
    HNAP12_Enum_PN_WiFiEncryption eWiFiEncryption;
    HDK_XML_Member* pmRadio = 0;
    HDK_XML_Struct* psRadios;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, make sure this is a valid RadioID */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioInfos, &sTemp, HNAP12_Element_PN_RadioInfos);
    psRadios = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_RadioInfos);
    if ((pmRadio = s_HNAP12_String_Exists_ArrayOfStruct(psRadios, HNAP12_Element_PN_RadioID, pszRadioId)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_BAD_RADIOID);
        goto finish;
    }

    eWiFiSecurity = *HNAP12_Get_PN_WiFiSecurity(pInput, HNAP12_Element_PN_Type);
    eWiFiEncryption = *HNAP12_Get_PN_WiFiEncryption(pInput, HNAP12_Element_PN_Encryption);

    /* Skip WiFiSecurity and WiFiEncryption validation if the client is not enabling */
    pfEnabled = HDK_XML_Get_Bool(pInput, HNAP12_Element_PN_Enabled);
    if (*pfEnabled)
    {
        char* pszKey;

        HDK_XML_Member* pmEncryption = 0;
        HDK_XML_Member* pmSecurityInfo = 0;
        HDK_XML_Struct* psEncryptions;
        HDK_XML_Struct* psSecurityInfos;

        /* Validate security type */
        psSecurityInfos = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmRadio), HNAP12_Element_PN_SupportedSecurity);
        if (eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_ ||
            (pmSecurityInfo = s_HNAP12_Enum_Exists_ArrayOfStruct(psSecurityInfos, HNAP12_Element_PN_SecurityType,
                                                           eWiFiSecurity, HNAP12_EnumType_PN_WiFiSecurity)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_TYPE_NOT_SUPPORTED);
            goto finish;
        }

        /* Validate encryption type */
        psEncryptions = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmSecurityInfo), HNAP12_Element_PN_Encryptions);
        if (eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_ ||
            (pmEncryption = s_HNAP12_Enum_Exists_ArrayOfEnum(psEncryptions, eWiFiEncryption, HNAP12_EnumType_PN_WiFiEncryption)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_ENCRYPTION_NOT_SUPPORTED);
            goto finish;
        }

        /* If security type is not WPA2* or encryption is not AES/TKIP, make sure that the WiFiMode is not 802.11n */
        if ((eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK &&
             eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS) ||
            (eWiFiEncryption != HNAP12_Enum_PN_WiFiEncryption_AES &&
             (eWiFiEncryption != HNAP12_Enum_PN_WiFiEncryption_TKIP ||
              eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK)))
        {
            RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanMode, pInput, &sTemp, PN_Mode, HNAP12_EnumType_PN_WiFiMode);

            if (HNAP12_Enum_PN_WiFiMode_802_11n ==
                HNAP12_GetEx_PN_WiFiMode(&sTemp, HNAP12_Element_PN_Mode, HNAP12_Enum_PN_WiFiMode_802_11n))
            {
                if (eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK &&
                    eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS)
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_TYPE_NOT_SUPPORTED);
                }
                else
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_ENCRYPTION_NOT_SUPPORTED);
                }
                goto finish;
            }
        }

        /* Validate if the key if not an WPA/WPA2-RADUIS type */
        pszKey = HDK_XML_Get_String(pInput, HNAP12_Element_PN_Key);
        if (eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA_RADIUS &&
            eWiFiSecurity != HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS &&
            !s_HNAP12_ValidateEncryptionKey(pszKey, eWiFiEncryption))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_ILLEGAL_KEY_VALUE);
            goto finish;
        }
    }
    else
    {
        /* Validate type and encryption */
        if (eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity__UNKNOWN__)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_TYPE_NOT_SUPPORTED);
            goto finish;
        }
        else if (eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption__UNKNOWN__)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_ENCRYPTION_NOT_SUPPORTED);
            goto finish;
        }
    }

    /* Set the wlan radio security values. If any fail, return an error. */
    if (!(RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSecurityEnabled, pInput, PN_Enabled, HDK_XML_BuiltinType_Bool) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanType, pInput, PN_Type, HNAP12_EnumType_PN_WiFiSecurity) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanEncryption, pInput, PN_Encryption, HNAP12_EnumType_PN_WiFiEncryption) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanKey, pInput, PN_Key, HDK_XML_BuiltinType_String)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR);
    }

    /* Set the renewal if disabling or the security type is an WPA variant */
    if ((!*pfEnabled ||
         eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA_PSK || eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK ||
         eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA_RADIUS || eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS ||
         eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA_AUTO_PSK) &&
        !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanKeyRenewal, pInput, PN_KeyRenewal, HDK_XML_BuiltinType_Int))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_KEY_RENEWAL_BAD_VALUE);
        goto finish;
    }

    /* Set the radius server values if disabling or the security type is a RADIUS variant */
    if (!*pfEnabled ||
        eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA_RADIUS ||
        eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS ||
        eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_WEP_RADIUS)
    {
        if (!(RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusIP1, pInput, PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress) &&
              RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusPort1, pInput, PN_RadiusPort1, HDK_XML_BuiltinType_Int) &&
              RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusSecret1, pInput, PN_RadiusSecret1, HDK_XML_BuiltinType_String)))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_BAD_RADIUS_VALUES);
            goto finish;
        }

        if (HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_Radius2Supported))
        {
            if (!(RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusIP2, pInput, PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress) &&
                  RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusPort2, pInput, PN_RadiusPort2, HDK_XML_BuiltinType_Int) &&
                  RadioIDKeyed_ADISet(pMethodCtx, PN_WLanRadiusSecret2, pInput, PN_RadiusSecret2, HDK_XML_BuiltinType_String)))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_BAD_RADIUS_VALUES);
                goto finish;
            }
        }
        /* Make sure they're setting defaults if Radius2 is not supported and they're enabling */
        else if (*pfEnabled)
        {
            char* pszRadiusSecret2 = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadiusSecret2);
            HDK_XML_IPAddress* pRadiusIP2 = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_RadiusIP2);
            int* piRadiusPort2 = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_RadiusPort2);

            if ((pszRadiusSecret2 && strlen(pszRadiusSecret2) > 0) ||
                (pRadiusIP2 && (pRadiusIP2->a || pRadiusIP2->b || pRadiusIP2->c || pRadiusIP2->d)) ||
                (piRadiusPort2 && *piRadiusPort2 != 0))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSecurity, ERROR_BAD_RADIUS_VALUES);
                goto finish;
            }
        }
    }

finish:

    /* Free the temprorary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETWLANRADIOSECURITY__ */


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioSettings
 */

#ifdef __HNAP12_METHOD_PN_SETWLANRADIOSETTINGS__

void HNAP12_Method_PN_SetWLanRadioSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszRadioId = HDK_XML_Get_String(pInput, HNAP12_Element_PN_RadioID);
    int* pfEnabled;
    int* piChannel;
    int* piChannelWidth;
    int fChannelInvalid = 0;
    HNAP12_Enum_PN_WiFiMode* peWiFiMode;
    HDK_XML_Member* pmChannel = 0;
    HDK_XML_Member* pmRadio = 0;
    HDK_XML_Struct* psChannels;
    HDK_XML_Struct* psRadios;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, make sure this is a valid RadioID */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WLanRadioInfos, &sTemp, HNAP12_Element_PN_RadioInfos);
    psRadios = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_RadioInfos);
    if ((pmRadio = s_HNAP12_String_Exists_ArrayOfStruct(psRadios, HNAP12_Element_PN_RadioID, pszRadioId)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_RADIOID);
        goto finish;
    }

    /* Skip WiFiMode validation if the client is not enabling */
    pfEnabled = HDK_XML_Get_Bool(pInput, HNAP12_Element_PN_Enabled);
    peWiFiMode = HNAP12_Get_PN_WiFiMode(pInput, HNAP12_Element_PN_Mode);
    if (*pfEnabled)
    {
        HDK_XML_Member* pmMode = 0;
        HDK_XML_Struct* psModes;

        /* Validate the WiFiMode */
        psModes = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmRadio), HNAP12_Element_PN_SupportedModes);
        if (!peWiFiMode ||
            (pmMode = s_HNAP12_Enum_Exists_ArrayOfEnum(psModes, *peWiFiMode, HNAP12_EnumType_PN_WiFiMode)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_MODE);
            goto finish;
        }

        /*
         * If the client is setting the WiFiMode to 802.11n and WiFi security is enabled,
         * only WPA2 security and AES encryption are supported.
         */
        if (*peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11n)
        {
            RadioIDKeyed_ADIGet(pMemberCtx, PN_WLanSecurityEnabled, pInput, &sTemp, PN_Enabled, HDK_XML_BuiltinType_Bool);
            if (HDK_XML_GetEx_Bool(&sTemp, HNAP12_Element_PN_Enabled, 1))
            {
                HNAP12_Enum_PN_WiFiSecurity eType;

                RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanType, pInput, &sTemp, PN_Type, HNAP12_EnumType_PN_WiFiSecurity);
                RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanEncryption, pInput, &sTemp, PN_Encryption, HNAP12_EnumType_PN_WiFiEncryption);

                eType = HNAP12_GetEx_PN_WiFiSecurity(&sTemp, HNAP12_Element_PN_Type, HNAP12_Enum_PN_WiFiSecurity_WEP_OPEN);

                if ((eType != HNAP12_Enum_PN_WiFiSecurity_WPA2_PSK &&
                     eType != HNAP12_Enum_PN_WiFiSecurity_WPA2_RADIUS) ||
                    HNAP12_Enum_PN_WiFiEncryption_AES !=
                    HNAP12_GetEx_PN_WiFiEncryption(&sTemp, HNAP12_Element_PN_Encryption, HNAP12_Enum_PN_WiFiEncryption_TKIPORAES))
                {
                    SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_MODE);
                    goto finish;
                }
            }
        }
    }
    else if (*peWiFiMode == HNAP12_Enum_PN_WiFiMode__UNKNOWN__)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_MODE);
        goto finish;
    }

    /* Validate the channel width, return corresponding error */
    piChannelWidth = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_ChannelWidth);
    if (!(*piChannelWidth == 0 || *piChannelWidth == 20 || *piChannelWidth == 40) ||
        (*piChannelWidth == 40 &&
         !(*peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11n || *peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11bn ||
           *peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11bgn || *peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11gn ||
           *peWiFiMode == HNAP12_Enum_PN_WiFiMode_802_11an)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_CHANNEL_WIDTH);
        goto finish;
    }

    /* Validate the channel if the channel width is not 40 */
    psChannels = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmRadio), HNAP12_Element_PN_Channels);
    piChannel = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_Channel);
    if (!piChannel ||
        (*piChannelWidth != 40 &&
         (pmChannel = s_HNAP12_Int_Exists_ArrayOfInt(psChannels, *piChannel)) == 0))
    {
        /* Don't fail yet if the channel width is auto because it may be using wide channels */
        if (*piChannelWidth == 0)
        {
            fChannelInvalid = 1;
        }
        else
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_CHANNEL);
            goto finish;
        }
    }

    /* Validate the secondary channel if the channel width is Auto or 40 and the primary channel is not 0 */
    if ((*piChannelWidth == 0 || *piChannelWidth == 40) && *piChannel != 0)
    {
        int* piSecChannel;
        HDK_XML_Member* pmSecChannel = 0;
        HDK_XML_Member* pmWideChannel = 0;
        HDK_XML_Struct* psSecChannels = 0;
        HDK_XML_Struct* psWideChannels;

        /* Validate the wide channels */
        psWideChannels = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmRadio), HNAP12_Element_PN_WideChannels);
        if (!piChannel ||
            (pmWideChannel = s_HNAP12_Int_Exists_ArrayOfStruct(psWideChannels, HNAP12_Element_PN_Channel, *piChannel)) == 0)
        {
            if (*piChannelWidth == 40 || (*piChannelWidth == 0 && fChannelInvalid))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_CHANNEL);
                goto finish;
            }
        }

        /* Validate the secondary channel */
        psSecChannels = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmWideChannel), HNAP12_Element_PN_SecondaryChannels);
        piSecChannel = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_SecondaryChannel);
        if (!piSecChannel ||
            (pmSecChannel = s_HNAP12_Int_Exists_ArrayOfInt(psSecChannels, *piSecChannel)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_SECONDARY_CHANNEL);
            goto finish;
        }
    }

    /* Set the wlan radio settings values. If any fail, return an error. */
    if (!(RadioIDKeyed_ADISet(pMethodCtx, PN_WLanEnabled, pInput, PN_Enabled, HDK_XML_BuiltinType_Bool) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanMode, pInput, PN_Mode, HNAP12_EnumType_PN_WiFiMode) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSSIDBroadcast, pInput, PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanChannelWidth, pInput, PN_ChannelWidth, HDK_XML_BuiltinType_Int) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanChannel, pInput, PN_Channel, HDK_XML_BuiltinType_Int) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanQoS, pInput, PN_QoS, HDK_XML_BuiltinType_Bool) &&
          RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSecondaryChannel, pInput, PN_SecondaryChannel, HDK_XML_BuiltinType_Int)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR);
    }

    /* Set the SSID. Return an error if it fails */
    if (!RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSSID, pInput, PN_SSID, HDK_XML_BuiltinType_String))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWLanRadioSettings, ERROR_BAD_SSID);
        goto finish;
    }

finish:

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETWLANRADIOSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/SetWanSettings
 */

#ifdef __HNAP12_METHOD_PN_SETWANSETTINGS__

void HNAP12_Method_PN_SetWanSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int* piMTU;
    HNAP12_Enum_PN_WANType* peWanType;
    HDK_XML_Member* pmSupportedType = 0;
    HDK_XML_Struct* psSupportedTypes;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the SupportedTypes array from the device */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WanSupportedTypes, &sTemp, HNAP12_Element_PN_SupportedTypes);

    /* First off, validate that the device supports the WANType requested */
    psSupportedTypes = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_SupportedTypes);
    peWanType = HNAP12_Get_PN_WANType(pInput, HNAP12_Element_PN_Type);
    if (!peWanType ||
        (pmSupportedType = s_HNAP12_Enum_Exists_ArrayOfEnum(psSupportedTypes, *peWanType, HNAP12_EnumType_PN_WANType)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR_BAD_WANTYPE);
        goto finish;
    }

    /* Validate the MTU value */
    piMTU = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_MTU);

    /* If the MTU is less than 0 or between 1 and 127 or greater than 1500, return an error */
    if ((*piMTU < 0) || (*piMTU > 0 && *piMTU < 128) || (*piMTU > 1500) ||
        /* The following WANTypes have a max MTU of 1492 */
        ((*peWanType == HNAP12_Enum_PN_WANType_DHCPPPPoE || *peWanType == HNAP12_Enum_PN_WANType_StaticPPPoE ||
          *peWanType == HNAP12_Enum_PN_WANType_Static1483Bridged || *peWanType == HNAP12_Enum_PN_WANType_Dynamic1483Bridged ||
          *peWanType == HNAP12_Enum_PN_WANType_Static1483Routed || *peWanType == HNAP12_Enum_PN_WANType_StaticPPPOA ||
          *peWanType == HNAP12_Enum_PN_WANType_DynamicPPPOA || *peWanType == HNAP12_Enum_PN_WANType_StaticIPOA) &&
         (*piMTU > 1492)) ||
        /* The following WANTypes have a max MTU of 1452 */
        ((*peWanType == HNAP12_Enum_PN_WANType_StaticPPTP || *peWanType == HNAP12_Enum_PN_WANType_DynamicPPTP) &&
         (*piMTU > 1452)) ||
        /* The following WANTypes have a max MTU of 1460 */
        ((*peWanType == HNAP12_Enum_PN_WANType_StaticL2TP || *peWanType == HNAP12_Enum_PN_WANType_DynamicL2TP) &&
         (*piMTU > 1460)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
        goto finish;
    }

    /* If MTU is 0, validate that the device supports auto mtu */
    if ((*piMTU == 0) &&
        !HDK_SRV_ADIGetBool(pMethodCtx, HNAP12_ADI_PN_WanAutoMTUSupported))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR_AUTO_MTU_NOT_SUPPORTED);
        goto finish;
    }

    /*
     * If the WANType is not one of the follow, then we need to validate the username, password,
     * max idle time, & auto reconnect.
     */
    if (!(*peWanType == HNAP12_Enum_PN_WANType_DHCP || *peWanType == HNAP12_Enum_PN_WANType_Static ||
          *peWanType == HNAP12_Enum_PN_WANType_BridgedOnly || *peWanType == HNAP12_Enum_PN_WANType_Dynamic1483Bridged ||
          *peWanType == HNAP12_Enum_PN_WANType_Static1483Bridged || *peWanType == HNAP12_Enum_PN_WANType_Static1483Routed ||
          *peWanType == HNAP12_Enum_PN_WANType_StaticIPOA))
    {
        int fSetServiceNameError = 0;

        char* pszUsername;
        char* pszPassword = 0;
        int* piMaxIdleTime = 0;
        int* pfAutoReconnect = 0;

        /* If AutoReconnect is true, then MaxIdleTime must be 0 and vice-versa */
        piMaxIdleTime = HDK_XML_Get_Int(pInput, HNAP12_Element_PN_MaxIdleTime);
        pfAutoReconnect = HDK_XML_Get_Bool(pInput, HNAP12_Element_PN_AutoReconnect);

        if ((*pfAutoReconnect && *piMaxIdleTime != 0) ||
            (*piMaxIdleTime == 0 && !*pfAutoReconnect))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        pszUsername = HDK_XML_Get_String(pInput, HNAP12_Element_PN_Username);
        pszPassword = HDK_XML_Get_String(pInput, HNAP12_Element_PN_Password);

        /* Make sure the username/password are not empty */
        if (strlen(pszUsername) == 0 || strlen(pszPassword) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* Set the following values */
        if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanUsername, pInput, HNAP12_Element_PN_Username) &&
              HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanPassword, pInput, HNAP12_Element_PN_Password) &&
              HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanMaxIdleTime, pInput, HNAP12_Element_PN_MaxIdleTime) &&
              HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanAutoReconnect, pInput, HNAP12_Element_PN_AutoReconnect)))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* The service name depends further upon the WANType */
        if (*peWanType == HNAP12_Enum_PN_WANType_BigPond)
        {
            fSetServiceNameError = !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanAuthService, pInput, HNAP12_Element_PN_ServiceName);
        }
        else if (*peWanType == HNAP12_Enum_PN_WANType_DHCPPPPoE || *peWanType == HNAP12_Enum_PN_WANType_StaticPPPoE ||
                 *peWanType == HNAP12_Enum_PN_WANType_StaticPPPOA || *peWanType == HNAP12_Enum_PN_WANType_DynamicPPPOA)
        {
            fSetServiceNameError = !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanPPPoEService, pInput, HNAP12_Element_PN_ServiceName);
        }
        else if (*peWanType == HNAP12_Enum_PN_WANType_DynamicL2TP || *peWanType == HNAP12_Enum_PN_WANType_DynamicPPTP ||
                 *peWanType == HNAP12_Enum_PN_WANType_StaticL2TP || *peWanType == HNAP12_Enum_PN_WANType_StaticPPTP)
        {
            fSetServiceNameError = !HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanLoginService, pInput, HNAP12_Element_PN_ServiceName);
        }

        /* Return error if the set service name failed */
        if (fSetServiceNameError)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }
    }

    /* If WANType is one of the following, validate the IPAddress, SubnetMask, and Gateway */
    if (*peWanType == HNAP12_Enum_PN_WANType_Static || *peWanType == HNAP12_Enum_PN_WANType_StaticL2TP ||
        *peWanType == HNAP12_Enum_PN_WANType_StaticPPPoE || *peWanType == HNAP12_Enum_PN_WANType_StaticPPTP ||
        *peWanType == HNAP12_Enum_PN_WANType_Static1483Bridged || *peWanType == HNAP12_Enum_PN_WANType_Static1483Routed ||
        *peWanType == HNAP12_Enum_PN_WANType_StaticPPPOA || *peWanType == HNAP12_Enum_PN_WANType_StaticIPOA)
    {
        HDK_XML_IPAddress* pIPAddr = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_IPAddress);
        HDK_XML_IPAddress* pSubnet = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_SubnetMask);
        HDK_XML_IPAddress* pGateway = HDK_XML_Get_IPAddress(pInput, HNAP12_Element_PN_Gateway);

        /* Validate the format of the SubnetMask */
        if (!HNAP12_UTL_IPAddress_IsValidSubnet(pSubnet))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* If it's a static type, then verify that the gateway is in the subnet */
        if (*peWanType == HNAP12_Enum_PN_WANType_Static &&
            !HNAP12_UTL_IPAddress_IsWithinSubnet(pIPAddr, pSubnet, pGateway))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* Validate the IPAddress and gateway */
        if (!HNAP12_UTL_IPAddress_IsValid(pIPAddr, pSubnet) ||
            !HNAP12_UTL_IPAddress_IsValid(pGateway, pSubnet))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* Set the IPAddress */
        if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanIPAddress, pInput, HNAP12_Element_PN_IPAddress))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }

        /* If the WANType is not the following, then set SubnetMask and Gateway */
        if (*peWanType != HNAP12_Enum_PN_WANType_StaticPPPoE &&
            *peWanType != HNAP12_Enum_PN_WANType_StaticPPPOA &&
            !(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanSubnetMask, pInput, HNAP12_Element_PN_SubnetMask) &&
              HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanGateway, pInput, HNAP12_Element_PN_Gateway)))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
            goto finish;
        }
    }

    /* Handle the DNS settings validation */
    {
        HDK_XML_Struct* psDNS = HDK_XML_Get_Struct(pInput, HNAP12_Element_PN_DNS);
        HDK_XML_IPAddress* pIPPrimary = HDK_XML_Get_IPAddress(psDNS, HNAP12_Element_PN_Primary);

        if (*peWanType == HNAP12_Enum_PN_WANType_Static)
        {
            /* DNS settings must have at lease a primary IPAddress for this type */
            if (!(pIPPrimary->a || pIPPrimary->b || pIPPrimary->c || pIPPrimary->d))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
                goto finish;
            }
        }
        else if (*peWanType == HNAP12_Enum_PN_WANType_BridgedOnly)
        {
            HDK_XML_IPAddress ipDefault = {0,0,0,0};
            HDK_XML_IPAddress* pIPSecondary = HDK_XML_Get_IPAddress(psDNS, HNAP12_Element_PN_Secondary);
            /* Tertiary is optional, so use the 'Ex' form of get with a default IPAddress */
            const HDK_XML_IPAddress* pIPTertiary = HDK_XML_GetEx_IPAddress(psDNS, HNAP12_Element_PN_Tertiary, &ipDefault);

            /* DNS settings must all be blank for this type */
            if ((pIPPrimary->a || pIPPrimary->b || pIPPrimary->c || pIPPrimary->d) ||
                (pIPSecondary->a || pIPSecondary->b || pIPSecondary->c || pIPSecondary->d) ||
                (pIPTertiary->a || pIPTertiary->b || pIPTertiary->c || pIPTertiary->d))
            {
                SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
                goto finish;
            }
        }
    }

    /* Attempt to set the following device values, returning an error if any fail */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanType, pInput, HNAP12_Element_PN_Type) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanMTU, pInput, HNAP12_Element_PN_MTU) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanDNS, pInput, HNAP12_Element_PN_DNS) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WanMacAddress, pInput, HNAP12_Element_PN_MacAddress)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWanSettings, ERROR);
        goto finish;
    }

finish:

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETWANSETTINGS__ */


/*
 * Method http://purenetworks.com/HNAP1/SetWirelessClientSettings
 */

#ifdef __HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__

void HNAP12_Method_PN_SetWirelessClientSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int* pfEnabled;
    HNAP12_Enum_PN_WiFiClientNetworkType eWiFiNetworkType;
    HDK_XML_Member* pmNetworkType = 0;
    HDK_XML_Struct* psNetworkTypes;
    HDK_XML_Struct sTemp;

    /* Initialize the temp struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Validate network type */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSupportedNetworkType, &sTemp, HNAP12_Element_PN_SupportedNetworkType);
    psNetworkTypes = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_SupportedNetworkType);
    eWiFiNetworkType = HNAP12_GetEx_PN_WiFiClientNetworkType(pInput, HNAP12_Element_PN_NetworkType, HNAP12_Enum_PN_WiFiClientNetworkType__UNKNOWN__);
    if ((pmNetworkType = s_HNAP12_Enum_Exists_ArrayOfEnum(psNetworkTypes, eWiFiNetworkType, HNAP12_EnumType_PN_WiFiClientNetworkType)) == 0)
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWirelessClientSettings, ERROR_NETWORK_TYPE_NOT_SUPPORTED);
        goto finish;
    }

    /* Skip WiFiSecurity and WiFiEncryption validation if the client is not enabling */
    pfEnabled = HDK_XML_Get_Bool(pInput, HNAP12_Element_PN_SecurityEnabled);
    if (*pfEnabled)
    {
        char* pszKey;
        HNAP12_Enum_PN_WiFiSecurity eWiFiSecurity;
        HNAP12_Enum_PN_WiFiEncryption eWiFiEncryption;
        HDK_XML_Member* pmEncryption = 0;
        HDK_XML_Member* pmSecurityInfo = 0;
        HDK_XML_Struct* psEncryptions;
        HDK_XML_Struct* psSecurityInfos;

        /* Validate security type */
        HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSupportedSecurity, &sTemp, HNAP12_Element_PN_SupportedSecurity);
        psSecurityInfos = HDK_XML_Get_Struct(&sTemp, HNAP12_Element_PN_SupportedSecurity);
        eWiFiSecurity = HNAP12_GetEx_PN_WiFiSecurity(pInput, HNAP12_Element_PN_SecurityType, HNAP12_Enum_PN_WiFiSecurity__UNKNOWN__);
        if (eWiFiSecurity == HNAP12_Enum_PN_WiFiSecurity_ ||
            (pmSecurityInfo = s_HNAP12_Enum_Exists_ArrayOfStruct(psSecurityInfos, HNAP12_Element_PN_SecurityType,
                                                           eWiFiSecurity, HNAP12_EnumType_PN_WiFiSecurity)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWirelessClientSettings, ERROR_TYPE_NOT_SUPPORTED);
            goto finish;
        }

        /* Validate encryption */
        psEncryptions = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmSecurityInfo), HNAP12_Element_PN_Encryptions);
        eWiFiEncryption = HNAP12_GetEx_PN_WiFiEncryption(pInput, HNAP12_Element_PN_Encryption, HNAP12_Enum_PN_WiFiEncryption__UNKNOWN__);
        if (eWiFiEncryption == HNAP12_Enum_PN_WiFiEncryption_ ||
            (pmEncryption = s_HNAP12_Enum_Exists_ArrayOfEnum(psEncryptions, eWiFiEncryption, HNAP12_EnumType_PN_WiFiEncryption)) == 0)
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWirelessClientSettings, ERROR_ENCRYPTION_NOT_SUPPORTED);
            goto finish;
        }

        /* Validate if the key */
        pszKey = HDK_XML_Get_String(pInput, HNAP12_Element_PN_Key);
        if (!s_HNAP12_ValidateEncryptionKey(pszKey, eWiFiEncryption))
        {
            SetHNAPResult(pOutput, HNAP12, PN_SetWirelessClientSettings, ERROR_ILLEGAL_KEY_VALUE);
            goto finish;
        }
    }

    /* Set the WirelessClientSettings values. If any fail, return an error. */
    if (!(HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSSID, pInput, HNAP12_Element_PN_SSID) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientNetworkType, pInput, HNAP12_Element_PN_NetworkType) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSecurityEnabled, pInput, HNAP12_Element_PN_SecurityEnabled) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientSecurityType, pInput, HNAP12_Element_PN_SecurityType) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientEncryption, pInput, HNAP12_Element_PN_Encryption) &&
          HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_WiFiClientKey, pInput, HNAP12_Element_PN_Key)))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetWirelessClientSettings, ERROR);
    }

finish:

    /* Free the temprorary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HNAP12_METHOD_PN_SETWIRELESSCLIENTSETTINGS__ */


/*
 * Method http://cisco.com/HNAP/Transaction
 */

#ifdef __HNAP12_METHOD_TRANSACTION__

void HNAP12_Method_Transaction(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* pRequests;
    HDK_XML_Member* pRequest;
    HDK_XML_Struct* pResponses;

    /* Create the responses array */
    pResponses = HDK_XML_Set_Struct(pOutput, HNAP12_Element_Responses);
    if (!pResponses)
    {
        SetHNAPResult(pOutput, HNAP12, Transaction, ERROR);
        return;
    }

    /* Iterate the transaction requests */
    pRequests = HDK_XML_Get_Struct(pInput, HNAP12_Element_Requests);
    for (pRequest = pRequests->pHead; pRequest; pRequest = pRequest->pNext)
    {
        const HDK_MOD_Method* pMethod = 0;
        HDK_SRV_ModuleContext** ppModuleCtx;

        /* Get the action URI and action request */
        const char* pszAction = HDK_XML_Get_String((HDK_XML_Struct*)pRequest, HNAP12_Element_Action);
        const char* pszRequest = HDK_XML_Get_String((HDK_XML_Struct*)pRequest, HNAP12_Element_Request);

        /* Find the action */
        for (ppModuleCtx = pMethodCtx->ppModuleCtxs; *ppModuleCtx; ++ppModuleCtx)
        {
            const HDK_MOD_Method* pMethodCur;
            for (pMethodCur = (*ppModuleCtx)->pModule->pMethods; pMethodCur->pfnMethod; ++pMethodCur)
            {
                if (pMethodCur->pszSOAPAction && strcmp(pszAction, pMethodCur->pszSOAPAction) == 0)
                {
                    pMethod = pMethodCur;
                    break;
                }
            }
            if (pMethod)
            {
                break;
            }
        }
        if (!pMethod)
        {
            SetHNAPResult(pOutput, HNAP12, Transaction, ERROR_INVALID_ACTION);
            break;
        }
        else
        {
            HDK_XML_InputStream_BufferContext inputCtx;
            HDK_XML_OutputStream_BufferContext outputCtx;
            int fSuccess;
            HDK_XML_Struct* pResponse;

            /* Input buffer stream */
            memset(&inputCtx, 0, sizeof(inputCtx));
            inputCtx.pBuf = pszRequest;
            inputCtx.cbBuf = (unsigned int)strlen(pszRequest);

            /* Output buffer stream */
            memset(&outputCtx, 0, sizeof(outputCtx));
            outputCtx.cbBuf = 1024;

            /* Execute the action */
            fSuccess = HDK_SRV_CallMethod(0 /* pServerCtx */,
                                          (int)(pMethod - (*ppModuleCtx)->pModule->pMethods),
                                          *ppModuleCtx,
                                          pMethodCtx->ppModuleCtxs,
                                          0 /* cbContentLength */,
                                          HDK_XML_InputStream_Buffer,
                                          &inputCtx,
                                          HDK_XML_OutputStream_GrowBuffer,
                                          &outputCtx,
                                          0 /* pszOutpuPrefix */,
                                          0 /* pfnHNAPResult */,
                                          0 /* cbMaxAlloc */,
                                          1 /* fNoHeaders */);

            /* Add the result */
            pResponse = HDK_XML_Append_Struct(pResponses, HNAP12_Element_TransactionResponse);
            if (pResponse)
            {
                HDK_XML_Set_String(pResponse, HNAP12_Element_Action, pszAction);
                if (outputCtx.pBuf)
                {
                    outputCtx.pBuf[outputCtx.ixCur] = '\0';
                    HDK_XML_Set_String(pResponse, HNAP12_Element_Response, outputCtx.pBuf);
                }
                else
                {
                    HDK_XML_Set_String(pResponse, HNAP12_Element_Response, "");
                }
            }

            /* Free the response string */
            free(outputCtx.pBuf);

            /* Error? */
            if (!pResponse || !fSuccess)
            {
                SetHNAPResult(pOutput, HNAP12, Transaction, ERROR);
                break;
            }
        }
    }
}

#endif /* __HNAP12_METHOD_TRANSACTION__ */


/*
 * Method http://purenetworks.com/HNAP1/GetConfigBlob
 */

#ifdef __HNAP12_METHOD_PN_GETCONFIGBLOB__

void HNAP12_Method_PN_GetConfigBlob(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused variables */
    (void) pInput;

    /* Get the config blob. */
    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_ConfigBlob, pOutput, HNAP12_Element_PN_ConfigBlob);
}

#endif /* __HNAP12_METHOD_PN_GETCONFIGBLOB__ */


/*
 * Method http://purenetworks.com/HNAP1/SetConfigBlob
 */

#ifdef __HNAP12_METHOD_PN_SETCONFIGBLOB__

void HNAP12_Method_PN_SetConfigBlob(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Set the config blob. If it fails, return an error. */
    if (!HDK_SRV_ADISet(pMethodCtx, HNAP12_ADI_PN_ConfigBlob, pInput, HNAP12_Element_PN_ConfigBlob))
    {
        SetHNAPResult(pOutput, HNAP12, PN_SetConfigBlob, ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_SETCONFIGBLOB__ */


/*
 * Method http://purenetworks.com/HNAP1/RestoreFactoryDefaults
 */

#ifdef __HNAP12_METHOD_PN_RESTOREFACTORYDEFAULTS__

void HNAP12_Method_PN_RestoreFactoryDefaults(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused variables */
    (void) pInput;
    (void) pOutput;

    HDK_SRV_ADIGet(pMethodCtx, HNAP12_ADI_PN_FactoryRestoreTrigger, 0, HNAP12_Element_PN_FactoryRestoreTrigger);
}

#endif /* __HNAP12_METHOD_PN_RESTOREFACTORYDEFAULTS__ */
