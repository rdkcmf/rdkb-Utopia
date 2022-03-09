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

DHCPV6_BINARY=/sbin/dhcp6c
DHCPV6_CONF_FILE=/etc/dhcp6c.conf
DHCPV6_PID_FILE=/var/run/dhcp6c.pid
DHCPV6_EXECUTE_UPON_COMPLETION=$0
# for development purposes let syscfg verbose ipv6 be a signal for logging
if [ "ipv6" = "`syscfg get verbose`" ] 
then
   LOG=/var/log/ipv6.log
else
   LOG=/dev/null
fi
SELF="$0[$$]"
EVENT=$1
if [ ! -z "$EVENT" ]
then
	VALUE=" (=`sysevent get "$EVENT"`)"
fi

ulog dhcpv6c status "$$: got EVENT=$EVENT$VALUE REASON=$REASON"

#------------------------------------------------------------------
# make the dhcpv6 client's configuration file
#------------------------------------------------------------------
prepare_dhcpv6c_config() {
   WAN_IF_NAME=$1
   LOCAL_DHCPV6_CONF_FILE=/tmp/dhcp6c.conf$$
   rm -f $LOCAL_DHCPV6_CONF_FILE

   # Check whether IA and PD needs to be enabled
   NEED_PD=0
   if [ "$SYSCFG_ipv6_static_enable" = "0" -o -z "$SYSCFG_lan_ipv6addr" ] # No need for DHCP-PD when LAN is statically defined
   then
	if [ "$SYSCFG_dhcpv6c_enable" = "1" -o "$SYSCFG_dhcpv6c_enable" = "3" ]
	then
		NEED_PD=1
	fi
   fi
   NEED_IA=0
   if [ "$SYSCFG_ipv6_static_enable" = "0" -o -z "$SYSCFG_wan_ipv6addr" ] # No need for NA when WAN is statically defined
   then
	if [ "$SYSCFG_dhcpv6c_enable" = "2" -o "$SYSCFG_dhcpv6c_enable" = "3" ]
	then
		NEED_IA=1
	fi
   fi

   # external interface where the DHCP requests are sent
   echo "interface $WAN_IF_NAME {" > $LOCAL_DHCPV6_CONF_FILE
   if [ "$NEED_PD" = "1" ]
   then
     echo "     send ia-pd 123;" >> $LOCAL_DHCPV6_CONF_FILE
   fi
   if [ "$NEED_IA" = "1" ]
   then
     echo "     send ia-na 456;" >> $LOCAL_DHCPV6_CONF_FILE
   fi
   echo "     request domain-name-servers ;" >> $LOCAL_DHCPV6_CONF_FILE
   echo "     request domain-name ;" >> $LOCAL_DHCPV6_CONF_FILE
   echo "     request ntp-servers ;" >> $LOCAL_DHCPV6_CONF_FILE
   echo "     script \"$DHCPV6_EXECUTE_UPON_COMPLETION\" ;" >> $LOCAL_DHCPV6_CONF_FILE
   echo "};" >> $LOCAL_DHCPV6_CONF_FILE

   if [ "$NEED_PD" = "1" ]
   then
     # For prefix-delegation, only one interface on the LAN side: br0
     echo "id-assoc pd 123 {" >> $LOCAL_DHCPV6_CONF_FILE
     echo "     prefix-interface br0 {" >> $LOCAL_DHCPV6_CONF_FILE
     echo "         sla-id 1;" >> $LOCAL_DHCPV6_CONF_FILE
     echo "         sla-len 2 ;" >> $LOCAL_DHCPV6_CONF_FILE
     echo "     };" >> $LOCAL_DHCPV6_CONF_FILE

     # For prefix-delegation, also configure interface lo for remote management
     echo "     prefix-interface lo {" >> $LOCAL_DHCPV6_CONF_FILE
     echo "         sla-id 0;" >> $LOCAL_DHCPV6_CONF_FILE
     echo "         sla-len 2 ;" >> $LOCAL_DHCPV6_CONF_FILE
     echo "     };" >> $LOCAL_DHCPV6_CONF_FILE
     echo "};" >> $LOCAL_DHCPV6_CONF_FILE
   fi

   if [ "$NEED_IA" = "1" ]
   then
     # For WAN non temporary address
     echo "id-assoc na 456 {" >> $LOCAL_DHCPV6_CONF_FILE
     echo "};" >> $LOCAL_DHCPV6_CONF_FILE
   fi

   cat $LOCAL_DHCPV6_CONF_FILE > $DHCPV6_CONF_FILE
   rm -f $LOCAL_DHCPV6_CONF_FILE
}

#---------------------------------------------------------------
# Restore the dhcpv6 client DUID (interface identifier) from NVRAM
#--------------------------------------------------------------
restore_dhcpv6c_duid() {
   if [ ! -s /var/run/dhcpv6c_duid ] 
   then
	if [ -n "$SYSCFG_dhcpv6c_duid" ] 
        then
		echo -n "$SYSCFG_dhcpv6c_duid" > /var/run/dhcp6c_duid
		touch /var/run/dhcp6c_duid_saved
#   		echo "[utopia] dhcpv6 client DUID restored as $SYSCFG_dhcpv6c_duid" > /dev/console
   		echo "$SELF: dhcpv6 client DUID restored as $SYSCFG_dhcpv6c_duid" >> $LOG
	fi
   fi
}

