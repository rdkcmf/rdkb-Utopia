#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
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

SERVICE_NAME="mcastproxy"
SELF_NAME="`basename $0`"

BIN=igmpproxy
CONF_FILE=/tmp/igmpproxy.conf

do_start_igmpproxy () {
   LOCAL_CONF_FILE=/tmp/igmpproxy.conf$$
   INTERFACE_LIST=`ip link show up | cut -d' ' -f2 | sed -e 's/:$//' | sed -e 's/@[_a-zA-Z0-9]*//'`

   killall $BIN

   rm -rf $LOCAL_CONF_FILE

   #echo "quickleave" >> $LOCAL_CONF_FILE
   if [ "started" = "`sysevent get wan-status`" ] ; then
      echo "phyint $WAN_IFNAME upstream" >> $LOCAL_CONF_FILE
      #echo "altnet 0.0.0.0/0" >> $LOCAL_CONF_FILE
   else
      echo "phyint $WAN_IFNAME disabled" >> $LOCAL_CONF_FILE
   fi
   echo "phyint $SYSCFG_lan_ifname downstream" >> $LOCAL_CONF_FILE

   # Disable all other interfaces
   for interface in $INTERFACE_LIST
   do
       if [ $interface != $SYSCFG_lan_ifname ] && [ $interface != $WAN_IFNAME ]; then
           echo "phyint $interface disabled" >> $LOCAL_CONF_FILE
       fi
   done

   cat $LOCAL_CONF_FILE > $CONF_FILE
   rm -f $LOCAL_CONF_FILE 
   $BIN -c $CONF_FILE &
}

service_init ()
{
   eval `utctx_cmd get igmpproxy_enabled lan_ifname`
   WAN_IFNAME=`sysevent get current_wan_ifname`
}

service_start () 
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   if [ "" != "$WAN_IFNAME" ] && [ "1" == "$SYSCFG_igmpproxy_enabled" ] ; then
      do_start_igmpproxy
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status "started"
   fi
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   killall $BIN
   rm -rf $CONF_FILE

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

# Entry

service_init

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
