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
source /etc/device.properties

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

# This handler is called not only to start/stop/restart the service
# but also when LAN status is updated
# and when stateless information is changed (like DNS server)

#------------------------------------------------------------------
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="dhcpv6_server"

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


ulog dhcpv6s status "$$: got EVENT=$EVENT$VALUE"

#------------------------------------------------------------------
# make the dhcpv6 server's configuration file
#------------------------------------------------------------------
prepare_dhcpv6s_config() {
   CONFIG_EMPTY=yes
   LOCAL_DHCPV6_CONF_FILE=/tmp/dhcp6s.conf$$
   rm -f $LOCAL_DHCPV6_CONF_FILE

   # Let's generate the configuration file now
   echo "# Automatically generated on `date` by $SELF for $EVENT" > $LOCAL_DHCPV6_CONF_FILE
  
   # The IPv6 DNS servers are this router and any DNS servers received through any other means
   if [ ! -z "`sysevent get current_lan_ipv6address`" -o ! -z "`sysevent get ipv6_nameserver`" ]
   then
      echo "option domain-name-servers `sysevent get current_lan_ipv6address` `sysevent get ipv6_nameserver` ;" >> $LOCAL_DHCPV6_CONF_FILE
      CONFIG_EMPTY=no
   fi

   # The search domain names are the union of the domains received through DHCPv6 and DHCPv6 (doublons will probably occur...)
   if [ ! -z "`sysevent get ipv6_domain`" -o ! -z "`sysevent get dhcp_domain`" ]
   then
      echo "option domain-name \"`sysevent get ipv6_domain` `sysevent get dhcp_domain`\" ;" >> $LOCAL_DHCPV6_CONF_FILE
      CONFIG_EMPTY=no
   fi
  
   # The NTP server
   if [ ! -z "`sysevent get ipv6_ntp_server`" ]
   then
      echo "option ntp-servers `sysevent get ipv6_ntp_server` ;" >> $LOCAL_DHCPV6_CONF_FILE
      CONFIG_EMPTY=no
   fi

   cat $LOCAL_DHCPV6_CONF_FILE > "$DHCPV6_CONF_FILE"
   rm -f $LOCAL_DHCPV6_CONF_FILE
}

#---------------------------------------------------------------
# Restore the dhcpv6 server DUID (interface identifier) from NVRAM
#--------------------------------------------------------------
restore_dhcpv6s_duid() {
   if [ ! -s /var/run/dhcp6s_duid ] 
   then
	if [ -n "$SYSCFG_dhcpv6s_duid" ] 
        then
		echo -n "$SYSCFG_dhcpv6s_duid" > /var/run/dhcp6s_duid
		touch /var/run/dhcp6s_duid_saved
#   		echo "[utopia] dhcpv6 server DUID restored as $SYSCFG_dhcpv6s_duid" > /dev/console
   		echo "$SELF: dhcpv6 server DUID restored as $SYSCFG_dhcpv6s_duid" >> $LOG
	fi
   fi
}

#----------------------------------------------------------------------------------------
#                     Default Event Handlers
#
# Each service has three default events that it should handle
#    ${SERVICE_NAME}-start
#    ${SERVICE_NAME}-stop
#    ${SERVICE_NAME}-restart
#
# For each case:
#   - Clear the service's errinfo
#   - Set the service's status 
#   - Do the work
#   - Check the error code (check_err will set service's status and service's errinfo upon error)
#   - If no error then set the service's status
#----------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
#  function   : service_init
#  - optional procedure to retrieve syscfg configuration data using utctx_cmd
#    this is a protected way of accessing syscfg
#-------------------------------------------------------------------------------------------
service_init ()
{
   # First some SYSCFG
   eval `utctx_cmd get dhcpv6s_duid lan_ifname dhcpv6s_enable`
   LAN_INTERFACE_NAME=$SYSCFG_lan_ifname

   # The more information from SYSEVENT
   LAN_STATE=`sysevent get lan-status`

   DHCPV6_BINARY=/sbin/dhcp6s
   DHCPV6_CONF_FILE=/etc/dhcp6s.conf
   DHCPV6_PID_FILE=/var/run/dhcp6s.pid
}

