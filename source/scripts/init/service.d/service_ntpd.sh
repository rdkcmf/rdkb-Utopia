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
source /etc/log_timestamp.sh    # define 'echo_t' ASAP!
source /etc/waninfo.sh

if [ -f /etc/device.properties ]; then
	. /etc/device.properties
fi

SERVICE_NAME="ntpd"
SELF_NAME="`basename "$0"`"
NTP_CONF=/etc/ntp.conf
NTP_CONF_TMP=/tmp/ntp.conf
NTP_CONF_QUICK_SYNC=/tmp/ntp_quick_sync.conf
LOCKFILE=/var/tmp/service_ntpd.pid
BIN=ntpd
EROUTER_IPV6_UP=0
QUICK_SYNC_PID=""
QUICK_SYNC_DONE=0

WAN_INTERFACE=$(getWanInterfaceName)

if [ "x$NTPD_LOG_NAME" == "x" ];then
NTPD_LOG_NAME=/rdklogs/logs/ntpLog.log
fi

erouter_wait () 
{
    local EROUTER_UP=""
    local EROUTER_IPv4=""
    local EROUTER_IPv6=""
    retry=0
    MAX_RETRY=20

    while [ ! "$EROUTER_UP" ]
    do
       retry=`expr $retry + 1`

       #Make sure erouter0 has an IPv4 or IPv6 address before telling NTP to listen on Interface
       EROUTER_IPv4=`ifconfig -a "$NTPD_INTERFACE" | grep inet | grep -v inet6 | tr -s " " | cut -d ":" -f2 | cut -d " " -f1 | head -n1`

       if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ]; then
           CURRENT_WAN_IPV6_STATUS=`sysevent get ipv6_connection_state`
           if [ "xup" = "x$CURRENT_WAN_IPV6_STATUS" ] ; then
               EROUTER_IPv6=`ifconfig "$NTPD_IPV6_INTERFACE" | grep inet6 | grep Global | awk '/inet6/{print $3}' | grep -v 'fdd7' | cut -d '/' -f1 | head -n1`
               EROUTER_IPV6_UP=1
           fi
       else
           EROUTER_IPv6=`ifconfig "$NTPD_INTERFACE" | grep inet6 | grep Global | awk '/inet6/{print $3}' | cut -d '/' -f1 | head -n1`
       fi

       if [ -n "$EROUTER_IPv4" ] || [ -n "$EROUTER_IPv6" ]; then
          if [ "x$2" = "xquickSync" ];then
          	# Quick Sync Needs an IP and Not Interface As changes in Interface as Quick Sync Runs Causes Errors
          	if [ "x$EROUTER_IPv6" != "x" ];then
          		EROUTER_UP=$EROUTER_IPv6
          	else
          		EROUTER_UP=$EROUTER_IPv4
          	fi
          else
          	EROUTER_UP=$WAN_INTERFACE
          fi
          break
       fi
       sleep 6
       WAN_INTERFACE=$(getWanInterfaceName)
       if [ $retry -eq $MAX_RETRY ];then
          echo_t "SERVICE_NTPD : EROUTER IP not acquired after max etries. Exiting !!!" >> $NTPD_LOG_NAME
          break
       fi
    done

 eval $1=\$EROUTER_UP
}

set_ntp_quicksync_status ()
{
    ### -gq the expectation is ntpd exits once it successfully sets the clock, which may happen as quick as 10 sec
    ### reap the exit code of ntpd quick sync,set the ntp status accordingly..
    ### in problematic case,wait for 120 seconds,then break off.
    retry_timeout=1
    while true
    do
       if [ "$retry_timeout" -gt "24" ]; then
          echo_t "NTP quick sync not succeeded,retry exceeded" >> $NTPD_LOG_NAME
          break
       fi
       if [ ! -d "/proc/$QUICK_SYNC_PID" ]; then
          wait $QUICK_SYNC_PID
          ntpd_exit_code=$?
          if [ "$ntpd_exit_code" -eq 0 ]; then
             echo_t "NTP quick sync succeeded,set ntp status" >> $NTPD_LOG_NAME
             syscfg set ntp_status 3
             #Set FirstUseDate in Syscfg if this is the first time we are doing a successful NTP Sych
             DEVICEFIRSTUSEDATE=`syscfg get device_first_use_date`
             if [ "" = "$DEVICEFIRSTUSEDATE" ] || [ "0" = "$DEVICEFIRSTUSEDATE" ]; then
                FIRSTUSEDATE=`date +%Y-%m-%dT%H:%M:%S`
                syscfg set device_first_use_date "$FIRSTUSEDATE"
             fi
             sysevent set ntp_time_sync 1
             QUICK_SYNC_DONE=1
             break
          fi
       else
         retry_timeout=`expr $retry_timeout + 1`
         sleep 5
       fi
    done
}

