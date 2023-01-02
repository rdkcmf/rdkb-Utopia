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

#------------------------------------------------------------------
#  Setup 5GHz backhaul using WDS for 610N v2 
#
# This script is used to set up WDS link 
# $1 is the calling event such as (wds-restart, bridge-status,lan-status)
#
# syscfg variables
#   1) wifi_bridge_mode    --- 0 (disabled, default), 1 (wds enabled), 2 (sta enabled, future use)
#   2) wifi_bridge_bssid   --- mac address of the other router
#   3) wifi_bridge_ssid    --- ssid of the other router (future use, don't set)
#   4) wifi_bridge_chan    --- future use, don't set
#
# Assumptions
#   Both wifi radios should use exactly the same wireless settings like,
#   mode (N-only), channel (36). Don't use any "Auto" selection, instead
#   select a specific setting
#
# IMPORTANT:
#   1) Only of the APs SHOULD run in bridge mode enabled
#   2) wireless security should be disabled. It should left as open
#      WDS will not work if WPA2, WPA or WEP security is enabled
#   3) Due to above restriction and to ensure security, no other 
#      clients can attach to 5GHz radio. MAC filtering is implicity
#      enabled on this radio and only the partner AP is whitelisted
#              
#------------------------------------------------------------------
source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="wds"
SELF_NAME="`basename $0`"

service_init ()
{
    eval `utctx_cmd get wifi_bridge_mode wifi_bridge_ssid wifi_bridge_bssid wifi_bridge_chan`
}

#
# Get mac address of other side AP for WDS link
# Parse scanresults and extract BSSID associated with Lego SSID
#
get_wds_mac() 
{
   WDS_MAC=""

   if [ -n "$SYSCFG_wifi_bridge_ssid" ] ; then
       ulog wds info "searching for partner AP ssid \"$SYSCFG_wifi_bridge_ssid\""
       wl -i eth1 scan -s $SYSCFG_wifi_bridge_ssid
       sleep 1
       WDS_MAC=`wl -i eth1 scanresults | egrep -e '^BSSID' | head -n 1 | awk '{print $2}'`
       if [ -n "$WDS_MAC" ] ; then
           ulog wds info "found for partner AP bssid $WDS_MAC for ssid \"$SYSCFG_wifi_bridge_ssid\""
       else
           ulog wds info "couldn't find partner AP ssid \"$SYSCFG_wifi_bridge_ssid\""
       fi
   fi

   if [ ! -n "$WDS_MAC" ] ; then
       if [ -n "$SYSCFG_wifi_bridge_bssid" ] ; then
           ulog wds info "using configured BSSID instead - \"$SYSCFG_wifi_bridge_bssid\""
           WDS_MAC=$SYSCFG_wifi_bridge_bssid
       fi
   fi
}

setup_wds() 
{
  ulog wds info "setting up wds" 

  get_wds_mac

  if [ ! -n "$WDS_MAC" ] ; then
      ulog wds info "partner AP BSSID not available, please check if its up, else configure its bssid"
      return 0;
  fi

  CHANNEL=$SYSCFG_wifi_bridge_chan
  if [ -z "$CHANNEL" ]; then
      CHANNEL=36
  fi

  #
  # set eth1 down first
  #
  ulog wds info "bring down eth1"
  wl -i eth1 down

  #
  # disable lazywds
  # set up timeout
  # match channel to be same on WDS link
  #
  # ulog wds info "setting up nvram"
  # nvram set wl1_mode=ap
  # nvram set wl1_wds=$WDS_MAC
  # nvram set wl1_lazywds=0
  # nvram set wl1_channel=$CHANNEL
  # nvram set wl1_wds_timeout=10
  # nvram set wl1_macmode=allow
  # nvram set wl1_maclist=$WDS_MAC
  # nvram commit

  ulog wds info "configuring wds"
  wl -i eth1 ap 1
  wl -i eth1 wds clear
  wl -i eth1 wds $WDS_MAC
  wl -i eth1 lazywds 0
  wl -i eth1 channel $CHANNEL
  wl -i eth1 macmode 2
  wl -i eth1 mac $WDS_MAC
  nvram set wl1_wds_timeout=10

  #
  # bring wds interface up and enslave into brlan0 LAN
  #
  wl -i eth1 up
  ulog wds info "bring up eth1 - status[$?]"
  sleep 1
  ifconfig wds1.1 up
  ulog wds info "ifconfig wds1.1 - status[$?]"
  sleep 1
  brctl addif brlan0 wds1.1
  ulog wds info "enslave wds1.1 to brlan0 - status[$?]"
}

service_start()
{
   if [ 1 -ne "$SYSCFG_wifi_bridge_mode" ] ; then
       return 0
   fi

   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   LAN_STATUS=`sysevent get lan-status`
   BRIDGE_STATUS=`sysevent get bridge-status`
   if [ "started" = "$LAN_STATUS" ] || [ "started" = "$BRIDGE_STATUS" ] ; then
       setup_wds
   else 
       ulog ${SERVICE_NAME} status "either bridge or lan status should be started to bring up wds" 
       return 0
   fi

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   brctl delif brlan0 wds1.1
   ulog wds info "remove wds1.1 to brlan0 - status[$?]"
   sleep 1

   ifconfig wds1.1 down
   ulog wds info "ifconfig wds1.1 down - status[$?]"
   sleep 1

   wl -i eth1 wds clear
   ulog wds info "clear wds setting - status[$?]"

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_lan_bridge_status  ()
{
   LAN_STATUS=`sysevent get lan-status`
   BRIDGE_STATUS=`sysevent get bridge-status`
   if [ "started" = "$LAN_STATUS" ] || [ "started" = "$BRIDGE_STATUS" ] ; then
       service_start
   else 
       ulog ${SERVICE_NAME} status "either bridge or lan status should be started to bring up wds" 
       return 0
   fi
}

# Entry

service_init

case "$1" in
  ${SERVICE_NAME}-start)
      service_start
      ;;
  ${SERVICE_NAME}-stop)
      service_stop
      ;;
  ${SERVICE_NAME}-restart)
      service_stop
      service_start
      ;;
  lan-status)
      service_lan_bridge_status
      ;;
  bridge-status)
      service_lan_bridge_status
      ;;
  *)
	echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-restart | ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop ]" > /dev/console
      exit 3
      ;;
esac

