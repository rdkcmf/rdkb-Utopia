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
# This script is called when the value of <desired_ipv4_link_state, *> changes.
# desired_ipv4_link_state is one of:
#    up             - The system wants to bring the wan ipv4 link up
#    down           - The system wants to bring the wan ipv4 link down
#
# The script is called with one parameter:
#   The value of the parameter is phylink_wan_state if the physical link state has changed
#   and it is desired_state_change if the desired_ipv4_link_state has changed
#
# This script is responsible for bringing up connectivity with
# the ISP using pppoe provisioning.
#
# It is responsible for provisioning the interface IP Address, and the
# routing table. And also /etc/resolv.conf
#
# Upon success it must set:
#    sysevent ipv4_wan_ipaddr
#    sysevent ipv4_wan_subnet
#    sysevent current_ipv4_link_state
#
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
PID="($$)"

#------------------------------------------------------------------
# do_start_pppoe
#------------------------------------------------------------------
do_start_pppoe() {
   ulog pppoe_link status "$PID setting current_ipv4_link_state up"
   sysevent set current_ipv4_link_state up
}

#------------------------------------------------------------------
# do_stop_pppoe
#------------------------------------------------------------------
do_stop_pppoe() {
   ulog pppoe_link status "$PID setting current_ipv4_link_state down"
   sysevent set current_ipv4_link_state down
}


#--------------------------------------------------------------
# Main entry point
#
#--------------------------------------------------------------
CURRENT_STATE=`sysevent get current_ipv4_link_state`
DESIRED_STATE=`sysevent get desired_ipv4_link_state`
PHYLINK_STATE=`sysevent get phylink_wan_state`

case "$1" in
   phylink_wan_state)
      ulog pppoe_link status "$PID physical link is $PHYLINK_STATE"
      if [ "up" != "$PHYLINK_STATE" ] ; then
         if [ "up" = "$CURRENT_STATE" ] ; then
            ulog pppoe_link status "$PID physical link is down. Setting link down."
            do_stop_pppoe
            exit 0
         else
            ulog pppoe_link status "$PID physical link is down. Link is already down."
            exit 0
         fi
      else
         if [ "up" = "$CURRENT_STATE" ] ; then
            ulog pppoe_link status "$PID physical link is up. Link is already up."
         else
            if [ "up" = "$DESIRED_STATE" ] ; then
                  ulog pppoe_link status "$PID starting pppoe link"
                  do_start_pppoe
                  exit 0
            else
               ulog pppoe_link status "$PID physical link is up, but desired link state is down."
               exit 0;
            fi
         fi
      fi
      ;;

   desired_ipv4_link_state)
      if [ "$CURRENT_STATE" = "$DESIRED_STATE" ] ; then
         ulog pppoe_link status "$PID wan is already is desired state $CURRENT_STATE"
         exit
      fi

      if [ "up" = "$DESIRED_STATE" ] ; then
         if [ "down" = "$PHYLINK_STATE" ] ; then
            ulog pppoe_link status "$PID up requested but physical link is still down"
            exit 0;
         fi

         ulog pppoe_link status "$PID up requested"
         do_start_pppoe
         exit 0
      else
         ulog pppoe_link status "$PID down requested"
         do_stop_pppoe
         exit 0
      fi
      ;;
  *)
        ulog pppoe_link status "$PID Invalid parameter $1 "
        exit 3
        ;;
esac
