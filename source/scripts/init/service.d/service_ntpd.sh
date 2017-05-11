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
# Copyright (c) 2008, 2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

#------------------------------------------------------------------
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]; then
	. /etc/device.properties
fi

SERVICE_NAME="ntpd"
SELF_NAME="`basename $0`"
CM_INTERFACE="wan0"
CONF_FILE=/tmp/ntp.conf
BIN=ntpd
service_start ()
{

    # this needs to be hooked up to syscfg for specific timezone
   if [ -n "$SYSCFG_ntp_enabled" ] && [ "0" = "$SYSCFG_ntp_enabled" ] ; then
# Setting Time status as disabled
      syscfg set ntp_status 1
      sysevent set ${SERVICE_NAME}-status "stopped"
      if [ "x`pidof $BIN`" = "x" ]
      then
		echo "NTPD is not running, starting in Server mode"
		ntpd -I $SOURCE_PING_INTF
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

       rm -rf $CONF_FILE
	SYSCFG_ntp_server1=`echo $SYSCFG_ntp_server1 | cut -d ":" -f 1`
# Create a config
	echo "SERVICE_NTPD : Creating NTP config"
	echo "server $SYSCFG_ntp_server1" >> $CONF_FILE
    
	if [ "x$SOURCE_PING_INTF" == "x" ]
	then
		MASK=ifconfig $SOURCE_PING_INTF | sed -rn '2s/ .*:(.*)$/\1/p'
	else
		MASK="255.255.255.0"
	fi
	echo "restrict $ATOM_INTERFACE_IP mask $MASK nomodify notrap" >> $CONF_FILE
	echo "restrict default kod nomodify notrap nopeer noquery" >> $CONF_FILE
	echo "restrict -6 default kod nomodify notrap nopeer noquery" >> $CONF_FILE
	echo "restrict 127.0.0.1" >> $CONF_FILE
	echo "restrict -6 ::1" >> $CONF_FILE

    kill -9 `pidof $BIN` > /dev/null 2>&1
    if [ "x$BOX_TYPE" = "xXB3" ]
    then
	   echo "SERVICE_NTPD : Starting NTP daemon"
	   ntpd -c $CONF_FILE -I $CM_INTERFACE -I $SOURCE_PING_INTF  -g
    else
	   echo "SERVICE_NTPD : Starting NTP daemon"
	   ntpd -c $CONF_FILE -I $CM_INTERFACE -g
    fi

    ret_val=$?
    if [ "$ret_val" -ne 0 ]
    then
	echo "SERVICE_NTPD : NTP failed to start, retrying"
	    if [ "x$BOX_TYPE" = "xXB3" ]
	    then
		   ntpd -c $CONF_FILE -I $CM_INTERFACE -I $SOURCE_PING_INTF  -g
	    else
		   ntpd -c $CONF_FILE -I $CM_INTERFACE -g
	    fi
        ret_val=$?
    fi

   echo "ntpd started , setting the status as started"
   sysevent set ${SERVICE_NAME}-status "started"
# Setting Time status as synchronized
   syscfg set ntp_status 3


}

service_stop ()
{
   kill -9 `pidof $BIN` > /dev/null 2>&1
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

