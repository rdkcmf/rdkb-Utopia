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

/*
 * hotspot_methods.c - HNAP callback function definitions for the HotSpot HNAP extensions.
 */

#include "hotspot.h"

#include "hnap12_util.h"
#include "hdk_srv.h"

#include <stdio.h>
#include <string.h>


#ifndef __cplusplus
/* Function declaration so that we can compile as ansi c */
int strcasecmp(const char *str1, const char *str2);
#endif

/* Helper method for HOTSPOT results */
#define SetHNAPResult(pStruct, prefix, method, result)                    \
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)

/* Helper method for doing an ADIGet on RadioID indexed values */
#define RadioIDKeyed_ADIGet(pMemberCtx, adiElement, pInput, pOutput, valueElement, valueType) \
    HDK_SRV_ADIGetValue_ByString(pMethodCtx, HOTSPOT_ADI_##adiElement, pOutput, pInput, \
                                 HOTSPOT_Element_PN_RadioID, HOTSPOT_Element_##valueElement, valueType)

/* Helper method for doing an ADISet on RadioID indexed values */
#define RadioIDKeyed_ADISet(pMethodCtx, adiElement, pInput, valueElement, valueType) \
    HDK_SRV_ADISetValue_ByString(pMethodCtx, HOTSPOT_ADI_##adiElement, pInput, HOTSPOT_Element_##adiElement##Info, \
                                 HOTSPOT_Element_PN_RadioID, HOTSPOT_Element_##valueElement, valueType)


#if defined(__HOTSPOT_METHOD_CISCO_HOTSPOT_SETDEFAULTWIRELESS__) || \
    defined(__HOTSPOT_METHOD_CISCO_HOTSPOT_SETGUESTNETWORK__)

/* Helper function for SetGuestNetwork */
static int HOTSPOT_Method_Cisco_HotSpot_SetGuestNetwork_Helper(
    HDK_MOD_MethodContext* pMethodCtx,
    int fEnabled,
    char* pszSSID,
    char* pszPassword,
    int iMaxGuestsAllowed,
    int fDefaultWireless)
{
    HDK_XML_Struct sTemp;
    int iMaxGuestsDeviceAllows;
    int eResult = fDefaultWireless ?
        (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_OK :
        (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_OK;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Validate the guest ssid does not collide with any private ssids */
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_PN_WLanRadioInfos, &sTemp, HOTSPOT_Element_PN_WLanRadioInfos))
    {
        HDK_XML_Struct* psRadioInfos = HDK_XML_Get_Struct(&sTemp, HOTSPOT_Element_PN_WLanRadioInfos);
        if (psRadioInfos)
        {
            HDK_XML_Member* pRadioInfo;
            for (pRadioInfo = psRadioInfos->pHead; pRadioInfo; pRadioInfo = pRadioInfo->pNext)
            {
                char* pszSSIDPrivate;
                HDK_XML_Struct* psRadioInfo = HDK_XML_GetMember_Struct(pRadioInfo);

                RadioIDKeyed_ADIGet(pMemberCtx, PN_WLanSSID, psRadioInfo, &sTemp, PN_SSID, HDK_XML_BuiltinType_String);
                pszSSIDPrivate = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_PN_SSID);

                if (pszSSIDPrivate && strcmp(pszSSIDPrivate, pszSSID) == 0)
                {
                    eResult = fDefaultWireless ?
                        (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_ERROR_INVALID_SSID :
                        (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_ERROR_INVALID_SSID;

                    goto exit;
                }
            }
        }
    }

    /* Validate and set the SSID */
    HDK_XML_Set_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_SSID, pszSSID);
    if (!HNAP12_UTL_SSID_IsValid(pszSSID) ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_SSID, &sTemp, HOTSPOT_Element_Cisco_HotSpot_SSID))
    {
        eResult = fDefaultWireless ?
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_ERROR_INVALID_SSID :
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_ERROR_INVALID_SSID;

        goto exit;
    }

    /* Validate and set the password */
    HDK_XML_Set_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_Password, pszPassword);
    if (!HNAP12_UTL_Password_IsValid(pszPassword) ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_Password, &sTemp, HOTSPOT_Element_Cisco_HotSpot_Password))
    {
        eResult = fDefaultWireless ?
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_ERROR_INVALID_PASSWORD :
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_ERROR_INVALID_PASSWORD;

        goto exit;
    }

    /* Validate and set the max guests allowed */
    HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_MaxGuestsDeviceAllows, &sTemp, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed);
    iMaxGuestsDeviceAllows = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed, 10);
    HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed, iMaxGuestsAllowed);
    if (iMaxGuestsAllowed <= 0 || iMaxGuestsAllowed > iMaxGuestsDeviceAllows ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_MaxGuestsAllowed, &sTemp, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed))
    {
        eResult = fDefaultWireless ?
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_ERROR_INVALID_MAXGUESTSALLOWED :
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_ERROR_INVALID_MAXGUESTSALLOWED;

        goto exit;
    }

    /* Set the remaining values */
    HDK_XML_Set_Bool(&sTemp, HOTSPOT_Element_Cisco_HotSpot_Enabled, fEnabled);
    if (!HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_Enabled, &sTemp, HOTSPOT_Element_Cisco_HotSpot_Enabled))
    {
        eResult = fDefaultWireless ?
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_ERROR :
            (int)HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_ERROR;
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);

    return eResult;
}

#endif

/*
 * http://cisco.com/HNAPExt/HotSpot/CheckParentalControlsPassword
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_CHECKPARENTALCONTROLSPASSWORD__

void HOTSPOT_Method_Cisco_HotSpot_CheckParentalControlsPassword(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszPassword = 0;
    char* pszInputPassword;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the password */
    HDK_XML_Struct_Init(&sTemp);
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, &sTemp, HOTSPOT_Element_Cisco_HotSpot_NewPassword))
    {
        pszPassword = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_NewPassword);
    }

    /* Compare the input password and the password */
    pszInputPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Password);
    if (!pszPassword || strcmp(pszPassword, pszInputPassword) != 0)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_CheckParentalControlsPassword, ERROR_INCORRECT_PASSWORD);
        goto exit;
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_CHECKPARENTALCONTROLSPASSWORD__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/GetDeviceInfo
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETDEVICEINFO__

