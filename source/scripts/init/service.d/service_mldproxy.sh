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

SERVICE_NAME="mldproxy"
SELF_NAME="`basename "$0"`"

BIN=mldproxy
CONF_FILE=/tmp/mldproxy.conf

do_start_mldproxy () {
   LOCAL_CONF_FILE=/tmp/mldproxy.conf$$

   killall $BIN

   rm -rf $LOCAL_CONF_FILE

   #echo "fastleave" >> $LOCAL_CONF_FILE
   if [ "started" = "`sysevent get wan-status`" ] ; then
      echo "phyint $WAN_IFNAME upstream" >> $LOCAL_CONF_FILE
   else
      echo "phyint $WAN_IFNAME disabled" >> $LOCAL_CONF_FILE
   fi

#   Commenting brlan0 downstream from mldproxy config (RDKB-10413)
#   echo "phyint $SYSCFG_lan_ifname downstream" >> $LOCAL_CONF_FILE

   cat $LOCAL_CONF_FILE > $CONF_FILE
   rm -f $LOCAL_CONF_FILE 
   mld=`which mldproxy`
   if [ -z "$mld" ]; then	   
	   echo " mldproxy not found "
   else
	   $BIN -c $CONF_FILE -f
   fi
}

service_init ()
{
   eval "`utctx_cmd get mldproxy_enabled lan_ifname`"
   WAN_IFNAME=`sysevent get current_wan_ifname`
}

service_start () 
{
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   if [ -n "$WAN_IFNAME" ] && [ "1" == "$SYSCFG_mldproxy_enabled" ] ; then
      do_start_mldproxy
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
  *)
      echo "Usage: $SELF_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart | wan-status | lan-status ]" >&2
      exit 3
      ;;
esac
