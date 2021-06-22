#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################


#HHS nonvol validation for DPC3939
#Tightly coupled with defaults file. Must match.
#To be replaced with PSM enhancements.

CHECK=`psmcli get dmsb.l2net.5.Members.WiFi`

if [ xath5 != x"$CHECK" ]; then
    exit 0
fi

#Failed validation. Repair with defaults

#remove wifi from bridge 5
dmcli eRT setv Device.Bridging.Bridge.5.Port.2.LowerLayers string ""

#change port 2 to ath5
dmcli eRT setv Device.Bridging.Bridge.4.Port.2.LowerLayers string "Device.WiFi.SSID.6"

dmcli eRT setv Device.Bridging.Bridge.4.Port.2.X_CISCO_COM_Mode string "Tagging"

#Enable bridge 4
dmcli eRT setv Device.Bridging.Bridge.4.Enable bool true

#Reload PSM defaults
psmcli set dmsb.hotspot.tunnel.1.ReconnectToPrimaryRemoteEndpoint 43200
psmcli set dmsb.hotspot.tunnel.1.RemoteEndpointHealthCheckPingInterval 60
psmcli set dmsb.hotspot.tunnel.1.RemoteEndpointHealthCheckPingFailThreshold 3
#zqiu: default should be  300 sec
psmcli set dmsb.hotspot.tunnel.1.RemoteEndpointHealthCheckPingIntervalInFailure 300