set_ntp_driftsync_status ()
{
  ####QUICK SYNC not succeeded,lets wait for ntpd daemon to adjust the clock
  ####this may slow set compared to ntpd -gq, ntpd takes some ntp poll intervals(def 64s)
  ####our local clock,stratum will be 16,until synchronised with the server.
  ####in worst case if we are not synced with server, break the loop after 20 mins
  #### Note we can't wait more than 300 secs in sysevent context, this supposed to be run
  #### in background
   if [ "x$(which ntpq)" != "x" ];then
      retry=1
      while true
      do
        sync_status=`ntpq -c rv | grep "stratum=16"`
        if [ "x$sync_status" = "x" ]; then
           echo_t "SERVICE_NTPD : ntpd time synced , setting the status" >> $NTPD_LOG_NAME
           syscfg set ntp_status 3
           sysevent set ntp_time_sync 1
           #Set FirstUseDate in Syscfg if this is the first time we are doing a successful NTP Sych
           DEVICEFIRSTUSEDATE=`syscfg get device_first_use_date`
           if [ "" = "$DEVICEFIRSTUSEDATE" ] || [ "0" = "$DEVICEFIRSTUSEDATE" ]; then
              FIRSTUSEDATE=`date +%Y-%m-%dT%H:%M:%S`
              syscfg set device_first_use_date "$FIRSTUSEDATE"
           fi
           break
        elif [ "$retry" -gt "20" ]; then
             echo_t "Time is not synced after 20 min retry. Breaking loop" >> $NTPD_LOG_NAME
             break
        else
             echo_t "SERVICE_NTPD : Time not yet synced, Sleeping. Retry:$retry" >> $NTPD_LOG_NAME
             retry=`expr $retry + 1`
             sleep 60
        fi
      done
   else
      echo_t "SERVICE_NTPD : ntpq not available,unable to check sync status" >> $NTPD_LOG_NAME
   fi
   exit
}

service_start ()
{

    # this needs to be hooked up to syscfg for specific timezone
   if [ -n "$SYSCFG_ntp_enabled" ] && [ "0" = "$SYSCFG_ntp_enabled" ] ; then
# RDKB-37275 setting status as unsynchronised.
      syscfg set ntp_status 2
      sysevent set ${SERVICE_NAME}-status "stopped"
      if [ "x`pidof $BIN`" = "x" ]; then
          if [ "x$MULTI_CORE" = "xyes" ] && [ "x$NTPD_IMMED_PEER_SYNC" != "xtrue" ]; then
             echo_t "SERVICE_NTPD : NTPD is not running, starting in Server mode" >> $NTPD_LOG_NAME
             cp $NTP_CONF $NTP_CONF_TMP
             echo "interface ignore wildcard" >> $NTP_CONF_TMP
             echo "interface listen $HOST_INTERFACE_IP" >> $NTP_CONF_TMP

             if [ "x$BOX_TYPE" = "xXB3" ]; then
                ntpd -c $NTP_CONF_TMP -l $NTPD_LOG_NAME
             else
                systemctl start ntpd.service
             fi
          fi
      fi

      return 0
   fi

# Setting Time status as Unsynchronized
   syscfg set ntp_status 2

   if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ] || [ "x$BOX_TYPE" = "xWNXL11BWL" ]; then
       WAN_IPV6_STATUS=`sysevent get ipv6_connection_state`
       if [ "xstarted" != "x$CURRENT_WAN_STATUS" ] && [ "xup" != "x$WAN_IPV6_STATUS" ] ; then
           syscfg set ntp_status 2
           sysevent set ${SERVICE_NAME}-status "wan-down"
           return 0
       fi
   else
       if [ "started" != "$CURRENT_WAN_STATUS" ] ; then
