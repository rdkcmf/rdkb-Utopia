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
source /etc/utopia/service.d/log_capture_path.sh
. /etc/device.properties
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

DIBBLER_ENABLED=`syscfg get dibbler_client_enable`

if ([ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ]) && [[ "$DIBBLER_ENABLED" != "true" ]] ;then
	DHCPV6_BINARY=/sbin/ti_dhcp6c
        DHCPV6_PID_FILE=/var/run/erouter_dhcp6c.pid
else
	DHCPV6_BINARY=dibbler-client
	DHCPV6_PID_FILE=/tmp/dibbler/client.pid
fi
DHCPV6_CONF_FILE=/etc/dhcp6c.conf

DHCPV6_REGISTER_FILE=/tmp/dhcpv6_registered_events
DHCP6C_PROGRESS_FILE=/tmp/dhcpv6c_inprogress

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
   mkdir -p /var/log/dibbler
   echo_t "SERVICE_DHCP6C : SERVICE START"
   if [ "$SYSCFG_last_erouter_mode" != "2" -a "$SYSCFG_last_erouter_mode" != "3" ]
   then
      # Non IPv6 Mode
     echo_t "SERVICE_DHCP6C : Non IPv6 Mode, service_stop"
      service_stop
   elif [ "$WAN_LINK_STATUS" = "down" ]
   then
      # WAN LINK is Down
     echo_t "SERVICE_DHCP6C : WAN LINK is Down, service_stop"
      service_stop
   elif [ "$WAN_INTERFACE_NAME" = "" ]
   then
      # WAN Interface not configured
     echo_t "SERVICE_DHCP6C : WAN Interface not configured, service_stop"
      service_stop
   elif [ "$BRIDGE_MODE" = "1" ]
   then
     echo_t "SERVICE_DHCP6C : BridgeMode, service_stop"
      service_stop
   elif [ "$WAN_STATE" = "stopped" ]
   then
        echo_t "SERVICE_DHCP6C : WAN state stopped, service_stop"
        service_stop
   elif [ ! -f $DHCPV6_PID_FILE ]
   then
   		mkdir -p /tmp/.dibbler-info
	  	if [ ! -f $DHCP6C_PROGRESS_FILE ]
		then
			touch $DHCP6C_PROGRESS_FILE
			echo_t "SERVICE_DHCP6C : Starting DHCPv6 Client from service_dhcpv6_client"
                        if ([ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ]) && [[ "$DIBBLER_ENABLED" != "true" ]] ;then
        			ti_dhcp6c -i $WAN_INTERFACE_NAME -p $DHCPV6_PID_FILE -plugin /fss/gw/lib/libgw_dhcp6plg.so
				echo_t "SERVICE_DHCP6C : dhcp6c PID is `cat $DHCPV6_PID_FILE`"
			else
				echo_t "SERVICE_DHCP6C : Starting dibbler client"
				sh /etc/dibbler/dibbler-init.sh
				$DHCPV6_BINARY start
			fi
			rm -f $DHCP6C_PROGRESS_FILE

		else
			echo "SERVICE_DHCP6C : ti_dhcp6c process start in progress, not starting one more"
		fi	
   fi
}

service_stop()
{
   echo_t "SERVICE_DHCP6C : SERVICE STOP"
   if ([ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ]) && [[ "$DIBBLER_ENABLED" != "true" ]] ;then
   	if [ -f $DHCPV6_PID_FILE ]
   	then
           # We need to make sure the erouter0 interface is UP when the DHCPv6 client process plan to send
           # the RELEASE message. Otherwise it will wait to send the message and get messed when another
           # DHCPv6 client process plan to start in service_start().
           EROUTER0_STATUS=`ip -d link show erouter0 | grep state | awk '/erouter0/{print $9}'`
           if [ "$EROUTER0_STATUS" != "UP" ]
           then
              ip link set erouter0 up
              sleep 1
           fi
  	   echo_t "SERVICE_DHCP6C : Killing `cat $DHCPV6_PID_FILE`"
    	   kill -9 `cat $DHCPV6_PID_FILE`
   	   rm -f $DHCPV6_PID_FILE
           # After stop the DHCPv6 client, need to clear the sysevent tr_erouter0_dhcpv6_client_v6addr
           # So that it can be triggered again once DHCPv6 client got the same IPv6 address with the old address
           sysevent set tr_erouter0_dhcpv6_client_v6addr
 	fi
    else
		$DHCPV6_BINARY stop
                rm -f $DHCPV6_PID_FILE
    fi
}

service_update()
{
   echo_t "SERVICE_DHCP6C : SERVICE UPDATE, DHCPV6C_ENABLED is $DHCPV6C_ENABLED"
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
#   register_sysevent_handler $SERVICE_NAME wan-status /etc/utopia/service.d/service_dhcpv6_client.sh
   touch $DHCPV6_REGISTER_FILE
}

unregister_dhcpv6_client_handler()
{
   unregister_sysevent_handler $SERVICE_NAME erouter_mode-updated
   unregister_sysevent_handler $SERVICE_NAME phylink_wan_state
   unregister_sysevent_handler $SERVICE_NAME current_wan_ifname
   unregister_sysevent_handler $SERVICE_NAME bridge_mode
#   unregister_sysevent_handler $SERVICE_NAME wan-status
   rm -f $DHCPV6_REGISTER_FILE
}

service_enable ()
{
   echo_t "SERVICE_DHCP6C : SERVICE ENABLE"
   if [ "$DHCPV6C_ENABLED" = "1" ]
   then
      ulog dhcpv6c status "DHCPv6 Client is already enabled"
      echo_t "SERVICE_DHCP6C : DHCPv6 Client is already enabled"
      if [ ! -f $DHCPV6_REGISTER_FILE ]; then
          echo "DHCPv6 Client is enabled but events are not registered, registering it now"
          register_dhcpv6_client_handler    
      else
          echo "DHCPv6 Client is enabled and events are registered"
      fi
      return
   fi

   service_start
   sysevent set dhcpv6c_enabled 1
   register_dhcpv6_client_handler
   DHCPV6C_ENABLED=1
}

service_disable ()
{
   echo_t "SERVICE_DHCP6C : SERVICE DISABLE, DHCPV6C_ENABLED is $DHCPV6C_ENABLED"
   if [ ! "$DHCPV6C_ENABLED" = "1" ]
   then
      ulog dhcpv6c status "DHCPv6 Client is not enabled"
      echo_t "SERVICE_DHCP6C : DHCPv6 Client is not enabled"
      return
   fi

   sysevent set dhcpv6c_enabled 0
   unregister_dhcpv6_client_handler
   DHCPV6C_ENABLED=0
   
   if [ -f $DHCP6C_PROGRESS_FILE ]	
   then	
       echo "Removing file:$DHCP6C_PROGRESS_FILE"
	   rm -f $DHCP6C_PROGRESS_FILE	
   fi
			
   service_stop
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------
echo_t "SERVICE_DHCP6C : service_dhcpv6_client 1st param is $1, 2nd param is $2"
service_init 

case "$1" in
   enable)	
      service_enable
      ;;
   disable)
      service_disable
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   erouter_mode-updated|phylink_wan_state|lan-status|ipv6-status|current_ipv4_link_state|current_wan_ifname|bridge_mode)
      service_update
      ;;

   wan-status)
	if [ "$2" = "stopped" ] ||  [ "$2" = "started" ]
  	 then
		service_update
        fi
	;;

   *)
      echo "Usage: $SERVICE_NAME enable | disable" > /dev/console
      exit 3
      ;;
esac
