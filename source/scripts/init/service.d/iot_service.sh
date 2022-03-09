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

source /etc/utopia/service.d/log_capture_path.sh
source /lib/rdk/t2Shared_api.sh
iot_ipaddress=`syscfg get iot_ipaddr`
iot_interface=`syscfg get iot_ifname`
if [ "$iot_interface" == "l2sd0.106" ]; then
 iot_interface=`syscfg get iot_brname`
fi
iot_mask=`syscfg get iot_netmask`
VID="106"

# Function to extract first 3 octets of IP
subnet() {
    if [ "$2" ]; then
        NM="$2"
    else
        NM="248.0.0.0"
    fi
    if [ "$1" ]; then
        IP="$1"
    else
        IP="255.253.252.100"
    fi
    #
    n="${NM%.*}";m="${NM##*.}"
    l="${IP%.*}";r="${IP##*.}";c=""
    if [ "$m" = "0" ]; then
        c=".0"
        m="${n##*.}";n="${n%.*}"
        r="${l##*.}";l="${l%.*}"
        if [ "$m" = "0" ]; then
            c=".0$c"
            m="${n##*.}";n="${n%.*}"
            r="${l##*.}";l="${l%.*}"
            if [ "$m" = "0" ]; then
                c=".0$c"
                m=$n
                r=$l;l=""
            fi
        fi
    fi
    let s=256-$m
    let r=$r/$s*$s
    if [ "$l" ]; then
        SNW="$l.$r$c"
    else
        SNW="$r$c"
    fi

    echo $SNW
}

# Function to restart DHCP server and firewall
restartServices()
{
  echo_t "IOT_LOG : Restart services"

  sysevent set dhcp_server-restart lan_not_restart

  echo_t "iotservice : Triggering RDKB_FIREWALL_RESTART"
  t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
  sysevent set firewall-restart
}

echo_t "IOT_LOG : iot_service received $1"

mask=`subnet "$iot_ipaddress" "$iot_mask"`

if [ "$1" = "up" ]
then
   echo_t "IOT_LOG : Add ip rules, ip routes and restart services"
   ip rule add from "$iot_ipaddress" lookup $VID
   ip rule add from all iif "$iot_interface" lookup erouter
   ip rule add from all iif "$iot_interface" lookup $VID
   ip route add table $VID "$mask"/24 dev "$iot_interface"
   restartServices
   echo_t "IOT_LOG : Completed ip rules, ip routes and restart services"
elif [ "$1" = "down" ]
then
   echo_t "IOT_LOG : Remove ip rules, ip routes and restart services"
   ip rule del from "$iot_ipaddress" lookup $VID
   ip rule del from all iif "$iot_interface" lookup erouter
   ip rule del from all iif "$iot_interface" lookup $VID
   ip route del table $VID "$mask"/24 dev "$iot_interface"

   ifconfig "$iot_interface" down
   vconfig rem "$iot_interface"

   restartServices
   echo_t "IOT_LOG : Completed removing ip rules, ip routes and restart services"
elif [ "$1" = "bootup" ]
then
   #No need to call restart services as during bootup all configuration will be done
   isIotEnabled=`syscfg get lost_and_found_enable`
   echo_t "IOT_LOG : IOT enabled is : $isIotEnabled"   
   if [ "$isIotEnabled" = "true" ]
   then
      echo_t "IOT_LOG : Add ip rules, ip routes and restart services"
      ip rule add from "$iot_ipaddress" lookup $VID
      ip rule add from all iif "$iot_interface" lookup erouter
      ip rule add from all iif "$iot_interface" lookup $VID
      ip route add table $VID "$mask"/24 dev "$iot_interface"
      echo_t "IOT_LOG : Completed ip rules, ip routes and restart services"
   fi
fi