#-------------------------------------------------------------------------------------------
#  function   : service_start
#  - Set service-status to starting
#  - Add code to read normalized configuration data from syscfg and/or sysevent 
#  - Create configuration files for the service
#  - Start any service processes 
#  - Set service-status to started
#
#  check_err will check for a non zero return code of the last called function
#  set the service-status to error, and set the service-errinfo, and then exit
#-------------------------------------------------------------------------------------------
service_start ()
{
   # Start the DHCP server only when it is enabled of course
   if [ -z "$SYSCFG_dhcpv6s_enable" ] || [ "$SYSCFG_dhcpv6s_enable" = "0" ]
   then
	echo "$SELF: DHCPv6 server cannot start because it is disabled" >> $LOG
      	sysevent set ${SERVICE_NAME}-errinfo "Cannot start: disabled"
	return
   fi

   # wait_till_end_state will wait a reasonable amount of time waiting for ${SERVICE_NAME}
   # to finish transitional states (stopping | starting)
   wait_till_end_state ${SERVICE_NAME}

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" ] 
   then

      # Start the DHCP server only when LAN is up obviously 
      if [ "$LAN_STATE" != "started" ]
      then
	echo "$SELF: DHCPv6 server cannot start LAN=$LAN_STATE" >> $LOG
      	sysevent set ${SERVICE_NAME}-errinfo "Cannot start LAN=$LAN_STATE"
      	sysevent set ${SERVICE_NAME}-status stopped
	return
      fi

      sysevent set ${SERVICE_NAME}-errinfo 
      sysevent set ${SERVICE_NAME}-status starting

#      echo "[utopia] Starting dhcpv6 server on LAN ($LAN_INTERFACE_NAME) event=$EVENT" > /dev/console
      echo "$SELF: Starting dhcpv6 server on LAN ($LAN_INTERFACE_NAME) event=$EVENT" >> $LOG

      prepare_dhcpv6s_config
      restore_dhcpv6s_duid
      if [ "$CONFIG_EMPTY" = "no" ]
      then
	      # dhcp6s [-c configfile] [-dDfi] [-p pid-file] interface [interfaces...]
	      $DHCPV6_BINARY -P $DHCPV6_PID_FILE "$LAN_INTERFACE_NAME" >> $LOG 2>&1

	      check_err $? "Couldnt handle start"
	      sysevent set ${SERVICE_NAME}-status started
      else
	      sysevent set ${SERVICE_NAME}-errinfo "Nothing to announce with DHCPv6"
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

      # Save the server DUID in NVRAM if not yet done
      if [ ! -f /var/run/dhcp6s_duid_saved -a -f /var/run/dhcp6s_duid ] 
      then
           syscfg set dhcpv6s_duid "`cat /var/run/dhcp6s_duid`" >> $LOG 2>&1
           syscfg commit >> $LOG 2>&1
           echo "$SELF: Saving the DHCPv6 server DUID in NVRAM" >> $LOG
           touch /var/run/dhcp6s_duid_saved
      fi
      
#      echo "[utopia] Stopping DHCPv6 Server (LAN state=$LAN_STATE, event=$EVENT) in $SELF" > /dev/console
      echo "$SELF: Stopping DHCPv6 Server (LAN state=$LAN_STATE, event=$EVENT)" >> $LOG
      killall dhcp6s > /dev/null 2>&1
      sysevent set ${SERVICE_NAME}-status stopped
   fi
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

service_init 

case "$1" in
   "${SERVICE_NAME}-start")
if [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]; then
      service_ipv6 start
else
      service_ipv6 dhcpv6s-start
fi
      ;;
   "${SERVICE_NAME}-stop")
if [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]; then
      service_ipv6 stop
else
      service_ipv6 dhcpv6s-stop
fi
      ;;
   "${SERVICE_NAME}-restart")
if [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]; then
      service_ipv6 restart
else
      service_ipv6 dhcpv6s-restart
fi
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

#   ipv6_nameserver|ipv6_dnssl)
#      service_ipv6 dhcpv6s-restart
   dhcpv6_option_changed)
if [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]; then
      service_ipv6 restart
else
      service_ipv6 dhcpv6s-restart
fi
      ;;

   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