void HOTSPOT_Method_Cisco_HotSpot_GetDeviceInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the device info structures */
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_DeviceList_DeviceInfos, pOutput, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos))
    {
        /* Provide reasonable default */
        HDK_XML_Set_Struct(pOutput, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETDEVICEINFO */


/*
 * http://cisco.com/HNAPExt/HotSpot/GetGuestNetwork
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETGUESTNETWORK__

void HOTSPOT_Method_Cisco_HotSpot_GetGuestNetwork(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char pszGuestSSID[33] = {'\0'};
    HDK_XML_Struct sTemp;
    int fRadio24GHzEnabled = 0;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Return the guest network settings */
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_Enabled, pOutput, HOTSPOT_Element_Cisco_HotSpot_Enabled))
    {
        /* Provide reasonable default */
        HDK_XML_Set_Bool(pOutput, HOTSPOT_Element_Cisco_HotSpot_Enabled, 0);
    }
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_Password, pOutput, HOTSPOT_Element_Cisco_HotSpot_Password))
    {
        /* Provide reasonable default */
        HDK_XML_Set_String(pOutput, HOTSPOT_Element_Cisco_HotSpot_Password, "guest");
    }
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_MaxGuestsAllowed, pOutput, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed))
    {
        /* Provide reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed, 5);
    }

    HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_SSID, pOutput, HOTSPOT_Element_Cisco_HotSpot_SSID);

    /* Is the 2.4 GHz radio enabled? */
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_PN_WLanRadioInfos, &sTemp, HOTSPOT_Element_PN_WLanRadioInfos))
    {
        HDK_XML_Struct* psRadioInfos = HDK_XML_Get_Struct(&sTemp, HOTSPOT_Element_PN_WLanRadioInfos);
        if (psRadioInfos)
        {
            HDK_XML_Member* pRadioInfo;
            for (pRadioInfo = psRadioInfos->pHead; pRadioInfo; pRadioInfo = pRadioInfo->pNext)
            {
                HDK_XML_Struct* psRadioInfo = HDK_XML_GetMember_Struct(pRadioInfo);
                int radioFrequency = HDK_XML_GetEx_Int(psRadioInfo, HOTSPOT_Element_PN_Frequency, 0);
                if (radioFrequency == 2)
                {
                    RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanEnabled, psRadioInfo, &sTemp, PN_Enabled, HDK_XML_BuiltinType_Bool);
                    fRadio24GHzEnabled = HDK_XML_GetEx_Bool(&sTemp, HOTSPOT_Element_PN_Enabled, 0);

                    /* Get the wireless guest SSID, if we don't have one */
                    if (!HDK_XML_Get_String(pOutput, HOTSPOT_Element_Cisco_HotSpot_SSID))
                    {
                        char* pszSSID;

                        RadioIDKeyed_ADIGet(pMethodCtx, PN_WLanSSID, psRadioInfo, &sTemp, PN_SSID, HDK_XML_BuiltinType_String);
                        pszSSID = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_PN_SSID);

                        /* Format and set a reasonable default guest ssid */
                        strncpy(pszGuestSSID, pszSSID ? pszSSID : "SSID", 26);
                        strcat(pszGuestSSID, "-guest");
                    }
                    break;
                }
            }
        }
    }

    /* Set the can-be-active state */
    HDK_XML_Set_Bool(pOutput, HOTSPOT_Element_Cisco_HotSpot_CanBeActive, fRadio24GHzEnabled);

    /* Set a reasonable default for SSID */
    if (!HDK_XML_Get_String(pOutput, HOTSPOT_Element_Cisco_HotSpot_SSID))
    {
        HDK_XML_Set_String(pOutput, HOTSPOT_Element_Cisco_HotSpot_SSID, *pszGuestSSID ? pszGuestSSID : "SSID-guest");
    }

    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETGUESTNETWORK__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/GetGuestNetworkLANSettings
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETGUESTNETWORKLANSETTINGS__

void HOTSPOT_Method_Cisco_HotSpot_GetGuestNetworkLANSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the guest network LAN settings */
    if (!(HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_IPAddress, pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddress) &&
          HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_SubnetMask, pOutput, HOTSPOT_Element_Cisco_HotSpot_SubnetMask) &&
          HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_IPAddressFirst, pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddressFirst) &&
          HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_GuestNetwork_IPAddressLast, pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddressLast)))
    {
        HDK_XML_IPAddress ipAddress = {192,168,33,1};
        HDK_XML_IPAddress subnetMask = {255,255,255,0};
        HDK_XML_IPAddress ipFirst = {192,168,33,100};
        HDK_XML_IPAddress ipLast = {192,168,33,149};

        /* Provide reasonable defaults */
        HDK_XML_Set_IPAddress(pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddress, &ipAddress);
        HDK_XML_Set_IPAddress(pOutput, HOTSPOT_Element_Cisco_HotSpot_SubnetMask, &subnetMask);
        HDK_XML_Set_IPAddress(pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddressFirst, &ipFirst);
        HDK_XML_Set_IPAddress(pOutput, HOTSPOT_Element_Cisco_HotSpot_IPAddressLast, &ipLast);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETGUESTNETWORKLANSETTINGS__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/GetWANAccesStatuses
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETWANACCESSSTATUSES__

void HOTSPOT_Method_Cisco_HotSpot_GetWANAccessStatuses(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the WANAccessStatus structures */
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_WANAccessStatuses, pOutput, HOTSPOT_Element_Cisco_HotSpot_WANAccessStatuses))
    {
        /* Provide reasonable default */
        HDK_XML_Set_Struct(pOutput, HOTSPOT_Element_Cisco_HotSpot_WANAccessStatuses);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETWANACCESSSTATUS__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/HasParentalControlsPassword
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_HASPARENTALCONTROLSPASSWORD__

void HOTSPOT_Method_Cisco_HotSpot_HasParentalControlsPassword(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszPassword = 0;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the old WAN access password */
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, &sTemp, HOTSPOT_Element_Cisco_HotSpot_OldPassword))
    {
        pszPassword = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_OldPassword);
    }

    /* Set the result */
    HDK_XML_Set_Bool(pOutput, HOTSPOT_Element_Cisco_HotSpot_HasPassword, pszPassword != 0);

    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_HASPARENTALCONTROLSPASSWORD__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/SetDefaultWireless
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETDEFAULTWIRELESS__

