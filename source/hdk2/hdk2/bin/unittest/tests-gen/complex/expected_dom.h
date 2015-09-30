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

#ifndef __ACTUAL_DOM_H__
#define __ACTUAL_DOM_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_DOM_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_DOM_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_DOM_Element
{
    ACTUAL_DOM_Element_ADI = 0,
    ACTUAL_DOM_Element_GetServiceInfo = 1,
    ACTUAL_DOM_Element_GetServiceInfoResponse = 2,
    ACTUAL_DOM_Element_GetServiceInfoResult = 3,
    ACTUAL_DOM_Element_GetServices = 4,
    ACTUAL_DOM_Element_GetServicesResponse = 5,
    ACTUAL_DOM_Element_GetServicesResult = 6,
    ACTUAL_DOM_Element_Info = 7,
    ACTUAL_DOM_Element_ServiceInfo = 8,
    ACTUAL_DOM_Element_ServiceLevel = 9,
    ACTUAL_DOM_Element_ServiceName = 10,
    ACTUAL_DOM_Element_Services = 11,
    ACTUAL_DOM_Element_PN_Active = 12,
    ACTUAL_DOM_Element_PN_AddPortMapping = 13,
    ACTUAL_DOM_Element_PN_AddPortMappingResponse = 14,
    ACTUAL_DOM_Element_PN_AddPortMappingResult = 15,
    ACTUAL_DOM_Element_PN_AdminPassword = 16,
    ACTUAL_DOM_Element_PN_AdminPasswordDefault = 17,
    ACTUAL_DOM_Element_PN_AutoAdjustDST = 18,
    ACTUAL_DOM_Element_PN_AutoDetectType = 19,
    ACTUAL_DOM_Element_PN_AutoReconnect = 20,
    ACTUAL_DOM_Element_PN_Base64Image = 21,
    ACTUAL_DOM_Element_PN_BufferSize = 22,
    ACTUAL_DOM_Element_PN_ByteStream = 23,
    ACTUAL_DOM_Element_PN_Bytes = 24,
    ACTUAL_DOM_Element_PN_BytesReceived = 25,
    ACTUAL_DOM_Element_PN_BytesSent = 26,
    ACTUAL_DOM_Element_PN_Channel = 27,
    ACTUAL_DOM_Element_PN_ChannelWidth = 28,
    ACTUAL_DOM_Element_PN_Channels = 29,
    ACTUAL_DOM_Element_PN_ClientStat = 30,
    ACTUAL_DOM_Element_PN_ClientStats = 31,
    ACTUAL_DOM_Element_PN_ConfigBlob = 32,
    ACTUAL_DOM_Element_PN_ConnectTime = 33,
    ACTUAL_DOM_Element_PN_ConnectedClient = 34,
    ACTUAL_DOM_Element_PN_ConnectedClients = 35,
    ACTUAL_DOM_Element_PN_DHCPIPAddressFirst = 36,
    ACTUAL_DOM_Element_PN_DHCPIPAddressLast = 37,
    ACTUAL_DOM_Element_PN_DHCPLeaseTime = 38,
    ACTUAL_DOM_Element_PN_DHCPReservation = 39,
    ACTUAL_DOM_Element_PN_DHCPReservations = 40,
    ACTUAL_DOM_Element_PN_DHCPReservationsSupported = 41,
    ACTUAL_DOM_Element_PN_DHCPServerEnabled = 42,
    ACTUAL_DOM_Element_PN_DNS = 43,
    ACTUAL_DOM_Element_PN_DNSSettings = 44,
    ACTUAL_DOM_Element_PN_DeletePortMapping = 45,
    ACTUAL_DOM_Element_PN_DeletePortMappingResponse = 46,
    ACTUAL_DOM_Element_PN_DeletePortMappingResult = 47,
    ACTUAL_DOM_Element_PN_DeviceName = 48,
    ACTUAL_DOM_Element_PN_DeviceNetworkStats = 49,
    ACTUAL_DOM_Element_PN_DeviceType = 50,
    ACTUAL_DOM_Element_PN_DomainName = 51,
    ACTUAL_DOM_Element_PN_DomainNameChangeSupported = 52,
    ACTUAL_DOM_Element_PN_DownloadSpeedTest = 53,
    ACTUAL_DOM_Element_PN_DownloadSpeedTestResponse = 54,
    ACTUAL_DOM_Element_PN_DownloadSpeedTestResult = 55,
    ACTUAL_DOM_Element_PN_Enabled = 56,
    ACTUAL_DOM_Element_PN_Encryption = 57,
    ACTUAL_DOM_Element_PN_Encryptions = 58,
    ACTUAL_DOM_Element_PN_ExternalPort = 59,
    ACTUAL_DOM_Element_PN_FirmwareDate = 60,
    ACTUAL_DOM_Element_PN_FirmwareUpload = 61,
    ACTUAL_DOM_Element_PN_FirmwareUploadResponse = 62,
    ACTUAL_DOM_Element_PN_FirmwareUploadResult = 63,
    ACTUAL_DOM_Element_PN_FirmwareVersion = 64,
    ACTUAL_DOM_Element_PN_Frequencies = 65,
    ACTUAL_DOM_Element_PN_Frequency = 66,
    ACTUAL_DOM_Element_PN_Gateway = 67,
    ACTUAL_DOM_Element_PN_GetClientStats = 68,
    ACTUAL_DOM_Element_PN_GetClientStatsResponse = 69,
    ACTUAL_DOM_Element_PN_GetClientStatsResult = 70,
    ACTUAL_DOM_Element_PN_GetConfigBlob = 71,
    ACTUAL_DOM_Element_PN_GetConfigBlobResponse = 72,
    ACTUAL_DOM_Element_PN_GetConfigBlobResult = 73,
    ACTUAL_DOM_Element_PN_GetConnectedDevices = 74,
    ACTUAL_DOM_Element_PN_GetConnectedDevicesResponse = 75,
    ACTUAL_DOM_Element_PN_GetConnectedDevicesResult = 76,
    ACTUAL_DOM_Element_PN_GetDeviceSettings = 77,
    ACTUAL_DOM_Element_PN_GetDeviceSettings2 = 78,
    ACTUAL_DOM_Element_PN_GetDeviceSettings2Response = 79,
    ACTUAL_DOM_Element_PN_GetDeviceSettings2Result = 80,
    ACTUAL_DOM_Element_PN_GetDeviceSettingsResponse = 81,
    ACTUAL_DOM_Element_PN_GetDeviceSettingsResult = 82,
    ACTUAL_DOM_Element_PN_GetFirmwareSettings = 83,
    ACTUAL_DOM_Element_PN_GetFirmwareSettingsResponse = 84,
    ACTUAL_DOM_Element_PN_GetFirmwareSettingsResult = 85,
    ACTUAL_DOM_Element_PN_GetMACFilters2 = 86,
    ACTUAL_DOM_Element_PN_GetMACFilters2Response = 87,
    ACTUAL_DOM_Element_PN_GetMACFilters2Result = 88,
    ACTUAL_DOM_Element_PN_GetNetworkStats = 89,
    ACTUAL_DOM_Element_PN_GetNetworkStatsResponse = 90,
    ACTUAL_DOM_Element_PN_GetNetworkStatsResult = 91,
    ACTUAL_DOM_Element_PN_GetPortMappings = 92,
    ACTUAL_DOM_Element_PN_GetPortMappingsResponse = 93,
    ACTUAL_DOM_Element_PN_GetPortMappingsResult = 94,
    ACTUAL_DOM_Element_PN_GetRouterLanSettings2 = 95,
    ACTUAL_DOM_Element_PN_GetRouterLanSettings2Response = 96,
    ACTUAL_DOM_Element_PN_GetRouterLanSettings2Result = 97,
    ACTUAL_DOM_Element_PN_GetRouterSettings = 98,
    ACTUAL_DOM_Element_PN_GetRouterSettingsResponse = 99,
    ACTUAL_DOM_Element_PN_GetRouterSettingsResult = 100,
    ACTUAL_DOM_Element_PN_GetWLanRadioFrequencies = 101,
    ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResponse = 102,
    ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResult = 103,
    ACTUAL_DOM_Element_PN_GetWLanRadioSecurity = 104,
    ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResponse = 105,
    ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResult = 106,
    ACTUAL_DOM_Element_PN_GetWLanRadioSettings = 107,
    ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResponse = 108,
    ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResult = 109,
    ACTUAL_DOM_Element_PN_GetWLanRadios = 110,
    ACTUAL_DOM_Element_PN_GetWLanRadiosResponse = 111,
    ACTUAL_DOM_Element_PN_GetWLanRadiosResult = 112,
    ACTUAL_DOM_Element_PN_GetWanInfo = 113,
    ACTUAL_DOM_Element_PN_GetWanInfoResponse = 114,
    ACTUAL_DOM_Element_PN_GetWanInfoResult = 115,
    ACTUAL_DOM_Element_PN_GetWanSettings = 116,
    ACTUAL_DOM_Element_PN_GetWanSettingsResponse = 117,
    ACTUAL_DOM_Element_PN_GetWanSettingsResult = 118,
    ACTUAL_DOM_Element_PN_IPAddress = 119,
    ACTUAL_DOM_Element_PN_IPAddressFirst = 120,
    ACTUAL_DOM_Element_PN_IPAddressLast = 121,
    ACTUAL_DOM_Element_PN_InternalClient = 122,
    ACTUAL_DOM_Element_PN_InternalPort = 123,
    ACTUAL_DOM_Element_PN_IsAccessPoint = 124,
    ACTUAL_DOM_Element_PN_IsAllowList = 125,
    ACTUAL_DOM_Element_PN_IsDeviceReady = 126,
    ACTUAL_DOM_Element_PN_IsDeviceReadyResponse = 127,
    ACTUAL_DOM_Element_PN_IsDeviceReadyResult = 128,
    ACTUAL_DOM_Element_PN_Key = 129,
    ACTUAL_DOM_Element_PN_KeyRenewal = 130,
    ACTUAL_DOM_Element_PN_LanIPAddress = 131,
    ACTUAL_DOM_Element_PN_LanSubnetMask = 132,
    ACTUAL_DOM_Element_PN_LeaseTime = 133,
    ACTUAL_DOM_Element_PN_LinkSpeedIn = 134,
    ACTUAL_DOM_Element_PN_LinkSpeedOut = 135,
    ACTUAL_DOM_Element_PN_Locale = 136,
    ACTUAL_DOM_Element_PN_MACInfo = 137,
    ACTUAL_DOM_Element_PN_MACList = 138,
    ACTUAL_DOM_Element_PN_MFEnabled = 139,
    ACTUAL_DOM_Element_PN_MFIsAllowList = 140,
    ACTUAL_DOM_Element_PN_MFMACList = 141,
    ACTUAL_DOM_Element_PN_MTU = 142,
    ACTUAL_DOM_Element_PN_MacAddress = 143,
    ACTUAL_DOM_Element_PN_ManageOnlyViaSSL = 144,
    ACTUAL_DOM_Element_PN_ManageRemote = 145,
    ACTUAL_DOM_Element_PN_ManageViaSSLSupported = 146,
    ACTUAL_DOM_Element_PN_ManageWireless = 147,
    ACTUAL_DOM_Element_PN_MaxIdleTime = 148,
    ACTUAL_DOM_Element_PN_Mode = 149,
    ACTUAL_DOM_Element_PN_ModelDescription = 150,
    ACTUAL_DOM_Element_PN_ModelName = 151,
    ACTUAL_DOM_Element_PN_ModelRevision = 152,
    ACTUAL_DOM_Element_PN_Name = 153,
    ACTUAL_DOM_Element_PN_NetworkStats = 154,
    ACTUAL_DOM_Element_PN_NewIPAddress = 155,
    ACTUAL_DOM_Element_PN_PMDescription = 156,
    ACTUAL_DOM_Element_PN_PMExternalPort = 157,
    ACTUAL_DOM_Element_PN_PMInternalClient = 158,
    ACTUAL_DOM_Element_PN_PMInternalPort = 159,
    ACTUAL_DOM_Element_PN_PMProtocol = 160,
    ACTUAL_DOM_Element_PN_PacketsReceived = 161,
    ACTUAL_DOM_Element_PN_PacketsSent = 162,
    ACTUAL_DOM_Element_PN_Password = 163,
    ACTUAL_DOM_Element_PN_PortMapping = 164,
    ACTUAL_DOM_Element_PN_PortMappingDescription = 165,
    ACTUAL_DOM_Element_PN_PortMappingProtocol = 166,
    ACTUAL_DOM_Element_PN_PortMappings = 167,
    ACTUAL_DOM_Element_PN_PortName = 168,
    ACTUAL_DOM_Element_PN_PresentationURL = 169,
    ACTUAL_DOM_Element_PN_Primary = 170,
    ACTUAL_DOM_Element_PN_QoS = 171,
    ACTUAL_DOM_Element_PN_RadioFrequencyInfo = 172,
    ACTUAL_DOM_Element_PN_RadioFrequencyInfos = 173,
    ACTUAL_DOM_Element_PN_RadioID = 174,
    ACTUAL_DOM_Element_PN_RadioInfo = 175,
    ACTUAL_DOM_Element_PN_RadioInfos = 176,
    ACTUAL_DOM_Element_PN_RadiusIP1 = 177,
    ACTUAL_DOM_Element_PN_RadiusIP2 = 178,
    ACTUAL_DOM_Element_PN_RadiusPort1 = 179,
    ACTUAL_DOM_Element_PN_RadiusPort2 = 180,
    ACTUAL_DOM_Element_PN_RadiusSecret1 = 181,
    ACTUAL_DOM_Element_PN_RadiusSecret2 = 182,
    ACTUAL_DOM_Element_PN_Reboot = 183,
    ACTUAL_DOM_Element_PN_RebootResponse = 184,
    ACTUAL_DOM_Element_PN_RebootResult = 185,
    ACTUAL_DOM_Element_PN_RemoteManagementSupported = 186,
    ACTUAL_DOM_Element_PN_RemotePort = 187,
    ACTUAL_DOM_Element_PN_RemoteSSL = 188,
    ACTUAL_DOM_Element_PN_RemoteSSLNeedsSSL = 189,
    ACTUAL_DOM_Element_PN_RenewTimeout = 190,
    ACTUAL_DOM_Element_PN_RenewWanConnection = 191,
    ACTUAL_DOM_Element_PN_RenewWanConnectionResponse = 192,
    ACTUAL_DOM_Element_PN_RenewWanConnectionResult = 193,
    ACTUAL_DOM_Element_PN_RestoreFactoryDefaults = 194,
    ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResponse = 195,
    ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResult = 196,
    ACTUAL_DOM_Element_PN_RouterIPAddress = 197,
    ACTUAL_DOM_Element_PN_RouterSubnetMask = 198,
    ACTUAL_DOM_Element_PN_SOAPActions = 199,
    ACTUAL_DOM_Element_PN_SSID = 200,
    ACTUAL_DOM_Element_PN_SSIDBroadcast = 201,
    ACTUAL_DOM_Element_PN_SSL = 202,
    ACTUAL_DOM_Element_PN_Secondary = 203,
    ACTUAL_DOM_Element_PN_SecondaryChannel = 204,
    ACTUAL_DOM_Element_PN_SecondaryChannels = 205,
    ACTUAL_DOM_Element_PN_SecurityInfo = 206,
    ACTUAL_DOM_Element_PN_SecurityType = 207,
    ACTUAL_DOM_Element_PN_SerialNumber = 208,
    ACTUAL_DOM_Element_PN_ServiceName = 209,
    ACTUAL_DOM_Element_PN_SetAccessPointMode = 210,
    ACTUAL_DOM_Element_PN_SetAccessPointModeResponse = 211,
    ACTUAL_DOM_Element_PN_SetAccessPointModeResult = 212,
    ACTUAL_DOM_Element_PN_SetConfigBlob = 213,
    ACTUAL_DOM_Element_PN_SetConfigBlobResponse = 214,
    ACTUAL_DOM_Element_PN_SetConfigBlobResult = 215,
    ACTUAL_DOM_Element_PN_SetDeviceSettings = 216,
    ACTUAL_DOM_Element_PN_SetDeviceSettings2 = 217,
    ACTUAL_DOM_Element_PN_SetDeviceSettings2Response = 218,
    ACTUAL_DOM_Element_PN_SetDeviceSettings2Result = 219,
    ACTUAL_DOM_Element_PN_SetDeviceSettingsResponse = 220,
    ACTUAL_DOM_Element_PN_SetDeviceSettingsResult = 221,
    ACTUAL_DOM_Element_PN_SetMACFilters2 = 222,
    ACTUAL_DOM_Element_PN_SetMACFilters2Response = 223,
    ACTUAL_DOM_Element_PN_SetMACFilters2Result = 224,
    ACTUAL_DOM_Element_PN_SetRouterLanSettings2 = 225,
    ACTUAL_DOM_Element_PN_SetRouterLanSettings2Response = 226,
    ACTUAL_DOM_Element_PN_SetRouterLanSettings2Result = 227,
    ACTUAL_DOM_Element_PN_SetRouterSettings = 228,
    ACTUAL_DOM_Element_PN_SetRouterSettingsResponse = 229,
    ACTUAL_DOM_Element_PN_SetRouterSettingsResult = 230,
    ACTUAL_DOM_Element_PN_SetWLanRadioFrequency = 231,
    ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResponse = 232,
    ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResult = 233,
    ACTUAL_DOM_Element_PN_SetWLanRadioSecurity = 234,
    ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResponse = 235,
    ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResult = 236,
    ACTUAL_DOM_Element_PN_SetWLanRadioSettings = 237,
    ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResponse = 238,
    ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResult = 239,
    ACTUAL_DOM_Element_PN_SetWanSettings = 240,
    ACTUAL_DOM_Element_PN_SetWanSettingsResponse = 241,
    ACTUAL_DOM_Element_PN_SetWanSettingsResult = 242,
    ACTUAL_DOM_Element_PN_SignalStrength = 243,
    ACTUAL_DOM_Element_PN_Stats = 244,
    ACTUAL_DOM_Element_PN_Status = 245,
    ACTUAL_DOM_Element_PN_SubDeviceURLs = 246,
    ACTUAL_DOM_Element_PN_SubnetMask = 247,
    ACTUAL_DOM_Element_PN_SupportedLocales = 248,
    ACTUAL_DOM_Element_PN_SupportedModes = 249,
    ACTUAL_DOM_Element_PN_SupportedSecurity = 250,
    ACTUAL_DOM_Element_PN_SupportedTypes = 251,
    ACTUAL_DOM_Element_PN_TaskExtension = 252,
    ACTUAL_DOM_Element_PN_TaskExtensions = 253,
    ACTUAL_DOM_Element_PN_Tasks = 254,
    ACTUAL_DOM_Element_PN_Tertiary = 255,
    ACTUAL_DOM_Element_PN_TimeZone = 256,
    ACTUAL_DOM_Element_PN_TimeZoneSupported = 257,
    ACTUAL_DOM_Element_PN_Type = 258,
    ACTUAL_DOM_Element_PN_URL = 259,
    ACTUAL_DOM_Element_PN_UpdateMethods = 260,
    ACTUAL_DOM_Element_PN_Username = 261,
    ACTUAL_DOM_Element_PN_UsernameSupported = 262,
    ACTUAL_DOM_Element_PN_VendorName = 263,
    ACTUAL_DOM_Element_PN_WLanChannel = 264,
    ACTUAL_DOM_Element_PN_WLanChannelInfo = 265,
    ACTUAL_DOM_Element_PN_WLanChannelWidth = 266,
    ACTUAL_DOM_Element_PN_WLanChannelWidthInfo = 267,
    ACTUAL_DOM_Element_PN_WLanEnabled = 268,
    ACTUAL_DOM_Element_PN_WLanEnabledInfo = 269,
    ACTUAL_DOM_Element_PN_WLanEncryption = 270,
    ACTUAL_DOM_Element_PN_WLanEncryptionInfo = 271,
    ACTUAL_DOM_Element_PN_WLanFrequency = 272,
    ACTUAL_DOM_Element_PN_WLanFrequencyInfo = 273,
    ACTUAL_DOM_Element_PN_WLanKey = 274,
    ACTUAL_DOM_Element_PN_WLanKeyInfo = 275,
    ACTUAL_DOM_Element_PN_WLanKeyRenewal = 276,
    ACTUAL_DOM_Element_PN_WLanKeyRenewalInfo = 277,
    ACTUAL_DOM_Element_PN_WLanMacAddress = 278,
    ACTUAL_DOM_Element_PN_WLanMacAddressInfo = 279,
    ACTUAL_DOM_Element_PN_WLanMode = 280,
    ACTUAL_DOM_Element_PN_WLanModeInfo = 281,
    ACTUAL_DOM_Element_PN_WLanQoS = 282,
    ACTUAL_DOM_Element_PN_WLanQoSInfo = 283,
    ACTUAL_DOM_Element_PN_WLanRadioFrequencyInfos = 284,
    ACTUAL_DOM_Element_PN_WLanRadioInfos = 285,
    ACTUAL_DOM_Element_PN_WLanRadiusIP1 = 286,
    ACTUAL_DOM_Element_PN_WLanRadiusIP1Info = 287,
    ACTUAL_DOM_Element_PN_WLanRadiusIP2 = 288,
    ACTUAL_DOM_Element_PN_WLanRadiusIP2Info = 289,
    ACTUAL_DOM_Element_PN_WLanRadiusPort1 = 290,
    ACTUAL_DOM_Element_PN_WLanRadiusPort1Info = 291,
    ACTUAL_DOM_Element_PN_WLanRadiusPort2 = 292,
    ACTUAL_DOM_Element_PN_WLanRadiusPort2Info = 293,
    ACTUAL_DOM_Element_PN_WLanRadiusSecret1 = 294,
    ACTUAL_DOM_Element_PN_WLanRadiusSecret1Info = 295,
    ACTUAL_DOM_Element_PN_WLanRadiusSecret2 = 296,
    ACTUAL_DOM_Element_PN_WLanRadiusSecret2Info = 297,
    ACTUAL_DOM_Element_PN_WLanSSID = 298,
    ACTUAL_DOM_Element_PN_WLanSSIDBroadcast = 299,
    ACTUAL_DOM_Element_PN_WLanSSIDBroadcastInfo = 300,
    ACTUAL_DOM_Element_PN_WLanSSIDInfo = 301,
    ACTUAL_DOM_Element_PN_WLanSecondaryChannel = 302,
    ACTUAL_DOM_Element_PN_WLanSecondaryChannelInfo = 303,
    ACTUAL_DOM_Element_PN_WLanSecurityEnabled = 304,
    ACTUAL_DOM_Element_PN_WLanSecurityEnabledInfo = 305,
    ACTUAL_DOM_Element_PN_WLanType = 306,
    ACTUAL_DOM_Element_PN_WLanTypeInfo = 307,
    ACTUAL_DOM_Element_PN_WPSPin = 308,
    ACTUAL_DOM_Element_PN_WanAuthService = 309,
    ACTUAL_DOM_Element_PN_WanAutoDetectType = 310,
    ACTUAL_DOM_Element_PN_WanAutoMTUSupported = 311,
    ACTUAL_DOM_Element_PN_WanAutoReconnect = 312,
    ACTUAL_DOM_Element_PN_WanDNS = 313,
    ACTUAL_DOM_Element_PN_WanGateway = 314,
    ACTUAL_DOM_Element_PN_WanIPAddress = 315,
    ACTUAL_DOM_Element_PN_WanLoginService = 316,
    ACTUAL_DOM_Element_PN_WanMTU = 317,
    ACTUAL_DOM_Element_PN_WanMacAddress = 318,
    ACTUAL_DOM_Element_PN_WanMaxIdleTime = 319,
    ACTUAL_DOM_Element_PN_WanPPPoEService = 320,
    ACTUAL_DOM_Element_PN_WanPassword = 321,
    ACTUAL_DOM_Element_PN_WanRenewTimeout = 322,
    ACTUAL_DOM_Element_PN_WanStatus = 323,
    ACTUAL_DOM_Element_PN_WanSubnetMask = 324,
    ACTUAL_DOM_Element_PN_WanSupportedTypes = 325,
    ACTUAL_DOM_Element_PN_WanType = 326,
    ACTUAL_DOM_Element_PN_WanUsername = 327,
    ACTUAL_DOM_Element_PN_WideChannel = 328,
    ACTUAL_DOM_Element_PN_WideChannels = 329,
    ACTUAL_DOM_Element_PN_WiredQoS = 330,
    ACTUAL_DOM_Element_PN_WiredQoSSupported = 331,
    ACTUAL_DOM_Element_PN_Wireless = 332,
    ACTUAL_DOM_Element_PN_int = 333,
    ACTUAL_DOM_Element_PN_string = 334,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 335,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 336,
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 337
} ACTUAL_DOM_Element;


