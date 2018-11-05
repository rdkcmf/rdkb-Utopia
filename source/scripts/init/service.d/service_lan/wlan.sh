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

#--------------------------------------------------------------
# configure a wireless interface
#
# input parameter $1 is wlan interface name (eth1, eth2, etc.)
#                 $2 is wlan parameter prefix (wl0, wl1, etc.)
#
# Note that on Broadcom chip only eth2 is capable of dual band
#    eth1 -> wl0 -> 2.4GHz
#    eth2 -> wl1 -> 5GHz
#--------------------------------------------------------------
bringup_wireless_daemons () {
   WL0_ENABLED=`syscfg get wl0_state`
   WL1_ENABLED=`syscfg get wl1_state`

   if [ "up" = "$WL0_ENABLED" ] || [ "up" = "$WL1_ENABLED" ] ; then
       ulog lan status "bringing up wireless daemons"
       eapd
       nas 
       wps_monitor &

       # set the WPS HW button
       echo "trig edge single rising 16" > /proc/cns3xxx/gpio
   fi

   sysevent set wlan-status started
}

teardown_wireless_daemons () {
   ulog lan status "killing wireless daemons"
   killall -q wps_monitor
   killall -q eapd
   killall -q nas

   # turn off the WiFi LED
   echo set value 17 1 > /proc/cns3xxx/gpio

   sysevent set wlan-status stopped
}

