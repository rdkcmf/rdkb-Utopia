#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="ciscoconnect"
CC_PSM_BASE=dmsb.CiscoConnect

cc_preproc () 
{
    eval `psmcli get -e IP $CC_PSM_BASE.l3net BR $CC_PSM_BASE.l2net ENABLE $CC_PSM_BASE.guestEnabled POOL $CC_PSM_BASE.pool`
    
    sysevent set ${SERVICE_NAME}_guest_l3net $IP
    sysevent set ${SERVICE_NAME}_guest_l2net $BR
    sysevent set ${SERVICE_NAME}_guest_pool $POOL
    sysevent set ${SERVICE_NAME}_guest_enable $ENABLE
    
}

start_guestnet () 
{
    sysevent set ipv4-up `sysevent get ${SERVICE_NAME}_guest_l3net`
    sysevent set ciscoconnect-guest_status "started"
}

stop_guestnet () 
{
    sysevent set ipv4-down `sysevent get ${SERVICE_NAME}_guest_l3net`
    sysevent set multinet-down `sysevent get ${SERVICE_NAME}_guest_l2net`
    sysevent set firewall-restart
    sysevent set ciscoconnect-guest_status "stopped"
}

service_init ()
{
    echo
}


service_start ()
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 
   
   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () 
{
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 
   

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_restart () 
{
   service_stop
   service_start
}


service_init

case "$1" in
  ${SERVICE_NAME}-start)
     service_start
     ;;
  ${SERVICE_NAME}-stop)
     service_stop
     ;;
  ${SERVICE_NAME}-restart)
     ENABLED=`psmcli get dmsb.CiscoConnect.guestEnabled`
     sysevent set ${SERVICE_NAME}_guest_enable $ENABLED
     
     if [ x"started" = x`sysevent get ciscoconnect-guest_status` ]; then
        stop_guestnet
     fi
     
     
     if [ x1 = x$ENABLED ]; then
        start_guestnet
     fi
     ;;
  snmp_subagent-status)
     cc_preproc
     ;;

  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart]" >&2
      exit 3
      ;;
esac