/*
 * Enum types enumeration
 */

typedef enum _ACTUAL_DOM_EnumType
{
    ACTUAL_DOM_EnumType_GetServiceInfoResult = -1,
    ACTUAL_DOM_EnumType_GetServicesResult = -2,
    ACTUAL_DOM_EnumType_PN_AddPortMappingResult = -3,
    ACTUAL_DOM_EnumType_PN_DeletePortMappingResult = -4,
    ACTUAL_DOM_EnumType_PN_DeviceType = -5,
    ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult = -6,
    ACTUAL_DOM_EnumType_PN_FirmwareUploadResult = -7,
    ACTUAL_DOM_EnumType_PN_GetClientStatsResult = -8,
    ACTUAL_DOM_EnumType_PN_GetConfigBlobResult = -9,
    ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult = -10,
    ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result = -11,
    ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult = -12,
    ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult = -13,
    ACTUAL_DOM_EnumType_PN_GetMACFilters2Result = -14,
    ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult = -15,
    ACTUAL_DOM_EnumType_PN_GetPortMappingsResult = -16,
    ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result = -17,
    ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult = -18,
    ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult = -19,
    ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult = -20,
    ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult = -21,
    ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult = -22,
    ACTUAL_DOM_EnumType_PN_GetWanInfoResult = -23,
    ACTUAL_DOM_EnumType_PN_GetWanSettingsResult = -24,
    ACTUAL_DOM_EnumType_PN_IPProtocol = -25,
    ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult = -26,
    ACTUAL_DOM_EnumType_PN_LANConnection = -27,
    ACTUAL_DOM_EnumType_PN_RebootResult = -28,
    ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult = -29,
    ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult = -30,
    ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult = -31,
    ACTUAL_DOM_EnumType_PN_SetConfigBlobResult = -32,
    ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result = -33,
    ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult = -34,
    ACTUAL_DOM_EnumType_PN_SetMACFilters2Result = -35,
    ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result = -36,
    ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult = -37,
    ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult = -38,
    ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult = -39,
    ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult = -40,
    ACTUAL_DOM_EnumType_PN_SetWanSettingsResult = -41,
    ACTUAL_DOM_EnumType_PN_TaskExtType = -42,
    ACTUAL_DOM_EnumType_PN_UpdateMethod = -43,
    ACTUAL_DOM_EnumType_PN_WANStatus = -44,
    ACTUAL_DOM_EnumType_PN_WANType = -45,
    ACTUAL_DOM_EnumType_PN_WiFiEncryption = -46,
    ACTUAL_DOM_EnumType_PN_WiFiMode = -47,
    ACTUAL_DOM_EnumType_PN_WiFiSecurity = -48
} ACTUAL_DOM_EnumType;