# Setting Time status as unsynchronised,as per RDKB-37275
           syscfg set ntp_status 2
           sysevent set ${SERVICE_NAME}-status "wan-down"
           return 0
       fi
   fi

   rm -rf $NTP_CONF_TMP $NTP_CONF_QUICK_SYNC

   # Add Initial Interface Security Rules
   echo "restrict default kod nomodify notrap nopeer noquery" >> $NTP_CONF_TMP
   echo "restrict -6 default kod nomodify notrap nopeer noquery" >> $NTP_CONF_TMP
   echo "restrict 127.0.0.1" >> $NTP_CONF_TMP
   echo "restrict -6 ::1" >> $NTP_CONF_TMP

   if [ "$SYSCFG_new_ntp_enabled" = "true" ]; then
       # Start NTP Config Creation with Multiple Server Setup
       echo_t "SERVICE_NTPD : Creating NTP config with New NTP Enabled" >> $NTPD_LOG_NAME
       if [ "x$SYSCFG_ntp_server1" != "x" ] && [ "x$SYSCFG_ntp_server1" != "xno_ntp_address" ]; then
           echo "server $SYSCFG_ntp_server1 true" >> $NTP_CONF_TMP
           echo "restrict $SYSCFG_ntp_server1 nomodify notrap noquery" >> $NTP_CONF_TMP
           VALID_SERVER="true"
       fi
       if [ "x$SYSCFG_ntp_server2" != "x" ] && [ "x$SYSCFG_ntp_server2" != "xno_ntp_address" ]; then
           echo "server $SYSCFG_ntp_server2" >> $NTP_CONF_TMP
           echo "restrict $SYSCFG_ntp_server2 nomodify notrap noquery" >> $NTP_CONF_TMP
           VALID_SERVER="true"
       fi
       if [ "x$SYSCFG_ntp_server3" != "x" ] && [ "x$SYSCFG_ntp_server3" != "xno_ntp_address" ]; then
           echo "server $SYSCFG_ntp_server3" >> $NTP_CONF_TMP
           echo "restrict $SYSCFG_ntp_server3 nomodify notrap noquery" >> $NTP_CONF_TMP
           VALID_SERVER="true"
       fi
       if [ "x$SYSCFG_ntp_server4" != "x" ] && [ "x$SYSCFG_ntp_server4" != "xno_ntp_address" ]; then
           echo "server $SYSCFG_ntp_server4" >> $NTP_CONF_TMP
           echo "restrict $SYSCFG_ntp_server4 nomodify notrap noquery" >> $NTP_CONF_TMP
           VALID_SERVER="true"
       fi
       if [ "x$SYSCFG_ntp_server5" != "x" ] && [ "x$SYSCFG_ntp_server5" != "xno_ntp_address" ]; then
           echo "server $SYSCFG_ntp_server5" >> $NTP_CONF_TMP
           echo "restrict $SYSCFG_ntp_server5 nomodify notrap noquery" >> $NTP_CONF_TMP
           VALID_SERVER="true"
       fi

       if [ "x$VALID_SERVER" = "x" ]; then
           if [ -f "/nvram/ETHWAN_ENABLE" ]; then
              echo_t "SERVICE_NTPD : NTP SERVERS 1-5 not available, using the default ntp server." >> $NTPD_LOG_NAME
              SYSCFG_ntp_server1="time1.google.com"
           else
              echo_t "SERVICE_NTPD : NTP SERVERS 1-5 not available, not starting ntpd" >> $NTPD_LOG_NAME
              return 0
           fi
       fi
   else

       PARTNER_ID=`syscfg get PartnerID`

       if [ "x$SYSCFG_ntp_server1" = "x" ] || [ "x$SYSCFG_ntp_server1" = "xno_ntp_address" ]; then
           if [ "x$PARTNER_ID" = "x" ]; then
               echo_t "SERVICE_NTPD : NTP SERVER 1 not available & PARTNER_ID is null, using the default ntp server." >> $NTPD_LOG_NAME
               SYSCFG_ntp_server1="time1.google.com"
           else
               if [ -f "/nvram/ETHWAN_ENABLE" ]; then
                  echo_t "SERVICE_NTPD : NTP SERVER 1 not available, using the default ntp server." >> $NTPD_LOG_NAME
                  SYSCFG_ntp_server1="time1.google.com"
               else
                  echo_t "SERVICE_NTPD : NTP SERVER 1 not available, not starting ntpd" >> $NTPD_LOG_NAME
                  return 0
               fi
           fi
       fi

       # Start NTP Config Creation with Legacy Single Server Setup
       echo_t "SERVICE_NTPD : Creating NTP config" >> $NTPD_LOG_NAME

       echo "server $SYSCFG_ntp_server1 true" >> $NTP_CONF_TMP
       echo "restrict $SYSCFG_ntp_server1 nomodify notrap noquery" >> $NTP_CONF_TMP

   fi # if [ "$SYSCFG_new_ntp_enabled" = "true" ]; then

   # Continue with Rest of NTP Config Creation
   if [ "x$SOURCE_PING_INTF" == "x" ]; then
       MASK=ifconfig $SOURCE_PING_INTF | sed -rn '2s/ .*:(.*)$/\1/p'
   else
       MASK="255.255.255.0"
   fi

   WAN_IP=""
   QUICK_SYNC_WAN_IP=""

   if [ "$NTPD_INTERFACE" == $WAN_INTERFACE ]; then

       # Enable Basic NTPD Daemon Logging in Newer Devices
       echo "logconfig =syncall +clockall +sysall +peerall" >> $NTP_CONF_TMP

       sleep 30
       erouter_wait QUICK_SYNC_WAN_IP quickSync
       erouter_wait WAN_IP
   else
       PROVISIONED_TYPE=""
       PROVISIONED_TYPE=$(dmcli eRT getv Device.X_CISCO_COM_CableModem.ProvIpType | grep value | awk '/value/{print $5}')

       if [ "$PROVISIONED_TYPE" == "IPV4" ]; then
           WAN_IP=`ifconfig -a $NTPD_INTERFACE | grep inet | grep -v inet6 | tr -s " " | cut -d ":" -f2 | cut -d " " -f1`
       else
           WAN_IP=`ifconfig $NTPD_INTERFACE | grep inet6 | grep Global | awk '/inet6/{print $3}' | cut -d '/' -f1`
       fi
   fi

   if [ "$QUICK_SYNC_WAN_IP" != "" ]; then
       # Quick Sync doesn't allow NIC Rules in Configuration File So create Quick Sync Version Prior to writing NIC rules.
       echo_t "SERVICE_NTPD : Creating NTP Quick Sync Conf file: $NTP_CONF_QUICK_SYNC" >> $NTPD_LOG_NAME
       cp $NTP_CONF_TMP $NTP_CONF_QUICK_SYNC  
   fi #if [ "$QUICK_SYNC_WAN_IP" != "" ]; then

   if [ "x$BOX_TYPE" != "xHUB4" ]  && [ "x$BOX_TYPE" != "xSR300" ] && [ "x$BOX_TYPE" != "xSE501" ] && [ "x$NTPD_IMMED_PEER_SYNC" != "xtrue" ] ; then
       echo "restrict $PEER_INTERFACE_IP mask $MASK nomodify notrap" >> $NTP_CONF_TMP
   fi

   # interface rules can't be written to quick sync conf file so write here after quick sync conf file creation.
   echo "interface ignore wildcard" >> $NTP_CONF_TMP
   echo "interface listen 127.0.0.1" >> $NTP_CONF_TMP
   echo "interface listen ::1" >> $NTP_CONF_TMP

   if [ "$WAN_IP" != "" ]; then
       echo "interface listen $WAN_IP" >> $NTP_CONF_TMP
   fi  

   if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ]; then
       # SKYH4-2006: To listen v6 server, update the conf file after getting valid v6 IP(CURRENT_WAN_V6_PREFIX)
       CURRENT_WAN_IPV6_STATUS=`sysevent get ipv6_connection_state`

       if [ "xup" = "x$CURRENT_WAN_IPV6_STATUS" ] ; then
           CURRENT_WAN_V6_PREFIX=`syscfg get ipv6_prefix_address`
           if [ "x$CURRENT_WAN_V6_PREFIX" != "x" ]; then
               echo "interface listen $CURRENT_WAN_V6_PREFIX" >> $NTP_CONF_TMP
               sysevent set ntp_ipv6_listen "set"
           else
               sysevent set ntp_ipv6_listen "unset"
           fi
       fi
   fi

   if [ "x$MULTI_CORE" = "xyes" ]  && [ "x$NTPD_IMMED_PEER_SYNC" != "xtrue" ]; then
       echo "interface listen $HOST_INTERFACE_IP" >> $NTP_CONF_TMP
   fi

   if [ "x$BOX_TYPE" = "xXB3" ]; then
       kill -9 "`pidof $BIN`" > /dev/null 2>&1
       echo_t "SERVICE_NTPD : Starting NTP Daemon" >> $NTPD_LOG_NAME
       $BIN -c $NTP_CONF_TMP -l $NTPD_LOG_NAME -g
   else
       systemctl stop $BIN

       echo_t "SERVICE_NTPD : Killing All Instances of NTP" >> $NTPD_LOG_NAME
       killall $BIN ### This to ensure there is no instance of NTPD running because of multiple wan-start events
       sleep 5

       if [ -n "$QUICK_SYNC_WAN_IP" ]; then
           # Try and Force Quick Sync to Run on a single interface
           echo_t "SERVICE_NTPD : Starting NTP Quick Sync" >> $NTPD_LOG_NAME
           if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ]; then
               if [ $EROUTER_IPV6_UP -eq 1 ]; then
                   $BIN -c $NTP_CONF_QUICK_SYNC --interface "$QUICK_SYNC_WAN_IP" -x -gq -l $NTPD_LOG_NAME & 
                   QUICK_SYNC_PID=$!
               else
                   $BIN -c $NTP_CONF_QUICK_SYNC --interface "$QUICK_SYNC_WAN_IP" -x -gq -4 -l $NTPD_LOG_NAME &
                   QUICK_SYNC_PID=$!
               fi
           else
               $BIN -c $NTP_CONF_QUICK_SYNC --interface "$QUICK_SYNC_WAN_IP" -x -gq -l $NTPD_LOG_NAME &
               QUICK_SYNC_PID=$!
           fi
           if [ "x$QUICK_SYNC_PID" != "x" ];then
              set_ntp_quicksync_status
           fi
       else
           echo_t "SERVICE_NTPD : Quick Sync Not Run" >> $NTPD_LOG_NAME
       fi

       echo_t "SERVICE_NTPD : Killing All Instances of NTP" >> $NTPD_LOG_NAME
       killall $BIN

       echo_t "SERVICE_NTPD : Starting NTP Daemon" >> $NTPD_LOG_NAME
       systemctl start $BIN

       if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ]; then
           sysevent set firewall-restart
       fi
   fi

   ret_val=$?
   if [ "$ret_val" -ne 0 ]; then
       echo_t "SERVICE_NTPD : NTP failed to start, retrying" >> $NTPD_LOG_NAME
       if [ "x$BOX_TYPE" = "xXB3" ]; then
           echo_t "SERVICE_NTPD : Starting NTP Daemon" >> $NTPD_LOG_NAME
           $BIN -c $NTP_CONF_TMP -l $NTPD_LOG_NAME -g
       else
           echo_t "SERVICE_NTPD : Killing All Instances of NTP" >> $NTPD_LOG_NAME
           killall $BIN ### This to ensure there is no instance of NTPD running because of multiple wan-start events
           sleep 5

           if [ -n "$QUICK_SYNC_WAN_IP" ]; then
               # Try and Force Quick Sync to Run on a single interface
               echo_t "SERVICE_NTPD : Starting NTP Quick Sync" >> $NTPD_LOG_NAME
               $BIN -c $NTP_CONF_QUICK_SYNC --interface "$QUICK_SYNC_WAN_IP" -x -gq -l $NTPD_LOG_NAME &
	       QUICK_SYNC_PID=$!
	       if [ "x$QUICK_SYNC_PID" != "x" ];then
                  set_ntp_quicksync_status
               fi
           else
               echo_t "SERVICE_NTPD : Quick Sync Not Run" >> $NTPD_LOG_NAME
           fi

           echo_t "SERVICE_NTPD : Killing All Instances of NTP" >> $NTPD_LOG_NAME
           killall $BIN

           echo_t "SERVICE_NTPD : Starting NTP Daemon" >> $NTPD_LOG_NAME
           systemctl start $BIN
       fi
   fi

   if [ "x$BOX_TYPE" = "xXB3" ]; then
       # Setting Time status as synchronized
       syscfg set ntp_status 3

       #Set FirstUseDate in Syscfg if this is the first time we are doing a successful NTP Sych
       DEVICEFIRSTUSEDATE=`syscfg get device_first_use_date`
       if [ "" = "$DEVICEFIRSTUSEDATE" ] || [ "0" = "$DEVICEFIRSTUSEDATE" ]; then
           FIRSTUSEDATE=`date +%Y-%m-%dT%H:%M:%S`
           syscfg set device_first_use_date "$FIRSTUSEDATE"
       fi
   else
       ####QUICK SYNC not succeeded,lets wait for ntpd daemon to adjust the clock
       if [ "$QUICK_SYNC_DONE" -eq 0 ];then
          set_ntp_driftsync_status &
       fi

   fi
   echo_t "SERVICE_NTPD : ntpd started , setting the status as started" >> $NTPD_LOG_NAME
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop ()
{
   if [ "x$BOX_TYPE" = "xXB3" ]; then
   	kill -9 "`pidof $BIN`" > /dev/null 2>&1
   else
   	systemctl stop $BIN
   fi
   sysevent set ${SERVICE_NAME}-status "stopped"
}