void HOTSPOT_Method_Cisco_HotSpot_SetDefaultWireless(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszSSID;
    char* pszKey;
    int fGuestEnabled;
    char* pszGuestSSID;
    char* pszGuestPassword;
    int maxGuestsAllowed;
    int eResult;

    /* Initialize the temporary struct */
    HDK_XML_Struct_Init(&sTemp);

    /* Validate the SSID */
    pszSSID = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_SSID);
    if (!HNAP12_UTL_SSID_IsValid(pszSSID))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDefaultWireless, ERROR_INVALID_SSID);
        goto exit;
    }

    /* Validate the Key */
    pszKey = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Key);
    if (!HNAP12_UTL_WPAKey_IsValid(pszKey, 0) ||
        !HNAP12_UTL_AdminPassword_IsValid(pszKey))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDefaultWireless, ERROR_ILLEGAL_KEY_VALUE);
        goto exit;
    }

    /* Iterate the radios */
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_PN_WLanRadioInfos, &sTemp, HOTSPOT_Element_PN_WLanRadioInfos))
    {
        HDK_XML_Struct* psRadioInfos = HDK_XML_Get_Struct(&sTemp, HOTSPOT_Element_PN_WLanRadioInfos);
        if (psRadioInfos)
        {
            HDK_XML_Member* pRadioInfo;
            for (pRadioInfo = psRadioInfos->pHead; pRadioInfo; pRadioInfo = pRadioInfo->pNext)
            {
                HDK_XML_Struct* psRadioInfo = HDK_XML_GetMember_Struct(pRadioInfo);
                char* pszRadioID = HDK_XML_Get_String(psRadioInfo, HOTSPOT_Element_PN_RadioID);
                int iFrequency = HDK_XML_GetEx_Int(psRadioInfo, HOTSPOT_Element_PN_Frequency, 2);

                /* Build the device value temporary struct */
                if (!HDK_XML_Set_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_Key, pszKey) ||
                    !HDK_XML_Set_String(&sTemp, HOTSPOT_Element_PN_RadioID, pszRadioID) ||
#if defined(__HOTSPOT_METHOD_PN_SETWLANRADIOFREQUENCY__)
                    !HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_PN_Frequency, 2) ||
#endif
                    !HDK_XML_Set_Bool(&sTemp, HOTSPOT_Element_PN_Enabled, 1) ||
                    !HOTSPOT_Set_PN_WiFiMode(&sTemp, HOTSPOT_Element_PN_Mode, iFrequency == 2 ?
                                             HOTSPOT_Enum_PN_WiFiMode_802_11bgn : HOTSPOT_Enum_PN_WiFiMode_802_11an) ||
                    !HDK_XML_Set_String(&sTemp, HOTSPOT_Element_PN_SSID, pszSSID) ||
                    !HDK_XML_Set_Bool(&sTemp, HOTSPOT_Element_PN_SSIDBroadcast, 1) ||
                    !HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_PN_ChannelWidth, iFrequency == 2 ? 20 : 0) ||
                    !HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_PN_Channel, 0) ||
                    !HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_PN_SecondaryChannel, 0) ||
                    !HDK_XML_Set_Bool(&sTemp, HOTSPOT_Element_PN_QoS, 1) ||
                    !HDK_XML_Set_Bool(&sTemp, HOTSPOT_Element_PN_Enabled, 1) ||
                    !HOTSPOT_Set_PN_WiFiSecurity(&sTemp, HOTSPOT_Element_PN_Type, HOTSPOT_Enum_PN_WiFiSecurity_WPA_AUTO_PSK) ||
                    !HOTSPOT_Set_PN_WiFiEncryption(&sTemp, HOTSPOT_Element_PN_Encryption, HOTSPOT_Enum_PN_WiFiEncryption_TKIPORAES) ||
                    !HDK_XML_Set_String(&sTemp, HOTSPOT_Element_PN_Key, pszKey) ||
                    !HDK_XML_Set_Int(&sTemp, HOTSPOT_Element_PN_KeyRenewal, 3600))
                {
                    SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDefaultWireless, ERROR);
                    goto exit;
                }

                /* Set the device values. If any fail, return an error. */
                if (!HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_PN_AdminPassword, &sTemp, HOTSPOT_Element_Cisco_HotSpot_Key) ||
#if defined(__HOTSPOT_METHOD_PN_SETWLANRADIOFREQUENCY__)
                    !RadioID_Keyed_ADISet(pMethodCtx, PN_WLanFrequency, &sTemp, PN_Frequency, HDK_XML_BuiltinType_Int) ||
#endif
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanEnabled, &sTemp, PN_Enabled, HDK_XML_BuiltinType_Bool) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanMode, &sTemp, PN_Mode, HOTSPOT_EnumType_PN_WiFiMode) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSSID, &sTemp, PN_SSID, HDK_XML_BuiltinType_String) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSSIDBroadcast, &sTemp, PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanChannelWidth, &sTemp, PN_ChannelWidth, HDK_XML_BuiltinType_Int) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanChannel, &sTemp, PN_Channel, HDK_XML_BuiltinType_Int) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanQoS, &sTemp, PN_QoS, HDK_XML_BuiltinType_Bool) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSecondaryChannel, &sTemp, PN_SecondaryChannel, HDK_XML_BuiltinType_Int) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanSecurityEnabled, &sTemp, PN_Enabled, HDK_XML_BuiltinType_Bool) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanType, &sTemp, PN_Type, HOTSPOT_EnumType_PN_WiFiSecurity) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanEncryption, &sTemp, PN_Encryption, HOTSPOT_EnumType_PN_WiFiEncryption) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanKey, &sTemp, PN_Key, HDK_XML_BuiltinType_String) ||
                    !RadioIDKeyed_ADISet(pMethodCtx, PN_WLanKeyRenewal, &sTemp, PN_KeyRenewal, HDK_XML_BuiltinType_Int))
                {
                    SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDefaultWireless, ERROR);
                    goto exit;
                }
            }
        }
    }

    /* Validate and set the guest network settings */
    fGuestEnabled = *HDK_XML_Get_Bool(pInput, HOTSPOT_Element_Cisco_HotSpot_GuestEnabled);
    pszGuestSSID = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_GuestSSID);
    pszGuestPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_GuestPassword);
    maxGuestsAllowed = *HDK_XML_Get_Int(pInput, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed);
    if ((eResult = HOTSPOT_Method_Cisco_HotSpot_SetGuestNetwork_Helper(
             pMethodCtx,
             fGuestEnabled,
             pszGuestSSID,
             pszGuestPassword,
             maxGuestsAllowed,
             1)) != HOTSPOT_Enum_Cisco_HotSpot_SetDefaultWirelessResult_OK)
    {
        HOTSPOT_Set_Cisco_HotSpot_SetDefaultWirelessResult(pOutput, HOTSPOT_Element_Cisco_HotSpot_SetDefaultWirelessResult, eResult);
    }

exit:
    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETDEFAULTWIRELESS__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/SetDeviceInfo
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETDEVICEINFO__