/*
 * Enumeration http://cisco.com/HNAP/GetServiceInfoResult
 */

typedef enum _ACTUAL_DOM_Enum_GetServiceInfoResult
{
    ACTUAL_DOM_Enum_GetServiceInfoResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_GetServiceInfoResult_OK = 0,
    ACTUAL_DOM_Enum_GetServiceInfoResult_ERROR = 1
} ACTUAL_DOM_Enum_GetServiceInfoResult;

#define ACTUAL_DOM_Set_GetServiceInfoResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServiceInfoResult, 0 ? ACTUAL_DOM_Enum_GetServiceInfoResult_OK : (value))
#define ACTUAL_DOM_Append_GetServiceInfoResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServiceInfoResult, 0 ? ACTUAL_DOM_Enum_GetServiceInfoResult_OK : (value))
#define ACTUAL_DOM_Get_GetServiceInfoResult(pStruct, element) (ACTUAL_DOM_Enum_GetServiceInfoResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServiceInfoResult)
#define ACTUAL_DOM_GetEx_GetServiceInfoResult(pStruct, element, value) (ACTUAL_DOM_Enum_GetServiceInfoResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServiceInfoResult, 0 ? ACTUAL_DOM_Enum_GetServiceInfoResult_OK : (value))
#define ACTUAL_DOM_GetMember_GetServiceInfoResult(pMember) (ACTUAL_DOM_Enum_GetServiceInfoResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_GetServiceInfoResult)


/*
 * Enumeration http://cisco.com/HNAP/GetServicesResult
 */

typedef enum _ACTUAL_DOM_Enum_GetServicesResult
{
    ACTUAL_DOM_Enum_GetServicesResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_GetServicesResult_OK = 0,
    ACTUAL_DOM_Enum_GetServicesResult_ERROR = 1
} ACTUAL_DOM_Enum_GetServicesResult;

