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
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]; then
	. /etc/device.properties
fi

SERVICE_NAME="ntpd"
SELF_NAME="`basename $0`"
NTP_CONF=/etc/ntp.conf
NTP_CONF_TMP=/tmp/ntp.conf
NTP_CONF_QUICK_SYNC=/tmp/ntp_quick_sync.conf
BIN=ntpd

erouter_wait () 
{
    EROUTER_IP=""
    retry=0
    MAX_RETRY=20
    while [ ! "$EROUTER_IP" ]
    do
       retry=`expr $retry + 1`
	   EROUTER_IP=`ifconfig $NTPD_INTERFACE | grep inet6 | grep Global | awk '/inet6/{print $3}' | cut -d '/' -f1`

       if [ ! -z "$EROUTER_IP" ]; then
          break
       fi
       sleep 6
       if [ $retry -eq $MAX_RETRY ];then
          echo "EROUTER IP not acquired after max etries. Exiting !!!"
          break
       fi
    done
	
	echo $EROUTER_IP
}

service_start ()
{

    # this needs to be hooked up to syscfg for specific timezone
   if [ -n "$SYSCFG_ntp_enabled" ] && [ "0" = "$SYSCFG_ntp_enabled" ] ; then
# Setting Time status as disabled
      syscfg set ntp_status 1
      sysevent set ${SERVICE_NAME}-status "stopped"
      if [ "x`pidof $BIN`" = "x" ]
      then
		if [ "x$MULTI_CORE" = "xyes" ]
		then
			echo "NTPD is not running, starting in Server mode"
			cp $NTP_CONF $NTP_CONF_TMP
			echo "interface ignore wildcard" >> $NTP_CONF_TMP
			echo "interface listen $HOST_INTERFACE_IP" >> $NTP_CONF_TMP

			if [ "x$BOX_TYPE" = "xXB3" ]; then
				ntpd -c $NTP_CONF_TMP -l /rdklogs/logs/ntpLog.log
			else
				systemctl start ntpd.service
			fi
		fi
      fi
      return 0

   fi

# Setting Time status as Unsynchronized
   syscfg set ntp_status 2

   if [ "started" != "$CURRENT_WAN_STATUS" ] ; then
# Setting Time status as Error_FailedToSynchronize
      syscfg set ntp_status 5
      sysevent set ${SERVICE_NAME}-status "wan-down"
       return 0
   fi

   if [ "x$SYSCFG_ntp_server1" = "x" ] || [ "x$SYSCFG_ntp_server1" = x"no_ntp_address" ]; then
	if [ "x$PARTNER_ID" = "x" ]; then
		echo "SERVICE_NTPD : PARTNER_ID is null, using the default ntp server."
		SYSCFG_ntp_server1="ntp.ccp.xcal.tv"
	else
		echo "NTP SERVER not available, not starting ntpd"
		return 0
	fi 

   fi

       rm -rf $NTP_CONF_TMP
	SYSCFG_ntp_server1=`echo $SYSCFG_ntp_server1 | cut -d ":" -f 1`
# Create a config
	echo "SERVICE_NTPD : Creating NTP config"
	if [ -f "/nvram/ETHWAN_ENABLE" ];then
	echo "server $SYSCFG_ntp_server1 true" >> $NTP_CONF_TMP
	echo "server time1.google.com" >> $NTP_CONF_TMP # adding open source NTP server URL for fallback case.
	echo "server time2.google.com" >> $NTP_CONF_TMP # adding open source NTP server URL for fallback case.
	echo "server time3.google.com" >> $NTP_CONF_TMP # adding open source NTP server URL for fallback case.
    else
	echo "server $SYSCFG_ntp_server1" >> $NTP_CONF_TMP
	fi
	if [ "x$SOURCE_PING_INTF" == "x" ]
	then
		MASK=ifconfig $SOURCE_PING_INTF | sed -rn '2s/ .*:(.*)$/\1/p'
	else
		MASK="255.255.255.0"
	fi
	
	WAN_IP=""
	
	if [ "$NTPD_INTERFACE" == "erouter0" ]; then
		sleep 30
		WAN_IP=$(erouter_wait)
	else
		PROVISIONED_TYPE=""
		PROVISIONED_TYPE=$(dmcli eRT getv Device.X_CISCO_COM_CableModem.ProvIpType | grep value | awk '/value/{print $5}')
		
		if [ "$PROVISIONED_TYPE" == "IPV4" ]; then
			WAN_IP=`ifconfig -a $NTPD_INTERFACE | grep inet | grep -v inet6 | tr -s " " | cut -d ":" -f2 | cut -d " " -f1`
		else
			WAN_IP=`ifconfig $NTPD_INTERFACE | grep inet6 | grep Global | awk '/inet6/{print $3}' | cut -d '/' -f1`
		fi
	fi

	echo "restrict $PEER_INTERFACE_IP mask $MASK nomodify notrap" >> $NTP_CONF_TMP
	echo "restrict default kod nomodify notrap nopeer noquery" >> $NTP_CONF_TMP
	echo "restrict -6 default kod nomodify notrap nopeer noquery" >> $NTP_CONF_TMP
	echo "restrict 127.0.0.1" >> $NTP_CONF_TMP
	echo "restrict -6 ::1" >> $NTP_CONF_TMP
	if [ -f "/nvram/ETHWAN_ENABLE" ];then
	cp $NTP_CONF_TMP $NTP_CONF_QUICK_SYNC
	fi
	echo "interface ignore wildcard" >> $NTP_CONF_TMP

   	if [ "$WAN_IP" != "" ]
        then
		echo "interface listen $WAN_IP" >> $NTP_CONF_TMP
        fi   

    if [ "x$MULTI_CORE" = "xyes" ]
    then
	   echo "interface listen $HOST_INTERFACE_IP" >> $NTP_CONF_TMP
    fi

    echo "SERVICE_NTPD : Starting NTP daemon"
	if [ "x$BOX_TYPE" = "xXB3" ]; then
    	kill -9 `pidof $BIN` > /dev/null 2>&1
		ntpd -c $NTP_CONF_TMP -l /rdklogs/logs/ntpLog.log -g
	else
		systemctl stop ntpd.service
		if [ -f "/nvram/ETHWAN_ENABLE" ];then
		ntpd -c $NTP_CONF_QUICK_SYNC -x -gq
		fi
		systemctl start ntpd.service
		systemctl stop systemd-timesyncd
	fi		

    ret_val=$?
    if [ "$ret_val" -ne 0 ]
    then
       echo "SERVICE_NTPD : NTP failed to start, retrying"
	   if [ "x$BOX_TYPE" = "xXB3" ]; then
	   	ntpd -c $NTP_CONF_TMP -l /rdklogs/logs/ntpLog.log -g
	   else
		
		if [ -f "/nvram/ETHWAN_ENABLE" ];then
		ntpd -c $NTP_CONF_QUICK_SYNC -x -gq
		fi
	   	systemctl start ntpd.service
	   fi
    fi

   echo "ntpd started , setting the status as started"
   sysevent set ${SERVICE_NAME}-status "started"
# Setting Time status as synchronized
   syscfg set ntp_status 3


}

service_stop ()
{
   if [ "x$BOX_TYPE" = "xXB3" ]; then
   	kill -9 `pidof $BIN` > /dev/null 2>&1
   else
   	systemctl stop ntpd.service
   fi
   sysevent set ${SERVICE_NAME}-status "stopped"
}


service_init ()
{
    FOO=`utctx_cmd get ntp_server1 ntp_enabled`
    eval $FOO
}


# Entry

service_init

CURRENT_WAN_STATUS=`sysevent get wan-status`
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
  wan-status)
      if [ "started" = "$CURRENT_WAN_STATUS" ] ; then
         service_start
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status ]" >&2
      exit 3
      ;;
esac

