#!/bin/sh

#HHS nonvol validation for DPC3939
#Tightly coupled with defaults file. Must match.
#To be replaced with PSM enhancements.

CHECK=`psmcli get dmsb.l2net.5.Members.WiFi`

if [ xath5 != x$CHECK ]; then
    exit 0
fi

#Failed validation. Repair with defaults

#remove wifi from bridge 5
ccsp_bus_client_tool eRT setv Device.Bridging.Bridge.5.Port.2.LowerLayers string ""

#change port 2 to ath5
ccsp_bus_client_tool eRT setv Device.Bridging.Bridge.4.Port.2.LowerLayers string "Device.WiFi.SSID.6"

ccsp_bus_client_tool eRT setv Device.Bridging.Bridge.4.Port.2.X_CISCO_COM_Mode string "Tagging"

#Enable bridge 4
ccsp_bus_client_tool eRT setv Device.Bridging.Bridge.4.Enable bool true

#Reload PSM defaults
psmcli set dmsb.hotspot.gre.1.ReconnPrimary 43200
psmcli set dmsb.hotspot.gre.1.KeepAlive.Interval 60
psmcli set dmsb.hotspot.gre.1.KeepAlive.Threshold 3
psmcli set dmsb.hotspot.gre.1.KeepAlive.FailInterval 2000