#define ACTUAL_DOM_Set_GetServicesResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServicesResult, 0 ? ACTUAL_DOM_Enum_GetServicesResult_OK : (value))
#define ACTUAL_DOM_Append_GetServicesResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServicesResult, 0 ? ACTUAL_DOM_Enum_GetServicesResult_OK : (value))
#define ACTUAL_DOM_Get_GetServicesResult(pStruct, element) (ACTUAL_DOM_Enum_GetServicesResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServicesResult)
#define ACTUAL_DOM_GetEx_GetServicesResult(pStruct, element, value) (ACTUAL_DOM_Enum_GetServicesResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_GetServicesResult, 0 ? ACTUAL_DOM_Enum_GetServicesResult_OK : (value))
#define ACTUAL_DOM_GetMember_GetServicesResult(pMember) (ACTUAL_DOM_Enum_GetServicesResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_GetServicesResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/AddPortMappingResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_AddPortMappingResult
{
    ACTUAL_DOM_Enum_PN_AddPortMappingResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_AddPortMappingResult_OK = 0,
    ACTUAL_DOM_Enum_PN_AddPortMappingResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_AddPortMappingResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_AddPortMappingResult;

#define ACTUAL_DOM_Set_PN_AddPortMappingResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_AddPortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_AddPortMappingResult_OK : (value))
#define ACTUAL_DOM_Append_PN_AddPortMappingResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_AddPortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_AddPortMappingResult_OK : (value))
#define ACTUAL_DOM_Get_PN_AddPortMappingResult(pStruct, element) (ACTUAL_DOM_Enum_PN_AddPortMappingResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_AddPortMappingResult)
#define ACTUAL_DOM_GetEx_PN_AddPortMappingResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_AddPortMappingResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_AddPortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_AddPortMappingResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_AddPortMappingResult(pMember) (ACTUAL_DOM_Enum_PN_AddPortMappingResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_AddPortMappingResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/DeletePortMappingResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_DeletePortMappingResult
{
    ACTUAL_DOM_Enum_PN_DeletePortMappingResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK = 0,
    ACTUAL_DOM_Enum_PN_DeletePortMappingResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_DeletePortMappingResult;

#define ACTUAL_DOM_Set_PN_DeletePortMappingResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK : (value))
#define ACTUAL_DOM_Append_PN_DeletePortMappingResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK : (value))
#define ACTUAL_DOM_Get_PN_DeletePortMappingResult(pStruct, element) (ACTUAL_DOM_Enum_PN_DeletePortMappingResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult)
#define ACTUAL_DOM_GetEx_PN_DeletePortMappingResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_DeletePortMappingResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult, 0 ? ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_DeletePortMappingResult(pMember) (ACTUAL_DOM_Enum_PN_DeletePortMappingResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/DeviceType
 */

typedef enum _ACTUAL_DOM_Enum_PN_DeviceType
{
    ACTUAL_DOM_Enum_PN_DeviceType__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_DeviceType_Computer = 0,
    ACTUAL_DOM_Enum_PN_DeviceType_ComputerServer = 1,
    ACTUAL_DOM_Enum_PN_DeviceType_DigitalDVR = 2,
    ACTUAL_DOM_Enum_PN_DeviceType_DigitalJukebox = 3,
    ACTUAL_DOM_Enum_PN_DeviceType_Gateway = 4,
    ACTUAL_DOM_Enum_PN_DeviceType_GatewayWithWiFi = 5,
    ACTUAL_DOM_Enum_PN_DeviceType_LaptopComputer = 6,
    ACTUAL_DOM_Enum_PN_DeviceType_MediaAdapter = 7,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkCamera = 8,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkDevice = 9,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkDrive = 10,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkGameConsole = 11,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkPDA = 12,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkPrintServer = 13,
    ACTUAL_DOM_Enum_PN_DeviceType_NetworkPrinter = 14,
    ACTUAL_DOM_Enum_PN_DeviceType_PhotoFrame = 15,
    ACTUAL_DOM_Enum_PN_DeviceType_SetTopBox = 16,
    ACTUAL_DOM_Enum_PN_DeviceType_VOIPDevice = 17,
    ACTUAL_DOM_Enum_PN_DeviceType_WiFiAccessPoint = 18,
    ACTUAL_DOM_Enum_PN_DeviceType_WiFiBridge = 19,
    ACTUAL_DOM_Enum_PN_DeviceType_WorkstationComputer = 20
} ACTUAL_DOM_Enum_PN_DeviceType;

#define ACTUAL_DOM_Set_PN_DeviceType(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeviceType, 0 ? ACTUAL_DOM_Enum_PN_DeviceType_Computer : (value))
#define ACTUAL_DOM_Append_PN_DeviceType(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeviceType, 0 ? ACTUAL_DOM_Enum_PN_DeviceType_Computer : (value))
#define ACTUAL_DOM_Get_PN_DeviceType(pStruct, element) (ACTUAL_DOM_Enum_PN_DeviceType*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeviceType)
#define ACTUAL_DOM_GetEx_PN_DeviceType(pStruct, element, value) (ACTUAL_DOM_Enum_PN_DeviceType)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DeviceType, 0 ? ACTUAL_DOM_Enum_PN_DeviceType_Computer : (value))
#define ACTUAL_DOM_GetMember_PN_DeviceType(pMember) (ACTUAL_DOM_Enum_PN_DeviceType*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_DeviceType)


/*
 * Enumeration http://purenetworks.com/HNAP1/DownloadSpeedTestResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult
{
    ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK = 0,
    ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult;

#define ACTUAL_DOM_Set_PN_DownloadSpeedTestResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult, 0 ? ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK : (value))
#define ACTUAL_DOM_Append_PN_DownloadSpeedTestResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult, 0 ? ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK : (value))
#define ACTUAL_DOM_Get_PN_DownloadSpeedTestResult(pStruct, element) (ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult)
#define ACTUAL_DOM_GetEx_PN_DownloadSpeedTestResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult, 0 ? ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_DownloadSpeedTestResult(pMember) (ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/FirmwareUploadResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_FirmwareUploadResult
{
    ACTUAL_DOM_Enum_PN_FirmwareUploadResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_FirmwareUploadResult_OK = 0,
    ACTUAL_DOM_Enum_PN_FirmwareUploadResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_FirmwareUploadResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_FirmwareUploadResult;

#define ACTUAL_DOM_Set_PN_FirmwareUploadResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult, 0 ? ACTUAL_DOM_Enum_PN_FirmwareUploadResult_OK : (value))
#define ACTUAL_DOM_Append_PN_FirmwareUploadResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult, 0 ? ACTUAL_DOM_Enum_PN_FirmwareUploadResult_OK : (value))
#define ACTUAL_DOM_Get_PN_FirmwareUploadResult(pStruct, element) (ACTUAL_DOM_Enum_PN_FirmwareUploadResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult)
#define ACTUAL_DOM_GetEx_PN_FirmwareUploadResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_FirmwareUploadResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult, 0 ? ACTUAL_DOM_Enum_PN_FirmwareUploadResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_FirmwareUploadResult(pMember) (ACTUAL_DOM_Enum_PN_FirmwareUploadResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetClientStatsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetClientStatsResult
{
    ACTUAL_DOM_Enum_PN_GetClientStatsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetClientStatsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetClientStatsResult;

#define ACTUAL_DOM_Set_PN_GetClientStatsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetClientStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetClientStatsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetClientStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetClientStatsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetClientStatsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetClientStatsResult)
#define ACTUAL_DOM_GetEx_PN_GetClientStatsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetClientStatsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetClientStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetClientStatsResult(pMember) (ACTUAL_DOM_Enum_PN_GetClientStatsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetClientStatsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetConfigBlobResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetConfigBlobResult
{
    ACTUAL_DOM_Enum_PN_GetConfigBlobResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetConfigBlobResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetConfigBlobResult;

#define ACTUAL_DOM_Set_PN_GetConfigBlobResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetConfigBlobResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetConfigBlobResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetConfigBlobResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult)
#define ACTUAL_DOM_GetEx_PN_GetConfigBlobResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetConfigBlobResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetConfigBlobResult(pMember) (ACTUAL_DOM_Enum_PN_GetConfigBlobResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetConnectedDevicesResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult
{
    ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult;

#define ACTUAL_DOM_Set_PN_GetConnectedDevicesResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult, 0 ? ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetConnectedDevicesResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult, 0 ? ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetConnectedDevicesResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult)
#define ACTUAL_DOM_GetEx_PN_GetConnectedDevicesResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult, 0 ? ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetConnectedDevicesResult(pMember) (ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetDeviceSettings2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result
{
    ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result;

#define ACTUAL_DOM_Set_PN_GetDeviceSettings2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_GetDeviceSettings2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_GetDeviceSettings2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result)
#define ACTUAL_DOM_GetEx_PN_GetDeviceSettings2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetDeviceSettings2Result(pMember) (ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetDeviceSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult
{
    ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult;

#define ACTUAL_DOM_Set_PN_GetDeviceSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetDeviceSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetDeviceSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult)
#define ACTUAL_DOM_GetEx_PN_GetDeviceSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetDeviceSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetFirmwareSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult
{
    ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult;

#define ACTUAL_DOM_Set_PN_GetFirmwareSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetFirmwareSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetFirmwareSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult)
#define ACTUAL_DOM_GetEx_PN_GetFirmwareSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetFirmwareSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetMACFilters2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetMACFilters2Result
{
    ACTUAL_DOM_Enum_PN_GetMACFilters2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_GetMACFilters2Result_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetMACFilters2Result;

#define ACTUAL_DOM_Set_PN_GetMACFilters2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_GetMACFilters2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_GetMACFilters2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_GetMACFilters2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result)
#define ACTUAL_DOM_GetEx_PN_GetMACFilters2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetMACFilters2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetMACFilters2Result(pMember) (ACTUAL_DOM_Enum_PN_GetMACFilters2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetNetworkStatsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetNetworkStatsResult
{
    ACTUAL_DOM_Enum_PN_GetNetworkStatsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetNetworkStatsResult;

#define ACTUAL_DOM_Set_PN_GetNetworkStatsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetNetworkStatsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetNetworkStatsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetNetworkStatsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult)
#define ACTUAL_DOM_GetEx_PN_GetNetworkStatsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetNetworkStatsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult, 0 ? ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetNetworkStatsResult(pMember) (ACTUAL_DOM_Enum_PN_GetNetworkStatsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetPortMappingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetPortMappingsResult
{
    ACTUAL_DOM_Enum_PN_GetPortMappingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetPortMappingsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetPortMappingsResult;

#define ACTUAL_DOM_Set_PN_GetPortMappingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetPortMappingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetPortMappingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetPortMappingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult)
#define ACTUAL_DOM_GetEx_PN_GetPortMappingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetPortMappingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetPortMappingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetPortMappingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetRouterLanSettings2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result
{
    ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result;

#define ACTUAL_DOM_Set_PN_GetRouterLanSettings2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_GetRouterLanSettings2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_GetRouterLanSettings2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result)
#define ACTUAL_DOM_GetEx_PN_GetRouterLanSettings2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetRouterLanSettings2Result(pMember) (ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetRouterSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetRouterSettingsResult
{
    ACTUAL_DOM_Enum_PN_GetRouterSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetRouterSettingsResult;

#define ACTUAL_DOM_Set_PN_GetRouterSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetRouterSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetRouterSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetRouterSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult)
#define ACTUAL_DOM_GetEx_PN_GetRouterSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetRouterSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetRouterSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetRouterSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioFrequenciesResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult
{
    ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult;

#define ACTUAL_DOM_Set_PN_GetWLanRadioFrequenciesResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWLanRadioFrequenciesResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWLanRadioFrequenciesResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult)
#define ACTUAL_DOM_GetEx_PN_GetWLanRadioFrequenciesResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWLanRadioFrequenciesResult(pMember) (ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioSecurityResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult
{
    ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_ERROR = 1,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_ERROR_BAD_RADIOID = 2
} ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult;

#define ACTUAL_DOM_Set_PN_GetWLanRadioSecurityResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWLanRadioSecurityResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWLanRadioSecurityResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult)
#define ACTUAL_DOM_GetEx_PN_GetWLanRadioSecurityResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWLanRadioSecurityResult(pMember) (ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult
{
    ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_ERROR = 1,
    ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_ERROR_BAD_RADIOID = 2
} ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult;

#define ACTUAL_DOM_Set_PN_GetWLanRadioSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWLanRadioSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWLanRadioSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult)
#define ACTUAL_DOM_GetEx_PN_GetWLanRadioSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWLanRadioSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadiosResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWLanRadiosResult
{
    ACTUAL_DOM_Enum_PN_GetWLanRadiosResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetWLanRadiosResult;

#define ACTUAL_DOM_Set_PN_GetWLanRadiosResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWLanRadiosResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWLanRadiosResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWLanRadiosResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult)
#define ACTUAL_DOM_GetEx_PN_GetWLanRadiosResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWLanRadiosResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult, 0 ? ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWLanRadiosResult(pMember) (ACTUAL_DOM_Enum_PN_GetWLanRadiosResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWanInfoResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWanInfoResult
{
    ACTUAL_DOM_Enum_PN_GetWanInfoResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWanInfoResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetWanInfoResult;

#define ACTUAL_DOM_Set_PN_GetWanInfoResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanInfoResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWanInfoResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanInfoResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWanInfoResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWanInfoResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanInfoResult)
#define ACTUAL_DOM_GetEx_PN_GetWanInfoResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWanInfoResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanInfoResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWanInfoResult(pMember) (ACTUAL_DOM_Enum_PN_GetWanInfoResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWanInfoResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWanSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_GetWanSettingsResult
{
    ACTUAL_DOM_Enum_PN_GetWanSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_GetWanSettingsResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_GetWanSettingsResult;

#define ACTUAL_DOM_Set_PN_GetWanSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_GetWanSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_GetWanSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_GetWanSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult)
#define ACTUAL_DOM_GetEx_PN_GetWanSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_GetWanSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_GetWanSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_GetWanSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/IPProtocol
 */

typedef enum _ACTUAL_DOM_Enum_PN_IPProtocol
{
    ACTUAL_DOM_Enum_PN_IPProtocol__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_IPProtocol_TCP = 0,
    ACTUAL_DOM_Enum_PN_IPProtocol_UDP = 1
} ACTUAL_DOM_Enum_PN_IPProtocol;

#define ACTUAL_DOM_Set_PN_IPProtocol(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 ? ACTUAL_DOM_Enum_PN_IPProtocol_TCP : (value))
#define ACTUAL_DOM_Append_PN_IPProtocol(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 ? ACTUAL_DOM_Enum_PN_IPProtocol_TCP : (value))
#define ACTUAL_DOM_Get_PN_IPProtocol(pStruct, element) (ACTUAL_DOM_Enum_PN_IPProtocol*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IPProtocol)
#define ACTUAL_DOM_GetEx_PN_IPProtocol(pStruct, element, value) (ACTUAL_DOM_Enum_PN_IPProtocol)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 ? ACTUAL_DOM_Enum_PN_IPProtocol_TCP : (value))
#define ACTUAL_DOM_GetMember_PN_IPProtocol(pMember) (ACTUAL_DOM_Enum_PN_IPProtocol*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_IPProtocol)


/*
 * Enumeration http://purenetworks.com/HNAP1/IsDeviceReadyResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_IsDeviceReadyResult
{
    ACTUAL_DOM_Enum_PN_IsDeviceReadyResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK = 0,
    ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_IsDeviceReadyResult;

#define ACTUAL_DOM_Set_PN_IsDeviceReadyResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult, 0 ? ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK : (value))
#define ACTUAL_DOM_Append_PN_IsDeviceReadyResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult, 0 ? ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK : (value))
#define ACTUAL_DOM_Get_PN_IsDeviceReadyResult(pStruct, element) (ACTUAL_DOM_Enum_PN_IsDeviceReadyResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult)
#define ACTUAL_DOM_GetEx_PN_IsDeviceReadyResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_IsDeviceReadyResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult, 0 ? ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_IsDeviceReadyResult(pMember) (ACTUAL_DOM_Enum_PN_IsDeviceReadyResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/LANConnection
 */

typedef enum _ACTUAL_DOM_Enum_PN_LANConnection
{
    ACTUAL_DOM_Enum_PN_LANConnection__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_LANConnection_LAN = 0,
    ACTUAL_DOM_Enum_PN_LANConnection_WAN = 1,
    ACTUAL_DOM_Enum_PN_LANConnection_WLAN_802_11a = 2,
    ACTUAL_DOM_Enum_PN_LANConnection_WLAN_802_11b = 3,
    ACTUAL_DOM_Enum_PN_LANConnection_WLAN_802_11g = 4,
    ACTUAL_DOM_Enum_PN_LANConnection_WLAN_802_11n = 5
} ACTUAL_DOM_Enum_PN_LANConnection;

#define ACTUAL_DOM_Set_PN_LANConnection(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_LANConnection, 0 ? ACTUAL_DOM_Enum_PN_LANConnection_LAN : (value))
#define ACTUAL_DOM_Append_PN_LANConnection(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_LANConnection, 0 ? ACTUAL_DOM_Enum_PN_LANConnection_LAN : (value))
#define ACTUAL_DOM_Get_PN_LANConnection(pStruct, element) (ACTUAL_DOM_Enum_PN_LANConnection*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_LANConnection)
#define ACTUAL_DOM_GetEx_PN_LANConnection(pStruct, element, value) (ACTUAL_DOM_Enum_PN_LANConnection)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_LANConnection, 0 ? ACTUAL_DOM_Enum_PN_LANConnection_LAN : (value))
#define ACTUAL_DOM_GetMember_PN_LANConnection(pMember) (ACTUAL_DOM_Enum_PN_LANConnection*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_LANConnection)


/*
 * Enumeration http://purenetworks.com/HNAP1/RebootResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_RebootResult
{
    ACTUAL_DOM_Enum_PN_RebootResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_RebootResult_OK = 0,
    ACTUAL_DOM_Enum_PN_RebootResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_RebootResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_RebootResult;

#define ACTUAL_DOM_Set_PN_RebootResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RebootResult, 0 ? ACTUAL_DOM_Enum_PN_RebootResult_OK : (value))
#define ACTUAL_DOM_Append_PN_RebootResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RebootResult, 0 ? ACTUAL_DOM_Enum_PN_RebootResult_OK : (value))
#define ACTUAL_DOM_Get_PN_RebootResult(pStruct, element) (ACTUAL_DOM_Enum_PN_RebootResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RebootResult)
#define ACTUAL_DOM_GetEx_PN_RebootResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_RebootResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RebootResult, 0 ? ACTUAL_DOM_Enum_PN_RebootResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_RebootResult(pMember) (ACTUAL_DOM_Enum_PN_RebootResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_RebootResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/RenewWanConnectionResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_RenewWanConnectionResult
{
    ACTUAL_DOM_Enum_PN_RenewWanConnectionResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK = 0,
    ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_ERROR = 1
} ACTUAL_DOM_Enum_PN_RenewWanConnectionResult;

#define ACTUAL_DOM_Set_PN_RenewWanConnectionResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult, 0 ? ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK : (value))
#define ACTUAL_DOM_Append_PN_RenewWanConnectionResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult, 0 ? ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK : (value))
#define ACTUAL_DOM_Get_PN_RenewWanConnectionResult(pStruct, element) (ACTUAL_DOM_Enum_PN_RenewWanConnectionResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult)
#define ACTUAL_DOM_GetEx_PN_RenewWanConnectionResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_RenewWanConnectionResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult, 0 ? ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_RenewWanConnectionResult(pMember) (ACTUAL_DOM_Enum_PN_RenewWanConnectionResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/RestoreFactoryDefaultsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult
{
    ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult;

#define ACTUAL_DOM_Set_PN_RestoreFactoryDefaultsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult, 0 ? ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_RestoreFactoryDefaultsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult, 0 ? ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_RestoreFactoryDefaultsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult)
#define ACTUAL_DOM_GetEx_PN_RestoreFactoryDefaultsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult, 0 ? ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_RestoreFactoryDefaultsResult(pMember) (ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetAccessPointModeResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetAccessPointModeResult
{
    ACTUAL_DOM_Enum_PN_SetAccessPointModeResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_SetAccessPointModeResult;

#define ACTUAL_DOM_Set_PN_SetAccessPointModeResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult, 0 ? ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetAccessPointModeResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult, 0 ? ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetAccessPointModeResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetAccessPointModeResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult)
#define ACTUAL_DOM_GetEx_PN_SetAccessPointModeResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetAccessPointModeResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult, 0 ? ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetAccessPointModeResult(pMember) (ACTUAL_DOM_Enum_PN_SetAccessPointModeResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetConfigBlobResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetConfigBlobResult
{
    ACTUAL_DOM_Enum_PN_SetConfigBlobResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetConfigBlobResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetConfigBlobResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetConfigBlobResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_SetConfigBlobResult;

#define ACTUAL_DOM_Set_PN_SetConfigBlobResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_SetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetConfigBlobResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_SetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetConfigBlobResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetConfigBlobResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult)
#define ACTUAL_DOM_GetEx_PN_SetConfigBlobResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetConfigBlobResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult, 0 ? ACTUAL_DOM_Enum_PN_SetConfigBlobResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetConfigBlobResult(pMember) (ACTUAL_DOM_Enum_PN_SetConfigBlobResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetDeviceSettings2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result
{
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_ERROR_REMOTE_SSL_NEEDS_SSL = 3,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_ERROR_TIMEZONE_NOT_SUPPORTED = 4,
    ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_ERROR_USERNAME_NOT_SUPPORTED = 5
} ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result;

#define ACTUAL_DOM_Set_PN_SetDeviceSettings2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_SetDeviceSettings2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_SetDeviceSettings2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result)
#define ACTUAL_DOM_GetEx_PN_SetDeviceSettings2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetDeviceSettings2Result(pMember) (ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetDeviceSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult
{
    ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_ERROR = 2
} ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult;

#define ACTUAL_DOM_Set_PN_SetDeviceSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetDeviceSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetDeviceSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult)
#define ACTUAL_DOM_GetEx_PN_SetDeviceSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetDeviceSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetMACFilters2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetMACFilters2Result
{
    ACTUAL_DOM_Enum_PN_SetMACFilters2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetMACFilters2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_SetMACFilters2Result_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetMACFilters2Result_ERROR = 2
} ACTUAL_DOM_Enum_PN_SetMACFilters2Result;

#define ACTUAL_DOM_Set_PN_SetMACFilters2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_SetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_SetMACFilters2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_SetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_SetMACFilters2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_SetMACFilters2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result)
#define ACTUAL_DOM_GetEx_PN_SetMACFilters2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetMACFilters2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result, 0 ? ACTUAL_DOM_Enum_PN_SetMACFilters2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetMACFilters2Result(pMember) (ACTUAL_DOM_Enum_PN_SetMACFilters2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetRouterLanSettings2Result
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result
{
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK = 0,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR = 1,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR_BAD_IP_ADDRESS = 2,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR_BAD_IP_RANGE = 3,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR_BAD_RESERVATION = 4,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR_BAD_SUBNET = 5,
    ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_ERROR_RESERVATIONS_NOT_SUPPORTED = 6
} ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result;

#define ACTUAL_DOM_Set_PN_SetRouterLanSettings2Result(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_Append_PN_SetRouterLanSettings2Result(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_Get_PN_SetRouterLanSettings2Result(pStruct, element) (ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result)
#define ACTUAL_DOM_GetEx_PN_SetRouterLanSettings2Result(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result, 0 ? ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetRouterLanSettings2Result(pMember) (ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetRouterSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetRouterSettingsResult
{
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_DOMAIN_NOT_SUPPORTED = 3,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_QOS_NOT_SUPPORTED = 4,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_REMOTE_MANAGE_DEFAULT_PASSWORD = 5,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_REMOTE_MANAGE_MUST_BE_SSL = 6,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_REMOTE_MANAGE_NOT_SUPPORTED = 7,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_REMOTE_SSL_NEEDS_SSL = 8,
    ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_ERROR_REMOTE_SSL_NOT_SUPPORTED = 9
} ACTUAL_DOM_Enum_PN_SetRouterSettingsResult;

#define ACTUAL_DOM_Set_PN_SetRouterSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetRouterSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetRouterSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetRouterSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult)
#define ACTUAL_DOM_GetEx_PN_SetRouterSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetRouterSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetRouterSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_SetRouterSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioFrequencyResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult
{
    ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_ERROR_BAD_RADIOID = 3
} ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult;

#define ACTUAL_DOM_Set_PN_SetWLanRadioFrequencyResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetWLanRadioFrequencyResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetWLanRadioFrequencyResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult)
#define ACTUAL_DOM_GetEx_PN_SetWLanRadioFrequencyResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetWLanRadioFrequencyResult(pMember) (ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioSecurityResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult
{
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_BAD_RADIOID = 3,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_BAD_RADIUS_VALUES = 4,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_ENCRYPTION_NOT_SUPPORTED = 5,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_ILLEGAL_KEY_VALUE = 6,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_KEY_RENEWAL_BAD_VALUE = 7,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_ERROR_TYPE_NOT_SUPPORTED = 8
} ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult;

#define ACTUAL_DOM_Set_PN_SetWLanRadioSecurityResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetWLanRadioSecurityResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetWLanRadioSecurityResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult)
#define ACTUAL_DOM_GetEx_PN_SetWLanRadioSecurityResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetWLanRadioSecurityResult(pMember) (ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult
{
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_CHANNEL = 3,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_CHANNEL_WIDTH = 4,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_MODE = 5,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_RADIOID = 6,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_SECONDARY_CHANNEL = 7,
    ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_ERROR_BAD_SSID = 8
} ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult;

#define ACTUAL_DOM_Set_PN_SetWLanRadioSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetWLanRadioSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetWLanRadioSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult)
#define ACTUAL_DOM_GetEx_PN_SetWLanRadioSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetWLanRadioSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWanSettingsResult
 */

typedef enum _ACTUAL_DOM_Enum_PN_SetWanSettingsResult
{
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult_OK = 0,
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult_REBOOT = 1,
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult_ERROR = 2,
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult_ERROR_AUTO_MTU_NOT_SUPPORTED = 3,
    ACTUAL_DOM_Enum_PN_SetWanSettingsResult_ERROR_BAD_WANTYPE = 4
} ACTUAL_DOM_Enum_PN_SetWanSettingsResult;

#define ACTUAL_DOM_Set_PN_SetWanSettingsResult(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_Append_PN_SetWanSettingsResult(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_Get_PN_SetWanSettingsResult(pStruct, element) (ACTUAL_DOM_Enum_PN_SetWanSettingsResult*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult)
#define ACTUAL_DOM_GetEx_PN_SetWanSettingsResult(pStruct, element, value) (ACTUAL_DOM_Enum_PN_SetWanSettingsResult)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult, 0 ? ACTUAL_DOM_Enum_PN_SetWanSettingsResult_OK : (value))
#define ACTUAL_DOM_GetMember_PN_SetWanSettingsResult(pMember) (ACTUAL_DOM_Enum_PN_SetWanSettingsResult*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult)


/*
 * Enumeration http://purenetworks.com/HNAP1/TaskExtType
 */

typedef enum _ACTUAL_DOM_Enum_PN_TaskExtType
{
    ACTUAL_DOM_Enum_PN_TaskExtType__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_TaskExtType_Browser = 0,
    ACTUAL_DOM_Enum_PN_TaskExtType_MessageBox = 1,
    ACTUAL_DOM_Enum_PN_TaskExtType_PUI = 2,
    ACTUAL_DOM_Enum_PN_TaskExtType_Silent = 3
} ACTUAL_DOM_Enum_PN_TaskExtType;

#define ACTUAL_DOM_Set_PN_TaskExtType(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 ? ACTUAL_DOM_Enum_PN_TaskExtType_Browser : (value))
#define ACTUAL_DOM_Append_PN_TaskExtType(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 ? ACTUAL_DOM_Enum_PN_TaskExtType_Browser : (value))
#define ACTUAL_DOM_Get_PN_TaskExtType(pStruct, element) (ACTUAL_DOM_Enum_PN_TaskExtType*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_TaskExtType)
#define ACTUAL_DOM_GetEx_PN_TaskExtType(pStruct, element, value) (ACTUAL_DOM_Enum_PN_TaskExtType)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 ? ACTUAL_DOM_Enum_PN_TaskExtType_Browser : (value))
#define ACTUAL_DOM_GetMember_PN_TaskExtType(pMember) (ACTUAL_DOM_Enum_PN_TaskExtType*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_TaskExtType)


/*
 * Enumeration http://purenetworks.com/HNAP1/UpdateMethod
 */

typedef enum _ACTUAL_DOM_Enum_PN_UpdateMethod
{
    ACTUAL_DOM_Enum_PN_UpdateMethod__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_UpdateMethod_HNAP_UPLOAD = 0,
    ACTUAL_DOM_Enum_PN_UpdateMethod_TFTP_UPLOAD = 1
} ACTUAL_DOM_Enum_PN_UpdateMethod;

#define ACTUAL_DOM_Set_PN_UpdateMethod(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_UpdateMethod, 0 ? ACTUAL_DOM_Enum_PN_UpdateMethod_HNAP_UPLOAD : (value))
#define ACTUAL_DOM_Append_PN_UpdateMethod(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_UpdateMethod, 0 ? ACTUAL_DOM_Enum_PN_UpdateMethod_HNAP_UPLOAD : (value))
#define ACTUAL_DOM_Get_PN_UpdateMethod(pStruct, element) (ACTUAL_DOM_Enum_PN_UpdateMethod*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_UpdateMethod)
#define ACTUAL_DOM_GetEx_PN_UpdateMethod(pStruct, element, value) (ACTUAL_DOM_Enum_PN_UpdateMethod)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_UpdateMethod, 0 ? ACTUAL_DOM_Enum_PN_UpdateMethod_HNAP_UPLOAD : (value))
#define ACTUAL_DOM_GetMember_PN_UpdateMethod(pMember) (ACTUAL_DOM_Enum_PN_UpdateMethod*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_UpdateMethod)


/*
 * Enumeration http://purenetworks.com/HNAP1/WANStatus
 */

typedef enum _ACTUAL_DOM_Enum_PN_WANStatus
{
    ACTUAL_DOM_Enum_PN_WANStatus__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_WANStatus_CONNECTED = 0,
    ACTUAL_DOM_Enum_PN_WANStatus_CONNECTING = 1,
    ACTUAL_DOM_Enum_PN_WANStatus_DISCONNECTED = 2,
    ACTUAL_DOM_Enum_PN_WANStatus_LIMITED_CONNECTION = 3,
    ACTUAL_DOM_Enum_PN_WANStatus_UNKNOWN = 4
} ACTUAL_DOM_Enum_PN_WANStatus;

#define ACTUAL_DOM_Set_PN_WANStatus(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANStatus, 0 ? ACTUAL_DOM_Enum_PN_WANStatus_CONNECTED : (value))
#define ACTUAL_DOM_Append_PN_WANStatus(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANStatus, 0 ? ACTUAL_DOM_Enum_PN_WANStatus_CONNECTED : (value))
#define ACTUAL_DOM_Get_PN_WANStatus(pStruct, element) (ACTUAL_DOM_Enum_PN_WANStatus*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANStatus)
#define ACTUAL_DOM_GetEx_PN_WANStatus(pStruct, element, value) (ACTUAL_DOM_Enum_PN_WANStatus)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANStatus, 0 ? ACTUAL_DOM_Enum_PN_WANStatus_CONNECTED : (value))
#define ACTUAL_DOM_GetMember_PN_WANStatus(pMember) (ACTUAL_DOM_Enum_PN_WANStatus*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_WANStatus)


/*
 * Enumeration http://purenetworks.com/HNAP1/WANType
 */

typedef enum _ACTUAL_DOM_Enum_PN_WANType
{
    ACTUAL_DOM_Enum_PN_WANType__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_WANType_BigPond = 0,
    ACTUAL_DOM_Enum_PN_WANType_BridgedOnly = 1,
    ACTUAL_DOM_Enum_PN_WANType_DETECTING = 2,
    ACTUAL_DOM_Enum_PN_WANType_DHCP = 3,
    ACTUAL_DOM_Enum_PN_WANType_DHCPPPPoE = 4,
    ACTUAL_DOM_Enum_PN_WANType_Dynamic1483Bridged = 5,
    ACTUAL_DOM_Enum_PN_WANType_DynamicL2TP = 6,
    ACTUAL_DOM_Enum_PN_WANType_DynamicPPPOA = 7,
    ACTUAL_DOM_Enum_PN_WANType_DynamicPPTP = 8,
    ACTUAL_DOM_Enum_PN_WANType_Static = 9,
    ACTUAL_DOM_Enum_PN_WANType_Static1483Bridged = 10,
    ACTUAL_DOM_Enum_PN_WANType_Static1483Routed = 11,
    ACTUAL_DOM_Enum_PN_WANType_StaticIPOA = 12,
    ACTUAL_DOM_Enum_PN_WANType_StaticL2TP = 13,
    ACTUAL_DOM_Enum_PN_WANType_StaticPPPOA = 14,
    ACTUAL_DOM_Enum_PN_WANType_StaticPPPoE = 15,
    ACTUAL_DOM_Enum_PN_WANType_StaticPPTP = 16,
    ACTUAL_DOM_Enum_PN_WANType_UNKNOWN = 17
} ACTUAL_DOM_Enum_PN_WANType;

#define ACTUAL_DOM_Set_PN_WANType(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANType, 0 ? ACTUAL_DOM_Enum_PN_WANType_BigPond : (value))
#define ACTUAL_DOM_Append_PN_WANType(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANType, 0 ? ACTUAL_DOM_Enum_PN_WANType_BigPond : (value))
#define ACTUAL_DOM_Get_PN_WANType(pStruct, element) (ACTUAL_DOM_Enum_PN_WANType*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANType)
#define ACTUAL_DOM_GetEx_PN_WANType(pStruct, element, value) (ACTUAL_DOM_Enum_PN_WANType)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WANType, 0 ? ACTUAL_DOM_Enum_PN_WANType_BigPond : (value))
#define ACTUAL_DOM_GetMember_PN_WANType(pMember) (ACTUAL_DOM_Enum_PN_WANType*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_WANType)


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiEncryption
 */

typedef enum _ACTUAL_DOM_Enum_PN_WiFiEncryption
{
    ACTUAL_DOM_Enum_PN_WiFiEncryption__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_ = 0,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_AES = 1,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_TKIP = 2,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_TKIPORAES = 3,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_WEP_128 = 4,
    ACTUAL_DOM_Enum_PN_WiFiEncryption_WEP_64 = 5
} ACTUAL_DOM_Enum_PN_WiFiEncryption;

#define ACTUAL_DOM_Set_PN_WiFiEncryption(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 ? ACTUAL_DOM_Enum_PN_WiFiEncryption_ : (value))
#define ACTUAL_DOM_Append_PN_WiFiEncryption(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 ? ACTUAL_DOM_Enum_PN_WiFiEncryption_ : (value))
#define ACTUAL_DOM_Get_PN_WiFiEncryption(pStruct, element) (ACTUAL_DOM_Enum_PN_WiFiEncryption*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiEncryption)
#define ACTUAL_DOM_GetEx_PN_WiFiEncryption(pStruct, element, value) (ACTUAL_DOM_Enum_PN_WiFiEncryption)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 ? ACTUAL_DOM_Enum_PN_WiFiEncryption_ : (value))
#define ACTUAL_DOM_GetMember_PN_WiFiEncryption(pMember) (ACTUAL_DOM_Enum_PN_WiFiEncryption*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_WiFiEncryption)


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiMode
 */

typedef enum _ACTUAL_DOM_Enum_PN_WiFiMode
{
    ACTUAL_DOM_Enum_PN_WiFiMode__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_WiFiMode_ = 0,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11a = 1,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11an = 2,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11b = 3,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11bg = 4,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11bgn = 5,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11bn = 6,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11g = 7,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11gn = 8,
    ACTUAL_DOM_Enum_PN_WiFiMode_802_11n = 9
} ACTUAL_DOM_Enum_PN_WiFiMode;

#define ACTUAL_DOM_Set_PN_WiFiMode(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 ? ACTUAL_DOM_Enum_PN_WiFiMode_ : (value))
#define ACTUAL_DOM_Append_PN_WiFiMode(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 ? ACTUAL_DOM_Enum_PN_WiFiMode_ : (value))
#define ACTUAL_DOM_Get_PN_WiFiMode(pStruct, element) (ACTUAL_DOM_Enum_PN_WiFiMode*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiMode)
#define ACTUAL_DOM_GetEx_PN_WiFiMode(pStruct, element, value) (ACTUAL_DOM_Enum_PN_WiFiMode)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 ? ACTUAL_DOM_Enum_PN_WiFiMode_ : (value))
#define ACTUAL_DOM_GetMember_PN_WiFiMode(pMember) (ACTUAL_DOM_Enum_PN_WiFiMode*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_WiFiMode)


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiSecurity
 */

typedef enum _ACTUAL_DOM_Enum_PN_WiFiSecurity
{
    ACTUAL_DOM_Enum_PN_WiFiSecurity__UNKNOWN__ = HDK_XML_Enum_Unknown,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_ = 0,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WEP = 1,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WEP_AUTO = 2,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WEP_OPEN = 3,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WEP_RADIUS = 4,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WEP_SHARED = 5,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WPA_AUTO_PSK = 6,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WPA_PSK = 7,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WPA_RADIUS = 8,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WPA2_PSK = 9,
    ACTUAL_DOM_Enum_PN_WiFiSecurity_WPA2_RADIUS = 10
} ACTUAL_DOM_Enum_PN_WiFiSecurity;

#define ACTUAL_DOM_Set_PN_WiFiSecurity(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 ? ACTUAL_DOM_Enum_PN_WiFiSecurity_ : (value))
#define ACTUAL_DOM_Append_PN_WiFiSecurity(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 ? ACTUAL_DOM_Enum_PN_WiFiSecurity_ : (value))
#define ACTUAL_DOM_Get_PN_WiFiSecurity(pStruct, element) (ACTUAL_DOM_Enum_PN_WiFiSecurity*)HDK_XML_Get_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiSecurity)
#define ACTUAL_DOM_GetEx_PN_WiFiSecurity(pStruct, element, value) (ACTUAL_DOM_Enum_PN_WiFiSecurity)HDK_XML_GetEx_Enum(pStruct, element, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 ? ACTUAL_DOM_Enum_PN_WiFiSecurity_ : (value))
#define ACTUAL_DOM_GetMember_PN_WiFiSecurity(pMember) (ACTUAL_DOM_Enum_PN_WiFiSecurity*)HDK_XML_GetMember_Enum(pMember, ACTUAL_DOM_EnumType_PN_WiFiSecurity)


/*
 * Method enumeration
 */

typedef enum _ACTUAL_DOM_MethodEnum
{
    ACTUAL_DOM_MethodEnum_GetServiceInfo = 0,
    ACTUAL_DOM_MethodEnum_GetServices = 1,
    ACTUAL_DOM_MethodEnum_PN_AddPortMapping = 2,
    ACTUAL_DOM_MethodEnum_PN_DeletePortMapping = 3,
    ACTUAL_DOM_MethodEnum_PN_DownloadSpeedTest = 4,
    ACTUAL_DOM_MethodEnum_PN_FirmwareUpload = 5,
    ACTUAL_DOM_MethodEnum_PN_GetClientStats = 6,
    ACTUAL_DOM_MethodEnum_PN_GetConfigBlob = 7,
    ACTUAL_DOM_MethodEnum_PN_GetConnectedDevices = 8,
    ACTUAL_DOM_MethodEnum_PN_GetDeviceSettings = 9,
    ACTUAL_DOM_MethodEnum_GET_PN_GetDeviceSettings = 10,
    ACTUAL_DOM_MethodEnum_PN_GetDeviceSettings2 = 11,
    ACTUAL_DOM_MethodEnum_PN_GetFirmwareSettings = 12,
    ACTUAL_DOM_MethodEnum_PN_GetMACFilters2 = 13,
    ACTUAL_DOM_MethodEnum_PN_GetNetworkStats = 14,
    ACTUAL_DOM_MethodEnum_PN_GetPortMappings = 15,
    ACTUAL_DOM_MethodEnum_PN_GetRouterLanSettings2 = 16,
    ACTUAL_DOM_MethodEnum_PN_GetRouterSettings = 17,
    ACTUAL_DOM_MethodEnum_PN_GetWLanRadioFrequencies = 18,
    ACTUAL_DOM_MethodEnum_PN_GetWLanRadioSecurity = 19,
    ACTUAL_DOM_MethodEnum_PN_GetWLanRadioSettings = 20,
    ACTUAL_DOM_MethodEnum_PN_GetWLanRadios = 21,
    ACTUAL_DOM_MethodEnum_PN_GetWanInfo = 22,
    ACTUAL_DOM_MethodEnum_PN_GetWanSettings = 23,
    ACTUAL_DOM_MethodEnum_PN_IsDeviceReady = 24,
    ACTUAL_DOM_MethodEnum_PN_Reboot = 25,
    ACTUAL_DOM_MethodEnum_PN_RenewWanConnection = 26,
    ACTUAL_DOM_MethodEnum_PN_RestoreFactoryDefaults = 27,
    ACTUAL_DOM_MethodEnum_PN_SetAccessPointMode = 28,
    ACTUAL_DOM_MethodEnum_PN_SetConfigBlob = 29,
    ACTUAL_DOM_MethodEnum_PN_SetDeviceSettings = 30,
    ACTUAL_DOM_MethodEnum_PN_SetDeviceSettings2 = 31,
    ACTUAL_DOM_MethodEnum_PN_SetMACFilters2 = 32,
    ACTUAL_DOM_MethodEnum_PN_SetRouterLanSettings2 = 33,
    ACTUAL_DOM_MethodEnum_PN_SetRouterSettings = 34,
    ACTUAL_DOM_MethodEnum_PN_SetWLanRadioFrequency = 35,
    ACTUAL_DOM_MethodEnum_PN_SetWLanRadioSecurity = 36,
    ACTUAL_DOM_MethodEnum_PN_SetWLanRadioSettings = 37,
    ACTUAL_DOM_MethodEnum_PN_SetWanSettings = 38
} ACTUAL_DOM_MethodEnum;


/*
 * Method sentinels
 */

#define __ACTUAL_DOM_METHOD_GETSERVICEINFO__
#define __ACTUAL_DOM_METHOD_GETSERVICES__
#define __ACTUAL_DOM_METHOD_PN_ADDPORTMAPPING__
#define __ACTUAL_DOM_METHOD_PN_DELETEPORTMAPPING__
#define __ACTUAL_DOM_METHOD_PN_DOWNLOADSPEEDTEST__
#define __ACTUAL_DOM_METHOD_PN_FIRMWAREUPLOAD__
#define __ACTUAL_DOM_METHOD_PN_GETCLIENTSTATS__
#define __ACTUAL_DOM_METHOD_PN_GETCONFIGBLOB__
#define __ACTUAL_DOM_METHOD_PN_GETCONNECTEDDEVICES__
#define __ACTUAL_DOM_METHOD_PN_GETDEVICESETTINGS__
#define __ACTUAL_DOM_METHOD_PN_GETDEVICESETTINGS2__
#define __ACTUAL_DOM_METHOD_PN_GETFIRMWARESETTINGS__
#define __ACTUAL_DOM_METHOD_PN_GETMACFILTERS2__
#define __ACTUAL_DOM_METHOD_PN_GETNETWORKSTATS__
#define __ACTUAL_DOM_METHOD_PN_GETPORTMAPPINGS__
#define __ACTUAL_DOM_METHOD_PN_GETROUTERLANSETTINGS2__
#define __ACTUAL_DOM_METHOD_PN_GETROUTERSETTINGS__
#define __ACTUAL_DOM_METHOD_PN_GETWLANRADIOFREQUENCIES__
#define __ACTUAL_DOM_METHOD_PN_GETWLANRADIOSECURITY__
#define __ACTUAL_DOM_METHOD_PN_GETWLANRADIOSETTINGS__
#define __ACTUAL_DOM_METHOD_PN_GETWLANRADIOS__
#define __ACTUAL_DOM_METHOD_PN_GETWANINFO__
#define __ACTUAL_DOM_METHOD_PN_GETWANSETTINGS__
#define __ACTUAL_DOM_METHOD_PN_ISDEVICEREADY__
#define __ACTUAL_DOM_METHOD_PN_REBOOT__
#define __ACTUAL_DOM_METHOD_PN_RENEWWANCONNECTION__
#define __ACTUAL_DOM_METHOD_PN_RESTOREFACTORYDEFAULTS__
#define __ACTUAL_DOM_METHOD_PN_SETACCESSPOINTMODE__
#define __ACTUAL_DOM_METHOD_PN_SETCONFIGBLOB__
#define __ACTUAL_DOM_METHOD_PN_SETDEVICESETTINGS__
#define __ACTUAL_DOM_METHOD_PN_SETDEVICESETTINGS2__
#define __ACTUAL_DOM_METHOD_PN_SETMACFILTERS2__
#define __ACTUAL_DOM_METHOD_PN_SETROUTERLANSETTINGS2__
#define __ACTUAL_DOM_METHOD_PN_SETROUTERSETTINGS__
#define __ACTUAL_DOM_METHOD_PN_SETWLANRADIOFREQUENCY__
#define __ACTUAL_DOM_METHOD_PN_SETWLANRADIOSECURITY__
#define __ACTUAL_DOM_METHOD_PN_SETWLANRADIOSETTINGS__
#define __ACTUAL_DOM_METHOD_PN_SETWANSETTINGS__


/*
 * Methods
 */

extern void ACTUAL_DOM_Method_GetServiceInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_GetServices(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_AddPortMapping(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_DeletePortMapping(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_DownloadSpeedTest(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_FirmwareUpload(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetClientStats(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetConfigBlob(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetConnectedDevices(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetDeviceSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetDeviceSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetFirmwareSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetMACFilters2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetNetworkStats(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetPortMappings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetRouterLanSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetRouterSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWLanRadioFrequencies(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWLanRadioSecurity(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWLanRadioSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWLanRadios(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWanInfo(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_GetWanSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_IsDeviceReady(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_Reboot(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_RenewWanConnection(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_RestoreFactoryDefaults(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetAccessPointMode(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetConfigBlob(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetDeviceSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetDeviceSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetMACFilters2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetRouterLanSettings2(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetRouterSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetWLanRadioFrequency(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetWLanRadioSecurity(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetWLanRadioSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
extern void ACTUAL_DOM_Method_PN_SetWanSettings(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);


/*
 * DOM Schemas
 */

ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_ServiceInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_ClientStat();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_ConnectedClient();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_DHCPReservation();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_DNSSettings();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_MACInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_NetworkStats();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_PortMapping();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_RadioFrequencyInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_RadioInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_SecurityInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_TaskExtension();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanChannelInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanChannelWidthInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanEnabledInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanEncryptionInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanFrequencyInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanKeyInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanKeyRenewalInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanMacAddressInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanModeInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanQoSInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusIP1Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusIP2Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusPort1Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusPort2Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusSecret1Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusSecret2Info();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSSIDBroadcastInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSSIDInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSecondaryChannelInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSecurityEnabledInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanTypeInfo();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WideChannel();


/*
 * ADI
 */

typedef enum _ACTUAL_DOM_ADI
{
    ACTUAL_DOM_ADI_PN_AdminPassword = 1,
    ACTUAL_DOM_ADI_PN_AdminPasswordDefault = 2,
    ACTUAL_DOM_ADI_PN_AutoAdjustDST = 3,
    ACTUAL_DOM_ADI_PN_ClientStats = 4,
    ACTUAL_DOM_ADI_PN_ConnectedClients = 5,
    ACTUAL_DOM_ADI_PN_DHCPIPAddressFirst = 6,
    ACTUAL_DOM_ADI_PN_DHCPIPAddressLast = 7,
    ACTUAL_DOM_ADI_PN_DHCPLeaseTime = 8,
    ACTUAL_DOM_ADI_PN_DHCPReservations = 9,
    ACTUAL_DOM_ADI_PN_DHCPReservationsSupported = 10,
    ACTUAL_DOM_ADI_PN_DHCPServerEnabled = 11,
    ACTUAL_DOM_ADI_PN_DeviceName = 12,
    ACTUAL_DOM_ADI_PN_DeviceNetworkStats = 13,
    ACTUAL_DOM_ADI_PN_DeviceType = 14,
    ACTUAL_DOM_ADI_PN_DomainName = 15,
    ACTUAL_DOM_ADI_PN_DomainNameChangeSupported = 16,
    ACTUAL_DOM_ADI_PN_FirmwareDate = 17,
    ACTUAL_DOM_ADI_PN_FirmwareVersion = 18,
    ACTUAL_DOM_ADI_PN_IsAccessPoint = 19,
    ACTUAL_DOM_ADI_PN_LanIPAddress = 20,
    ACTUAL_DOM_ADI_PN_LanSubnetMask = 21,
    ACTUAL_DOM_ADI_PN_Locale = 22,
    ACTUAL_DOM_ADI_PN_MFEnabled = 23,
    ACTUAL_DOM_ADI_PN_MFIsAllowList = 24,
    ACTUAL_DOM_ADI_PN_MFMACList = 25,
    ACTUAL_DOM_ADI_PN_ManageOnlyViaSSL = 26,
    ACTUAL_DOM_ADI_PN_ManageRemote = 27,
    ACTUAL_DOM_ADI_PN_ManageViaSSLSupported = 28,
    ACTUAL_DOM_ADI_PN_ManageWireless = 29,
    ACTUAL_DOM_ADI_PN_ModelDescription = 30,
    ACTUAL_DOM_ADI_PN_ModelName = 31,
    ACTUAL_DOM_ADI_PN_ModelRevision = 32,
    ACTUAL_DOM_ADI_PN_PMDescription = 33,
    ACTUAL_DOM_ADI_PN_PMExternalPort = 34,
    ACTUAL_DOM_ADI_PN_PMInternalClient = 35,
    ACTUAL_DOM_ADI_PN_PMInternalPort = 36,
    ACTUAL_DOM_ADI_PN_PMProtocol = 37,
    ACTUAL_DOM_ADI_PN_PortMappings = 38,
    ACTUAL_DOM_ADI_PN_PresentationURL = 39,
    ACTUAL_DOM_ADI_PN_RemoteManagementSupported = 40,
    ACTUAL_DOM_ADI_PN_RemotePort = 41,
    ACTUAL_DOM_ADI_PN_RemoteSSL = 42,
    ACTUAL_DOM_ADI_PN_RemoteSSLNeedsSSL = 43,
    ACTUAL_DOM_ADI_PN_SSL = 44,
    ACTUAL_DOM_ADI_PN_SerialNumber = 45,
    ACTUAL_DOM_ADI_PN_SubDeviceURLs = 46,
    ACTUAL_DOM_ADI_PN_SupportedLocales = 47,
    ACTUAL_DOM_ADI_PN_TaskExtensions = 48,
    ACTUAL_DOM_ADI_PN_TimeZone = 49,
    ACTUAL_DOM_ADI_PN_TimeZoneSupported = 50,
    ACTUAL_DOM_ADI_PN_UpdateMethods = 51,
    ACTUAL_DOM_ADI_PN_Username = 52,
    ACTUAL_DOM_ADI_PN_UsernameSupported = 53,
    ACTUAL_DOM_ADI_PN_VendorName = 54,
    ACTUAL_DOM_ADI_PN_WLanChannel = 55,
    ACTUAL_DOM_ADI_PN_WLanChannelWidth = 56,
    ACTUAL_DOM_ADI_PN_WLanEnabled = 57,
    ACTUAL_DOM_ADI_PN_WLanEncryption = 58,
    ACTUAL_DOM_ADI_PN_WLanFrequency = 59,
    ACTUAL_DOM_ADI_PN_WLanKey = 60,
    ACTUAL_DOM_ADI_PN_WLanKeyRenewal = 61,
    ACTUAL_DOM_ADI_PN_WLanMacAddress = 62,
    ACTUAL_DOM_ADI_PN_WLanMode = 63,
    ACTUAL_DOM_ADI_PN_WLanQoS = 64,
    ACTUAL_DOM_ADI_PN_WLanRadioFrequencyInfos = 65,
    ACTUAL_DOM_ADI_PN_WLanRadioInfos = 66,
    ACTUAL_DOM_ADI_PN_WLanRadiusIP1 = 67,
    ACTUAL_DOM_ADI_PN_WLanRadiusIP2 = 68,
    ACTUAL_DOM_ADI_PN_WLanRadiusPort1 = 69,
    ACTUAL_DOM_ADI_PN_WLanRadiusPort2 = 70,
    ACTUAL_DOM_ADI_PN_WLanRadiusSecret1 = 71,
    ACTUAL_DOM_ADI_PN_WLanRadiusSecret2 = 72,
    ACTUAL_DOM_ADI_PN_WLanSSID = 73,
    ACTUAL_DOM_ADI_PN_WLanSSIDBroadcast = 74,
    ACTUAL_DOM_ADI_PN_WLanSecondaryChannel = 75,
    ACTUAL_DOM_ADI_PN_WLanSecurityEnabled = 76,
    ACTUAL_DOM_ADI_PN_WLanType = 77,
    ACTUAL_DOM_ADI_PN_WPSPin = 78,
    ACTUAL_DOM_ADI_PN_WanAuthService = 79,
    ACTUAL_DOM_ADI_PN_WanAutoDetectType = 80,
    ACTUAL_DOM_ADI_PN_WanAutoMTUSupported = 81,
    ACTUAL_DOM_ADI_PN_WanAutoReconnect = 82,
    ACTUAL_DOM_ADI_PN_WanDNS = 83,
    ACTUAL_DOM_ADI_PN_WanGateway = 84,
    ACTUAL_DOM_ADI_PN_WanIPAddress = 85,
    ACTUAL_DOM_ADI_PN_WanLoginService = 86,
    ACTUAL_DOM_ADI_PN_WanMTU = 87,
    ACTUAL_DOM_ADI_PN_WanMacAddress = 88,
    ACTUAL_DOM_ADI_PN_WanMaxIdleTime = 89,
    ACTUAL_DOM_ADI_PN_WanPPPoEService = 90,
    ACTUAL_DOM_ADI_PN_WanPassword = 91,
    ACTUAL_DOM_ADI_PN_WanRenewTimeout = 92,
    ACTUAL_DOM_ADI_PN_WanStatus = 93,
    ACTUAL_DOM_ADI_PN_WanSubnetMask = 94,
    ACTUAL_DOM_ADI_PN_WanSupportedTypes = 95,
    ACTUAL_DOM_ADI_PN_WanType = 96,
    ACTUAL_DOM_ADI_PN_WanUsername = 97,
    ACTUAL_DOM_ADI_PN_WiredQoS = 98,
    ACTUAL_DOM_ADI_PN_WiredQoSSupported = 99
} ACTUAL_DOM_ADI;


/*
 * ADI sentinels
 */

#define __ACTUAL_DOM_ADIGET_PN_ADMINPASSWORD__
#define __ACTUAL_DOM_ADISET_PN_ADMINPASSWORD__
#define __ACTUAL_DOM_ADIGET_PN_ADMINPASSWORDDEFAULT__
#define __ACTUAL_DOM_ADIGET_PN_AUTOADJUSTDST__
#define __ACTUAL_DOM_ADISET_PN_AUTOADJUSTDST__
#define __ACTUAL_DOM_ADIGET_PN_CLIENTSTATS__
#define __ACTUAL_DOM_ADIGET_PN_CONNECTEDCLIENTS__
#define __ACTUAL_DOM_ADIGET_PN_DHCPIPADDRESSFIRST__
#define __ACTUAL_DOM_ADISET_PN_DHCPIPADDRESSFIRST__
#define __ACTUAL_DOM_ADIGET_PN_DHCPIPADDRESSLAST__
#define __ACTUAL_DOM_ADISET_PN_DHCPIPADDRESSLAST__
#define __ACTUAL_DOM_ADIGET_PN_DHCPLEASETIME__
#define __ACTUAL_DOM_ADISET_PN_DHCPLEASETIME__
#define __ACTUAL_DOM_ADIGET_PN_DHCPRESERVATIONS__
#define __ACTUAL_DOM_ADISET_PN_DHCPRESERVATIONS__
#define __ACTUAL_DOM_ADIGET_PN_DHCPRESERVATIONSSUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_DHCPSERVERENABLED__
#define __ACTUAL_DOM_ADISET_PN_DHCPSERVERENABLED__
#define __ACTUAL_DOM_ADIGET_PN_DEVICENAME__
#define __ACTUAL_DOM_ADISET_PN_DEVICENAME__
#define __ACTUAL_DOM_ADIGET_PN_DEVICENETWORKSTATS__
#define __ACTUAL_DOM_ADIGET_PN_DEVICETYPE__
#define __ACTUAL_DOM_ADIGET_PN_DOMAINNAME__
#define __ACTUAL_DOM_ADISET_PN_DOMAINNAME__
#define __ACTUAL_DOM_ADIGET_PN_DOMAINNAMECHANGESUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_FIRMWAREDATE__
#define __ACTUAL_DOM_ADIGET_PN_FIRMWAREVERSION__
#define __ACTUAL_DOM_ADISET_PN_ISACCESSPOINT__
#define __ACTUAL_DOM_ADIGET_PN_LANIPADDRESS__
#define __ACTUAL_DOM_ADISET_PN_LANIPADDRESS__
#define __ACTUAL_DOM_ADIGET_PN_LANSUBNETMASK__
#define __ACTUAL_DOM_ADISET_PN_LANSUBNETMASK__
#define __ACTUAL_DOM_ADIGET_PN_LOCALE__
#define __ACTUAL_DOM_ADISET_PN_LOCALE__
#define __ACTUAL_DOM_ADIGET_PN_MFENABLED__
#define __ACTUAL_DOM_ADISET_PN_MFENABLED__
#define __ACTUAL_DOM_ADIGET_PN_MFISALLOWLIST__
#define __ACTUAL_DOM_ADISET_PN_MFISALLOWLIST__
#define __ACTUAL_DOM_ADIGET_PN_MFMACLIST__
#define __ACTUAL_DOM_ADISET_PN_MFMACLIST__
#define __ACTUAL_DOM_ADIGET_PN_MANAGEONLYVIASSL__
#define __ACTUAL_DOM_ADIGET_PN_MANAGEREMOTE__
#define __ACTUAL_DOM_ADISET_PN_MANAGEREMOTE__
#define __ACTUAL_DOM_ADIGET_PN_MANAGEVIASSLSUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_MANAGEWIRELESS__
#define __ACTUAL_DOM_ADISET_PN_MANAGEWIRELESS__
#define __ACTUAL_DOM_ADIGET_PN_MODELDESCRIPTION__
#define __ACTUAL_DOM_ADIGET_PN_MODELNAME__
#define __ACTUAL_DOM_ADIGET_PN_MODELREVISION__
#define __ACTUAL_DOM_ADIGET_PN_PMDESCRIPTION__
#define __ACTUAL_DOM_ADIGET_PN_PMEXTERNALPORT__
#define __ACTUAL_DOM_ADISET_PN_PMEXTERNALPORT__
#define __ACTUAL_DOM_ADIGET_PN_PMINTERNALCLIENT__
#define __ACTUAL_DOM_ADIGET_PN_PMINTERNALPORT__
#define __ACTUAL_DOM_ADIGET_PN_PMPROTOCOL__
#define __ACTUAL_DOM_ADISET_PN_PMPROTOCOL__
#define __ACTUAL_DOM_ADIGET_PN_PORTMAPPINGS__
#define __ACTUAL_DOM_ADIGET_PN_PRESENTATIONURL__
#define __ACTUAL_DOM_ADIGET_PN_REMOTEMANAGEMENTSUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_REMOTEPORT__
#define __ACTUAL_DOM_ADISET_PN_REMOTEPORT__
#define __ACTUAL_DOM_ADIGET_PN_REMOTESSL__
#define __ACTUAL_DOM_ADISET_PN_REMOTESSL__
#define __ACTUAL_DOM_ADIGET_PN_REMOTESSLNEEDSSSL__
#define __ACTUAL_DOM_ADIGET_PN_SSL__
#define __ACTUAL_DOM_ADISET_PN_SSL__
#define __ACTUAL_DOM_ADIGET_PN_SERIALNUMBER__
#define __ACTUAL_DOM_ADIGET_PN_SUBDEVICEURLS__
#define __ACTUAL_DOM_ADIGET_PN_SUPPORTEDLOCALES__
#define __ACTUAL_DOM_ADIGET_PN_TASKEXTENSIONS__
#define __ACTUAL_DOM_ADIGET_PN_TIMEZONE__
#define __ACTUAL_DOM_ADISET_PN_TIMEZONE__
#define __ACTUAL_DOM_ADIGET_PN_TIMEZONESUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_UPDATEMETHODS__
#define __ACTUAL_DOM_ADISET_PN_USERNAME__
#define __ACTUAL_DOM_ADIGET_PN_USERNAMESUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_VENDORNAME__
#define __ACTUAL_DOM_ADIGET_PN_WLANCHANNEL__
#define __ACTUAL_DOM_ADISET_PN_WLANCHANNEL__
#define __ACTUAL_DOM_ADIGET_PN_WLANCHANNELWIDTH__
#define __ACTUAL_DOM_ADISET_PN_WLANCHANNELWIDTH__
#define __ACTUAL_DOM_ADIGET_PN_WLANENABLED__
#define __ACTUAL_DOM_ADISET_PN_WLANENABLED__
#define __ACTUAL_DOM_ADIGET_PN_WLANENCRYPTION__
#define __ACTUAL_DOM_ADISET_PN_WLANENCRYPTION__
#define __ACTUAL_DOM_ADISET_PN_WLANFREQUENCY__
#define __ACTUAL_DOM_ADIGET_PN_WLANKEY__
#define __ACTUAL_DOM_ADISET_PN_WLANKEY__
#define __ACTUAL_DOM_ADIGET_PN_WLANKEYRENEWAL__
#define __ACTUAL_DOM_ADISET_PN_WLANKEYRENEWAL__
#define __ACTUAL_DOM_ADIGET_PN_WLANMACADDRESS__
#define __ACTUAL_DOM_ADISET_PN_WLANMACADDRESS__
#define __ACTUAL_DOM_ADIGET_PN_WLANMODE__
#define __ACTUAL_DOM_ADISET_PN_WLANMODE__
#define __ACTUAL_DOM_ADIGET_PN_WLANQOS__
#define __ACTUAL_DOM_ADISET_PN_WLANQOS__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIOFREQUENCYINFOS__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIOINFOS__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSIP1__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSIP1__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSIP2__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSIP2__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSPORT1__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSPORT1__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSPORT2__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSPORT2__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSSECRET1__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSSECRET1__
#define __ACTUAL_DOM_ADIGET_PN_WLANRADIUSSECRET2__
#define __ACTUAL_DOM_ADISET_PN_WLANRADIUSSECRET2__
#define __ACTUAL_DOM_ADIGET_PN_WLANSSID__
#define __ACTUAL_DOM_ADISET_PN_WLANSSID__
#define __ACTUAL_DOM_ADIGET_PN_WLANSSIDBROADCAST__
#define __ACTUAL_DOM_ADISET_PN_WLANSSIDBROADCAST__
#define __ACTUAL_DOM_ADIGET_PN_WLANSECONDARYCHANNEL__
#define __ACTUAL_DOM_ADISET_PN_WLANSECONDARYCHANNEL__
#define __ACTUAL_DOM_ADIGET_PN_WLANSECURITYENABLED__
#define __ACTUAL_DOM_ADISET_PN_WLANSECURITYENABLED__
#define __ACTUAL_DOM_ADIGET_PN_WLANTYPE__
#define __ACTUAL_DOM_ADISET_PN_WLANTYPE__
#define __ACTUAL_DOM_ADIGET_PN_WPSPIN__
#define __ACTUAL_DOM_ADIGET_PN_WANAUTHSERVICE__
#define __ACTUAL_DOM_ADISET_PN_WANAUTHSERVICE__
#define __ACTUAL_DOM_ADIGET_PN_WANAUTODETECTTYPE__
#define __ACTUAL_DOM_ADIGET_PN_WANAUTOMTUSUPPORTED__
#define __ACTUAL_DOM_ADIGET_PN_WANAUTORECONNECT__
#define __ACTUAL_DOM_ADISET_PN_WANAUTORECONNECT__
#define __ACTUAL_DOM_ADIGET_PN_WANDNS__
#define __ACTUAL_DOM_ADISET_PN_WANDNS__
#define __ACTUAL_DOM_ADIGET_PN_WANGATEWAY__
#define __ACTUAL_DOM_ADISET_PN_WANGATEWAY__
#define __ACTUAL_DOM_ADIGET_PN_WANIPADDRESS__
#define __ACTUAL_DOM_ADISET_PN_WANIPADDRESS__
#define __ACTUAL_DOM_ADIGET_PN_WANLOGINSERVICE__
#define __ACTUAL_DOM_ADISET_PN_WANLOGINSERVICE__
#define __ACTUAL_DOM_ADIGET_PN_WANMTU__
#define __ACTUAL_DOM_ADISET_PN_WANMTU__
#define __ACTUAL_DOM_ADIGET_PN_WANMACADDRESS__
#define __ACTUAL_DOM_ADISET_PN_WANMACADDRESS__
#define __ACTUAL_DOM_ADIGET_PN_WANMAXIDLETIME__
#define __ACTUAL_DOM_ADISET_PN_WANMAXIDLETIME__
#define __ACTUAL_DOM_ADIGET_PN_WANPPPOESERVICE__
#define __ACTUAL_DOM_ADISET_PN_WANPPPOESERVICE__
#define __ACTUAL_DOM_ADIGET_PN_WANPASSWORD__
#define __ACTUAL_DOM_ADISET_PN_WANPASSWORD__
#define __ACTUAL_DOM_ADISET_PN_WANRENEWTIMEOUT__
#define __ACTUAL_DOM_ADIGET_PN_WANSTATUS__
#define __ACTUAL_DOM_ADIGET_PN_WANSUBNETMASK__
#define __ACTUAL_DOM_ADISET_PN_WANSUBNETMASK__
#define __ACTUAL_DOM_ADIGET_PN_WANSUPPORTEDTYPES__
#define __ACTUAL_DOM_ADIGET_PN_WANTYPE__
#define __ACTUAL_DOM_ADISET_PN_WANTYPE__
#define __ACTUAL_DOM_ADIGET_PN_WANUSERNAME__
#define __ACTUAL_DOM_ADISET_PN_WANUSERNAME__
#define __ACTUAL_DOM_ADIGET_PN_WIREDQOS__
#define __ACTUAL_DOM_ADISET_PN_WIREDQOS__
#define __ACTUAL_DOM_ADIGET_PN_WIREDQOSSUPPORTED__


/*
 * Module
 */

ACTUAL_DOM_EXPORT const HDK_MOD_Module* ACTUAL_DOM_Module(void);

/* Dynamic server module export */
ACTUAL_DOM_EXPORT const HDK_MOD_Module* HDK_SRV_Module(void);

#endif /* __ACTUAL_DOM_H__ */
