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

#include "actual_dom.h"

#include <string.h>


/*
 * Namespaces
 */

static const HDK_XML_Namespace s_namespaces[] =
{
    /* 0 */ "http://cisco.com/HNAP/",
    /* 1 */ "http://purenetworks.com/HNAP1/",
    /* 2 */ "http://schemas.xmlsoap.org/soap/envelope/",
    HDK_XML_Schema_NamespacesEnd
};


/*
 * Elements
 */

static const HDK_XML_ElementNode s_elements[] =
{
    /* ACTUAL_DOM_Element_ADI = 0 */ { 0, "ADI" },
    /* ACTUAL_DOM_Element_GetServiceInfo = 1 */ { 0, "GetServiceInfo" },
    /* ACTUAL_DOM_Element_GetServiceInfoResponse = 2 */ { 0, "GetServiceInfoResponse" },
    /* ACTUAL_DOM_Element_GetServiceInfoResult = 3 */ { 0, "GetServiceInfoResult" },
    /* ACTUAL_DOM_Element_GetServices = 4 */ { 0, "GetServices" },
    /* ACTUAL_DOM_Element_GetServicesResponse = 5 */ { 0, "GetServicesResponse" },
    /* ACTUAL_DOM_Element_GetServicesResult = 6 */ { 0, "GetServicesResult" },
    /* ACTUAL_DOM_Element_Info = 7 */ { 0, "Info" },
    /* ACTUAL_DOM_Element_ServiceInfo = 8 */ { 0, "ServiceInfo" },
    /* ACTUAL_DOM_Element_ServiceLevel = 9 */ { 0, "ServiceLevel" },
    /* ACTUAL_DOM_Element_ServiceName = 10 */ { 0, "ServiceName" },
    /* ACTUAL_DOM_Element_Services = 11 */ { 0, "Services" },
    /* ACTUAL_DOM_Element_PN_Active = 12 */ { 1, "Active" },
    /* ACTUAL_DOM_Element_PN_AddPortMapping = 13 */ { 1, "AddPortMapping" },
    /* ACTUAL_DOM_Element_PN_AddPortMappingResponse = 14 */ { 1, "AddPortMappingResponse" },
    /* ACTUAL_DOM_Element_PN_AddPortMappingResult = 15 */ { 1, "AddPortMappingResult" },
    /* ACTUAL_DOM_Element_PN_AdminPassword = 16 */ { 1, "AdminPassword" },
    /* ACTUAL_DOM_Element_PN_AdminPasswordDefault = 17 */ { 1, "AdminPasswordDefault" },
    /* ACTUAL_DOM_Element_PN_AutoAdjustDST = 18 */ { 1, "AutoAdjustDST" },
    /* ACTUAL_DOM_Element_PN_AutoDetectType = 19 */ { 1, "AutoDetectType" },
    /* ACTUAL_DOM_Element_PN_AutoReconnect = 20 */ { 1, "AutoReconnect" },
    /* ACTUAL_DOM_Element_PN_Base64Image = 21 */ { 1, "Base64Image" },
    /* ACTUAL_DOM_Element_PN_BufferSize = 22 */ { 1, "BufferSize" },
    /* ACTUAL_DOM_Element_PN_ByteStream = 23 */ { 1, "ByteStream" },
    /* ACTUAL_DOM_Element_PN_Bytes = 24 */ { 1, "Bytes" },
    /* ACTUAL_DOM_Element_PN_BytesReceived = 25 */ { 1, "BytesReceived" },
    /* ACTUAL_DOM_Element_PN_BytesSent = 26 */ { 1, "BytesSent" },
    /* ACTUAL_DOM_Element_PN_Channel = 27 */ { 1, "Channel" },
    /* ACTUAL_DOM_Element_PN_ChannelWidth = 28 */ { 1, "ChannelWidth" },
    /* ACTUAL_DOM_Element_PN_Channels = 29 */ { 1, "Channels" },
    /* ACTUAL_DOM_Element_PN_ClientStat = 30 */ { 1, "ClientStat" },
    /* ACTUAL_DOM_Element_PN_ClientStats = 31 */ { 1, "ClientStats" },
    /* ACTUAL_DOM_Element_PN_ConfigBlob = 32 */ { 1, "ConfigBlob" },
    /* ACTUAL_DOM_Element_PN_ConnectTime = 33 */ { 1, "ConnectTime" },
    /* ACTUAL_DOM_Element_PN_ConnectedClient = 34 */ { 1, "ConnectedClient" },
    /* ACTUAL_DOM_Element_PN_ConnectedClients = 35 */ { 1, "ConnectedClients" },
    /* ACTUAL_DOM_Element_PN_DHCPIPAddressFirst = 36 */ { 1, "DHCPIPAddressFirst" },
    /* ACTUAL_DOM_Element_PN_DHCPIPAddressLast = 37 */ { 1, "DHCPIPAddressLast" },
    /* ACTUAL_DOM_Element_PN_DHCPLeaseTime = 38 */ { 1, "DHCPLeaseTime" },
    /* ACTUAL_DOM_Element_PN_DHCPReservation = 39 */ { 1, "DHCPReservation" },
    /* ACTUAL_DOM_Element_PN_DHCPReservations = 40 */ { 1, "DHCPReservations" },
    /* ACTUAL_DOM_Element_PN_DHCPReservationsSupported = 41 */ { 1, "DHCPReservationsSupported" },
    /* ACTUAL_DOM_Element_PN_DHCPServerEnabled = 42 */ { 1, "DHCPServerEnabled" },
    /* ACTUAL_DOM_Element_PN_DNS = 43 */ { 1, "DNS" },
    /* ACTUAL_DOM_Element_PN_DNSSettings = 44 */ { 1, "DNSSettings" },
    /* ACTUAL_DOM_Element_PN_DeletePortMapping = 45 */ { 1, "DeletePortMapping" },
    /* ACTUAL_DOM_Element_PN_DeletePortMappingResponse = 46 */ { 1, "DeletePortMappingResponse" },
    /* ACTUAL_DOM_Element_PN_DeletePortMappingResult = 47 */ { 1, "DeletePortMappingResult" },
    /* ACTUAL_DOM_Element_PN_DeviceName = 48 */ { 1, "DeviceName" },
    /* ACTUAL_DOM_Element_PN_DeviceNetworkStats = 49 */ { 1, "DeviceNetworkStats" },
    /* ACTUAL_DOM_Element_PN_DeviceType = 50 */ { 1, "DeviceType" },
    /* ACTUAL_DOM_Element_PN_DomainName = 51 */ { 1, "DomainName" },
    /* ACTUAL_DOM_Element_PN_DomainNameChangeSupported = 52 */ { 1, "DomainNameChangeSupported" },
    /* ACTUAL_DOM_Element_PN_DownloadSpeedTest = 53 */ { 1, "DownloadSpeedTest" },
    /* ACTUAL_DOM_Element_PN_DownloadSpeedTestResponse = 54 */ { 1, "DownloadSpeedTestResponse" },
    /* ACTUAL_DOM_Element_PN_DownloadSpeedTestResult = 55 */ { 1, "DownloadSpeedTestResult" },
    /* ACTUAL_DOM_Element_PN_Enabled = 56 */ { 1, "Enabled" },
    /* ACTUAL_DOM_Element_PN_Encryption = 57 */ { 1, "Encryption" },
    /* ACTUAL_DOM_Element_PN_Encryptions = 58 */ { 1, "Encryptions" },
    /* ACTUAL_DOM_Element_PN_ExternalPort = 59 */ { 1, "ExternalPort" },
    /* ACTUAL_DOM_Element_PN_FirmwareDate = 60 */ { 1, "FirmwareDate" },
    /* ACTUAL_DOM_Element_PN_FirmwareUpload = 61 */ { 1, "FirmwareUpload" },
    /* ACTUAL_DOM_Element_PN_FirmwareUploadResponse = 62 */ { 1, "FirmwareUploadResponse" },
    /* ACTUAL_DOM_Element_PN_FirmwareUploadResult = 63 */ { 1, "FirmwareUploadResult" },
    /* ACTUAL_DOM_Element_PN_FirmwareVersion = 64 */ { 1, "FirmwareVersion" },
    /* ACTUAL_DOM_Element_PN_Frequencies = 65 */ { 1, "Frequencies" },
    /* ACTUAL_DOM_Element_PN_Frequency = 66 */ { 1, "Frequency" },
    /* ACTUAL_DOM_Element_PN_Gateway = 67 */ { 1, "Gateway" },
    /* ACTUAL_DOM_Element_PN_GetClientStats = 68 */ { 1, "GetClientStats" },
    /* ACTUAL_DOM_Element_PN_GetClientStatsResponse = 69 */ { 1, "GetClientStatsResponse" },
    /* ACTUAL_DOM_Element_PN_GetClientStatsResult = 70 */ { 1, "GetClientStatsResult" },
    /* ACTUAL_DOM_Element_PN_GetConfigBlob = 71 */ { 1, "GetConfigBlob" },
    /* ACTUAL_DOM_Element_PN_GetConfigBlobResponse = 72 */ { 1, "GetConfigBlobResponse" },
    /* ACTUAL_DOM_Element_PN_GetConfigBlobResult = 73 */ { 1, "GetConfigBlobResult" },
    /* ACTUAL_DOM_Element_PN_GetConnectedDevices = 74 */ { 1, "GetConnectedDevices" },
    /* ACTUAL_DOM_Element_PN_GetConnectedDevicesResponse = 75 */ { 1, "GetConnectedDevicesResponse" },
    /* ACTUAL_DOM_Element_PN_GetConnectedDevicesResult = 76 */ { 1, "GetConnectedDevicesResult" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettings = 77 */ { 1, "GetDeviceSettings" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettings2 = 78 */ { 1, "GetDeviceSettings2" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettings2Response = 79 */ { 1, "GetDeviceSettings2Response" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettings2Result = 80 */ { 1, "GetDeviceSettings2Result" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettingsResponse = 81 */ { 1, "GetDeviceSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetDeviceSettingsResult = 82 */ { 1, "GetDeviceSettingsResult" },
    /* ACTUAL_DOM_Element_PN_GetFirmwareSettings = 83 */ { 1, "GetFirmwareSettings" },
    /* ACTUAL_DOM_Element_PN_GetFirmwareSettingsResponse = 84 */ { 1, "GetFirmwareSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetFirmwareSettingsResult = 85 */ { 1, "GetFirmwareSettingsResult" },
    /* ACTUAL_DOM_Element_PN_GetMACFilters2 = 86 */ { 1, "GetMACFilters2" },
    /* ACTUAL_DOM_Element_PN_GetMACFilters2Response = 87 */ { 1, "GetMACFilters2Response" },
    /* ACTUAL_DOM_Element_PN_GetMACFilters2Result = 88 */ { 1, "GetMACFilters2Result" },
    /* ACTUAL_DOM_Element_PN_GetNetworkStats = 89 */ { 1, "GetNetworkStats" },
    /* ACTUAL_DOM_Element_PN_GetNetworkStatsResponse = 90 */ { 1, "GetNetworkStatsResponse" },
    /* ACTUAL_DOM_Element_PN_GetNetworkStatsResult = 91 */ { 1, "GetNetworkStatsResult" },
    /* ACTUAL_DOM_Element_PN_GetPortMappings = 92 */ { 1, "GetPortMappings" },
    /* ACTUAL_DOM_Element_PN_GetPortMappingsResponse = 93 */ { 1, "GetPortMappingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetPortMappingsResult = 94 */ { 1, "GetPortMappingsResult" },
    /* ACTUAL_DOM_Element_PN_GetRouterLanSettings2 = 95 */ { 1, "GetRouterLanSettings2" },
    /* ACTUAL_DOM_Element_PN_GetRouterLanSettings2Response = 96 */ { 1, "GetRouterLanSettings2Response" },
    /* ACTUAL_DOM_Element_PN_GetRouterLanSettings2Result = 97 */ { 1, "GetRouterLanSettings2Result" },
    /* ACTUAL_DOM_Element_PN_GetRouterSettings = 98 */ { 1, "GetRouterSettings" },
    /* ACTUAL_DOM_Element_PN_GetRouterSettingsResponse = 99 */ { 1, "GetRouterSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetRouterSettingsResult = 100 */ { 1, "GetRouterSettingsResult" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioFrequencies = 101 */ { 1, "GetWLanRadioFrequencies" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResponse = 102 */ { 1, "GetWLanRadioFrequenciesResponse" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResult = 103 */ { 1, "GetWLanRadioFrequenciesResult" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSecurity = 104 */ { 1, "GetWLanRadioSecurity" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResponse = 105 */ { 1, "GetWLanRadioSecurityResponse" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResult = 106 */ { 1, "GetWLanRadioSecurityResult" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSettings = 107 */ { 1, "GetWLanRadioSettings" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResponse = 108 */ { 1, "GetWLanRadioSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResult = 109 */ { 1, "GetWLanRadioSettingsResult" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadios = 110 */ { 1, "GetWLanRadios" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadiosResponse = 111 */ { 1, "GetWLanRadiosResponse" },
    /* ACTUAL_DOM_Element_PN_GetWLanRadiosResult = 112 */ { 1, "GetWLanRadiosResult" },
    /* ACTUAL_DOM_Element_PN_GetWanInfo = 113 */ { 1, "GetWanInfo" },
    /* ACTUAL_DOM_Element_PN_GetWanInfoResponse = 114 */ { 1, "GetWanInfoResponse" },
    /* ACTUAL_DOM_Element_PN_GetWanInfoResult = 115 */ { 1, "GetWanInfoResult" },
    /* ACTUAL_DOM_Element_PN_GetWanSettings = 116 */ { 1, "GetWanSettings" },
    /* ACTUAL_DOM_Element_PN_GetWanSettingsResponse = 117 */ { 1, "GetWanSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_GetWanSettingsResult = 118 */ { 1, "GetWanSettingsResult" },
    /* ACTUAL_DOM_Element_PN_IPAddress = 119 */ { 1, "IPAddress" },
    /* ACTUAL_DOM_Element_PN_IPAddressFirst = 120 */ { 1, "IPAddressFirst" },
    /* ACTUAL_DOM_Element_PN_IPAddressLast = 121 */ { 1, "IPAddressLast" },
    /* ACTUAL_DOM_Element_PN_InternalClient = 122 */ { 1, "InternalClient" },
    /* ACTUAL_DOM_Element_PN_InternalPort = 123 */ { 1, "InternalPort" },
    /* ACTUAL_DOM_Element_PN_IsAccessPoint = 124 */ { 1, "IsAccessPoint" },
    /* ACTUAL_DOM_Element_PN_IsAllowList = 125 */ { 1, "IsAllowList" },
    /* ACTUAL_DOM_Element_PN_IsDeviceReady = 126 */ { 1, "IsDeviceReady" },
    /* ACTUAL_DOM_Element_PN_IsDeviceReadyResponse = 127 */ { 1, "IsDeviceReadyResponse" },
    /* ACTUAL_DOM_Element_PN_IsDeviceReadyResult = 128 */ { 1, "IsDeviceReadyResult" },
    /* ACTUAL_DOM_Element_PN_Key = 129 */ { 1, "Key" },
    /* ACTUAL_DOM_Element_PN_KeyRenewal = 130 */ { 1, "KeyRenewal" },
    /* ACTUAL_DOM_Element_PN_LanIPAddress = 131 */ { 1, "LanIPAddress" },
    /* ACTUAL_DOM_Element_PN_LanSubnetMask = 132 */ { 1, "LanSubnetMask" },
    /* ACTUAL_DOM_Element_PN_LeaseTime = 133 */ { 1, "LeaseTime" },
    /* ACTUAL_DOM_Element_PN_LinkSpeedIn = 134 */ { 1, "LinkSpeedIn" },
    /* ACTUAL_DOM_Element_PN_LinkSpeedOut = 135 */ { 1, "LinkSpeedOut" },
    /* ACTUAL_DOM_Element_PN_Locale = 136 */ { 1, "Locale" },
    /* ACTUAL_DOM_Element_PN_MACInfo = 137 */ { 1, "MACInfo" },
    /* ACTUAL_DOM_Element_PN_MACList = 138 */ { 1, "MACList" },
    /* ACTUAL_DOM_Element_PN_MFEnabled = 139 */ { 1, "MFEnabled" },
    /* ACTUAL_DOM_Element_PN_MFIsAllowList = 140 */ { 1, "MFIsAllowList" },
    /* ACTUAL_DOM_Element_PN_MFMACList = 141 */ { 1, "MFMACList" },
    /* ACTUAL_DOM_Element_PN_MTU = 142 */ { 1, "MTU" },
    /* ACTUAL_DOM_Element_PN_MacAddress = 143 */ { 1, "MacAddress" },
    /* ACTUAL_DOM_Element_PN_ManageOnlyViaSSL = 144 */ { 1, "ManageOnlyViaSSL" },
    /* ACTUAL_DOM_Element_PN_ManageRemote = 145 */ { 1, "ManageRemote" },
    /* ACTUAL_DOM_Element_PN_ManageViaSSLSupported = 146 */ { 1, "ManageViaSSLSupported" },
    /* ACTUAL_DOM_Element_PN_ManageWireless = 147 */ { 1, "ManageWireless" },
    /* ACTUAL_DOM_Element_PN_MaxIdleTime = 148 */ { 1, "MaxIdleTime" },
    /* ACTUAL_DOM_Element_PN_Mode = 149 */ { 1, "Mode" },
    /* ACTUAL_DOM_Element_PN_ModelDescription = 150 */ { 1, "ModelDescription" },
    /* ACTUAL_DOM_Element_PN_ModelName = 151 */ { 1, "ModelName" },
    /* ACTUAL_DOM_Element_PN_ModelRevision = 152 */ { 1, "ModelRevision" },
    /* ACTUAL_DOM_Element_PN_Name = 153 */ { 1, "Name" },
    /* ACTUAL_DOM_Element_PN_NetworkStats = 154 */ { 1, "NetworkStats" },
    /* ACTUAL_DOM_Element_PN_NewIPAddress = 155 */ { 1, "NewIPAddress" },
    /* ACTUAL_DOM_Element_PN_PMDescription = 156 */ { 1, "PMDescription" },
    /* ACTUAL_DOM_Element_PN_PMExternalPort = 157 */ { 1, "PMExternalPort" },
    /* ACTUAL_DOM_Element_PN_PMInternalClient = 158 */ { 1, "PMInternalClient" },
    /* ACTUAL_DOM_Element_PN_PMInternalPort = 159 */ { 1, "PMInternalPort" },
    /* ACTUAL_DOM_Element_PN_PMProtocol = 160 */ { 1, "PMProtocol" },
    /* ACTUAL_DOM_Element_PN_PacketsReceived = 161 */ { 1, "PacketsReceived" },
    /* ACTUAL_DOM_Element_PN_PacketsSent = 162 */ { 1, "PacketsSent" },
    /* ACTUAL_DOM_Element_PN_Password = 163 */ { 1, "Password" },
    /* ACTUAL_DOM_Element_PN_PortMapping = 164 */ { 1, "PortMapping" },
    /* ACTUAL_DOM_Element_PN_PortMappingDescription = 165 */ { 1, "PortMappingDescription" },
    /* ACTUAL_DOM_Element_PN_PortMappingProtocol = 166 */ { 1, "PortMappingProtocol" },
    /* ACTUAL_DOM_Element_PN_PortMappings = 167 */ { 1, "PortMappings" },
    /* ACTUAL_DOM_Element_PN_PortName = 168 */ { 1, "PortName" },
    /* ACTUAL_DOM_Element_PN_PresentationURL = 169 */ { 1, "PresentationURL" },
    /* ACTUAL_DOM_Element_PN_Primary = 170 */ { 1, "Primary" },
    /* ACTUAL_DOM_Element_PN_QoS = 171 */ { 1, "QoS" },
    /* ACTUAL_DOM_Element_PN_RadioFrequencyInfo = 172 */ { 1, "RadioFrequencyInfo" },
    /* ACTUAL_DOM_Element_PN_RadioFrequencyInfos = 173 */ { 1, "RadioFrequencyInfos" },
    /* ACTUAL_DOM_Element_PN_RadioID = 174 */ { 1, "RadioID" },
    /* ACTUAL_DOM_Element_PN_RadioInfo = 175 */ { 1, "RadioInfo" },
    /* ACTUAL_DOM_Element_PN_RadioInfos = 176 */ { 1, "RadioInfos" },
    /* ACTUAL_DOM_Element_PN_RadiusIP1 = 177 */ { 1, "RadiusIP1" },
    /* ACTUAL_DOM_Element_PN_RadiusIP2 = 178 */ { 1, "RadiusIP2" },
    /* ACTUAL_DOM_Element_PN_RadiusPort1 = 179 */ { 1, "RadiusPort1" },
    /* ACTUAL_DOM_Element_PN_RadiusPort2 = 180 */ { 1, "RadiusPort2" },
    /* ACTUAL_DOM_Element_PN_RadiusSecret1 = 181 */ { 1, "RadiusSecret1" },
    /* ACTUAL_DOM_Element_PN_RadiusSecret2 = 182 */ { 1, "RadiusSecret2" },
    /* ACTUAL_DOM_Element_PN_Reboot = 183 */ { 1, "Reboot" },
    /* ACTUAL_DOM_Element_PN_RebootResponse = 184 */ { 1, "RebootResponse" },
    /* ACTUAL_DOM_Element_PN_RebootResult = 185 */ { 1, "RebootResult" },
    /* ACTUAL_DOM_Element_PN_RemoteManagementSupported = 186 */ { 1, "RemoteManagementSupported" },
    /* ACTUAL_DOM_Element_PN_RemotePort = 187 */ { 1, "RemotePort" },
    /* ACTUAL_DOM_Element_PN_RemoteSSL = 188 */ { 1, "RemoteSSL" },
    /* ACTUAL_DOM_Element_PN_RemoteSSLNeedsSSL = 189 */ { 1, "RemoteSSLNeedsSSL" },
    /* ACTUAL_DOM_Element_PN_RenewTimeout = 190 */ { 1, "RenewTimeout" },
    /* ACTUAL_DOM_Element_PN_RenewWanConnection = 191 */ { 1, "RenewWanConnection" },
    /* ACTUAL_DOM_Element_PN_RenewWanConnectionResponse = 192 */ { 1, "RenewWanConnectionResponse" },
    /* ACTUAL_DOM_Element_PN_RenewWanConnectionResult = 193 */ { 1, "RenewWanConnectionResult" },
    /* ACTUAL_DOM_Element_PN_RestoreFactoryDefaults = 194 */ { 1, "RestoreFactoryDefaults" },
    /* ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResponse = 195 */ { 1, "RestoreFactoryDefaultsResponse" },
    /* ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResult = 196 */ { 1, "RestoreFactoryDefaultsResult" },
    /* ACTUAL_DOM_Element_PN_RouterIPAddress = 197 */ { 1, "RouterIPAddress" },
    /* ACTUAL_DOM_Element_PN_RouterSubnetMask = 198 */ { 1, "RouterSubnetMask" },
    /* ACTUAL_DOM_Element_PN_SOAPActions = 199 */ { 1, "SOAPActions" },
    /* ACTUAL_DOM_Element_PN_SSID = 200 */ { 1, "SSID" },
    /* ACTUAL_DOM_Element_PN_SSIDBroadcast = 201 */ { 1, "SSIDBroadcast" },
    /* ACTUAL_DOM_Element_PN_SSL = 202 */ { 1, "SSL" },
    /* ACTUAL_DOM_Element_PN_Secondary = 203 */ { 1, "Secondary" },
    /* ACTUAL_DOM_Element_PN_SecondaryChannel = 204 */ { 1, "SecondaryChannel" },
    /* ACTUAL_DOM_Element_PN_SecondaryChannels = 205 */ { 1, "SecondaryChannels" },
    /* ACTUAL_DOM_Element_PN_SecurityInfo = 206 */ { 1, "SecurityInfo" },
    /* ACTUAL_DOM_Element_PN_SecurityType = 207 */ { 1, "SecurityType" },
    /* ACTUAL_DOM_Element_PN_SerialNumber = 208 */ { 1, "SerialNumber" },
    /* ACTUAL_DOM_Element_PN_ServiceName = 209 */ { 1, "ServiceName" },
    /* ACTUAL_DOM_Element_PN_SetAccessPointMode = 210 */ { 1, "SetAccessPointMode" },
    /* ACTUAL_DOM_Element_PN_SetAccessPointModeResponse = 211 */ { 1, "SetAccessPointModeResponse" },
    /* ACTUAL_DOM_Element_PN_SetAccessPointModeResult = 212 */ { 1, "SetAccessPointModeResult" },
    /* ACTUAL_DOM_Element_PN_SetConfigBlob = 213 */ { 1, "SetConfigBlob" },
    /* ACTUAL_DOM_Element_PN_SetConfigBlobResponse = 214 */ { 1, "SetConfigBlobResponse" },
    /* ACTUAL_DOM_Element_PN_SetConfigBlobResult = 215 */ { 1, "SetConfigBlobResult" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettings = 216 */ { 1, "SetDeviceSettings" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettings2 = 217 */ { 1, "SetDeviceSettings2" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettings2Response = 218 */ { 1, "SetDeviceSettings2Response" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettings2Result = 219 */ { 1, "SetDeviceSettings2Result" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettingsResponse = 220 */ { 1, "SetDeviceSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_SetDeviceSettingsResult = 221 */ { 1, "SetDeviceSettingsResult" },
    /* ACTUAL_DOM_Element_PN_SetMACFilters2 = 222 */ { 1, "SetMACFilters2" },
    /* ACTUAL_DOM_Element_PN_SetMACFilters2Response = 223 */ { 1, "SetMACFilters2Response" },
    /* ACTUAL_DOM_Element_PN_SetMACFilters2Result = 224 */ { 1, "SetMACFilters2Result" },
    /* ACTUAL_DOM_Element_PN_SetRouterLanSettings2 = 225 */ { 1, "SetRouterLanSettings2" },
    /* ACTUAL_DOM_Element_PN_SetRouterLanSettings2Response = 226 */ { 1, "SetRouterLanSettings2Response" },
    /* ACTUAL_DOM_Element_PN_SetRouterLanSettings2Result = 227 */ { 1, "SetRouterLanSettings2Result" },
    /* ACTUAL_DOM_Element_PN_SetRouterSettings = 228 */ { 1, "SetRouterSettings" },
    /* ACTUAL_DOM_Element_PN_SetRouterSettingsResponse = 229 */ { 1, "SetRouterSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_SetRouterSettingsResult = 230 */ { 1, "SetRouterSettingsResult" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioFrequency = 231 */ { 1, "SetWLanRadioFrequency" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResponse = 232 */ { 1, "SetWLanRadioFrequencyResponse" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResult = 233 */ { 1, "SetWLanRadioFrequencyResult" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSecurity = 234 */ { 1, "SetWLanRadioSecurity" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResponse = 235 */ { 1, "SetWLanRadioSecurityResponse" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResult = 236 */ { 1, "SetWLanRadioSecurityResult" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSettings = 237 */ { 1, "SetWLanRadioSettings" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResponse = 238 */ { 1, "SetWLanRadioSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResult = 239 */ { 1, "SetWLanRadioSettingsResult" },
    /* ACTUAL_DOM_Element_PN_SetWanSettings = 240 */ { 1, "SetWanSettings" },
    /* ACTUAL_DOM_Element_PN_SetWanSettingsResponse = 241 */ { 1, "SetWanSettingsResponse" },
    /* ACTUAL_DOM_Element_PN_SetWanSettingsResult = 242 */ { 1, "SetWanSettingsResult" },
    /* ACTUAL_DOM_Element_PN_SignalStrength = 243 */ { 1, "SignalStrength" },
    /* ACTUAL_DOM_Element_PN_Stats = 244 */ { 1, "Stats" },
    /* ACTUAL_DOM_Element_PN_Status = 245 */ { 1, "Status" },
    /* ACTUAL_DOM_Element_PN_SubDeviceURLs = 246 */ { 1, "SubDeviceURLs" },
    /* ACTUAL_DOM_Element_PN_SubnetMask = 247 */ { 1, "SubnetMask" },
    /* ACTUAL_DOM_Element_PN_SupportedLocales = 248 */ { 1, "SupportedLocales" },
    /* ACTUAL_DOM_Element_PN_SupportedModes = 249 */ { 1, "SupportedModes" },
    /* ACTUAL_DOM_Element_PN_SupportedSecurity = 250 */ { 1, "SupportedSecurity" },
    /* ACTUAL_DOM_Element_PN_SupportedTypes = 251 */ { 1, "SupportedTypes" },
    /* ACTUAL_DOM_Element_PN_TaskExtension = 252 */ { 1, "TaskExtension" },
    /* ACTUAL_DOM_Element_PN_TaskExtensions = 253 */ { 1, "TaskExtensions" },
    /* ACTUAL_DOM_Element_PN_Tasks = 254 */ { 1, "Tasks" },
    /* ACTUAL_DOM_Element_PN_Tertiary = 255 */ { 1, "Tertiary" },
    /* ACTUAL_DOM_Element_PN_TimeZone = 256 */ { 1, "TimeZone" },
    /* ACTUAL_DOM_Element_PN_TimeZoneSupported = 257 */ { 1, "TimeZoneSupported" },
    /* ACTUAL_DOM_Element_PN_Type = 258 */ { 1, "Type" },
    /* ACTUAL_DOM_Element_PN_URL = 259 */ { 1, "URL" },
    /* ACTUAL_DOM_Element_PN_UpdateMethods = 260 */ { 1, "UpdateMethods" },
    /* ACTUAL_DOM_Element_PN_Username = 261 */ { 1, "Username" },
    /* ACTUAL_DOM_Element_PN_UsernameSupported = 262 */ { 1, "UsernameSupported" },
    /* ACTUAL_DOM_Element_PN_VendorName = 263 */ { 1, "VendorName" },
    /* ACTUAL_DOM_Element_PN_WLanChannel = 264 */ { 1, "WLanChannel" },
    /* ACTUAL_DOM_Element_PN_WLanChannelInfo = 265 */ { 1, "WLanChannelInfo" },
    /* ACTUAL_DOM_Element_PN_WLanChannelWidth = 266 */ { 1, "WLanChannelWidth" },
    /* ACTUAL_DOM_Element_PN_WLanChannelWidthInfo = 267 */ { 1, "WLanChannelWidthInfo" },
    /* ACTUAL_DOM_Element_PN_WLanEnabled = 268 */ { 1, "WLanEnabled" },
    /* ACTUAL_DOM_Element_PN_WLanEnabledInfo = 269 */ { 1, "WLanEnabledInfo" },
    /* ACTUAL_DOM_Element_PN_WLanEncryption = 270 */ { 1, "WLanEncryption" },
    /* ACTUAL_DOM_Element_PN_WLanEncryptionInfo = 271 */ { 1, "WLanEncryptionInfo" },
    /* ACTUAL_DOM_Element_PN_WLanFrequency = 272 */ { 1, "WLanFrequency" },
    /* ACTUAL_DOM_Element_PN_WLanFrequencyInfo = 273 */ { 1, "WLanFrequencyInfo" },
    /* ACTUAL_DOM_Element_PN_WLanKey = 274 */ { 1, "WLanKey" },
    /* ACTUAL_DOM_Element_PN_WLanKeyInfo = 275 */ { 1, "WLanKeyInfo" },
    /* ACTUAL_DOM_Element_PN_WLanKeyRenewal = 276 */ { 1, "WLanKeyRenewal" },
    /* ACTUAL_DOM_Element_PN_WLanKeyRenewalInfo = 277 */ { 1, "WLanKeyRenewalInfo" },
    /* ACTUAL_DOM_Element_PN_WLanMacAddress = 278 */ { 1, "WLanMacAddress" },
    /* ACTUAL_DOM_Element_PN_WLanMacAddressInfo = 279 */ { 1, "WLanMacAddressInfo" },
    /* ACTUAL_DOM_Element_PN_WLanMode = 280 */ { 1, "WLanMode" },
    /* ACTUAL_DOM_Element_PN_WLanModeInfo = 281 */ { 1, "WLanModeInfo" },
    /* ACTUAL_DOM_Element_PN_WLanQoS = 282 */ { 1, "WLanQoS" },
    /* ACTUAL_DOM_Element_PN_WLanQoSInfo = 283 */ { 1, "WLanQoSInfo" },
    /* ACTUAL_DOM_Element_PN_WLanRadioFrequencyInfos = 284 */ { 1, "WLanRadioFrequencyInfos" },
    /* ACTUAL_DOM_Element_PN_WLanRadioInfos = 285 */ { 1, "WLanRadioInfos" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusIP1 = 286 */ { 1, "WLanRadiusIP1" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusIP1Info = 287 */ { 1, "WLanRadiusIP1Info" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusIP2 = 288 */ { 1, "WLanRadiusIP2" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusIP2Info = 289 */ { 1, "WLanRadiusIP2Info" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusPort1 = 290 */ { 1, "WLanRadiusPort1" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusPort1Info = 291 */ { 1, "WLanRadiusPort1Info" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusPort2 = 292 */ { 1, "WLanRadiusPort2" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusPort2Info = 293 */ { 1, "WLanRadiusPort2Info" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusSecret1 = 294 */ { 1, "WLanRadiusSecret1" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusSecret1Info = 295 */ { 1, "WLanRadiusSecret1Info" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusSecret2 = 296 */ { 1, "WLanRadiusSecret2" },
    /* ACTUAL_DOM_Element_PN_WLanRadiusSecret2Info = 297 */ { 1, "WLanRadiusSecret2Info" },
    /* ACTUAL_DOM_Element_PN_WLanSSID = 298 */ { 1, "WLanSSID" },
    /* ACTUAL_DOM_Element_PN_WLanSSIDBroadcast = 299 */ { 1, "WLanSSIDBroadcast" },
    /* ACTUAL_DOM_Element_PN_WLanSSIDBroadcastInfo = 300 */ { 1, "WLanSSIDBroadcastInfo" },
    /* ACTUAL_DOM_Element_PN_WLanSSIDInfo = 301 */ { 1, "WLanSSIDInfo" },
    /* ACTUAL_DOM_Element_PN_WLanSecondaryChannel = 302 */ { 1, "WLanSecondaryChannel" },
    /* ACTUAL_DOM_Element_PN_WLanSecondaryChannelInfo = 303 */ { 1, "WLanSecondaryChannelInfo" },
    /* ACTUAL_DOM_Element_PN_WLanSecurityEnabled = 304 */ { 1, "WLanSecurityEnabled" },
    /* ACTUAL_DOM_Element_PN_WLanSecurityEnabledInfo = 305 */ { 1, "WLanSecurityEnabledInfo" },
    /* ACTUAL_DOM_Element_PN_WLanType = 306 */ { 1, "WLanType" },
    /* ACTUAL_DOM_Element_PN_WLanTypeInfo = 307 */ { 1, "WLanTypeInfo" },
    /* ACTUAL_DOM_Element_PN_WPSPin = 308 */ { 1, "WPSPin" },
    /* ACTUAL_DOM_Element_PN_WanAuthService = 309 */ { 1, "WanAuthService" },
    /* ACTUAL_DOM_Element_PN_WanAutoDetectType = 310 */ { 1, "WanAutoDetectType" },
    /* ACTUAL_DOM_Element_PN_WanAutoMTUSupported = 311 */ { 1, "WanAutoMTUSupported" },
    /* ACTUAL_DOM_Element_PN_WanAutoReconnect = 312 */ { 1, "WanAutoReconnect" },
    /* ACTUAL_DOM_Element_PN_WanDNS = 313 */ { 1, "WanDNS" },
    /* ACTUAL_DOM_Element_PN_WanGateway = 314 */ { 1, "WanGateway" },
    /* ACTUAL_DOM_Element_PN_WanIPAddress = 315 */ { 1, "WanIPAddress" },
    /* ACTUAL_DOM_Element_PN_WanLoginService = 316 */ { 1, "WanLoginService" },
    /* ACTUAL_DOM_Element_PN_WanMTU = 317 */ { 1, "WanMTU" },
    /* ACTUAL_DOM_Element_PN_WanMacAddress = 318 */ { 1, "WanMacAddress" },
    /* ACTUAL_DOM_Element_PN_WanMaxIdleTime = 319 */ { 1, "WanMaxIdleTime" },
    /* ACTUAL_DOM_Element_PN_WanPPPoEService = 320 */ { 1, "WanPPPoEService" },
    /* ACTUAL_DOM_Element_PN_WanPassword = 321 */ { 1, "WanPassword" },
    /* ACTUAL_DOM_Element_PN_WanRenewTimeout = 322 */ { 1, "WanRenewTimeout" },
    /* ACTUAL_DOM_Element_PN_WanStatus = 323 */ { 1, "WanStatus" },
    /* ACTUAL_DOM_Element_PN_WanSubnetMask = 324 */ { 1, "WanSubnetMask" },
    /* ACTUAL_DOM_Element_PN_WanSupportedTypes = 325 */ { 1, "WanSupportedTypes" },
    /* ACTUAL_DOM_Element_PN_WanType = 326 */ { 1, "WanType" },
    /* ACTUAL_DOM_Element_PN_WanUsername = 327 */ { 1, "WanUsername" },
    /* ACTUAL_DOM_Element_PN_WideChannel = 328 */ { 1, "WideChannel" },
    /* ACTUAL_DOM_Element_PN_WideChannels = 329 */ { 1, "WideChannels" },
    /* ACTUAL_DOM_Element_PN_WiredQoS = 330 */ { 1, "WiredQoS" },
    /* ACTUAL_DOM_Element_PN_WiredQoSSupported = 331 */ { 1, "WiredQoSSupported" },
    /* ACTUAL_DOM_Element_PN_Wireless = 332 */ { 1, "Wireless" },
    /* ACTUAL_DOM_Element_PN_int = 333 */ { 1, "int" },
    /* ACTUAL_DOM_Element_PN_string = 334 */ { 1, "string" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body = 335 */ { 2, "Body" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope = 336 */ { 2, "Envelope" },
    /* ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header = 337 */ { 2, "Header" },
    HDK_XML_Schema_ElementsEnd
};


/*
 * Enumeration http://cisco.com/HNAP/GetServiceInfoResult
 */

static const HDK_XML_EnumValue s_enum_GetServiceInfoResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://cisco.com/HNAP/GetServicesResult
 */

static const HDK_XML_EnumValue s_enum_GetServicesResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/AddPortMappingResult
 */

static const HDK_XML_EnumValue s_enum_PN_AddPortMappingResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/DeletePortMappingResult
 */

static const HDK_XML_EnumValue s_enum_PN_DeletePortMappingResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/DeviceType
 */

static const HDK_XML_EnumValue s_enum_PN_DeviceType[] =
{
    "Computer",
    "ComputerServer",
    "DigitalDVR",
    "DigitalJukebox",
    "Gateway",
    "GatewayWithWiFi",
    "LaptopComputer",
    "MediaAdapter",
    "NetworkCamera",
    "NetworkDevice",
    "NetworkDrive",
    "NetworkGameConsole",
    "NetworkPDA",
    "NetworkPrintServer",
    "NetworkPrinter",
    "PhotoFrame",
    "SetTopBox",
    "VOIPDevice",
    "WiFiAccessPoint",
    "WiFiBridge",
    "WorkstationComputer",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/DownloadSpeedTestResult
 */

static const HDK_XML_EnumValue s_enum_PN_DownloadSpeedTestResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/FirmwareUploadResult
 */

static const HDK_XML_EnumValue s_enum_PN_FirmwareUploadResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetClientStatsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetClientStatsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetConfigBlobResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetConfigBlobResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetConnectedDevicesResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetConnectedDevicesResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetDeviceSettings2Result
 */

static const HDK_XML_EnumValue s_enum_PN_GetDeviceSettings2Result[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetDeviceSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetDeviceSettingsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetFirmwareSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetFirmwareSettingsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetMACFilters2Result
 */

static const HDK_XML_EnumValue s_enum_PN_GetMACFilters2Result[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetNetworkStatsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetNetworkStatsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetPortMappingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetPortMappingsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetRouterLanSettings2Result
 */

static const HDK_XML_EnumValue s_enum_PN_GetRouterLanSettings2Result[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetRouterSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetRouterSettingsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioFrequenciesResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWLanRadioFrequenciesResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioSecurityResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWLanRadioSecurityResult[] =
{
    "OK",
    "ERROR",
    "ERROR_BAD_RADIOID",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadioSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWLanRadioSettingsResult[] =
{
    "OK",
    "ERROR",
    "ERROR_BAD_RADIOID",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWLanRadiosResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWLanRadiosResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWanInfoResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWanInfoResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/GetWanSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_GetWanSettingsResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/IPProtocol
 */

static const HDK_XML_EnumValue s_enum_PN_IPProtocol[] =
{
    "TCP",
    "UDP",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/IsDeviceReadyResult
 */

static const HDK_XML_EnumValue s_enum_PN_IsDeviceReadyResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/LANConnection
 */

static const HDK_XML_EnumValue s_enum_PN_LANConnection[] =
{
    "LAN",
    "WAN",
    "WLAN 802.11a",
    "WLAN 802.11b",
    "WLAN 802.11g",
    "WLAN 802.11n",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/RebootResult
 */

static const HDK_XML_EnumValue s_enum_PN_RebootResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/RenewWanConnectionResult
 */

static const HDK_XML_EnumValue s_enum_PN_RenewWanConnectionResult[] =
{
    "OK",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/RestoreFactoryDefaultsResult
 */

static const HDK_XML_EnumValue s_enum_PN_RestoreFactoryDefaultsResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetAccessPointModeResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetAccessPointModeResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetConfigBlobResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetConfigBlobResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetDeviceSettings2Result
 */

static const HDK_XML_EnumValue s_enum_PN_SetDeviceSettings2Result[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_REMOTE_SSL_NEEDS_SSL",
    "ERROR_TIMEZONE_NOT_SUPPORTED",
    "ERROR_USERNAME_NOT_SUPPORTED",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetDeviceSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetDeviceSettingsResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetMACFilters2Result
 */

static const HDK_XML_EnumValue s_enum_PN_SetMACFilters2Result[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetRouterLanSettings2Result
 */

static const HDK_XML_EnumValue s_enum_PN_SetRouterLanSettings2Result[] =
{
    "OK",
    "ERROR",
    "ERROR_BAD_IP_ADDRESS",
    "ERROR_BAD_IP_RANGE",
    "ERROR_BAD_RESERVATION",
    "ERROR_BAD_SUBNET",
    "ERROR_RESERVATIONS_NOT_SUPPORTED",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetRouterSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetRouterSettingsResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_DOMAIN_NOT_SUPPORTED",
    "ERROR_QOS_NOT_SUPPORTED",
    "ERROR_REMOTE_MANAGE_DEFAULT_PASSWORD",
    "ERROR_REMOTE_MANAGE_MUST_BE_SSL",
    "ERROR_REMOTE_MANAGE_NOT_SUPPORTED",
    "ERROR_REMOTE_SSL_NEEDS_SSL",
    "ERROR_REMOTE_SSL_NOT_SUPPORTED",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioFrequencyResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetWLanRadioFrequencyResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_BAD_RADIOID",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioSecurityResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetWLanRadioSecurityResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_BAD_RADIOID",
    "ERROR_BAD_RADIUS_VALUES",
    "ERROR_ENCRYPTION_NOT_SUPPORTED",
    "ERROR_ILLEGAL_KEY_VALUE",
    "ERROR_KEY_RENEWAL_BAD_VALUE",
    "ERROR_TYPE_NOT_SUPPORTED",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWLanRadioSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetWLanRadioSettingsResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_BAD_CHANNEL",
    "ERROR_BAD_CHANNEL_WIDTH",
    "ERROR_BAD_MODE",
    "ERROR_BAD_RADIOID",
    "ERROR_BAD_SECONDARY_CHANNEL",
    "ERROR_BAD_SSID",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/SetWanSettingsResult
 */

static const HDK_XML_EnumValue s_enum_PN_SetWanSettingsResult[] =
{
    "OK",
    "REBOOT",
    "ERROR",
    "ERROR_AUTO_MTU_NOT_SUPPORTED",
    "ERROR_BAD_WANTYPE",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/TaskExtType
 */

static const HDK_XML_EnumValue s_enum_PN_TaskExtType[] =
{
    "Browser",
    "MessageBox",
    "PUI",
    "Silent",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/UpdateMethod
 */

static const HDK_XML_EnumValue s_enum_PN_UpdateMethod[] =
{
    "HNAP_UPLOAD",
    "TFTP_UPLOAD",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/WANStatus
 */

static const HDK_XML_EnumValue s_enum_PN_WANStatus[] =
{
    "CONNECTED",
    "CONNECTING",
    "DISCONNECTED",
    "LIMITED_CONNECTION",
    "UNKNOWN",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/WANType
 */

static const HDK_XML_EnumValue s_enum_PN_WANType[] =
{
    "BigPond",
    "BridgedOnly",
    "DETECTING",
    "DHCP",
    "DHCPPPPoE",
    "Dynamic1483Bridged",
    "DynamicL2TP",
    "DynamicPPPOA",
    "DynamicPPTP",
    "Static",
    "Static1483Bridged",
    "Static1483Routed",
    "StaticIPOA",
    "StaticL2TP",
    "StaticPPPOA",
    "StaticPPPoE",
    "StaticPPTP",
    "UNKNOWN",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiEncryption
 */

static const HDK_XML_EnumValue s_enum_PN_WiFiEncryption[] =
{
    "",
    "AES",
    "TKIP",
    "TKIPORAES",
    "WEP-128",
    "WEP-64",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiMode
 */

static const HDK_XML_EnumValue s_enum_PN_WiFiMode[] =
{
    "",
    "802.11a",
    "802.11an",
    "802.11b",
    "802.11bg",
    "802.11bgn",
    "802.11bn",
    "802.11g",
    "802.11gn",
    "802.11n",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration http://purenetworks.com/HNAP1/WiFiSecurity
 */

static const HDK_XML_EnumValue s_enum_PN_WiFiSecurity[] =
{
    "",
    "WEP",
    "WEP-AUTO",
    "WEP-OPEN",
    "WEP-RADIUS",
    "WEP-SHARED",
    "WPA-AUTO-PSK",
    "WPA-PSK",
    "WPA-RADIUS",
    "WPA2-PSK",
    "WPA2-RADIUS",
    HDK_XML_Schema_EnumTypeValuesEnd
};


/*
 * Enumeration types array
 */

static const HDK_XML_EnumType s_enumTypes[] =
{
    s_enum_GetServiceInfoResult,
    s_enum_GetServicesResult,
    s_enum_PN_AddPortMappingResult,
    s_enum_PN_DeletePortMappingResult,
    s_enum_PN_DeviceType,
    s_enum_PN_DownloadSpeedTestResult,
    s_enum_PN_FirmwareUploadResult,
    s_enum_PN_GetClientStatsResult,
    s_enum_PN_GetConfigBlobResult,
    s_enum_PN_GetConnectedDevicesResult,
    s_enum_PN_GetDeviceSettings2Result,
    s_enum_PN_GetDeviceSettingsResult,
    s_enum_PN_GetFirmwareSettingsResult,
    s_enum_PN_GetMACFilters2Result,
    s_enum_PN_GetNetworkStatsResult,
    s_enum_PN_GetPortMappingsResult,
    s_enum_PN_GetRouterLanSettings2Result,
    s_enum_PN_GetRouterSettingsResult,
    s_enum_PN_GetWLanRadioFrequenciesResult,
    s_enum_PN_GetWLanRadioSecurityResult,
    s_enum_PN_GetWLanRadioSettingsResult,
    s_enum_PN_GetWLanRadiosResult,
    s_enum_PN_GetWanInfoResult,
    s_enum_PN_GetWanSettingsResult,
    s_enum_PN_IPProtocol,
    s_enum_PN_IsDeviceReadyResult,
    s_enum_PN_LANConnection,
    s_enum_PN_RebootResult,
    s_enum_PN_RenewWanConnectionResult,
    s_enum_PN_RestoreFactoryDefaultsResult,
    s_enum_PN_SetAccessPointModeResult,
    s_enum_PN_SetConfigBlobResult,
    s_enum_PN_SetDeviceSettings2Result,
    s_enum_PN_SetDeviceSettingsResult,
    s_enum_PN_SetMACFilters2Result,
    s_enum_PN_SetRouterLanSettings2Result,
    s_enum_PN_SetRouterSettingsResult,
    s_enum_PN_SetWLanRadioFrequencyResult,
    s_enum_PN_SetWLanRadioSecurityResult,
    s_enum_PN_SetWLanRadioSettingsResult,
    s_enum_PN_SetWanSettingsResult,
    s_enum_PN_TaskExtType,
    s_enum_PN_UpdateMethod,
    s_enum_PN_WANStatus,
    s_enum_PN_WANType,
    s_enum_PN_WiFiEncryption,
    s_enum_PN_WiFiMode,
    s_enum_PN_WiFiSecurity
};


/*
 * Method http://cisco.com/HNAP/GetServiceInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_GetServiceInfo_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_GetServiceInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_ServiceName, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_GetServiceInfo_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_GetServiceInfo,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_GetServiceInfo_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_GetServiceInfo_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_GetServiceInfo_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_GetServiceInfoResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_GetServiceInfoResult, ACTUAL_DOM_EnumType_GetServiceInfoResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_ServiceName, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 5, ACTUAL_DOM_Element_ServiceLevel, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_GetServiceInfo_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_GetServiceInfoResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_GetServiceInfo_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_GetServiceInfo_Output,
    s_enumTypes
};


/*
 * Method http://cisco.com/HNAP/GetServices
 */

static const HDK_XML_SchemaNode s_schemaNodes_GetServices_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_GetServices, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_GetServices_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_GetServices,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_GetServices_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_GetServices_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_GetServices_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_GetServicesResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_GetServicesResult, ACTUAL_DOM_EnumType_GetServicesResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_Services, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_ServiceInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_ServiceName, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_ServiceLevel, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_GetServices_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_GetServicesResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_GetServices_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_GetServices_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/AddPortMapping
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_AddPortMapping_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_AddPortMapping, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_PortMappingDescription, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_InternalClient, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_PortMappingProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_ExternalPort, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_InternalPort, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_AddPortMapping_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_AddPortMapping,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_AddPortMapping_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_AddPortMapping_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_AddPortMapping_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_AddPortMappingResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_AddPortMappingResult, ACTUAL_DOM_EnumType_PN_AddPortMappingResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_AddPortMapping_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_AddPortMappingResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_AddPortMapping_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_AddPortMapping_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/DeletePortMapping
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_DeletePortMapping_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_DeletePortMapping, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_PortMappingProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ExternalPort, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_DeletePortMapping_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_DeletePortMapping,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_DeletePortMapping_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DeletePortMapping_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_DeletePortMapping_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_DeletePortMappingResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_DeletePortMappingResult, ACTUAL_DOM_EnumType_PN_DeletePortMappingResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_DeletePortMapping_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_DeletePortMappingResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_DeletePortMapping_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DeletePortMapping_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/DownloadSpeedTest
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_DownloadSpeedTest_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_DownloadSpeedTest, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Bytes, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_BufferSize, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_DownloadSpeedTest_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_DownloadSpeedTest,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_DownloadSpeedTest_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DownloadSpeedTest_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_DownloadSpeedTest_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_DownloadSpeedTestResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_DownloadSpeedTestResult, ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ByteStream, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_DownloadSpeedTest_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_DownloadSpeedTestResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_DownloadSpeedTest_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DownloadSpeedTest_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/FirmwareUpload
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_FirmwareUpload_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_FirmwareUpload, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Base64Image, HDK_XML_BuiltinType_Blob, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_FirmwareUpload_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_FirmwareUpload,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_FirmwareUpload_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_FirmwareUpload_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_FirmwareUpload_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_FirmwareUploadResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_FirmwareUploadResult, ACTUAL_DOM_EnumType_PN_FirmwareUploadResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_FirmwareUpload_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_FirmwareUploadResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_FirmwareUpload_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_FirmwareUpload_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetClientStats
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetClientStats_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetClientStats, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetClientStats_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetClientStats,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetClientStats_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetClientStats_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetClientStats_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetClientStatsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetClientStatsResult, ACTUAL_DOM_EnumType_PN_GetClientStatsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ClientStats, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_ClientStat, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_PN_LinkSpeedIn, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_LinkSpeedOut, HDK_XML_BuiltinType_Int, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_PN_SignalStrength, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetClientStats_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetClientStatsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetClientStats_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetClientStats_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetConfigBlob
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetConfigBlob_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetConfigBlob, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetConfigBlob_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetConfigBlob,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetConfigBlob_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetConfigBlob_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetConfigBlob_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetConfigBlobResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetConfigBlobResult, ACTUAL_DOM_EnumType_PN_GetConfigBlobResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ConfigBlob, HDK_XML_BuiltinType_Blob, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetConfigBlob_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetConfigBlobResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetConfigBlob_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetConfigBlob_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetConnectedDevices
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetConnectedDevices_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetConnectedDevices, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetConnectedDevices_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetConnectedDevices,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetConnectedDevices_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetConnectedDevices_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetConnectedDevices_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetConnectedDevicesResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetConnectedDevicesResult, ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ConnectedClients, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_ConnectedClient, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_ConnectTime, HDK_XML_BuiltinType_DateTime, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 12 */ { 6, ACTUAL_DOM_Element_PN_Active, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetConnectedDevices_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetConnectedDevicesResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetConnectedDevices_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetConnectedDevices_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetDeviceSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetDeviceSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetDeviceSettings, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetDeviceSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetDeviceSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetDeviceSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetDeviceSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetDeviceSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetDeviceSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetDeviceSettingsResult, ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_DeviceType, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_VendorName, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_ModelDescription, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_ModelName, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_FirmwareVersion, HDK_XML_BuiltinType_String, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_PresentationURL, HDK_XML_BuiltinType_String, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_SOAPActions, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_SubDeviceURLs, HDK_XML_BuiltinType_Struct, 0 },
    /* 14 */ { 3, ACTUAL_DOM_Element_PN_Tasks, HDK_XML_BuiltinType_Struct, 0 },
    /* 15 */ { 12, ACTUAL_DOM_Element_PN_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 16 */ { 13, ACTUAL_DOM_Element_PN_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 17 */ { 14, ACTUAL_DOM_Element_PN_TaskExtension, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 18 */ { 17, ACTUAL_DOM_Element_PN_Name, HDK_XML_BuiltinType_String, 0 },
    /* 19 */ { 17, ACTUAL_DOM_Element_PN_URL, HDK_XML_BuiltinType_String, 0 },
    /* 20 */ { 17, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetDeviceSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetDeviceSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetDeviceSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetDeviceSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetDeviceSettings2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetDeviceSettings2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetDeviceSettings2, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetDeviceSettings2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetDeviceSettings2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetDeviceSettings2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetDeviceSettings2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetDeviceSettings2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetDeviceSettings2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetDeviceSettings2Result, ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_SerialNumber, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_TimeZone, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_AutoAdjustDST, HDK_XML_BuiltinType_Bool, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_Locale, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_SupportedLocales, HDK_XML_BuiltinType_Struct, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_SSL, HDK_XML_BuiltinType_Bool, 0 },
    /* 11 */ { 9, ACTUAL_DOM_Element_PN_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetDeviceSettings2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetDeviceSettings2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetDeviceSettings2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetDeviceSettings2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetFirmwareSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetFirmwareSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetFirmwareSettings, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetFirmwareSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetFirmwareSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetFirmwareSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetFirmwareSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetFirmwareSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetFirmwareSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetFirmwareSettingsResult, ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_VendorName, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_ModelName, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_ModelRevision, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_FirmwareVersion, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_FirmwareDate, HDK_XML_BuiltinType_DateTime, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_UpdateMethods, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 10, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_UpdateMethod, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetFirmwareSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetFirmwareSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetFirmwareSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetFirmwareSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetMACFilters2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetMACFilters2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetMACFilters2, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetMACFilters2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetMACFilters2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetMACFilters2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetMACFilters2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetMACFilters2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetMACFilters2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetMACFilters2Result, ACTUAL_DOM_EnumType_PN_GetMACFilters2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_IsAllowList, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_MACList, HDK_XML_BuiltinType_Struct, 0 },
    /* 8 */ { 7, ACTUAL_DOM_Element_PN_MACInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 9 */ { 8, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 10 */ { 8, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetMACFilters2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetMACFilters2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetMACFilters2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetMACFilters2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetNetworkStats
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetNetworkStats_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetNetworkStats, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetNetworkStats_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetNetworkStats,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetNetworkStats_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetNetworkStats_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetNetworkStats_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetNetworkStatsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetNetworkStatsResult, ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Stats, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_NetworkStats, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_PacketsReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_PN_PacketsSent, HDK_XML_BuiltinType_Long, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_BytesReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_PN_BytesSent, HDK_XML_BuiltinType_Long, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetNetworkStats_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetNetworkStatsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetNetworkStats_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetNetworkStats_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetPortMappings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetPortMappings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetPortMappings, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetPortMappings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetPortMappings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetPortMappings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetPortMappings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetPortMappings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetPortMappingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetPortMappingsResult, ACTUAL_DOM_EnumType_PN_GetPortMappingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_PortMappings, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_PortMapping, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_PortMappingDescription, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_InternalClient, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_PN_PortMappingProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_ExternalPort, HDK_XML_BuiltinType_Int, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_PN_InternalPort, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetPortMappings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetPortMappingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetPortMappings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetPortMappings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetRouterLanSettings2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetRouterLanSettings2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetRouterLanSettings2, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetRouterLanSettings2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetRouterLanSettings2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetRouterLanSettings2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetRouterLanSettings2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetRouterLanSettings2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetRouterLanSettings2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetRouterLanSettings2Result, ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_RouterIPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_RouterSubnetMask, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_DHCPServerEnabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_IPAddressFirst, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_IPAddressLast, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_LeaseTime, HDK_XML_BuiltinType_Int, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_DHCPReservations, HDK_XML_BuiltinType_Struct, 0 },
    /* 12 */ { 11, ACTUAL_DOM_Element_PN_DHCPReservation, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 13 */ { 12, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 14 */ { 12, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 15 */ { 12, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetRouterLanSettings2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetRouterLanSettings2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetRouterLanSettings2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetRouterLanSettings2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetRouterSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetRouterSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetRouterSettings, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetRouterSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetRouterSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetRouterSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetRouterSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetRouterSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetRouterSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetRouterSettingsResult, ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ManageRemote, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_ManageWireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_RemotePort, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_RemoteSSL, HDK_XML_BuiltinType_Bool, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_DomainName, HDK_XML_BuiltinType_String, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_WiredQoS, HDK_XML_BuiltinType_Bool, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_WPSPin, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetRouterSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetRouterSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetRouterSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetRouterSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioFrequencies
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioFrequencies_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioFrequencies, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioFrequencies_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioFrequencies,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioFrequencies_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioFrequencies_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioFrequencies_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResult, ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_RadioFrequencyInfos, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_RadioFrequencyInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_Frequencies, HDK_XML_BuiltinType_Struct, 0 },
    /* 9 */ { 8, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioFrequencies_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioFrequencies_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioFrequencies_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioSecurity
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioSecurity_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioSecurity, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioSecurity_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioSecurity,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioSecurity_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioSecurity_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioSecurity_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResult, ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_Encryption, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_Key, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_KeyRenewal, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_RadiusPort1, HDK_XML_BuiltinType_Int, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_RadiusSecret1, HDK_XML_BuiltinType_String, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 14 */ { 3, ACTUAL_DOM_Element_PN_RadiusPort2, HDK_XML_BuiltinType_Int, 0 },
    /* 15 */ { 3, ACTUAL_DOM_Element_PN_RadiusSecret2, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioSecurity_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioSecurity_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioSecurity_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadioSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadioSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResult, ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Mode, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_SSID, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_ChannelWidth, HDK_XML_BuiltinType_Int, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_SecondaryChannel, HDK_XML_BuiltinType_Int, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_QoS, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadioSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadioSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadioSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWLanRadios
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadios_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadios, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadios_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadios,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadios_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadios_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWLanRadios_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWLanRadiosResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWLanRadiosResult, ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_RadioInfos, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 5, ACTUAL_DOM_Element_PN_RadioInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 6, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    /* 9 */ { 6, ACTUAL_DOM_Element_PN_SupportedModes, HDK_XML_BuiltinType_Struct, 0 },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_Channels, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 6, ACTUAL_DOM_Element_PN_WideChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 12 */ { 6, ACTUAL_DOM_Element_PN_SupportedSecurity, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 9, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiMode, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 14 */ { 10, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 15 */ { 11, ACTUAL_DOM_Element_PN_WideChannel, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 16 */ { 12, ACTUAL_DOM_Element_PN_SecurityInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 17 */ { 15, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 18 */ { 15, ACTUAL_DOM_Element_PN_SecondaryChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 19 */ { 16, ACTUAL_DOM_Element_PN_SecurityType, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 20 */ { 16, ACTUAL_DOM_Element_PN_Encryptions, HDK_XML_BuiltinType_Struct, 0 },
    /* 21 */ { 18, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 22 */ { 20, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiEncryption, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWLanRadios_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWLanRadiosResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWLanRadios_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWLanRadios_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWanInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWanInfo_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWanInfo, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWanInfo_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWanInfo,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWanInfo_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWanInfo_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWanInfo_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWanInfoResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWanInfoResult, ACTUAL_DOM_EnumType_PN_GetWanInfoResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_SupportedTypes, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_AutoDetectType, ACTUAL_DOM_EnumType_PN_WANType, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_Status, ACTUAL_DOM_EnumType_PN_WANStatus, 0 },
    /* 8 */ { 5, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WANType, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWanInfo_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWanInfoResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWanInfo_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWanInfo_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/GetWanSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWanSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWanSettings, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWanSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWanSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWanSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWanSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_GetWanSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_GetWanSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_GetWanSettingsResult, ACTUAL_DOM_EnumType_PN_GetWanSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WANType, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Username, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_Password, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_MaxIdleTime, HDK_XML_BuiltinType_Int, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_MTU, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_ServiceName, HDK_XML_BuiltinType_String, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_AutoReconnect, HDK_XML_BuiltinType_Bool, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_SubnetMask, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 14 */ { 3, ACTUAL_DOM_Element_PN_Gateway, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 15 */ { 3, ACTUAL_DOM_Element_PN_DNS, HDK_XML_BuiltinType_Struct, 0 },
    /* 16 */ { 3, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 17 */ { 15, ACTUAL_DOM_Element_PN_Primary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 18 */ { 15, ACTUAL_DOM_Element_PN_Secondary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 19 */ { 15, ACTUAL_DOM_Element_PN_Tertiary, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_GetWanSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_GetWanSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_GetWanSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_GetWanSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/IsDeviceReady
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_IsDeviceReady_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_IsDeviceReady, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_IsDeviceReady_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_IsDeviceReady,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_IsDeviceReady_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_IsDeviceReady_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_IsDeviceReady_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_IsDeviceReadyResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_IsDeviceReadyResult, ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_IsDeviceReady_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_IsDeviceReadyResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_IsDeviceReady_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_IsDeviceReady_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/Reboot
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_Reboot_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_Reboot, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_Reboot_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_Reboot,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_Reboot_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_Reboot_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_Reboot_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_RebootResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RebootResult, ACTUAL_DOM_EnumType_PN_RebootResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_Reboot_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_RebootResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_Reboot_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_Reboot_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/RenewWanConnection
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_RenewWanConnection_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_RenewWanConnection, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RenewTimeout, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_RenewWanConnection_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_RenewWanConnection,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_RenewWanConnection_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RenewWanConnection_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_RenewWanConnection_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_RenewWanConnectionResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RenewWanConnectionResult, ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_RenewWanConnection_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_RenewWanConnectionResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_RenewWanConnection_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RenewWanConnection_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/RestoreFactoryDefaults
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_RestoreFactoryDefaults_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_RestoreFactoryDefaults, HDK_XML_BuiltinType_Struct, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_RestoreFactoryDefaults_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_RestoreFactoryDefaults,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_RestoreFactoryDefaults_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RestoreFactoryDefaults_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_RestoreFactoryDefaults_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResult, ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_RestoreFactoryDefaults_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_RestoreFactoryDefaults_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RestoreFactoryDefaults_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetAccessPointMode
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetAccessPointMode_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetAccessPointMode, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_IsAccessPoint, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetAccessPointMode_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetAccessPointMode,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetAccessPointMode_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetAccessPointMode_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetAccessPointMode_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetAccessPointModeResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetAccessPointModeResult, ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_NewIPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetAccessPointMode_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetAccessPointModeResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetAccessPointMode_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetAccessPointMode_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetConfigBlob
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetConfigBlob_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetConfigBlob, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_ConfigBlob, HDK_XML_BuiltinType_Blob, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetConfigBlob_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetConfigBlob,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetConfigBlob_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetConfigBlob_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetConfigBlob_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetConfigBlobResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetConfigBlobResult, ACTUAL_DOM_EnumType_PN_SetConfigBlobResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetConfigBlob_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetConfigBlobResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetConfigBlob_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetConfigBlob_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetDeviceSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetDeviceSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetDeviceSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_AdminPassword, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetDeviceSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetDeviceSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetDeviceSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetDeviceSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetDeviceSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetDeviceSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetDeviceSettingsResult, ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetDeviceSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetDeviceSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetDeviceSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetDeviceSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetDeviceSettings2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetDeviceSettings2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetDeviceSettings2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Username, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_TimeZone, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_AutoAdjustDST, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_Locale, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_SSL, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetDeviceSettings2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetDeviceSettings2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetDeviceSettings2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetDeviceSettings2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetDeviceSettings2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetDeviceSettings2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetDeviceSettings2Result, ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetDeviceSettings2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetDeviceSettings2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetDeviceSettings2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetDeviceSettings2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetMACFilters2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetMACFilters2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetMACFilters2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_IsAllowList, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_MACList, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 6, ACTUAL_DOM_Element_PN_MACInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 8 */ { 7, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 9 */ { 7, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetMACFilters2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetMACFilters2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetMACFilters2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetMACFilters2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetMACFilters2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetMACFilters2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetMACFilters2Result, ACTUAL_DOM_EnumType_PN_SetMACFilters2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetMACFilters2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetMACFilters2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetMACFilters2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetMACFilters2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetRouterLanSettings2
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetRouterLanSettings2_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetRouterLanSettings2, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RouterIPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_RouterSubnetMask, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_DHCPServerEnabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_IPAddressFirst, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_IPAddressLast, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_LeaseTime, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_DHCPReservations, HDK_XML_BuiltinType_Struct, 0 },
    /* 11 */ { 10, ACTUAL_DOM_Element_PN_DHCPReservation, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 12 */ { 11, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 13 */ { 11, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 14 */ { 11, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetRouterLanSettings2_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetRouterLanSettings2,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetRouterLanSettings2_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetRouterLanSettings2_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetRouterLanSettings2_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetRouterLanSettings2Response, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetRouterLanSettings2Result, ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetRouterLanSettings2_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetRouterLanSettings2Response,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetRouterLanSettings2_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetRouterLanSettings2_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetRouterSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetRouterSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetRouterSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_ManageRemote, HDK_XML_BuiltinType_Bool, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_ManageWireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_RemotePort, HDK_XML_BuiltinType_Int, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_RemoteSSL, HDK_XML_BuiltinType_Bool, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_DomainName, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_WiredQoS, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetRouterSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetRouterSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetRouterSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetRouterSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetRouterSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetRouterSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetRouterSettingsResult, ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetRouterSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetRouterSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetRouterSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetRouterSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioFrequency
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioFrequency_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioFrequency, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioFrequency_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioFrequency,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioFrequency_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioFrequency_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioFrequency_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResult, ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioFrequency_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioFrequency_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioFrequency_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioSecurity
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioSecurity_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioSecurity, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_Encryption, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_Key, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_KeyRenewal, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_RadiusPort1, HDK_XML_BuiltinType_Int, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_RadiusSecret1, HDK_XML_BuiltinType_String, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 14 */ { 3, ACTUAL_DOM_Element_PN_RadiusPort2, HDK_XML_BuiltinType_Int, 0 },
    /* 15 */ { 3, ACTUAL_DOM_Element_PN_RadiusSecret2, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioSecurity_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioSecurity,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioSecurity_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioSecurity_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioSecurity_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResult, ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioSecurity_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioSecurity_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioSecurity_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetWLanRadioSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Mode, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_SSID, HDK_XML_BuiltinType_String, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_ChannelWidth, HDK_XML_BuiltinType_Int, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_SecondaryChannel, HDK_XML_BuiltinType_Int, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_QoS, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWLanRadioSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResult, ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWLanRadioSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWLanRadioSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWLanRadioSettings_Output,
    s_enumTypes
};


/*
 * Method http://purenetworks.com/HNAP1/SetWanSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWanSettings_Input[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWanSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WANType, 0 },
    /* 5 */ { 3, ACTUAL_DOM_Element_PN_Username, HDK_XML_BuiltinType_String, 0 },
    /* 6 */ { 3, ACTUAL_DOM_Element_PN_Password, HDK_XML_BuiltinType_String, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_MaxIdleTime, HDK_XML_BuiltinType_Int, 0 },
    /* 8 */ { 3, ACTUAL_DOM_Element_PN_ServiceName, HDK_XML_BuiltinType_String, 0 },
    /* 9 */ { 3, ACTUAL_DOM_Element_PN_AutoReconnect, HDK_XML_BuiltinType_Bool, 0 },
    /* 10 */ { 3, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 11 */ { 3, ACTUAL_DOM_Element_PN_SubnetMask, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 12 */ { 3, ACTUAL_DOM_Element_PN_Gateway, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 13 */ { 3, ACTUAL_DOM_Element_PN_DNS, HDK_XML_BuiltinType_Struct, 0 },
    /* 14 */ { 3, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 15 */ { 3, ACTUAL_DOM_Element_PN_MTU, HDK_XML_BuiltinType_Int, 0 },
    /* 16 */ { 13, ACTUAL_DOM_Element_PN_Primary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 17 */ { 13, ACTUAL_DOM_Element_PN_Secondary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 18 */ { 13, ACTUAL_DOM_Element_PN_Tertiary, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWanSettings_Input[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWanSettings,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWanSettings_Input =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWanSettings_Input,
    s_enumTypes
};

static const HDK_XML_SchemaNode s_schemaNodes_PN_SetWanSettings_Output[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Envelope, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_AnyElement | HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 1 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Header, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_AnyElement },
    /* 2 */ { 0, ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_SetWanSettingsResponse, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_ErrorOutput },
    /* 4 */ { 3, ACTUAL_DOM_Element_PN_SetWanSettingsResult, ACTUAL_DOM_EnumType_PN_SetWanSettingsResult, HDK_XML_SchemaNodeProperty_ErrorOutput },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Element s_elementPath_PN_SetWanSettings_Output[] =
{
    ACTUAL_DOM_Element_Schemas_xmlsoap_org_soap_envelope_Body,
    ACTUAL_DOM_Element_PN_SetWanSettingsResponse,
    HDK_MOD_ElementPathEnd
};

static const HDK_XML_Schema s_schema_PN_SetWanSettings_Output =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SetWanSettings_Output,
    s_enumTypes
};


/*
 * Methods
 */

static const HDK_MOD_Method s_methods[] =
{
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP/GetServiceInfo",
        ACTUAL_DOM_Method_GetServiceInfo,
        &s_schema_GetServiceInfo_Input,
        &s_schema_GetServiceInfo_Output,
        s_elementPath_GetServiceInfo_Input,
        s_elementPath_GetServiceInfo_Output,
        0,
        ACTUAL_DOM_Element_GetServiceInfoResult,
        ACTUAL_DOM_EnumType_GetServiceInfoResult,
        ACTUAL_DOM_Enum_GetServiceInfoResult_OK,
        ACTUAL_DOM_Enum_GetServiceInfoResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://cisco.com/HNAP/GetServices",
        ACTUAL_DOM_Method_GetServices,
        &s_schema_GetServices_Input,
        &s_schema_GetServices_Output,
        s_elementPath_GetServices_Input,
        s_elementPath_GetServices_Output,
        0,
        ACTUAL_DOM_Element_GetServicesResult,
        ACTUAL_DOM_EnumType_GetServicesResult,
        ACTUAL_DOM_Enum_GetServicesResult_OK,
        ACTUAL_DOM_Enum_GetServicesResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/AddPortMapping",
        ACTUAL_DOM_Method_PN_AddPortMapping,
        &s_schema_PN_AddPortMapping_Input,
        &s_schema_PN_AddPortMapping_Output,
        s_elementPath_PN_AddPortMapping_Input,
        s_elementPath_PN_AddPortMapping_Output,
        0,
        ACTUAL_DOM_Element_PN_AddPortMappingResult,
        ACTUAL_DOM_EnumType_PN_AddPortMappingResult,
        ACTUAL_DOM_Enum_PN_AddPortMappingResult_OK,
        ACTUAL_DOM_Enum_PN_AddPortMappingResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/DeletePortMapping",
        ACTUAL_DOM_Method_PN_DeletePortMapping,
        &s_schema_PN_DeletePortMapping_Input,
        &s_schema_PN_DeletePortMapping_Output,
        s_elementPath_PN_DeletePortMapping_Input,
        s_elementPath_PN_DeletePortMapping_Output,
        0,
        ACTUAL_DOM_Element_PN_DeletePortMappingResult,
        ACTUAL_DOM_EnumType_PN_DeletePortMappingResult,
        ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK,
        ACTUAL_DOM_Enum_PN_DeletePortMappingResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/DownloadSpeedTest",
        ACTUAL_DOM_Method_PN_DownloadSpeedTest,
        &s_schema_PN_DownloadSpeedTest_Input,
        &s_schema_PN_DownloadSpeedTest_Output,
        s_elementPath_PN_DownloadSpeedTest_Input,
        s_elementPath_PN_DownloadSpeedTest_Output,
        0,
        ACTUAL_DOM_Element_PN_DownloadSpeedTestResult,
        ACTUAL_DOM_EnumType_PN_DownloadSpeedTestResult,
        ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK,
        ACTUAL_DOM_Enum_PN_DownloadSpeedTestResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/FirmwareUpload",
        ACTUAL_DOM_Method_PN_FirmwareUpload,
        &s_schema_PN_FirmwareUpload_Input,
        &s_schema_PN_FirmwareUpload_Output,
        s_elementPath_PN_FirmwareUpload_Input,
        s_elementPath_PN_FirmwareUpload_Output,
        0,
        ACTUAL_DOM_Element_PN_FirmwareUploadResult,
        ACTUAL_DOM_EnumType_PN_FirmwareUploadResult,
        ACTUAL_DOM_Enum_PN_FirmwareUploadResult_OK,
        ACTUAL_DOM_Enum_PN_FirmwareUploadResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetClientStats",
        ACTUAL_DOM_Method_PN_GetClientStats,
        &s_schema_PN_GetClientStats_Input,
        &s_schema_PN_GetClientStats_Output,
        s_elementPath_PN_GetClientStats_Input,
        s_elementPath_PN_GetClientStats_Output,
        0,
        ACTUAL_DOM_Element_PN_GetClientStatsResult,
        ACTUAL_DOM_EnumType_PN_GetClientStatsResult,
        ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK,
        ACTUAL_DOM_Enum_PN_GetClientStatsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetConfigBlob",
        ACTUAL_DOM_Method_PN_GetConfigBlob,
        &s_schema_PN_GetConfigBlob_Input,
        &s_schema_PN_GetConfigBlob_Output,
        s_elementPath_PN_GetConfigBlob_Input,
        s_elementPath_PN_GetConfigBlob_Output,
        0,
        ACTUAL_DOM_Element_PN_GetConfigBlobResult,
        ACTUAL_DOM_EnumType_PN_GetConfigBlobResult,
        ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK,
        ACTUAL_DOM_Enum_PN_GetConfigBlobResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetConnectedDevices",
        ACTUAL_DOM_Method_PN_GetConnectedDevices,
        &s_schema_PN_GetConnectedDevices_Input,
        &s_schema_PN_GetConnectedDevices_Output,
        s_elementPath_PN_GetConnectedDevices_Input,
        s_elementPath_PN_GetConnectedDevices_Output,
        0,
        ACTUAL_DOM_Element_PN_GetConnectedDevicesResult,
        ACTUAL_DOM_EnumType_PN_GetConnectedDevicesResult,
        ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK,
        ACTUAL_DOM_Enum_PN_GetConnectedDevicesResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetDeviceSettings",
        ACTUAL_DOM_Method_PN_GetDeviceSettings,
        &s_schema_PN_GetDeviceSettings_Input,
        &s_schema_PN_GetDeviceSettings_Output,
        s_elementPath_PN_GetDeviceSettings_Input,
        s_elementPath_PN_GetDeviceSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetDeviceSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult,
        ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK
    },
    {
        "GET",
        "/HNAP1",
        0,
        ACTUAL_DOM_Method_PN_GetDeviceSettings,
        &s_schema_PN_GetDeviceSettings_Input,
        &s_schema_PN_GetDeviceSettings_Output,
        0,
        s_elementPath_PN_GetDeviceSettings_Output,
        HDK_MOD_MethodOption_NoBasicAuth | HDK_MOD_MethodOption_NoInputStruct,
        ACTUAL_DOM_Element_PN_GetDeviceSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetDeviceSettingsResult,
        ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetDeviceSettingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetDeviceSettings2",
        ACTUAL_DOM_Method_PN_GetDeviceSettings2,
        &s_schema_PN_GetDeviceSettings2_Input,
        &s_schema_PN_GetDeviceSettings2_Output,
        s_elementPath_PN_GetDeviceSettings2_Input,
        s_elementPath_PN_GetDeviceSettings2_Output,
        0,
        ACTUAL_DOM_Element_PN_GetDeviceSettings2Result,
        ACTUAL_DOM_EnumType_PN_GetDeviceSettings2Result,
        ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK,
        ACTUAL_DOM_Enum_PN_GetDeviceSettings2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetFirmwareSettings",
        ACTUAL_DOM_Method_PN_GetFirmwareSettings,
        &s_schema_PN_GetFirmwareSettings_Input,
        &s_schema_PN_GetFirmwareSettings_Output,
        s_elementPath_PN_GetFirmwareSettings_Input,
        s_elementPath_PN_GetFirmwareSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetFirmwareSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetFirmwareSettingsResult,
        ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetFirmwareSettingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetMACFilters2",
        ACTUAL_DOM_Method_PN_GetMACFilters2,
        &s_schema_PN_GetMACFilters2_Input,
        &s_schema_PN_GetMACFilters2_Output,
        s_elementPath_PN_GetMACFilters2_Input,
        s_elementPath_PN_GetMACFilters2_Output,
        0,
        ACTUAL_DOM_Element_PN_GetMACFilters2Result,
        ACTUAL_DOM_EnumType_PN_GetMACFilters2Result,
        ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK,
        ACTUAL_DOM_Enum_PN_GetMACFilters2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetNetworkStats",
        ACTUAL_DOM_Method_PN_GetNetworkStats,
        &s_schema_PN_GetNetworkStats_Input,
        &s_schema_PN_GetNetworkStats_Output,
        s_elementPath_PN_GetNetworkStats_Input,
        s_elementPath_PN_GetNetworkStats_Output,
        0,
        ACTUAL_DOM_Element_PN_GetNetworkStatsResult,
        ACTUAL_DOM_EnumType_PN_GetNetworkStatsResult,
        ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK,
        ACTUAL_DOM_Enum_PN_GetNetworkStatsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetPortMappings",
        ACTUAL_DOM_Method_PN_GetPortMappings,
        &s_schema_PN_GetPortMappings_Input,
        &s_schema_PN_GetPortMappings_Output,
        s_elementPath_PN_GetPortMappings_Input,
        s_elementPath_PN_GetPortMappings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetPortMappingsResult,
        ACTUAL_DOM_EnumType_PN_GetPortMappingsResult,
        ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetPortMappingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetRouterLanSettings2",
        ACTUAL_DOM_Method_PN_GetRouterLanSettings2,
        &s_schema_PN_GetRouterLanSettings2_Input,
        &s_schema_PN_GetRouterLanSettings2_Output,
        s_elementPath_PN_GetRouterLanSettings2_Input,
        s_elementPath_PN_GetRouterLanSettings2_Output,
        0,
        ACTUAL_DOM_Element_PN_GetRouterLanSettings2Result,
        ACTUAL_DOM_EnumType_PN_GetRouterLanSettings2Result,
        ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK,
        ACTUAL_DOM_Enum_PN_GetRouterLanSettings2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetRouterSettings",
        ACTUAL_DOM_Method_PN_GetRouterSettings,
        &s_schema_PN_GetRouterSettings_Input,
        &s_schema_PN_GetRouterSettings_Output,
        s_elementPath_PN_GetRouterSettings_Input,
        s_elementPath_PN_GetRouterSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetRouterSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetRouterSettingsResult,
        ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetRouterSettingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWLanRadioFrequencies",
        ACTUAL_DOM_Method_PN_GetWLanRadioFrequencies,
        &s_schema_PN_GetWLanRadioFrequencies_Input,
        &s_schema_PN_GetWLanRadioFrequencies_Output,
        s_elementPath_PN_GetWLanRadioFrequencies_Input,
        s_elementPath_PN_GetWLanRadioFrequencies_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWLanRadioFrequenciesResult,
        ACTUAL_DOM_EnumType_PN_GetWLanRadioFrequenciesResult,
        ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK,
        ACTUAL_DOM_Enum_PN_GetWLanRadioFrequenciesResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWLanRadioSecurity",
        ACTUAL_DOM_Method_PN_GetWLanRadioSecurity,
        &s_schema_PN_GetWLanRadioSecurity_Input,
        &s_schema_PN_GetWLanRadioSecurity_Output,
        s_elementPath_PN_GetWLanRadioSecurity_Input,
        s_elementPath_PN_GetWLanRadioSecurity_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWLanRadioSecurityResult,
        ACTUAL_DOM_EnumType_PN_GetWLanRadioSecurityResult,
        ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK,
        ACTUAL_DOM_Enum_PN_GetWLanRadioSecurityResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWLanRadioSettings",
        ACTUAL_DOM_Method_PN_GetWLanRadioSettings,
        &s_schema_PN_GetWLanRadioSettings_Input,
        &s_schema_PN_GetWLanRadioSettings_Output,
        s_elementPath_PN_GetWLanRadioSettings_Input,
        s_elementPath_PN_GetWLanRadioSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWLanRadioSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetWLanRadioSettingsResult,
        ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetWLanRadioSettingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWLanRadios",
        ACTUAL_DOM_Method_PN_GetWLanRadios,
        &s_schema_PN_GetWLanRadios_Input,
        &s_schema_PN_GetWLanRadios_Output,
        s_elementPath_PN_GetWLanRadios_Input,
        s_elementPath_PN_GetWLanRadios_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWLanRadiosResult,
        ACTUAL_DOM_EnumType_PN_GetWLanRadiosResult,
        ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK,
        ACTUAL_DOM_Enum_PN_GetWLanRadiosResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWanInfo",
        ACTUAL_DOM_Method_PN_GetWanInfo,
        &s_schema_PN_GetWanInfo_Input,
        &s_schema_PN_GetWanInfo_Output,
        s_elementPath_PN_GetWanInfo_Input,
        s_elementPath_PN_GetWanInfo_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWanInfoResult,
        ACTUAL_DOM_EnumType_PN_GetWanInfoResult,
        ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK,
        ACTUAL_DOM_Enum_PN_GetWanInfoResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/GetWanSettings",
        ACTUAL_DOM_Method_PN_GetWanSettings,
        &s_schema_PN_GetWanSettings_Input,
        &s_schema_PN_GetWanSettings_Output,
        s_elementPath_PN_GetWanSettings_Input,
        s_elementPath_PN_GetWanSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_GetWanSettingsResult,
        ACTUAL_DOM_EnumType_PN_GetWanSettingsResult,
        ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_GetWanSettingsResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/IsDeviceReady",
        ACTUAL_DOM_Method_PN_IsDeviceReady,
        &s_schema_PN_IsDeviceReady_Input,
        &s_schema_PN_IsDeviceReady_Output,
        s_elementPath_PN_IsDeviceReady_Input,
        s_elementPath_PN_IsDeviceReady_Output,
        0,
        ACTUAL_DOM_Element_PN_IsDeviceReadyResult,
        ACTUAL_DOM_EnumType_PN_IsDeviceReadyResult,
        ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK,
        ACTUAL_DOM_Enum_PN_IsDeviceReadyResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/Reboot",
        ACTUAL_DOM_Method_PN_Reboot,
        &s_schema_PN_Reboot_Input,
        &s_schema_PN_Reboot_Output,
        s_elementPath_PN_Reboot_Input,
        s_elementPath_PN_Reboot_Output,
        0,
        ACTUAL_DOM_Element_PN_RebootResult,
        ACTUAL_DOM_EnumType_PN_RebootResult,
        ACTUAL_DOM_Enum_PN_RebootResult_OK,
        ACTUAL_DOM_Enum_PN_RebootResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/RenewWanConnection",
        ACTUAL_DOM_Method_PN_RenewWanConnection,
        &s_schema_PN_RenewWanConnection_Input,
        &s_schema_PN_RenewWanConnection_Output,
        s_elementPath_PN_RenewWanConnection_Input,
        s_elementPath_PN_RenewWanConnection_Output,
        0,
        ACTUAL_DOM_Element_PN_RenewWanConnectionResult,
        ACTUAL_DOM_EnumType_PN_RenewWanConnectionResult,
        ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK,
        ACTUAL_DOM_Enum_PN_RenewWanConnectionResult_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/RestoreFactoryDefaults",
        ACTUAL_DOM_Method_PN_RestoreFactoryDefaults,
        &s_schema_PN_RestoreFactoryDefaults_Input,
        &s_schema_PN_RestoreFactoryDefaults_Output,
        s_elementPath_PN_RestoreFactoryDefaults_Input,
        s_elementPath_PN_RestoreFactoryDefaults_Output,
        0,
        ACTUAL_DOM_Element_PN_RestoreFactoryDefaultsResult,
        ACTUAL_DOM_EnumType_PN_RestoreFactoryDefaultsResult,
        ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_OK,
        ACTUAL_DOM_Enum_PN_RestoreFactoryDefaultsResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetAccessPointMode",
        ACTUAL_DOM_Method_PN_SetAccessPointMode,
        &s_schema_PN_SetAccessPointMode_Input,
        &s_schema_PN_SetAccessPointMode_Output,
        s_elementPath_PN_SetAccessPointMode_Input,
        s_elementPath_PN_SetAccessPointMode_Output,
        0,
        ACTUAL_DOM_Element_PN_SetAccessPointModeResult,
        ACTUAL_DOM_EnumType_PN_SetAccessPointModeResult,
        ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_OK,
        ACTUAL_DOM_Enum_PN_SetAccessPointModeResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetConfigBlob",
        ACTUAL_DOM_Method_PN_SetConfigBlob,
        &s_schema_PN_SetConfigBlob_Input,
        &s_schema_PN_SetConfigBlob_Output,
        s_elementPath_PN_SetConfigBlob_Input,
        s_elementPath_PN_SetConfigBlob_Output,
        0,
        ACTUAL_DOM_Element_PN_SetConfigBlobResult,
        ACTUAL_DOM_EnumType_PN_SetConfigBlobResult,
        ACTUAL_DOM_Enum_PN_SetConfigBlobResult_OK,
        ACTUAL_DOM_Enum_PN_SetConfigBlobResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetDeviceSettings",
        ACTUAL_DOM_Method_PN_SetDeviceSettings,
        &s_schema_PN_SetDeviceSettings_Input,
        &s_schema_PN_SetDeviceSettings_Output,
        s_elementPath_PN_SetDeviceSettings_Input,
        s_elementPath_PN_SetDeviceSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_SetDeviceSettingsResult,
        ACTUAL_DOM_EnumType_PN_SetDeviceSettingsResult,
        ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_SetDeviceSettingsResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetDeviceSettings2",
        ACTUAL_DOM_Method_PN_SetDeviceSettings2,
        &s_schema_PN_SetDeviceSettings2_Input,
        &s_schema_PN_SetDeviceSettings2_Output,
        s_elementPath_PN_SetDeviceSettings2_Input,
        s_elementPath_PN_SetDeviceSettings2_Output,
        0,
        ACTUAL_DOM_Element_PN_SetDeviceSettings2Result,
        ACTUAL_DOM_EnumType_PN_SetDeviceSettings2Result,
        ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_OK,
        ACTUAL_DOM_Enum_PN_SetDeviceSettings2Result_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetMACFilters2",
        ACTUAL_DOM_Method_PN_SetMACFilters2,
        &s_schema_PN_SetMACFilters2_Input,
        &s_schema_PN_SetMACFilters2_Output,
        s_elementPath_PN_SetMACFilters2_Input,
        s_elementPath_PN_SetMACFilters2_Output,
        0,
        ACTUAL_DOM_Element_PN_SetMACFilters2Result,
        ACTUAL_DOM_EnumType_PN_SetMACFilters2Result,
        ACTUAL_DOM_Enum_PN_SetMACFilters2Result_OK,
        ACTUAL_DOM_Enum_PN_SetMACFilters2Result_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetRouterLanSettings2",
        ACTUAL_DOM_Method_PN_SetRouterLanSettings2,
        &s_schema_PN_SetRouterLanSettings2_Input,
        &s_schema_PN_SetRouterLanSettings2_Output,
        s_elementPath_PN_SetRouterLanSettings2_Input,
        s_elementPath_PN_SetRouterLanSettings2_Output,
        0,
        ACTUAL_DOM_Element_PN_SetRouterLanSettings2Result,
        ACTUAL_DOM_EnumType_PN_SetRouterLanSettings2Result,
        ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK,
        ACTUAL_DOM_Enum_PN_SetRouterLanSettings2Result_OK
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetRouterSettings",
        ACTUAL_DOM_Method_PN_SetRouterSettings,
        &s_schema_PN_SetRouterSettings_Input,
        &s_schema_PN_SetRouterSettings_Output,
        s_elementPath_PN_SetRouterSettings_Input,
        s_elementPath_PN_SetRouterSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_SetRouterSettingsResult,
        ACTUAL_DOM_EnumType_PN_SetRouterSettingsResult,
        ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_SetRouterSettingsResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetWLanRadioFrequency",
        ACTUAL_DOM_Method_PN_SetWLanRadioFrequency,
        &s_schema_PN_SetWLanRadioFrequency_Input,
        &s_schema_PN_SetWLanRadioFrequency_Output,
        s_elementPath_PN_SetWLanRadioFrequency_Input,
        s_elementPath_PN_SetWLanRadioFrequency_Output,
        0,
        ACTUAL_DOM_Element_PN_SetWLanRadioFrequencyResult,
        ACTUAL_DOM_EnumType_PN_SetWLanRadioFrequencyResult,
        ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_OK,
        ACTUAL_DOM_Enum_PN_SetWLanRadioFrequencyResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetWLanRadioSecurity",
        ACTUAL_DOM_Method_PN_SetWLanRadioSecurity,
        &s_schema_PN_SetWLanRadioSecurity_Input,
        &s_schema_PN_SetWLanRadioSecurity_Output,
        s_elementPath_PN_SetWLanRadioSecurity_Input,
        s_elementPath_PN_SetWLanRadioSecurity_Output,
        0,
        ACTUAL_DOM_Element_PN_SetWLanRadioSecurityResult,
        ACTUAL_DOM_EnumType_PN_SetWLanRadioSecurityResult,
        ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_OK,
        ACTUAL_DOM_Enum_PN_SetWLanRadioSecurityResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetWLanRadioSettings",
        ACTUAL_DOM_Method_PN_SetWLanRadioSettings,
        &s_schema_PN_SetWLanRadioSettings_Input,
        &s_schema_PN_SetWLanRadioSettings_Output,
        s_elementPath_PN_SetWLanRadioSettings_Input,
        s_elementPath_PN_SetWLanRadioSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_SetWLanRadioSettingsResult,
        ACTUAL_DOM_EnumType_PN_SetWLanRadioSettingsResult,
        ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_SetWLanRadioSettingsResult_REBOOT
    },
    {
        "POST",
        "/HNAP1",
        "http://purenetworks.com/HNAP1/SetWanSettings",
        ACTUAL_DOM_Method_PN_SetWanSettings,
        &s_schema_PN_SetWanSettings_Input,
        &s_schema_PN_SetWanSettings_Output,
        s_elementPath_PN_SetWanSettings_Input,
        s_elementPath_PN_SetWanSettings_Output,
        0,
        ACTUAL_DOM_Element_PN_SetWanSettingsResult,
        ACTUAL_DOM_EnumType_PN_SetWanSettingsResult,
        ACTUAL_DOM_Enum_PN_SetWanSettingsResult_OK,
        ACTUAL_DOM_Enum_PN_SetWanSettingsResult_REBOOT
    },
    HDK_MOD_MethodsEnd
};


/*
 * ADI
 */

static const HDK_XML_SchemaNode s_schemaNodes_ADI[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_ADI, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_AdminPassword, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_AdminPasswordDefault, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_AutoAdjustDST, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_ClientStats, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_ConnectedClients, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 6 */ { 0, ACTUAL_DOM_Element_PN_DHCPIPAddressFirst, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 7 */ { 0, ACTUAL_DOM_Element_PN_DHCPIPAddressLast, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 8 */ { 0, ACTUAL_DOM_Element_PN_DHCPLeaseTime, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 9 */ { 0, ACTUAL_DOM_Element_PN_DHCPReservations, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 10 */ { 0, ACTUAL_DOM_Element_PN_DHCPReservationsSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 11 */ { 0, ACTUAL_DOM_Element_PN_DHCPServerEnabled, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 12 */ { 0, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 13 */ { 0, ACTUAL_DOM_Element_PN_DeviceNetworkStats, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 14 */ { 0, ACTUAL_DOM_Element_PN_DeviceType, ACTUAL_DOM_EnumType_PN_DeviceType, HDK_XML_SchemaNodeProperty_Optional },
    /* 15 */ { 0, ACTUAL_DOM_Element_PN_DomainName, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 16 */ { 0, ACTUAL_DOM_Element_PN_DomainNameChangeSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 17 */ { 0, ACTUAL_DOM_Element_PN_FirmwareDate, HDK_XML_BuiltinType_DateTime, HDK_XML_SchemaNodeProperty_Optional },
    /* 18 */ { 0, ACTUAL_DOM_Element_PN_FirmwareVersion, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 19 */ { 0, ACTUAL_DOM_Element_PN_IsAccessPoint, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 20 */ { 0, ACTUAL_DOM_Element_PN_LanIPAddress, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 21 */ { 0, ACTUAL_DOM_Element_PN_LanSubnetMask, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 22 */ { 0, ACTUAL_DOM_Element_PN_Locale, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 23 */ { 0, ACTUAL_DOM_Element_PN_MFEnabled, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 24 */ { 0, ACTUAL_DOM_Element_PN_MFIsAllowList, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 25 */ { 0, ACTUAL_DOM_Element_PN_MFMACList, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 26 */ { 0, ACTUAL_DOM_Element_PN_ManageOnlyViaSSL, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 27 */ { 0, ACTUAL_DOM_Element_PN_ManageRemote, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 28 */ { 0, ACTUAL_DOM_Element_PN_ManageViaSSLSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 29 */ { 0, ACTUAL_DOM_Element_PN_ManageWireless, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 30 */ { 0, ACTUAL_DOM_Element_PN_ModelDescription, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 31 */ { 0, ACTUAL_DOM_Element_PN_ModelName, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 32 */ { 0, ACTUAL_DOM_Element_PN_ModelRevision, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 33 */ { 0, ACTUAL_DOM_Element_PN_PMDescription, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 34 */ { 0, ACTUAL_DOM_Element_PN_PMExternalPort, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 35 */ { 0, ACTUAL_DOM_Element_PN_PMInternalClient, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 36 */ { 0, ACTUAL_DOM_Element_PN_PMInternalPort, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 37 */ { 0, ACTUAL_DOM_Element_PN_PMProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, HDK_XML_SchemaNodeProperty_Optional },
    /* 38 */ { 0, ACTUAL_DOM_Element_PN_PortMappings, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 39 */ { 0, ACTUAL_DOM_Element_PN_PresentationURL, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 40 */ { 0, ACTUAL_DOM_Element_PN_RemoteManagementSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 41 */ { 0, ACTUAL_DOM_Element_PN_RemotePort, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 42 */ { 0, ACTUAL_DOM_Element_PN_RemoteSSL, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 43 */ { 0, ACTUAL_DOM_Element_PN_RemoteSSLNeedsSSL, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 44 */ { 0, ACTUAL_DOM_Element_PN_SSL, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 45 */ { 0, ACTUAL_DOM_Element_PN_SerialNumber, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 46 */ { 0, ACTUAL_DOM_Element_PN_SubDeviceURLs, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 47 */ { 0, ACTUAL_DOM_Element_PN_SupportedLocales, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 48 */ { 0, ACTUAL_DOM_Element_PN_TaskExtensions, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 49 */ { 0, ACTUAL_DOM_Element_PN_TimeZone, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 50 */ { 0, ACTUAL_DOM_Element_PN_TimeZoneSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 51 */ { 0, ACTUAL_DOM_Element_PN_UpdateMethods, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 52 */ { 0, ACTUAL_DOM_Element_PN_Username, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 53 */ { 0, ACTUAL_DOM_Element_PN_UsernameSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 54 */ { 0, ACTUAL_DOM_Element_PN_VendorName, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 55 */ { 0, ACTUAL_DOM_Element_PN_WLanChannel, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 56 */ { 0, ACTUAL_DOM_Element_PN_WLanChannelWidth, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 57 */ { 0, ACTUAL_DOM_Element_PN_WLanEnabled, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 58 */ { 0, ACTUAL_DOM_Element_PN_WLanEncryption, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 59 */ { 0, ACTUAL_DOM_Element_PN_WLanFrequency, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 60 */ { 0, ACTUAL_DOM_Element_PN_WLanKey, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 61 */ { 0, ACTUAL_DOM_Element_PN_WLanKeyRenewal, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 62 */ { 0, ACTUAL_DOM_Element_PN_WLanMacAddress, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 63 */ { 0, ACTUAL_DOM_Element_PN_WLanMode, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 64 */ { 0, ACTUAL_DOM_Element_PN_WLanQoS, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 65 */ { 0, ACTUAL_DOM_Element_PN_WLanRadioFrequencyInfos, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 66 */ { 0, ACTUAL_DOM_Element_PN_WLanRadioInfos, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 67 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusIP1, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 68 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusIP2, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 69 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusPort1, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 70 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusPort2, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 71 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusSecret1, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 72 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusSecret2, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 73 */ { 0, ACTUAL_DOM_Element_PN_WLanSSID, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 74 */ { 0, ACTUAL_DOM_Element_PN_WLanSSIDBroadcast, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 75 */ { 0, ACTUAL_DOM_Element_PN_WLanSecondaryChannel, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 76 */ { 0, ACTUAL_DOM_Element_PN_WLanSecurityEnabled, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 77 */ { 0, ACTUAL_DOM_Element_PN_WLanType, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 78 */ { 0, ACTUAL_DOM_Element_PN_WPSPin, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 79 */ { 0, ACTUAL_DOM_Element_PN_WanAuthService, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 80 */ { 0, ACTUAL_DOM_Element_PN_WanAutoDetectType, ACTUAL_DOM_EnumType_PN_WANType, HDK_XML_SchemaNodeProperty_Optional },
    /* 81 */ { 0, ACTUAL_DOM_Element_PN_WanAutoMTUSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 82 */ { 0, ACTUAL_DOM_Element_PN_WanAutoReconnect, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 83 */ { 0, ACTUAL_DOM_Element_PN_WanDNS, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 84 */ { 0, ACTUAL_DOM_Element_PN_WanGateway, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 85 */ { 0, ACTUAL_DOM_Element_PN_WanIPAddress, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 86 */ { 0, ACTUAL_DOM_Element_PN_WanLoginService, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 87 */ { 0, ACTUAL_DOM_Element_PN_WanMTU, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 88 */ { 0, ACTUAL_DOM_Element_PN_WanMacAddress, HDK_XML_BuiltinType_MACAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 89 */ { 0, ACTUAL_DOM_Element_PN_WanMaxIdleTime, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 90 */ { 0, ACTUAL_DOM_Element_PN_WanPPPoEService, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 91 */ { 0, ACTUAL_DOM_Element_PN_WanPassword, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 92 */ { 0, ACTUAL_DOM_Element_PN_WanRenewTimeout, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional },
    /* 93 */ { 0, ACTUAL_DOM_Element_PN_WanStatus, ACTUAL_DOM_EnumType_PN_WANStatus, HDK_XML_SchemaNodeProperty_Optional },
    /* 94 */ { 0, ACTUAL_DOM_Element_PN_WanSubnetMask, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 95 */ { 0, ACTUAL_DOM_Element_PN_WanSupportedTypes, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional },
    /* 96 */ { 0, ACTUAL_DOM_Element_PN_WanType, ACTUAL_DOM_EnumType_PN_WANType, HDK_XML_SchemaNodeProperty_Optional },
    /* 97 */ { 0, ACTUAL_DOM_Element_PN_WanUsername, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional },
    /* 98 */ { 0, ACTUAL_DOM_Element_PN_WiredQoS, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 99 */ { 0, ACTUAL_DOM_Element_PN_WiredQoSSupported, HDK_XML_BuiltinType_Bool, HDK_XML_SchemaNodeProperty_Optional },
    /* 100 */ { 4, ACTUAL_DOM_Element_PN_ClientStat, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 101 */ { 5, ACTUAL_DOM_Element_PN_ConnectedClient, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 102 */ { 9, ACTUAL_DOM_Element_PN_DHCPReservation, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 103 */ { 13, ACTUAL_DOM_Element_PN_NetworkStats, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 104 */ { 25, ACTUAL_DOM_Element_PN_MACInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 105 */ { 38, ACTUAL_DOM_Element_PN_PortMapping, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 106 */ { 46, ACTUAL_DOM_Element_PN_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 107 */ { 47, ACTUAL_DOM_Element_PN_string, HDK_XML_BuiltinType_String, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 108 */ { 48, ACTUAL_DOM_Element_PN_TaskExtension, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 109 */ { 51, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_UpdateMethod, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 110 */ { 55, ACTUAL_DOM_Element_PN_WLanChannelInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 111 */ { 56, ACTUAL_DOM_Element_PN_WLanChannelWidthInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 112 */ { 57, ACTUAL_DOM_Element_PN_WLanEnabledInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 113 */ { 58, ACTUAL_DOM_Element_PN_WLanEncryptionInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 114 */ { 59, ACTUAL_DOM_Element_PN_WLanFrequencyInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 115 */ { 60, ACTUAL_DOM_Element_PN_WLanKeyInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 116 */ { 61, ACTUAL_DOM_Element_PN_WLanKeyRenewalInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 117 */ { 62, ACTUAL_DOM_Element_PN_WLanMacAddressInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 118 */ { 63, ACTUAL_DOM_Element_PN_WLanModeInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 119 */ { 64, ACTUAL_DOM_Element_PN_WLanQoSInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 120 */ { 65, ACTUAL_DOM_Element_PN_RadioFrequencyInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 121 */ { 66, ACTUAL_DOM_Element_PN_RadioInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 122 */ { 67, ACTUAL_DOM_Element_PN_WLanRadiusIP1Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 123 */ { 68, ACTUAL_DOM_Element_PN_WLanRadiusIP2Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 124 */ { 69, ACTUAL_DOM_Element_PN_WLanRadiusPort1Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 125 */ { 70, ACTUAL_DOM_Element_PN_WLanRadiusPort2Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 126 */ { 71, ACTUAL_DOM_Element_PN_WLanRadiusSecret1Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 127 */ { 72, ACTUAL_DOM_Element_PN_WLanRadiusSecret2Info, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 128 */ { 73, ACTUAL_DOM_Element_PN_WLanSSIDInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 129 */ { 74, ACTUAL_DOM_Element_PN_WLanSSIDBroadcastInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 130 */ { 75, ACTUAL_DOM_Element_PN_WLanSecondaryChannelInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 131 */ { 76, ACTUAL_DOM_Element_PN_WLanSecurityEnabledInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 132 */ { 77, ACTUAL_DOM_Element_PN_WLanTypeInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 133 */ { 83, ACTUAL_DOM_Element_PN_Primary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 134 */ { 83, ACTUAL_DOM_Element_PN_Secondary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 135 */ { 83, ACTUAL_DOM_Element_PN_Tertiary, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    /* 136 */ { 95, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WANType, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 137 */ { 100, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 138 */ { 100, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 139 */ { 100, ACTUAL_DOM_Element_PN_LinkSpeedIn, HDK_XML_BuiltinType_Int, 0 },
    /* 140 */ { 100, ACTUAL_DOM_Element_PN_LinkSpeedOut, HDK_XML_BuiltinType_Int, 0 },
    /* 141 */ { 100, ACTUAL_DOM_Element_PN_SignalStrength, HDK_XML_BuiltinType_Int, 0 },
    /* 142 */ { 101, ACTUAL_DOM_Element_PN_ConnectTime, HDK_XML_BuiltinType_DateTime, 0 },
    /* 143 */ { 101, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 144 */ { 101, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 145 */ { 101, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 146 */ { 101, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 147 */ { 101, ACTUAL_DOM_Element_PN_Active, HDK_XML_BuiltinType_Bool, 0 },
    /* 148 */ { 102, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 149 */ { 102, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 150 */ { 102, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 151 */ { 103, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 152 */ { 103, ACTUAL_DOM_Element_PN_PacketsReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 153 */ { 103, ACTUAL_DOM_Element_PN_PacketsSent, HDK_XML_BuiltinType_Long, 0 },
    /* 154 */ { 103, ACTUAL_DOM_Element_PN_BytesReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 155 */ { 103, ACTUAL_DOM_Element_PN_BytesSent, HDK_XML_BuiltinType_Long, 0 },
    /* 156 */ { 104, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 157 */ { 104, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 158 */ { 105, ACTUAL_DOM_Element_PN_PortMappingDescription, HDK_XML_BuiltinType_String, 0 },
    /* 159 */ { 105, ACTUAL_DOM_Element_PN_InternalClient, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 160 */ { 105, ACTUAL_DOM_Element_PN_PortMappingProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 },
    /* 161 */ { 105, ACTUAL_DOM_Element_PN_ExternalPort, HDK_XML_BuiltinType_Int, 0 },
    /* 162 */ { 105, ACTUAL_DOM_Element_PN_InternalPort, HDK_XML_BuiltinType_Int, 0 },
    /* 163 */ { 108, ACTUAL_DOM_Element_PN_Name, HDK_XML_BuiltinType_String, 0 },
    /* 164 */ { 108, ACTUAL_DOM_Element_PN_URL, HDK_XML_BuiltinType_String, 0 },
    /* 165 */ { 108, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 },
    /* 166 */ { 110, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 167 */ { 110, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 168 */ { 111, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 169 */ { 111, ACTUAL_DOM_Element_PN_ChannelWidth, HDK_XML_BuiltinType_Int, 0 },
    /* 170 */ { 112, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 171 */ { 112, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 172 */ { 113, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 173 */ { 113, ACTUAL_DOM_Element_PN_Encryption, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 },
    /* 174 */ { 114, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 175 */ { 114, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    /* 176 */ { 115, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 177 */ { 115, ACTUAL_DOM_Element_PN_Key, HDK_XML_BuiltinType_String, 0 },
    /* 178 */ { 116, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 179 */ { 116, ACTUAL_DOM_Element_PN_KeyRenewal, HDK_XML_BuiltinType_Int, 0 },
    /* 180 */ { 117, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 181 */ { 117, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 182 */ { 118, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 183 */ { 118, ACTUAL_DOM_Element_PN_Mode, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 },
    /* 184 */ { 119, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 185 */ { 119, ACTUAL_DOM_Element_PN_QoS, HDK_XML_BuiltinType_Bool, 0 },
    /* 186 */ { 120, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 187 */ { 120, ACTUAL_DOM_Element_PN_Frequencies, HDK_XML_BuiltinType_Struct, 0 },
    /* 188 */ { 121, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 189 */ { 121, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    /* 190 */ { 121, ACTUAL_DOM_Element_PN_SupportedModes, HDK_XML_BuiltinType_Struct, 0 },
    /* 191 */ { 121, ACTUAL_DOM_Element_PN_Channels, HDK_XML_BuiltinType_Struct, 0 },
    /* 192 */ { 121, ACTUAL_DOM_Element_PN_WideChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 193 */ { 121, ACTUAL_DOM_Element_PN_SupportedSecurity, HDK_XML_BuiltinType_Struct, 0 },
    /* 194 */ { 122, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 195 */ { 122, ACTUAL_DOM_Element_PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 196 */ { 123, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 197 */ { 123, ACTUAL_DOM_Element_PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 198 */ { 124, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 199 */ { 124, ACTUAL_DOM_Element_PN_RadiusPort1, HDK_XML_BuiltinType_Int, 0 },
    /* 200 */ { 125, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 201 */ { 125, ACTUAL_DOM_Element_PN_RadiusPort2, HDK_XML_BuiltinType_Int, 0 },
    /* 202 */ { 126, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 203 */ { 126, ACTUAL_DOM_Element_PN_RadiusSecret1, HDK_XML_BuiltinType_String, 0 },
    /* 204 */ { 127, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 205 */ { 127, ACTUAL_DOM_Element_PN_RadiusSecret2, HDK_XML_BuiltinType_String, 0 },
    /* 206 */ { 128, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 207 */ { 128, ACTUAL_DOM_Element_PN_SSID, HDK_XML_BuiltinType_String, 0 },
    /* 208 */ { 129, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 209 */ { 129, ACTUAL_DOM_Element_PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool, 0 },
    /* 210 */ { 130, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 211 */ { 130, ACTUAL_DOM_Element_PN_SecondaryChannel, HDK_XML_BuiltinType_Int, 0 },
    /* 212 */ { 131, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 213 */ { 131, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    /* 214 */ { 132, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 215 */ { 132, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 216 */ { 187, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 217 */ { 190, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiMode, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 218 */ { 191, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 219 */ { 192, ACTUAL_DOM_Element_PN_WideChannel, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 220 */ { 193, ACTUAL_DOM_Element_PN_SecurityInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 221 */ { 219, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 222 */ { 219, ACTUAL_DOM_Element_PN_SecondaryChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 223 */ { 220, ACTUAL_DOM_Element_PN_SecurityType, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 224 */ { 220, ACTUAL_DOM_Element_PN_Encryptions, HDK_XML_BuiltinType_Struct, 0 },
    /* 225 */ { 222, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 226 */ { 224, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiEncryption, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ADI =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ADI,
    s_enumTypes
};


/*
 * Struct http://cisco.com/HNAP/ServiceInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_ServiceInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_ServiceInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_ServiceName, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_ServiceLevel, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_ServiceInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_ServiceInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_ServiceInfo()
{
    return &s_schema_ServiceInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/ClientStat
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_ClientStat[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_ClientStat, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_LinkSpeedIn, HDK_XML_BuiltinType_Int, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_LinkSpeedOut, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_SignalStrength, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_ClientStat =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_ClientStat,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_ClientStat()
{
    return &s_schema_PN_ClientStat;
}


/*
 * Struct http://purenetworks.com/HNAP1/ConnectedClient
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_ConnectedClient[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_ConnectedClient, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_ConnectTime, HDK_XML_BuiltinType_DateTime, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_Wireless, HDK_XML_BuiltinType_Bool, 0 },
    /* 6 */ { 0, ACTUAL_DOM_Element_PN_Active, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_ConnectedClient =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_ConnectedClient,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_ConnectedClient()
{
    return &s_schema_PN_ConnectedClient;
}


/*
 * Struct http://purenetworks.com/HNAP1/DHCPReservation
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_DHCPReservation[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_DHCPReservation, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_IPAddress, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_DHCPReservation =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DHCPReservation,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_DHCPReservation()
{
    return &s_schema_PN_DHCPReservation;
}


/*
 * Struct http://purenetworks.com/HNAP1/DNSSettings
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_DNSSettings[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_DNSSettings, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_Primary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Secondary, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_Tertiary, HDK_XML_BuiltinType_IPAddress, HDK_XML_SchemaNodeProperty_Optional },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_DNSSettings =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_DNSSettings,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_DNSSettings()
{
    return &s_schema_PN_DNSSettings;
}


/*
 * Struct http://purenetworks.com/HNAP1/MACInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_MACInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_MACInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_DeviceName, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_MACInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_MACInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_MACInfo()
{
    return &s_schema_PN_MACInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/NetworkStats
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_NetworkStats[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_NetworkStats, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_PortName, ACTUAL_DOM_EnumType_PN_LANConnection, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_PacketsReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_PacketsSent, HDK_XML_BuiltinType_Long, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_BytesReceived, HDK_XML_BuiltinType_Long, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_BytesSent, HDK_XML_BuiltinType_Long, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_NetworkStats =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_NetworkStats,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_NetworkStats()
{
    return &s_schema_PN_NetworkStats;
}


/*
 * Struct http://purenetworks.com/HNAP1/PortMapping
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_PortMapping[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_PortMapping, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_PortMappingDescription, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_InternalClient, HDK_XML_BuiltinType_IPAddress, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_PortMappingProtocol, ACTUAL_DOM_EnumType_PN_IPProtocol, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_ExternalPort, HDK_XML_BuiltinType_Int, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_InternalPort, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_PortMapping =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_PortMapping,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_PortMapping()
{
    return &s_schema_PN_PortMapping;
}


/*
 * Struct http://purenetworks.com/HNAP1/RadioFrequencyInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_RadioFrequencyInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_RadioFrequencyInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Frequencies, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_RadioFrequencyInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RadioFrequencyInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_RadioFrequencyInfo()
{
    return &s_schema_PN_RadioFrequencyInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/RadioInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_RadioInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_RadioInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_SupportedModes, HDK_XML_BuiltinType_Struct, 0 },
    /* 4 */ { 0, ACTUAL_DOM_Element_PN_Channels, HDK_XML_BuiltinType_Struct, 0 },
    /* 5 */ { 0, ACTUAL_DOM_Element_PN_WideChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 6 */ { 0, ACTUAL_DOM_Element_PN_SupportedSecurity, HDK_XML_BuiltinType_Struct, 0 },
    /* 7 */ { 3, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiMode, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 8 */ { 4, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 9 */ { 5, ACTUAL_DOM_Element_PN_WideChannel, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 10 */ { 6, ACTUAL_DOM_Element_PN_SecurityInfo, HDK_XML_BuiltinType_Struct, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 11 */ { 9, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 12 */ { 9, ACTUAL_DOM_Element_PN_SecondaryChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 13 */ { 10, ACTUAL_DOM_Element_PN_SecurityType, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 14 */ { 10, ACTUAL_DOM_Element_PN_Encryptions, HDK_XML_BuiltinType_Struct, 0 },
    /* 15 */ { 12, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    /* 16 */ { 14, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiEncryption, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_RadioInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_RadioInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_RadioInfo()
{
    return &s_schema_PN_RadioInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/SecurityInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_SecurityInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_SecurityInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_SecurityType, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Encryptions, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_string, ACTUAL_DOM_EnumType_PN_WiFiEncryption, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_SecurityInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_SecurityInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_SecurityInfo()
{
    return &s_schema_PN_SecurityInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/TaskExtension
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_TaskExtension[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_TaskExtension, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_Name, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_URL, HDK_XML_BuiltinType_String, 0 },
    /* 3 */ { 0, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_TaskExtType, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_TaskExtension =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_TaskExtension,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_TaskExtension()
{
    return &s_schema_PN_TaskExtension;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanChannelInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanChannelInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanChannelInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanChannelInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanChannelInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanChannelInfo()
{
    return &s_schema_PN_WLanChannelInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanChannelWidthInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanChannelWidthInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanChannelWidthInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_ChannelWidth, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanChannelWidthInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanChannelWidthInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanChannelWidthInfo()
{
    return &s_schema_PN_WLanChannelWidthInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanEnabledInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanEnabledInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanEnabledInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanEnabledInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanEnabledInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanEnabledInfo()
{
    return &s_schema_PN_WLanEnabledInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanEncryptionInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanEncryptionInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanEncryptionInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Encryption, ACTUAL_DOM_EnumType_PN_WiFiEncryption, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanEncryptionInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanEncryptionInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanEncryptionInfo()
{
    return &s_schema_PN_WLanEncryptionInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanFrequencyInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanFrequencyInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanFrequencyInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Frequency, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanFrequencyInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanFrequencyInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanFrequencyInfo()
{
    return &s_schema_PN_WLanFrequencyInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanKeyInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanKeyInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanKeyInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Key, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanKeyInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanKeyInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanKeyInfo()
{
    return &s_schema_PN_WLanKeyInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanKeyRenewalInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanKeyRenewalInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanKeyRenewalInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_KeyRenewal, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanKeyRenewalInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanKeyRenewalInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanKeyRenewalInfo()
{
    return &s_schema_PN_WLanKeyRenewalInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanMacAddressInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanMacAddressInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanMacAddressInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_MacAddress, HDK_XML_BuiltinType_MACAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanMacAddressInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanMacAddressInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanMacAddressInfo()
{
    return &s_schema_PN_WLanMacAddressInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanModeInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanModeInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanModeInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Mode, ACTUAL_DOM_EnumType_PN_WiFiMode, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanModeInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanModeInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanModeInfo()
{
    return &s_schema_PN_WLanModeInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanQoSInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanQoSInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanQoSInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_QoS, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanQoSInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanQoSInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanQoSInfo()
{
    return &s_schema_PN_WLanQoSInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusIP1Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusIP1Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusIP1Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusIP1, HDK_XML_BuiltinType_IPAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusIP1Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusIP1Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusIP1Info()
{
    return &s_schema_PN_WLanRadiusIP1Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusIP2Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusIP2Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusIP2Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusIP2, HDK_XML_BuiltinType_IPAddress, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusIP2Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusIP2Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusIP2Info()
{
    return &s_schema_PN_WLanRadiusIP2Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusPort1Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusPort1Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusPort1Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusPort1, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusPort1Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusPort1Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusPort1Info()
{
    return &s_schema_PN_WLanRadiusPort1Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusPort2Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusPort2Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusPort2Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusPort2, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusPort2Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusPort2Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusPort2Info()
{
    return &s_schema_PN_WLanRadiusPort2Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusSecret1Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusSecret1Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusSecret1Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusSecret1, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusSecret1Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusSecret1Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusSecret1Info()
{
    return &s_schema_PN_WLanRadiusSecret1Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanRadiusSecret2Info
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanRadiusSecret2Info[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanRadiusSecret2Info, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_RadiusSecret2, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanRadiusSecret2Info =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanRadiusSecret2Info,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanRadiusSecret2Info()
{
    return &s_schema_PN_WLanRadiusSecret2Info;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanSSIDBroadcastInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanSSIDBroadcastInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanSSIDBroadcastInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_SSIDBroadcast, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanSSIDBroadcastInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanSSIDBroadcastInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSSIDBroadcastInfo()
{
    return &s_schema_PN_WLanSSIDBroadcastInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanSSIDInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanSSIDInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanSSIDInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_SSID, HDK_XML_BuiltinType_String, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanSSIDInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanSSIDInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSSIDInfo()
{
    return &s_schema_PN_WLanSSIDInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanSecondaryChannelInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanSecondaryChannelInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanSecondaryChannelInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_SecondaryChannel, HDK_XML_BuiltinType_Int, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanSecondaryChannelInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanSecondaryChannelInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSecondaryChannelInfo()
{
    return &s_schema_PN_WLanSecondaryChannelInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanSecurityEnabledInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanSecurityEnabledInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanSecurityEnabledInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Enabled, HDK_XML_BuiltinType_Bool, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanSecurityEnabledInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanSecurityEnabledInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanSecurityEnabledInfo()
{
    return &s_schema_PN_WLanSecurityEnabledInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WLanTypeInfo
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WLanTypeInfo[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WLanTypeInfo, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_RadioID, HDK_XML_BuiltinType_String, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_Type, ACTUAL_DOM_EnumType_PN_WiFiSecurity, 0 },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WLanTypeInfo =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WLanTypeInfo,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WLanTypeInfo()
{
    return &s_schema_PN_WLanTypeInfo;
}


/*
 * Struct http://purenetworks.com/HNAP1/WideChannel
 */

static const HDK_XML_SchemaNode s_schemaNodes_PN_WideChannel[] =
{
    /* 0 */ { 0, ACTUAL_DOM_Element_PN_WideChannel, HDK_XML_BuiltinType_Struct, 0 },
    /* 1 */ { 0, ACTUAL_DOM_Element_PN_Channel, HDK_XML_BuiltinType_Int, 0 },
    /* 2 */ { 0, ACTUAL_DOM_Element_PN_SecondaryChannels, HDK_XML_BuiltinType_Struct, 0 },
    /* 3 */ { 2, ACTUAL_DOM_Element_PN_int, HDK_XML_BuiltinType_Int, HDK_XML_SchemaNodeProperty_Optional | HDK_XML_SchemaNodeProperty_Unbounded },
    HDK_XML_Schema_SchemaNodesEnd
};

static const HDK_XML_Schema s_schema_PN_WideChannel =
{
    s_namespaces,
    s_elements,
    s_schemaNodes_PN_WideChannel,
    s_enumTypes
};

/* extern */ const HDK_XML_Schema* ACTUAL_DOM_Schema_PN_WideChannel()
{
    return &s_schema_PN_WideChannel;
}


/*
 * Module
 */

/* 96b43d29-069d-41a4-9d00-9d8dd9cfdd32 */
static const HDK_XML_UUID s_uuid_NOID =
{
    { 0x96, 0xb4, 0x3d, 0x29, 0x06, 0x9d, 0x41, 0xa4, 0x9d, 0x00, 0x9d, 0x8d, 0xd9, 0xcf, 0xdd, 0x32 }
};

static const HDK_MOD_Module s_module =
{
    &s_uuid_NOID,
    0,
    0,
    s_methods,
    0,
    &s_schema_ADI
};

const HDK_MOD_Module* ACTUAL_DOM_Module(void)
{
    return &s_module;
}

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
