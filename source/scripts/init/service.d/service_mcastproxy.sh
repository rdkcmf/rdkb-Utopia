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
source /etc/device.properties

SERVICE_NAME="mcastproxy"
SELF_NAME="`basename "$0"`"

BIN=igmpproxy

if [ -f /usr/sbin/smcrouted ]; then
    NEW_SMCROUTE=1
    BIN2=smcrouted
else
    NEW_SMCROUTE=0
    BIN2=smcroute
fi

CONF_FILE=/tmp/igmpproxy.conf
CONF_FILE_2=/tmp/smcroute.conf

do_start_igmpproxy () {
   LOCAL_CONF_FILE=/tmp/igmpproxy.conf$$
   INTERFACE_LIST=`ip link show up | cut -d' ' -f2 | sed -e 's/:$//' | sed -e 's/@[_a-zA-Z0-9]*//'`

   killall $BIN
   killall $BIN2

   rm -rf $LOCAL_CONF_FILE

   #echo "quickleave" >> $LOCAL_CONF_FILE
   if [ "started" = "`sysevent get wan-status`" ] ; then
      echo "phyint $WAN_IFNAME upstream" >> $LOCAL_CONF_FILE
      #echo "altnet 0.0.0.0/0" >> $LOCAL_CONF_FILE
   else
      if [ "$NEW_SMCROUTE" != "1" ]; then
         echo "phyint $WAN_IFNAME disabled" >> $LOCAL_CONF_FILE
      fi
   fi
   echo "phyint $SYSCFG_lan_ifname downstream" >> $LOCAL_CONF_FILE

   # Disable all other interfaces
   for interface in $INTERFACE_LIST
   do
       if [ "$interface" != "$SYSCFG_lan_ifname" ] && [ "$interface" != "$WAN_IFNAME" ]; then
         if [ "$interface" = "$MOCA_INTERFACE" ];then
          echo "phyint $interface downstream" >> $LOCAL_CONF_FILE
          MOCA_LAN_UP=1
         else 
          if [ "$NEW_SMCROUTE" != "1" ]; then
            echo "phyint $interface disabled" >> $LOCAL_CONF_FILE
          fi
         fi
       fi
   done
#HOME_LAN_ISOLATION=`psmcli get dmsb.l2net.HomeNetworkIsolation`
if [ "$HOME_LAN_ISOLATION" = "1" ]; then
if [ "$MOCA_LAN_UP" = "1" ]; then
   echo "phyint $SYSCFG_lan_ifname enable ttl-threshold 1" >> $LOCAL_CONF_FILE
   echo "phyint $MOCA_INTERFACE enable ttl-threshold 1" >> $LOCAL_CONF_FILE
   if [ "$NEW_SMCROUTE" != "1" ]; then
       echo "mgroup from $SYSCFG_lan_ifname group 239.255.255.250" >> $LOCAL_CONF_FILE
       echo "mgroup from $MOCA_INTERFACE group 239.255.255.250" >> $LOCAL_CONF_FILE
   fi
   echo "mroute from $SYSCFG_lan_ifname group 239.255.255.250 to brlan10" >> $LOCAL_CONF_FILE
   echo "mroute from $MOCA_INTERFACE group 239.255.255.250 to brlan0" >> $LOCAL_CONF_FILE
fi
   cat $LOCAL_CONF_FILE > $CONF_FILE_2
   rm -f $LOCAL_CONF_FILE
   if [ "$NEW_SMCROUTE" = "1" ]; then
       $BIN2 -d 5 -f $CONF_FILE_2 -n -N -s &
   else
       $BIN2 -f $CONF_FILE_2 -d &
   fi

else
   cat $LOCAL_CONF_FILE > $CONF_FILE
   rm -f $LOCAL_CONF_FILE
   if [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ] || [ "$BOX_TYPE" = "SE501" ] || [ "$BOX_TYPE" = "SR213" ] || [ "$BOX_TYPE" == "WNXL11BWL" ] || [ "$BOX_TYPE" == "rpi" ]; then
       $BIN $CONF_FILE &
   else 
       $BIN -c $CONF_FILE &
   fi
fi
}

service_init ()
{
   eval "`utctx_cmd get igmpproxy_enabled lan_ifname`"
   WAN_IFNAME=`sysevent get current_wan_ifname`
   HOME_LAN_ISOLATION=`psmcli get dmsb.l2net.HomeNetworkIsolation`
   DSLite_Enabled=`syscfg get dslite_enable`
   if [ "$DSLITE_DHCP_OPTION_ENABLED" = "true" ] && [ "$DSLite_Enabled" = "1" ] ; then
       WAN_TUNNEL_IFNAME=`syscfg get dslite_tunnel_interface_1`
       if [ -n "$WAN_TUNNEL_IFNAME" ] ; then
            WAN_IFNAME=$WAN_TUNNEL_IFNAME
       fi
   fi
   MOCA_INTERFACE=`psmcli get dmsb.l2net.9.Name`

}

service_start () 
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   if [ -n "$WAN_IFNAME" ] && [ "1" = "$SYSCFG_igmpproxy_enabled" ] ; then
      do_start_igmpproxy
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status "started"
   fi
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

if [ "$HOME_LAN_ISOLATION" = "1" ]; then
   killall $BIN2
   rm -rf $CONF_FILE_2
else
   killall $BIN
   rm -rf $CONF_FILE
fi
   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

# Entry

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
  wan-status)
      CURRENT_WAN_STATUS=`sysevent get wan-status`
      CURRENT_LAN_STATUS=`sysevent get lan-status`
      if [ "started" = "$CURRENT_WAN_STATUS" ] && [ "started" = "$CURRENT_LAN_STATUS" ] ; then
         service_start
      elif [ "stopped" = "$CURRENT_WAN_STATUS" ] || [ "stopped" = "$CURRENT_LAN_STATUS" ] ; then
         service_stop 
      fi
      ;;
  lan-status)
      CURRENT_WAN_STATUS=`sysevent get wan-status`
      CURRENT_LAN_STATUS=`sysevent get lan-status`
      if [ "started" = "$CURRENT_WAN_STATUS" ] && [ "started" = "$CURRENT_LAN_STATUS" ] ; then
         service_start
      elif [ "stopped" = "$CURRENT_WAN_STATUS" ] || [ "stopped" = "$CURRENT_LAN_STATUS" ] ; then
         service_stop 
      fi
      ;;
  bridge-status)
      CURRENT_BRIDGE_STATUS=`sysevent get bridge-status`
      if [ "started" = "$CURRENT_BRIDGE_STATUS" ] ; then
         service_start
      elif [ "stopped" = "$CURRENT_BRIDGE_STATUS" ] ; then
         service_stop 
      fi
      ;;
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status | lan-status | bridge-status ]" >&2
      exit 3
      ;;
esac