service_init ()
{
   # First some SYSCFG
   eval "`utctx_cmd get dhcpv6c_enable ipv6_static_enable lan_ipv6addr wan_ipv6addr dhcpv6c_duid lan_ifname`"
   LAN_INTERFACE_NAME=$SYSCFG_lan_ifname

   if [ -z "$SYSCFG_ipv6_static_enable" ]
   then
        SYSCFG_ipv6_static_enable=0
   fi

   # The more information from SYSEVENT
   WAN_INTERFACE_NAME=`sysevent get current_wan_ifname`
   LAN_STATE=`sysevent get lan-status`
   WAN_STATE=`sysevent get wan-status`
   PHY_WAN_STATE=`sysevent get current_ipv4_link_state`
   IPV6_STATE=`sysevent get ipv6-status`
}

service_start ()
{
   # Start DHCP only when enabled ;-)
   if [ -z "$SYSCFG_dhcpv6c_enable" ] || [ "$SYSCFG_dhcpv6c_enable" = "0" ] 
   then
      ulog dhcpv6c status "DHCPv6 client is disabled"
      echo "$SELF: DHCPv6 client is disabled" >> $LOG
      return
   fi

   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" ] 
   then

      # Start the DHCP client only when WAN is up obviously and when LAN is up (so that LAN interface can be configured)
      if [ "$PHY_WAN_STATE" != "up" -o "$LAN_STATE" != "started" -o "$IPV6_STATE" != "started" ]
      then
	echo "$SELF: DHCPv6 client cannot start PHY_WAN=$PHY_WAN_STATE or LAN=$LAN_STATE or IPV6=$IPV6_STATE" >> $LOG
      	sysevent set ${SERVICE_NAME}-errinfo "Cannot start PHY_WAN=$PHY_WAN_STATE or LAN=$LAN_STATE or IPV6=$IPV6_STATE"
      	sysevent set ${SERVICE_NAME}-status stopped
	return
      fi

      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status starting

#      echo "[utopia] Starting dhcpv6 client on WAN ($WAN_INTERFACE_NAME) event=$EVENT" > /dev/console
      echo "$SELF: Starting dhcpv6 client on WAN ($WAN_INTERFACE_NAME) event=$EVENT" >> $LOG

      prepare_dhcpv6c_config "$WAN_INTERFACE_NAME"
      restore_dhcpv6c_duid
      if [ "$NEED_IA" = "1" -o "$NEED_PD" = "1" ]
      then
	      # dhcp6c [-c configfile] [-dDfi] [-p pid-file] interface [interfaces...]
	      $DHCPV6_BINARY -p $DHCPV6_PID_FILE "$WAN_INTERFACE_NAME" >> $LOG 2>&1

	      check_err $? "Couldnt handle start"
	      sysevent set ${SERVICE_NAME}-status started
      else
	      sysevent set ${SERVICE_NAME}-errinfo "No need to start DHCPv6 Client on the WAN side"
	      sysevent set ${SERVICE_NAME}-status stopped
      fi
   fi
}

#-------------------------------------------------------------------------------------------
#  function   : service_stop
#  - Set service-status to stopping
#  - Stop any service processes 
#  - Delete configuration files for the service
#  - Set service-status to stopped
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_stop ()
{
   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "stopped" != "$STATUS" ] 
   then
      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status stopping

      # Save the client DUID in NVRAM if not yet done
      if [ ! -f /var/run/dhcp6c_duid_saved -a -f /var/run/dhcp6c_duid ] 
      then
           syscfg set dhcp6c_duid "`cat /var/run/dhcp6c_duid`" >> $LOG 2>&1
           syscfg commit >> $LOG 2>&1
           echo "$SELF: Saving the DHCPv6 Client DUID in NVRAM" >> $LOG
           touch /var/run/dhcp6c_duid_saved
      fi

#      echo "[utopia] Stopping DHCPv6 Client (LAN state=$LAN_STATE, PHY_WAN state=$PHY_WAN_STATE, event=$EVENT) in $SELF" > /dev/console
      echo "$SELF: Stopping DHCPv6 Client (LAN state=$LAN_STATE, PHY_WAN state=$PHY_WAN_STATE, event=$EVENT)" >> $LOG
      killall dhcp6c > /dev/null 2>&1
      
      sysevent set ${SERVICE_NAME}-status stopped
   fi
}

