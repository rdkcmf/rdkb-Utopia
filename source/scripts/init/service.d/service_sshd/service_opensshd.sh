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
# This script is used to start ssh daemon
# $1 is the calling event (sshd-restart, lan-status, wan-status, etc)
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh

SERVICE_NAME="sshd"
SELF_NAME="`basename "$0"`"

SSHD=/sbin/sshd
PMON=/etc/utopia/service.d/pmon.sh

do_start() {

   SSHD_PID=`pidof sshd`
   [ "${SSHD_PID}" ] && return 0

   DIR_NAME=/tmp/home/admin
   if [ ! -d $DIR_NAME ] ; then
      # in order to use user admin for ssh we need to give it a home directory
      # echo "[utopia] Creating ssh user admin" > /dev/console
      mkdir -p $DIR_NAME
      chown admin $DIR_NAME
      chgrp admin $DIR_NAME
      chmod 755 $DIR_NAME
   fi

   # if there is no ssh credentials in our secret directory then make them now
   if [  ! -f /etc/ssh/ssh_host_dsa_key ] ; then
      mkdir -p /etc/ssh
      ssh-keygen -q -t dsa -N '' -C '' -f /etc/ssh/ssh_host_dsa_key
      chmod 600 /etc/ssh/ssh_host_dsa_key
      chmod 644 /etc/ssh/ssh_host_dsa_key.pub
   fi
   if [  ! -f /etc/ssh/ssh_host_rsa_key ] ; then
      ssh-keygen -q -t rsa -N '' -C '' -f /etc/ssh/ssh_host_rsa_key
      chmod 600 /etc/ssh/ssh_host_rsa_key
      chmod 644 /etc/ssh/ssh_host_rsa_key.pub
   fi

   # start a ssh daemon
   # echo "[utopia] Starting SSH daemon" > /dev/console
   mkdir -p /var/empty
   chown root:root /var/empty
   chmod 700 /var/empty
   [ -z "${SSHD_PID}" ] && ${SSHD} -f /etc/sshd.conf
   sysevent set ssh_daemon_state up
}

do_stop() {
   # echo "[utopia] Stopping SSH daemon" > /dev/console
    SSHD_PID=`pidof sshd`
    [ ! "${SSHD_PID}" ] && return 0
    kill "${SSHD_PID}"
   sysevent set ssh_daemon_state down
}

service_start() {
   ulog ${SERVICE_NAME} status "starting ${SERVICE_NAME} service" 

   REMOTE_ACCESS=`syscfg get mgmt_wan_access`
   SSH_REMOTE_ACCESS=`syscfg get mgmt_wan_sshaccess`
  
   do_start
   $PMON setproc ssh sshd `pidof -o $$ sshd` "/etc/utopia/service.d/service_sshd.sh sshd-restart"

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "started"
}

service_stop () {
   ulog ${SERVICE_NAME} status "stopping ${SERVICE_NAME} service" 

   do_stop
   $PMON unsetproc ssh 

   sysevent set ${SERVICE_NAME}-errinfo
   sysevent set ${SERVICE_NAME}-status "stopped"
}

service_lanwan_status ()
{
      CURRENT_LAN_STATE=`sysevent get lan-status`
      CURRENT_WAN_STATE=`sysevent get wan-status`
      if [ "stopped" = "$CURRENT_LAN_STATE" ] && [ "stopped" == "$CURRENT_WAN_STATE" ] ; then
         service_stop
      else
         service_start
      fi
}

service_bridge_status ()
{
      CURRENT_BRIDGE_STATE=`sysevent get bridge-status`
      if [ "stopped" = "$CURRENT_BRIDGE_STATE" ] ; then
         service_stop
      elif [ "started" = "$CURRENT_BRIDGE_STATE" ] ; then
         service_start
      fi
}

# Entry

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
  lan-status)
      service_lanwan_status
      ;;
  wan-status)
      service_lanwan_status
      ;;
  bridge-status)
      service_bridge_status
      ;;
  *)
        echo "Usage: $SELF_NAME [${SERVICE_NAME}-start|${SERVICE_NAME}-stop|${SERVICE_NAME}-restart|lan-status|wan-status|ssh_server_restart|wan-status|lan-status]" >&2
        exit 3
        ;;
esac