void HOTSPOT_Method_Cisco_HotSpot_SetDeviceInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    HDK_XML_Struct* psDeviceInfos;
    HDK_XML_Member* pDeviceInfo;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the device info structures */
    HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_DeviceList_DeviceInfos, &sTemp, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos);

    /* Iterate the input device infos */
    psDeviceInfos = HDK_XML_Get_Struct(pInput, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos);
    for (pDeviceInfo = psDeviceInfos->pHead; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext)
    {
        HDK_XML_Struct* psDeviceInfo = HDK_XML_GetMember_Struct(pDeviceInfo);
        HDK_XML_MACAddress* pMACAddress = HDK_XML_Get_MACAddress(psDeviceInfo, HOTSPOT_Element_Cisco_HotSpot_MACAddress);

        /* Is the MAC address present? */
        int fFound = 0;
        HDK_XML_Struct* psDeviceInfos2 = HDK_XML_Get_Struct(&sTemp, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos);
        HDK_XML_Member* pDeviceInfo2;
        if (psDeviceInfos2)
        {
            for (pDeviceInfo2 = psDeviceInfos2->pHead; pDeviceInfo2; pDeviceInfo2 = pDeviceInfo2->pNext)
            {
                HDK_XML_Struct* psDeviceInfo2 = HDK_XML_GetMember_Struct(pDeviceInfo2);
                HDK_XML_MACAddress* pMACAddress2 = HDK_XML_Get_MACAddress(psDeviceInfo2, HOTSPOT_Element_Cisco_HotSpot_MACAddress);
                if (pMACAddress2 && memcmp(pMACAddress, pMACAddress2, sizeof(*pMACAddress)) == 0)
                {
                    fFound = 1;
                }
            }
        }
        if (!fFound)
        {
            SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDeviceInfo, ERROR_UNKNOWN_MACADDRESS);
            goto exit;
        }

        /* Is the MAC address specified twice? */
        fFound = 0;
        for (pDeviceInfo2 = psDeviceInfos->pHead; pDeviceInfo2; pDeviceInfo2 = pDeviceInfo2->pNext)
        {
            if (pDeviceInfo != pDeviceInfo2)
            {
                HDK_XML_Struct* psDeviceInfo2 = HDK_XML_GetMember_Struct(pDeviceInfo2);
                HDK_XML_MACAddress* pMACAddress2 = HDK_XML_Get_MACAddress(psDeviceInfo2, HOTSPOT_Element_Cisco_HotSpot_MACAddress);
                if (memcmp(pMACAddress, pMACAddress2, sizeof(*pMACAddress)) == 0)
                {
                    fFound = 1;
                }
            }
        }
        if (fFound)
        {
            SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDeviceInfo, ERROR);
            goto exit;
        }
    }

    /* Set the device infos */
    if (!HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_DeviceList_DeviceInfos, pInput, HOTSPOT_Element_Cisco_HotSpot_DeviceInfos))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetDeviceInfo, ERROR);
        goto exit;
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETDEVICEINFO__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/SetGuestNetwork
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETGUESTNETWORK__

void HOTSPOT_Method_Cisco_HotSpot_SetGuestNetwork(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int eResult;
    int fEnabled = *HDK_XML_Get_Bool(pInput, HOTSPOT_Element_Cisco_HotSpot_Enabled);
    char* pszSSID = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_SSID);
    char* pszPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Password);
    int maxGuestsAllowed = *HDK_XML_Get_Int(pInput, HOTSPOT_Element_Cisco_HotSpot_MaxGuestsAllowed);

    if ((eResult = HOTSPOT_Method_Cisco_HotSpot_SetGuestNetwork_Helper(
             pMethodCtx,
             fEnabled,
             pszSSID,
             pszPassword,
             maxGuestsAllowed,
             0)) != HOTSPOT_Enum_Cisco_HotSpot_SetGuestNetworkResult_OK)
    {
        HOTSPOT_Set_Cisco_HotSpot_SetGuestNetworkResult(pOutput, HOTSPOT_Element_Cisco_HotSpot_SetGuestNetworkResult, eResult);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETGUESTNETWORK__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/SetParentalControlsPassword
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETPARENTALCONTROLSPASSWORD__

void HOTSPOT_Method_Cisco_HotSpot_SetParentalControlsPassword(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszPassword = 0;
    char* pszOldPassword = 0;
    char* pszNewPassword = 0;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the old WAN access password */
    HDK_XML_Struct_Init(&sTemp);
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, &sTemp, HOTSPOT_Element_Cisco_HotSpot_Password))
    {
        pszPassword = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_Password);
    }

    /* Compare the old password and the current password */
    pszOldPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_OldPassword);
    if (pszPassword && strcmp(pszPassword, pszOldPassword) != 0)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetParentalControlsPassword, ERROR_INCORRECT_PASSWORD);
        goto exit;
    }

    /* Validate and set the new password */
    pszNewPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_NewPassword);
    if (!HNAP12_UTL_Password_IsValid(pszNewPassword) ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, pInput, HOTSPOT_Element_Cisco_HotSpot_NewPassword))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetParentalControlsPassword, ERROR_INVALID_PASSWORD);
        goto exit;
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETPARENTALCONTROLSPASSWORD__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/GetParentalControlsResetQuestion
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETPARENTALCONTROLSRESETQUESTION__

void HOTSPOT_Method_Cisco_HotSpot_GetParentalControlsResetQuestion(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;

    /* Return the parental controls reset question */
    HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Question, pOutput, HOTSPOT_Element_Cisco_HotSpot_Question);
    if (!HDK_XML_Get_String(pOutput, HOTSPOT_Element_Cisco_HotSpot_Question))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_GetParentalControlsResetQuestion, ERROR);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_GETPARENTALCONTROLSRESETQUESTION__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/ResetParentalControlsPassword
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_RESETPARENTALCONTROLSPASSWORD__

void HOTSPOT_Method_Cisco_HotSpot_ResetParentalControlsPassword(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszAnswer = 0;
    char* pszInputAnswer;
    char* pszNewPassword;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the answer */
    HDK_XML_Struct_Init(&sTemp);
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Answer, &sTemp, HOTSPOT_Element_Cisco_HotSpot_Answer))
    {
        pszAnswer = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_Answer);
    }

    /* Compare the input answer and the answer */
    pszInputAnswer = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Answer);
#ifdef _MSC_VER
    if (!pszAnswer || stricmp(pszAnswer, pszInputAnswer) != 0)
#else
    if (!pszAnswer || strcasecmp(pszAnswer, pszInputAnswer) != 0)
#endif
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_ResetParentalControlsPassword, ERROR_INCORRECT_ANSWER);
        goto exit;
    }

    /* Validate and set the new password */
    pszNewPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_NewPassword);
    if (!HNAP12_UTL_Password_IsValid(pszNewPassword) ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, pInput, HOTSPOT_Element_Cisco_HotSpot_NewPassword))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_ResetParentalControlsPassword, ERROR_INVALID_PASSWORD);
        goto exit;
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_RESETPARENTALCONTROLSPASSWORD__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/SetParentalControlsResetQuestion
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETPARENTALCONTROLSRESETQUESTION__