#-------------------------------------------------------------------------------------------
# Process a call back with stateless information
#-------------------------------------------------------------------------------------------
call_back_nbi ()
{
	# /etc/resolv.conf locations in the /tmp area
	RESOLV_CONF='/tmp/resolv.conf'
	RESOLV_CONF_TEMP='/tmp/resolv.conf.temp'

	echo "$SELF: call back REASON=$REASON" >> $LOG
	if [ -n "$new_domain_name_servers" ] 
	then
		# Set a sysevent
		sysevent set ipv6_nameserver "$new_domain_name_servers"
		# Update the local /etc/resolv.conf
		egrep -v '^nameserver .*:.*' $RESOLV_CONF > $RESOLV_CONF_TEMP
		for server in $new_domain_name_servers
		do
			echo "nameserver $server" >> $RESOLV_CONF_TEMP
		done
		rm -f $RESOLV_CONF
		mv $RESOLV_CONF_TEMP $RESOLV_CONF
	fi
	if [ -n "$new_domain_name" ] 
	then
		# Set a sysevent
		sysevent set ipv6_domain "$new_domain_name"
		# Update the local /etc/resolv.conf
		egrep -v '^search' $RESOLV_CONF > $RESOLV_CONF_TEMP
		echo "search $new_domain_name" >> $RESOLV_CONF_TEMP
		rm -f $RESOLV_CONF
		mv $RESOLV_CONF_TEMP $RESOLV_CONF
	fi
	if [ -n "$new_ntp_servers" ] 
	then
		# Set a sysevent
		sysevent set ipv6_ntp_server "$new_ntp_servers"
	fi
	# Last step: save the WAN IPv6 interface name
	# Cannot save the IPv6 address because not yet set by DHCPv6 Client :-(
	sysevent set current_wan_ipv6_interface "$WAN_INTERFACE_NAME"
}


#-------------------------------------------------------------------------------------------
# Process a call back with statefull information: adding a prefix
#-------------------------------------------------------------------------------------------
call_back_add_prefix ()
{
	if [ "$interface" = "$LAN_INTERFACE_NAME" ]
	then
#		echo "[Utopia] Got an IPv6 prefix from DHCP for LAN" > /dev/console
		echo "$SELF: Got an IPv6 prefix from DHCP for LAN" >> $LOG
		# set the ipv6 prefix for router advertisement on the LAN side
		save_lan_ipv6_prefix "$prefix"/"$prefix_length"
		echo "$SELF: sysevent set dhcpv6_client-status add_prefix" >> $LOG
		sysevent set dhcpv6_client-status add_prefix
	else	
		# Special for loopback interface add $prefix::1
		if [ "$interface" = "lo" ]
		then
#			echo "[Utopia] Got an IPv6 prefix from DHCP for lo" > /dev/console
			echo "$SELF: Got an IPv6 prefix from DHCP for lo" >> $LOG
			echo "$SELF: ip -6 addr add ${prefix}1/$prefix_length dev $interface valid_lft $valid_lifetime preferred_lft $preferred_lifetime" >> $LOG 2>&1
			ip -6 addr add "${prefix}"1/"$prefix_length" dev "$interface" valid_lft "$valid_lifetime" preferred_lft "$preferred_lifetime" >> $LOG 2>&1
		else
			echo "$SELF: Unexpected interface ($interface) when adding a prefix $prefix/$prefix_length" >> $LOG
		fi
	fi
}


#-------------------------------------------------------------------------------------------
# Process a call back with statefull information: removing a prefix
#-------------------------------------------------------------------------------------------
call_back_remove_prefix ()
{
	if [ "$interface" = "$LAN_INTERFACE_NAME" ]
	then
		echo "$SELF: Remove an IPv6 prefix for LAN" >> $LOG
		# clear the ipv6 prefix for router advertisement on the LAN side
		save_lan_ipv6_prefix ipv6_prefix
		# TODO No need for DHCP server on the LAN side if there is no prefix!
		sysevent set dhcpv6_client-status remove_prefix
		
		# Last step, remove the status sysevent
		sysevent set current_lan_ipv6address
	else	
		# Special for loopback interface add $prefix::1
		if [ "$interface" = "lo" ]
		then
			echo "$SELF: ip -6 addr del ${prefix}1/$prefix_length dev $interface"  >> $LOG 2>&1
			ip -6 addr del "${prefix}"1/"$prefix_length" dev "$interface"  >> $LOG 2>&1
		else
			echo "$SELF: Unexpected interface ($interface) when removing a prefix" >> $LOG
		fi
	fi
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

service_init 

case "$1" in
   "${SERVICE_NAME}-start")
      service_start
      ;;
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      service_stop
      service_start
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   lan-status|wan-status|ipv6-status|current_ipv4_link_state|current_wan_ifname)
      service_stop
      service_start
      ;;

   *)
      echo "$SELF: call-back REASON=$REASON interface=$interface state=$state preferred_lifetime=$preferred_lifetime valid_lifetime=$valid_lifetime" >> $LOG
      # Need to check whether the script was called back by the client
      if [ "$REASON" = "NBI" ]
      then
	call_back_nbi
      elif [ "$REASON" = "add prefix" ] 
      then
	call_back_add_prefix
      elif [ "$REASON" = "remove prefix" ]
      then
  	call_back_remove_prefix
      else
	echo "$SELF: unknown event ($EVENT) or unknow DHCP call back ($REASON)" >> $LOG
	echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
	exit 3
      fi
      ;;
esac
