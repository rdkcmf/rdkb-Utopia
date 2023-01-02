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

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/ipv6_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
source /etc/utopia/service.d/sysevent_functions.sh

# This handler is called not only to start/stop/restart the service
# but also when WAN or LAN status is updated as well as current_wan_ifname
# and when DHCPv6 has received a reply or a time-out

#------------------------------------------------------------------
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="dhcpv6_client"

DHCPV6_BINARY=/sbin/ti_dhcp6c
DHCPV6_CONF_FILE=/etc/dhcp6c.conf

DHCPV6_PID_FILE=/var/run/erouter_dhcp6c.pid

service_init ()
{
   # First some SYSCFG
   eval `utctx_cmd get last_erouter_mode dhcpv6c_enable ipv6_static_enable lan_ipv6addr wan_ipv6addr dhcpv6c_duid lan_ifname`
   LAN_INTERFACE_NAME=$SYSCFG_lan_ifname

   if [ -z "$SYSCFG_ipv6_static_enable" ]
   then
        SYSCFG_ipv6_static_enable=0
   fi

   # The more information from SYSEVENT
   WAN_INTERFACE_NAME=`sysevent get wan_ifname`
   LAN_STATE=`sysevent get lan-status`
   WAN_STATE=`sysevent get wan-status`
   PHY_WAN_STATE=`sysevent get current_ipv4_link_state`
   IPV6_STATE=`sysevent get ipv6-status`

   DHCPV6C_ENABLED=`sysevent get dhcpv6c_enabled`
   WAN_LINK_STATUS=`sysevent get phylink_wan_state`
   BRIDGE_MODE=`sysevent get bridge_mode`
}

service_start()
{
   if [ "$SYSCFG_last_erouter_mode" != "2" -a "$SYSCFG_last_erouter_mode" != "3" ]
   then
      # Non IPv6 Mode
      service_stop
   elif [ "$WAN_LINK_STATUS" = "down" ]
   then
      # WAN LINK is Down
      service_stop
   elif [ -z "$WAN_INTERFACE_NAME" ]
   then
      # WAN Interface not configured
      service_stop
   elif [ "$BRIDGE_MODE" = "1" ]
   then
      service_stop
   elif [ ! -f $DHCPV6_PID_FILE ]
   then
      mkdir -p /tmp/.dibbler-info
      ti_dhcp6c -i "$WAN_INTERFACE_NAME" -p $DHCPV6_PID_FILE -plugin /fss/gw/lib/libgw_dhcp6plg.so
   fi
}

service_stop()
{
   if [ -f $DHCPV6_PID_FILE ]
   then
      kill "`cat $DHCPV6_PID_FILE`"
      rm -f $DHCPV6_PID_FILE
   fi
}

service_update()
{
   if [ "$DHCPV6C_ENABLED" = "1" ]
   then
      service_start
   else
      service_stop
   fi
}

register_dhcpv6_client_handler()
{
   register_sysevent_handler $SERVICE_NAME erouter_mode-updated /etc/utopia/service.d/service_dhcpv6_client.sh
   register_sysevent_handler $SERVICE_NAME phylink_wan_state /etc/utopia/service.d/service_dhcpv6_client.sh
   register_sysevent_handler $SERVICE_NAME current_wan_ifname /etc/utopia/service.d/service_dhcpv6_client.sh
   register_sysevent_handler $SERVICE_NAME bridge_mode /etc/utopia/service.d/service_dhcpv6_client.sh
}

unregister_dhcpv6_client_handler()
{
   unregister_sysevent_handler $SERVICE_NAME erouter_mode-updated
   unregister_sysevent_handler $SERVICE_NAME phylink_wan_state
   unregister_sysevent_handler $SERVICE_NAME current_wan_ifname
   unregister_sysevent_handler $SERVICE_NAME bridge_mode
}

service_enable ()
{
   if [ "$DHCPV6C_ENABLED" = "1" ]
   then
      ulog dhcpv6c status "DHCPv6 Client is already enabled"
      return
   fi

   sysevent set dhcpv6c_enabled 1
   #register_dhcpv6_client_handler
   DHCPV6C_ENABLED=1

   service_start
}

service_disable ()
{
   if [ ! "$DHCPV6C_ENABLED" = "1" ]
   then
      ulog dhcpv6c status "DHCPv6 Client is not enabled"
      return
   fi

   sysevent set dhcpv6c_enabled 0
   #unregister_dhcpv6_client_handler
   DHCPV6C_ENABLED=0

   service_stop
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

service_init 

case "$1" in
   "${SERVICE_NAME}-start")
      service_enable
      ;;
   "${SERVICE_NAME}-stop")
      service_disable
      ;;
   "${SERVICE_NAME}-restart")
      service_disable
      service_enable
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   erouter_mode-updated|phylink_wan_state|lan-status|wan-status|ipv6-status|current_ipv4_link_state|current_wan_ifname|bridge_mode)
      service_update
      ;;


   *)
      echo "Usage: $SERVICE_NAME enable | disable" > /dev/console
      exit 3
      ;;
esac