void HOTSPOT_Method_Cisco_HotSpot_SetParentalControlsResetQuestion(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    char* pszPassword = 0;
    char* pszInputPassword;
    char* pszQuestion;
    char* pszAnswer;
    size_t lenQuestion;
    size_t lenAnswer;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the password */
    HDK_XML_Struct_Init(&sTemp);
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Password, &sTemp, HOTSPOT_Element_Cisco_HotSpot_NewPassword))
    {
        pszPassword = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HotSpot_NewPassword);
    }

    /* Compare the input password and the password */
    pszInputPassword = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Password);
    if (!pszPassword || strcmp(pszPassword, pszInputPassword) != 0)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetParentalControlsResetQuestion, ERROR_INCORRECT_PASSWORD);
        goto exit;
    }

    /* Validate and set the question */
    pszQuestion = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Question);
    lenQuestion = strlen(pszQuestion);
    if (lenQuestion < 1 || lenQuestion > 64 ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Question, pInput, HOTSPOT_Element_Cisco_HotSpot_Question))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetParentalControlsResetQuestion, ERROR_INVALID_QUESTION);
    }

    /* Validate and set the answer */
    pszAnswer = HDK_XML_Get_String(pInput, HOTSPOT_Element_Cisco_HotSpot_Answer);
    lenAnswer = strlen(pszAnswer);
    if (lenAnswer < 1 || lenAnswer > 32 ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HotSpot_ParentalControls_Answer, pInput, HOTSPOT_Element_Cisco_HotSpot_Answer))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_SetParentalControlsResetQuestion, ERROR_INVALID_ANSWER);
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_SETPARENTALCONTROLSRESETQUESTION__ */


/*
 * http://cisco.com/HNAPExt/HotSpot/AddWebGUIAuthExemption
 */
#ifdef __HOTSPOT_METHOD_CISCO_HOTSPOT_ADDWEBGUIAUTHEXEMPTION__

void HOTSPOT_Method_Cisco_HotSpot_AddWebGUIAuthExemption(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Get the input arguments */
    int duration = *HDK_XML_Get_Int(pInput, HOTSPOT_Element_Cisco_HotSpot_Duration);

    /* Validate the duration */
    if (duration < 1)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HotSpot_AddWebGUIAuthExemption, ERROR);
        return;
    }

    /* TODO: Add exemption list management.  This is not necessary for the
     * emulator because it currently does not emulator a web GUI. */
    (void)pMethodCtx;
}

#endif /* __HOTSPOT_METHOD_CISCO_HOTSPOT_ADDWEBGUIAUTHEXEMPTION__ */


/*
 *
 * HND2 METHODS
 *
 */

#if defined(__HOTSPOT_METHOD_CISCO_HND_ACTIVATETMSSS__) ||           \
    defined(__HOTSPOT_METHOD_CISCO_HND_GETDEFAULTPOLICYSETTING__) || \
    defined(__HOTSPOT_METHOD_CISCO_HND_GETTMSSSLICENSE__) || \
    defined(__HOTSPOT_METHOD_CISCO_HND_SETTMSSSSETTINGS__)

/* Helper function to determine if Trend Micro service is supported */
static int IsTrendMicroServiceSupported(HDK_MOD_MethodContext* pMethodCtx)
{
    int fResult;
    HDK_XML_Struct sTemp;
    HDK_XML_Struct_Init(&sTemp);
    fResult = HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_FirstAccessPolicyNumber, &sTemp, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber) &&
        HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber, 1) > 1;
    HDK_XML_Struct_Free(&sTemp);
    return fResult;
}

#endif


#if defined(__HOTSPOT_METHOD_CISCO_HND_SETPOLICYSETTINGS__)

/* Helper function, which validates that pIPFirst is less than pIPLast */
static int ValidateIPRange(HDK_XML_IPAddress* pIpFirst, HDK_XML_IPAddress* pIpLast)
{
    unsigned int uiFirstIpDec;
    unsigned int uiLastIpDec;

    if (!pIpFirst || !pIpLast)
    {
        return 0;
    }

    /*
     * Convert the IP address to a decimal value by multiplying each
     * octet by the appropriate power of 256 and summing them together.
     * This makes it easy to determine if the last IP is greater than
     * the first.
     */
    uiFirstIpDec = (pIpFirst->a << 24) + (pIpFirst->b << 16) + (pIpFirst->c << 8) + pIpFirst->d;
    uiLastIpDec = (pIpLast->a << 24) + (pIpLast->b << 16) + (pIpLast->c << 8) + pIpLast->d;

    if (uiFirstIpDec > uiLastIpDec)
    {
        return 0;
    }

    return 1;
}

#endif /* defined(__HOTSPOT_METHOD_CISCO_HND_SETPOLICYSETTINGS__) */


#if defined(__HOTSPOT_METHOD_CISCO_HND_SETACCOUNTINFORMATION__)

/* Helper function to validate passwords */
static int ValidatePassword(char* pszPassword)
{
    size_t len = strlen(pszPassword);
    size_t lenOK = strspn(pszPassword, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    return len >= 4 && len <= 32 && len == lenOK;
}

#endif


/*
 * http://cisco.com/HNAPExt/HND/ActivateTMSSS
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_ACTIVATETMSSS__

void HOTSPOT_Method_Cisco_HND_ActivateTMSSS(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszTAVSN;
    HDK_XML_Struct sTemp;
    HOTSPOT_Enum_Cisco_HND_LocaleCode eLocale;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Validate the locale */
    eLocale = *HOTSPOT_Get_Cisco_HND_LocaleCode(pInput, HOTSPOT_Element_Cisco_HND_Locale);
    if (eLocale == HOTSPOT_Enum_Cisco_HND_LocaleCode__UNKNOWN__)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_ActivateTMSSS, ERROR);
        goto exit;
    }

    /* Is Trend Micro service supported? */
    if (!IsTrendMicroServiceSupported(pMethodCtx))
    {
        pszTAVSN = (char*)"";
    }
    else
    {
        /* Return the TAV serial number */
        HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_TAVSN, &sTemp, HOTSPOT_Element_Cisco_HND_TAVSN);
        pszTAVSN = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HND_TAVSN);
        if (!pszTAVSN || strlen(pszTAVSN) == 0)
        {
            /* Set the new TAVSN and expiration date */
            if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_TAVSN_New, &sTemp, HOTSPOT_Element_Cisco_HND_TAVSN) ||
                (pszTAVSN = HDK_XML_Get_String(&sTemp, HOTSPOT_Element_Cisco_HND_TAVSN)) == 0 ||
                !HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_ExpiredDate_New, &sTemp, HOTSPOT_Element_Cisco_HND_ExpiredDate) ||
                !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_TAVSN, &sTemp, HOTSPOT_Element_Cisco_HND_TAVSN) ||
                !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_ExpiredDate, &sTemp, HOTSPOT_Element_Cisco_HND_ExpiredDate))
            {
                SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_ActivateTMSSS, ERROR);
                goto exit;
            }
        }
    }

    /* Return the TAVSN */
    HDK_XML_Set_String(pOutput, HOTSPOT_Element_Cisco_HND_TAVSN, pszTAVSN);

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_ACTIVATETMSSS__ */