service_init ()
{
    FOO=`utctx_cmd get ntp_server1 ntp_server2 ntp_server3 ntp_server4 ntp_server5 ntp_enabled new_ntp_enabled`
    eval "$FOO"
}


# service_ntpd.sh Entry

while [ -e ${LOCKFILE} ] ; do
    #See if process is still running
    kill -0 "`cat ${LOCKFILE}`"
    if [ $? -ne 0 ]
    then
        break
    fi
    echo_t "SERVICE_NTPD : Waiting for parallel instance of $0 to finish..."
    sleep 1
done

#make sure the lockfile is removed when we exit and then claim it
trap 'rm -f ${LOCKFILE}; exit' INT TERM EXIT
echo $$ > ${LOCKFILE}

service_init

CURRENT_WAN_STATUS=`sysevent get wan-status`
case "$1" in
  "${SERVICE_NAME}-start")
      echo_t "SERVICE_NTPD : ${SERVICE_NAME}-start calling service_start" >> $NTPD_LOG_NAME
      service_start
      ;;
  "${SERVICE_NAME}-stop")
      echo_t "SERVICE_NTPD : ${SERVICE_NAME}-stop called" >> $NTPD_LOG_NAME
      service_stop
      ;;
  "${SERVICE_NAME}-restart")
      echo_t "SERVICE_NTPD : ${SERVICE_NAME}-restart called" >> $NTPD_LOG_NAME
      service_stop
      service_start
      ;;
  wan-status)
      if [ "started" = "$CURRENT_WAN_STATUS" ] ; then
         if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ]; then
            NTPD_PROCESS=`pidof $BIN`
            if [ "x$NTPD_PROCESS" != "x" ];then
               echo_t "SERVICE_NTPD : ntp process is already running and pid is = $NTPD_PROCESS" >> $NTPD_LOG_NAME
            else
               echo_t "SERVICE_NTPD : wan-status calling service_start" >> $NTPD_LOG_NAME
               service_start
            fi
         else
            echo_t "SERVICE_NTPD : wan-status calling service_start" >> $NTPD_LOG_NAME
            service_start
         fi
      fi
      ;;
  ipv6_connection_state)
      if [ "x$BOX_TYPE" = "xHUB4" ] || [ "x$BOX_TYPE" = "xSR300" ] || [ "x$BOX_TYPE" = "xSE501" ] || [ "x$BOX_TYPE" = "xWNXL11BWL" ]; then
          WAN_IPV6_STATUS=`sysevent get ipv6_connection_state`
          if [ "xup" = "x$WAN_IPV6_STATUS" ] ; then
              CURRENT_WAN_V6_PREFIX=`syscfg get ipv6_prefix_address`
              NTP_PREFIX=`sysevent get ntp_prefix`
              NTP_IPV6_LISTEN=`sysevent get ntp_ipv6_listen`
              if [ "x$CURRENT_WAN_V6_PREFIX" != "x" ] && ([ "x$NTP_PREFIX" != "x$CURRENT_WAN_V6_PREFIX" ] || [ "xset" != "x$NTP_IPV6_LISTEN" ]) ; then
                  echo_t "SERVICE_NTPD : ipv6_connection_state calling service_start" >> $NTPD_LOG_NAME
                  sysevent set ntp_prefix $CURRENT_WAN_V6_PREFIX
                  service_start
              fi
          fi
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status ]" >&2
      rm -f ${LOCKFILE}
      exit 3
      ;;
esac

echo_t "SERVICE_NTPD : End of shell script" >> $NTPD_LOG_NAME

#Script finished, remove lock file
rm -f ${LOCKFILE}
