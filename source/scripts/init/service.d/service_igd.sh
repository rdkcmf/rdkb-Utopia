#!/bin/sh

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
# Copyright (c) 2009 by Cisco Systems, Inc. All Rights Reserved.
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
# This script is used to start UPnP InternetGatewayDevice (IGD) daemon
#------------------------------------------------------------------
SERVICE_NAME="igd"
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/ut_plat.sh


SELF_NAME="`basename $0`"

#IGD=/usr/sbin/IGD
IGD=IGD
UPNP_TMP=/var/tmp/upnp.ttl

resync_upnp() {
    local CURRENT_NETS="`sysevent get ${SERVICE_NAME}_current_nets`"
    local REM_NETS="${CURRENT_NETS}"
    local LOAD_NETS="`psmcli getallinst ${IPV4_NV_PREFIX}.`"
    
    
    for i in  $LOAD_NETS; do 
        REM_NETS="`echo $REM_NETS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
        REQ_STRING="${REQ_STRING} UPNP_${i} ${IPV4_NV_PREFIX}.${i}.${UPNP_DM}"
    done
    
    eval `psmcli get -e ${REQ_STRING}`
    
    for i in $REM_NETS; do 
        async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
        sysevent rm_async $async
        CUR_IGD_PID=`sysevent get ${SERVICE_NAME}_${i}-pid`
        if [ x != x$CUR_IGD_PID ]; then 
            kill $CUR_IGD_PID
        fi
    done
    
    #for i in $REM_NETS; do 
    #    async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
    #    sysevent rm_async $async
    #done
    
    for i in $LOAD_NETS; do 
        async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
        eval sysevent set ${SERVICE_NAME}_${i}-enabled \${UPNP_${i}}
        if [ x = x"$async" ]; then
            async="`sysevent async ipv4_${i}-status ${UTOPIAROOT}/service_${SERVICE_NAME}.sh`"
            sysevent set ${SERVICE_NAME}_${i}-ipv4async "$async"
        fi
        handle_ipv4_status ${i} `sysevent get ipv4_${i}-status`
        
    done
    
    sysevent set ${SERVICE_NAME}_current_nets "$LOAD_NETS"
        
}

handle_ipv4_status() {
    
    CUR_IGD_PID=`sysevent get ${SERVICE_NAME}_${1}-pid`
    if [ x$L3_UP_STATUS = x$2 ] && [ x1 = x`sysevent get ${SERVICE_NAME}_${1}-enabled` ]; then
        if [ x = x$CUR_IGD_PID ] ; then
            IGD `sysevent get ipv4_${1}-ifname` &
            sysevent set ${SERVICE_NAME}_${1}-pid $!
        fi
    else
        if [ x != x$CUR_IGD_PID ]; then
            kill $CUR_IGD_PID
            sysevent set ${SERVICE_NAME}_${1}-pid 
        fi
    
    fi
}

service_start() {

#    killall IGD
#    rm -rf /var/IGD
# 
#    # start IGD daemon
    if [ "1" = "$SYSCFG_upnp_igd_enabled" -a x`sysevent get ${SERVICE_NAME}-status` = x"stopped" ] ; then
        ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 
        UPNP_TTL=`syscfg get upnp_igd_advr_ttl`
        touch $UPNP_TMP
        echo "$UPNP_TTL" > $UPNP_TMP
#        mkdir -p /var/IGD
#        (cd /var/IGD; ln -sf /etc/IGD/* .)
#        $IGD `sysevent get current_lan_ipaddr` &
#    fi
        resync_upnp
        sysevent set ${SERVICE_NAME}-errinfo
        sysevent set ${SERVICE_NAME}-status "started"
    fi
}

service_stop () {
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   local CURRENT_NETS="`sysevent get ${SERVICE_NAME}_current_nets`"
   for net in $CURRENT_NETS; do 
        handle_ipv4_status $net $IF_DOWN
   done
   
   rm -rf /var/IGD/IGDdevicedesc_*.xml

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_init() {
    FOO=`utctx_cmd get upnp_igd_enabled`
    eval $FOO
}

init_once () {
#    if [ "1" = "$SYSCFG_upnp_igd_enabled" ] ; then
#        ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 
#        UPNP_TTL=`syscfg get upnp_igd_advr_ttl`
#        touch $UPNP_TMP
#        echo "$UPNP_TTL" > $UPNP_TMP
        mkdir -p /var/IGD
        (cd /var/IGD; ln -sf /etc/IGD/* .)
        swctl -c 23 -p 7 -s 01:00:5E:7F:FF:FA
        swctl -c 23 -p 2 -s 01:00:5E:7F:FF:FA
		#external MoCA chip
	   	swctl -c 23 -p 3 -s 01:00:5E:7F:FF:FA
	   	#Intel MoCA chip
	   	swctl -c 23 -p 1 -s 01:00:5E:7F:FF:FA
#    fi
}


#---------------------------------------------------------------

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
  lan-status)
      CURRENT_LAN_STATUS=`sysevent get lan-status`
      if [ "started" = "$CURRENT_LAN_STATUS" ] ; then
         service_start
      elif [ "stopped" = "$CURRENT_LAN_STATUS" ] ; then 
         service_stop
      fi
      ;;
    snmp_subagent-status)
        init_once
        resync_upnp
   ;;
    ipv4-resyncAll)
        resync_upnp
   ;;
   ipv4_*-status)
        INST=${1%-*}
        INST=${INST#*_}
        handle_ipv4_status $INST $2
        ;;
  *)
      echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|lan-status]" >&2
      exit 3
      ;;
esac