/*
 * http://cisco.com/HNAPExt/HND/GetDefaultPolicySetting
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_GETDEFAULTPOLICYSETTING__

void HOTSPOT_Method_Cisco_HND_GetDefaultPolicySetting(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Is Trend Micro service supported? */
    if (!IsTrendMicroServiceSupported(pMethodCtx))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_GetDefaultPolicySetting, ERROR);
        return;
    }

    /* Get the HND access policy number */
    HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_AccessPolicyNumber, pOutput, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_GETDEFAULTPOLICYSETTING__ */


/*
 * http://cisco.com/HNAPExt/HND/GetPolicySettings
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_GETPOLICYSETTINGS__

void HOTSPOT_Method_Cisco_HND_GetPolicySettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the HND access policy list*/
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_AccessPolicyList, pOutput, HOTSPOT_Element_Cisco_HND_AccessPolicyList))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Struct(pOutput, HOTSPOT_Element_Cisco_HND_AccessPolicyList);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_GETPOLICYSETTINGS__ */


/*
 * http://cisco.com/HNAPExt/HND/GetPolicySettingsCapabilities
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_GETPOLICYSETTINGSCAPABILITIES__

void HOTSPOT_Method_Cisco_HND_GetPolicySettingsCapabilities(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the HND SetPolicySettings device capabilities */
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_FirstAccessPolicyNumber, pOutput, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber, 1);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxPolicyNumber, pOutput, HOTSPOT_Element_Cisco_HND_MaxPolicyNumber))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxPolicyNumber, 10);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxPolicyName, pOutput, HOTSPOT_Element_Cisco_HND_MaxPolicyName))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxPolicyName, 32);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxAppliedDeviceList, pOutput, HOTSPOT_Element_Cisco_HND_MaxAppliedDeviceList))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxAppliedDeviceList, 10);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxBlockedURLArray, pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedURLArray))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedURLArray, 10);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxBlockedURLString, pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedURLString))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedURLString, 32);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxBlockedKeywordArray, pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordArray))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordArray, 0);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxBlockedKeywordString, pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordString))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordString, 0);
    }

    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_MaxBlockedCategoryArray, pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedCategoryArray))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_MaxBlockedCategoryArray, 0);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_GETPOLICYSETTINGSCAPABILITIES__ */


/*
 * http://cisco.com/HNAPExt/HND/GetTMSSSLicense
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_GETTMSSSLICENSE__

void HOTSPOT_Method_Cisco_HND_GetTMSSSLicense(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszTAVSN;
    char* pszExpiredDate;

    /* Unused parameters */
    (void)pInput;

    /* Is Trend Micro service supported? */
    if (IsTrendMicroServiceSupported(pMethodCtx))
    {
        /* Get the HND TMSSS license */
        if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_TAVSN, pOutput, HOTSPOT_Element_Cisco_HND_TAVSN) ||
            !HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_ExpiredDate, pOutput, HOTSPOT_Element_Cisco_HND_ExpiredDate) ||
            (pszTAVSN = HDK_XML_Get_String(pOutput, HOTSPOT_Element_Cisco_HND_TAVSN)) == 0 ||
            strlen(pszTAVSN) == 0 ||
            (pszExpiredDate = HDK_XML_Get_String(pOutput, HOTSPOT_Element_Cisco_HND_ExpiredDate)) == 0 ||
            strlen(pszExpiredDate) == 0)
        {
            SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_GetTMSSSLicense, ERROR);
        }
    }
    else
    {
        /* Not supported, just return default values */
        HDK_XML_Set_String(pOutput, HOTSPOT_Element_Cisco_HND_TAVSN, "");
        HDK_XML_Set_String(pOutput, HOTSPOT_Element_Cisco_HND_ExpiredDate, "1970-01-01");
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_GETTMSSSLICENSE__ */


/*
 * http://cisco.com/HNAPExt/HND/GetTMSSSSettings
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_GETTMSSSSETTINGS__

void HOTSPOT_Method_Cisco_HND_GetTMSSSSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pInput;

    /* Get the HND TMSSS license settings */
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_WTPEnabled, pOutput, HOTSPOT_Element_Cisco_HND_WTPEnabled))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Bool(pOutput, HOTSPOT_Element_Cisco_HND_WTPEnabled, 0);
    }
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_WTPThreshold, pOutput, HOTSPOT_Element_Cisco_HND_WTPThreshold))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Int(pOutput, HOTSPOT_Element_Cisco_HND_WTPThreshold, 0);
    }
    if (!HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_PCEnabled, pOutput, HOTSPOT_Element_Cisco_HND_PCEnabled))
    {
        /* Set a reasonable default */
        HDK_XML_Set_Bool(pOutput, HOTSPOT_Element_Cisco_HND_PCEnabled, 0);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_GETTMSSSSETTINGS__ */


/*
 * http://cisco.com/HNAPExt/HND/SetDefaultPolicySetting
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_SETDEFAULTPOLICYSETTING__

void HOTSPOT_Method_Cisco_HND_SetDefaultPolicySetting(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int iFirstPolicyNumber = 0;
    int iPolicyNumber = 0;
    HDK_XML_Struct sTemp;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the max policy number */
    if (HDK_SRV_ADIGet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_FirstAccessPolicyNumber, &sTemp, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber))
    {
        iFirstPolicyNumber = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber, 1);
    }

    /* Validate the policy number and set */
    iPolicyNumber = HDK_XML_GetEx_Int(pInput, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber, 0);
    if (iPolicyNumber < 1 || iPolicyNumber > iFirstPolicyNumber ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_AccessPolicyNumber, pInput, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetDefaultPolicySetting, ERROR_SET_ACCESSPOLICY);
    }

    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_SETDEFAULTPOLICYSETTING__ */


/*
 * http://cisco.com/HNAPExt/HND/SetPolicySettings
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_SETPOLICYSETTINGS__

void HOTSPOT_Method_Cisco_HND_SetPolicySettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Member* pmAccessPolicy = 0;
    HDK_XML_Struct* psAccessPolicies;
    HDK_XML_Struct* psAccessPolicyResultList;
    HDK_XML_Struct sTemp;
    int iFirstPolicyNumber = 0;
    int iMaxPolicyNumber = 0;
    int iMaxBlockedURLArray = 0;
    int iMaxBlockedURLString = 0;
    int iMaxBlockedKeywordArray = 0;
    int iMaxBlockedKeywordString = 0;
    int iMaxBlockedCategoryArray = 0;
    int fPolicyError = 0;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* First off, append the access policy result list to the output struct */
    psAccessPolicyResultList = HDK_XML_Append_Struct(pOutput, HOTSPOT_Element_Cisco_HND_AccessPolicyResultList);
    if (!psAccessPolicyResultList)
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetPolicySettings, ERROR);
        goto exit;
    }

    /* Get capabilities */
    HOTSPOT_Method_Cisco_HND_GetPolicySettingsCapabilities(pMethodCtx, 0, &sTemp);
    iFirstPolicyNumber = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_FirstAccessPolicyNumber, 0);
    iMaxPolicyNumber = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxPolicyNumber, 0);
    iMaxBlockedURLArray = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxBlockedURLArray, 0);
    iMaxBlockedURLString = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxBlockedURLString, 0);
    iMaxBlockedKeywordArray = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordArray, 0);
    iMaxBlockedKeywordString = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxBlockedKeywordString, 0);
    iMaxBlockedCategoryArray = HDK_XML_GetEx_Int(&sTemp, HOTSPOT_Element_Cisco_HND_MaxBlockedCategoryArray, 0);

    /* Iterate over the access policies, validating them */
    psAccessPolicies = HDK_XML_Get_Struct(pInput, HOTSPOT_Element_Cisco_HND_AccessPolicyList);
    for (pmAccessPolicy = psAccessPolicies->pHead; pmAccessPolicy; pmAccessPolicy = pmAccessPolicy->pNext)
    {
        HDK_XML_Struct* psAccessPolicy = HDK_XML_GetMember_Struct(pmAccessPolicy);
        HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_OK;
        HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_OK;
        HOTSPOT_Enum_Cisco_HND_AccessPolicyStatus eStatus;
        HDK_XML_Member* pmAP = 0;
        int* piPolicyNumber;

        /* Is this policy number invalid? */
        piPolicyNumber = HDK_XML_Get_Int(psAccessPolicy, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber);
        if (*piPolicyNumber < 1 || *piPolicyNumber > iMaxPolicyNumber)
        {
            eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
            eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_POLICY_OVER_MAX;
            goto error;
        }

        /* Have we already seen this policy number */
        for (pmAP = psAccessPolicies->pHead; pmAP; pmAP = pmAP->pNext)
        {
            HDK_XML_Struct* psAP = HDK_XML_GetMember_Struct(pmAP);
            int* piPN;

            /* Skip if this is the same member */
            if (pmAP == pmAccessPolicy)
            {
                continue;
            }

            /* Is this policy number a duplicate? */
            piPN = HDK_XML_Get_Int(psAP, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber);
            if (*piPN == *piPolicyNumber)
            {
                eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                goto error;
            }
        }

        /* Validate the access policy status */
        eStatus = HOTSPOT_GetEx_Cisco_HND_AccessPolicyStatus(psAccessPolicy, HOTSPOT_Element_Cisco_HND_Status, HOTSPOT_Enum_Cisco_HND_AccessPolicyStatus__UNKNOWN__);
        if (eStatus == HOTSPOT_Enum_Cisco_HND_AccessPolicyStatus__UNKNOWN__)
        {
            eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY_STATUS;
            eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
            goto error;
        }

        /* Validate the device identifier structure */
        {
            HDK_XML_Struct* psDeviceList = HDK_XML_Get_Struct(psAccessPolicy, HOTSPOT_Element_Cisco_HND_AppliedDeviceList);
            HDK_XML_Member* pmDevice;
            for (pmDevice = psDeviceList->pHead; pmDevice; pmDevice = pmDevice->pNext)
            {
                HDK_XML_Struct* psDevice = HDK_XML_GetMember_Struct(pmDevice);
                HDK_XML_MACAddress* pMAC = HDK_XML_Get_MACAddress(psDevice, HOTSPOT_Element_Cisco_HND_MacAddress);
                HDK_XML_IPAddress* pIP = HDK_XML_Get_IPAddress(psDevice, HOTSPOT_Element_Cisco_HND_IPAddress);
                HDK_XML_IPAddress* pIPFirst = HDK_XML_Get_IPAddress(psDevice, HOTSPOT_Element_Cisco_HND_IPStart);
                HDK_XML_IPAddress* pIPLast = HDK_XML_Get_IPAddress(psDevice, HOTSPOT_Element_Cisco_HND_IPEnd);
                if (pMAC->a || pMAC->b || pMAC->c || pMAC->d || pMAC->e || pMAC->f)
                {
                    continue;
                }
                else if (pIP->a || pIP->b || pIP->c || pIP->d)
                {
                    continue;
                }
                else if (!(pIPFirst->a || pIPFirst->b || pIPFirst->c || pIPFirst->d) ||
                         !(pIPLast->a || pIPLast->b || pIPLast->c || pIPLast->d) ||
                         !ValidateIPRange(pIPFirst, pIPLast))
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
            }
        }

        /* Validate the access schedule */
        {
            HDK_XML_Struct* psSchedule = HDK_XML_Get_Struct(psAccessPolicy, HOTSPOT_Element_Cisco_HND_AccessSchedule);
            unsigned int i;
            static HOTSPOT_Element s_ScheduleDays[] =
                {
                    HOTSPOT_Element_Cisco_HND_Sunday,
                    HOTSPOT_Element_Cisco_HND_Monday,
                    HOTSPOT_Element_Cisco_HND_Tuesday,
                    HOTSPOT_Element_Cisco_HND_Wednesday,
                    HOTSPOT_Element_Cisco_HND_Thursday,
                    HOTSPOT_Element_Cisco_HND_Friday,
                    HOTSPOT_Element_Cisco_HND_Saturday
                };
            for (i = 0; i < sizeof(s_ScheduleDays) / sizeof(*s_ScheduleDays); ++i)
            {
                char* pszSchedule = HDK_XML_Get_String(psSchedule, s_ScheduleDays[i]);
                if (strlen(pszSchedule) != 48 || strspn(pszSchedule, "01") != 48)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
            }
        }

        /* Validate the blocked url array */
        {
            HDK_XML_Struct* psBlockedURLArray = HDK_XML_Get_Struct(psAccessPolicy, HOTSPOT_Element_Cisco_HND_BlockedURL);
            HDK_XML_Member* pmBlockedURL;
            int iURLCount = 0;
            for (pmBlockedURL = psBlockedURLArray->pHead; pmBlockedURL; pmBlockedURL = pmBlockedURL->pNext)
            {
                char* pszBlockedURL = HDK_XML_GetMember_String(pmBlockedURL);
                int cbBlockedURL = (int)strlen(pszBlockedURL);
                if (cbBlockedURL < 1 || cbBlockedURL > iMaxBlockedURLString)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
                else if (++iURLCount > iMaxBlockedURLArray)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
            }
        }

        /* Validate the blocked keyword array */
        {
            HDK_XML_Struct* psBlockedKeywordArray = HDK_XML_Get_Struct(psAccessPolicy, HOTSPOT_Element_Cisco_HND_BlockedKeyword);
            HDK_XML_Member* pmBlockedKeyword;
            int iKeywordCount = 0;
            for (pmBlockedKeyword = psBlockedKeywordArray->pHead; pmBlockedKeyword; pmBlockedKeyword = pmBlockedKeyword->pNext)
            {
                char* pszBlockedKeyword = HDK_XML_GetMember_String(pmBlockedKeyword);
                int cbBlockedKeyword = (int)strlen(pszBlockedKeyword);
                if (cbBlockedKeyword < 1 || cbBlockedKeyword > iMaxBlockedKeywordString)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
                else if (++iKeywordCount > iMaxBlockedKeywordArray)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }
            }
        }

        /* Validate the blocked catagory values are between 1 and 88 inclusive */
        {
            HDK_XML_Struct* psBlockedCategoryArray = HDK_XML_Get_Struct(psAccessPolicy, HOTSPOT_Element_Cisco_HND_BlockedCategory);
            HDK_XML_Member* pmBlockedCategory = 0;
            HDK_XML_Member* pmBlockedCategory2 = 0;
            int iCategoryCount = 0;
            for (pmBlockedCategory = psBlockedCategoryArray->pHead; pmBlockedCategory; pmBlockedCategory = pmBlockedCategory->pNext)
            {
                int* piCategory = HDK_XML_GetMember_Int(pmBlockedCategory);
                if (*piCategory < 1 || *piCategory > 88 || ++iCategoryCount > iMaxBlockedCategoryArray)
                {
                    eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                    eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                    goto error;
                }

                /* Any duplicate categories? */
                for (pmBlockedCategory2 = psBlockedCategoryArray->pHead; pmBlockedCategory2; pmBlockedCategory2 = pmBlockedCategory2->pNext)
                {
                    if (pmBlockedCategory2 != pmBlockedCategory)
                    {
                        int* piCategory2 = HDK_XML_GetMember_Int(pmBlockedCategory2);
                        if (*piCategory2 == *piCategory)
                        {
                            eResult = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResult_ERROR_SET_ACCESSPOLICY;
                            eError = HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_ERROR_GENERAL;
                            goto error;
                        }
                    }
                }
            }
        }

    error:
        /* Append the policy result for this policy member */
        {
            HDK_XML_Struct* psAccessPolicyResult;

            /* Note that there has been a policy error */
            if (eError != HOTSPOT_Enum_Cisco_HND_SetPolicySettingsResults_OK)
            {
                if (!fPolicyError)
                {
                    HOTSPOT_Set_Cisco_HND_SetPolicySettingsResult(pOutput, HOTSPOT_Element_Cisco_HND_SetPolicySettingsResult, eResult);
                }
                fPolicyError = 1;
            }

            /* Append the access policy result */
            psAccessPolicyResult = HDK_XML_Append_Struct(psAccessPolicyResultList, HOTSPOT_Element_Cisco_HND_AccessPolicyResult);
            if (psAccessPolicyResult)
            {
                HDK_XML_Set_Int(psAccessPolicyResult, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber, *piPolicyNumber);
                HOTSPOT_Set_Cisco_HND_SetPolicySettingsResults(psAccessPolicyResult, HOTSPOT_Element_Cisco_HND_Result, eError);
            }
            else
            {
                SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetPolicySettings, ERROR);
                break;
            }
        }
    }

    /* Set the result... */
    if (!fPolicyError)
    {
        /* Verify that the built-in policies are all provided */
        int iPolicy;
        for (iPolicy = 1; iPolicy < iFirstPolicyNumber; ++iPolicy)
        {
            /* Find the policy... */
            HDK_XML_Member* pmPolicy;
            for (pmPolicy = psAccessPolicies->pHead; pmPolicy; pmPolicy = pmPolicy->pNext)
            {
                HDK_XML_Struct* psPolicy = HDK_XML_GetMember_Struct(pmPolicy);
                int* piPN = HDK_XML_Get_Int(psPolicy, HOTSPOT_Element_Cisco_HND_AccessPolicyNumber);
                if (iPolicy == *piPN)
                {
                    break;
                }
            }
            if (!pmPolicy)
            {
                SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetPolicySettings, ERROR);
                goto exit;
            }
        }

        /* Set the policy list */
        if (!HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_AccessPolicyList, pInput, HOTSPOT_Element_Cisco_HND_AccessPolicyList))
        {
            SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetPolicySettings, ERROR);
            goto exit;
        }
    }

exit:
    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_SETPOLICYSETTINGS__ */


/*
 * http://cisco.com/HNAPExt/HND/SetTMSSSSettings
 */
#ifdef __HOTSPOT_METHOD_CISCO_HND_SETTMSSSSETTINGS__

void HOTSPOT_Method_Cisco_HND_SetTMSSSSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    int iWTPThreshold = -1;

    /* Validate the threshold value, validate, and set */
    int fTM = IsTrendMicroServiceSupported(pMethodCtx);
    iWTPThreshold = HDK_XML_GetEx_Int(pInput, HOTSPOT_Element_Cisco_HND_WTPThreshold, -1);
    if (iWTPThreshold < 0 || iWTPThreshold > 100 ||
        (fTM && !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_WTPEnabled, pInput, HOTSPOT_Element_Cisco_HND_WTPEnabled)) ||
        (fTM && !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_WTPThreshold, pInput, HOTSPOT_Element_Cisco_HND_WTPThreshold)) ||
        !HDK_SRV_ADISet(pMethodCtx, HOTSPOT_ADI_Cisco_HND_PCEnabled, pInput, HOTSPOT_Element_Cisco_HND_PCEnabled))
    {
        SetHNAPResult(pOutput, HOTSPOT, Cisco_HND_SetTMSSSSettings, ERROR);
    }
}

#endif /* #ifdef __HOTSPOT_METHOD_CISCO_HND_SETTMSSSSETTINGS__ */
