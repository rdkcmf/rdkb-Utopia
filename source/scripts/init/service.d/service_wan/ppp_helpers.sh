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

PPP_PEERS_DIRECTORY=/etc/ppp/peers
PPPOE_PEERS_FILE=$PPP_PEERS_DIRECTORY"/utopia-pppoe"
PPP_OPTIONS_FILE=/etc/ppp/options
PPP_CHAP_SECRETS_FILE=/etc/ppp/chap-secrets
PPP_PAP_SECRETS_FILE=/etc/ppp/pap-secrets

#------------------------------------------------------------------
# script to be run before bringing up ppp interface
#------------------------------------------------------------------
prepare_pppd_ip_pre_up_script() {
   IP_PRE_UP_FILENAME=/etc/ppp/ip-pre-up

   echo -n > $IP_PRE_UP_FILENAME
   echo "#!/bin/sh" >> $IP_PRE_UP_FILENAME
   echo "source /etc/utopia/service.d/ulog_functions.sh" >> $IP_PRE_UP_FILENAME
   echo "echo \"[utopia][pppd ip-pre-up] Parameter 1: \$1 Parameter 2: \$2 Parameter 3: \$3\" > /dev/console" >> $IP_PRE_UP_FILENAME
   echo "echo \"[utopia][pppd ip-pre-up] Parameter 4: \$4 Parameter 5: \$5 Parameter 6: \$6\" > /dev/console" >> $IP_PRE_UP_FILENAME

   echo "PPP_IPADDR=10.64.64.64" >> $IP_PRE_UP_FILENAME
   echo "PPP_SUBNET=255.255.255.255" >> $IP_PRE_UP_FILENAME

   echo "sysevent set wan_ppp_ifname \$1" >> $IP_PRE_UP_FILENAME

   echo "echo \"[utopia][pppd ip-pre-up] sysevent set pppd_current_wan_ifname \$1\" > /dev/console" >> $IP_PRE_UP_FILENAME
   echo "sysevent set pppd_current_wan_ifname \$1" >> $IP_PRE_UP_FILENAME
   echo "echo \"[utopia][pppd ip-pre-up] sysevent set pppd_current_wan_subnet \$PPP_SUBNET\" > /dev/console" >> $IP_PRE_UP_FILENAME
   echo "sysevent set pppd_current_wan_subnet \$PPP_SUBNET" >> $IP_PRE_UP_FILENAME
   echo "echo \"[utopia][pppd ip-pre-up] sysevent set pppd_current_wan_ipaddr \$PPP_IPADDR\" > /dev/console" >> $IP_PRE_UP_FILENAME
   echo "sysevent set pppd_current_wan_ipaddr \$PPP_IPADDR" >> $IP_PRE_UP_FILENAME

   echo "sysevent set ppp_status preup 2>&1 > /dev/console" >> $IP_PRE_UP_FILENAME
   echo "ulog ip-preup event \"sysevent set ppp_status preup\"" >> $IP_PRE_UP_FILENAME
   echo "echo \"[utopia][pppd ip-pre-up] sysevent set ppp_status preup <\`date\`>\" > /dev/console" >> $IP_PRE_UP_FILENAME
   chmod 777 $IP_PRE_UP_FILENAME
}

#------------------------------------------------------------------
# script to be run when a ppp link comes up
# When the ppp link comes up, this script is called with the following parameters
#       $1      the interface name used by pppd (e.g. ppp3)
#       $2      the tty device name
#       $3      the tty device speed
#       $4      the local IP address for the interface
#       $5      the remote IP address
#       $6      the parameter specified by the 'ipparam' option to pppd
#------------------------------------------------------------------
prepare_pppd_ip_up_script() {
   IP_UP_FILENAME=/etc/ppp/ip-up
   PPP_RESOLV_CONF=/var/run/ppp/resolv.conf
   WAN_DOMAIN_CONF=/etc/ppp/domain.conf

   echo -n > $IP_UP_FILENAME
   echo "#!/bin/sh" >> $IP_UP_FILENAME
   echo "source /etc/utopia/service.d/ulog_functions.sh" >> $IP_UP_FILENAME

   echo "echo \"[utopia][pppd ip-up] Interface: \$1 tty device name: \$2 tty device speed: \$3\" > /dev/console" >> $IP_UP_FILENAME
   echo "echo \"[utopia][pppd ip-up] Local IP address: \$4 Remote IP Address: \$5 ipparam value: \$6\" > /dev/console" >> $IP_UP_FILENAME

   echo "PPP_IPADDR=\$4" >> $IP_UP_FILENAME
   echo "PPP_SUBNET=255.255.255.255" >> $IP_UP_FILENAME

   echo "sysevent set wan_ppp_ifname \$1" >> $IP_UP_FILENAME
   echo "sysevent set ppp_local_ipaddr \$4" >> $IP_UP_FILENAME
   echo "sysevent set ppp_remote_ipaddr \$5" >> $IP_UP_FILENAME
   echo "sysevent set wan_default_gateway \$5" >> $IP_UP_FILENAME

   echo "echo \"[utopia][pppd ip-up] sysevent set pppd_current_wan_ifname \$1\" > /dev/console" >> $IP_UP_FILENAME
   echo "sysevent set pppd_current_wan_ifname \$1" >> $IP_UP_FILENAME
   echo "echo \"[utopia][pppd ip-up] sysevent set pppd_current_wan_subnet \$PPP_SUBNET\" > /dev/console" >> $IP_UP_FILENAME
   echo "sysevent set pppd_current_wan_subnet \$PPP_SUBNET" >> $IP_UP_FILENAME
   echo "echo \"[utopia][pppd ip-up] sysevent set pppd_current_wan_ipaddr \$PPP_IPADDR\" > /dev/console" >> $IP_UP_FILENAME
   echo "sysevent set pppd_current_wan_ipaddr \$PPP_IPADDR" >> $IP_UP_FILENAME

   echo "if [ -f $PPP_RESOLV_CONF ]; then" >> $IP_UP_FILENAME
   echo "   WAN_DOMAIN=\`sysevent get dhcp_domain\`" >> $IP_UP_FILENAME
   echo "   if [ \"\$WAN_DOMAIN\" != \"\" ] ; then" >> $IP_UP_FILENAME
   echo "      echo \"search \$WAN_DOMAIN\" > $WAN_DOMAIN_CONF" >> $IP_UP_FILENAME
   echo "      echo \"[utopia][pppd ip-up] sysevent get dhcp_domain \$WAN_DOMAIN\" > /dev/console" >> $IP_UP_FILENAME
   echo "   fi" >> $IP_UP_FILENAME
   echo "   cat $WAN_DOMAIN_CONF $PPP_RESOLV_CONF > /etc/resolv.conf" >> $IP_UP_FILENAME
   echo "   PPP_DNS=\`awk '{ print \$2 }' $PPP_RESOLV_CONF\`" >> $IP_UP_FILENAME
   echo "   sysevent set wan_ppp_dns \"\$PPP_DNS\"" >> $IP_UP_FILENAME
   echo "   echo \"[utopia][pppd ip-up] sysevent set wan_ppp_dns \$PPP_DNS\" > /dev/console" >> $IP_UP_FILENAME
   echo "fi" >> $IP_UP_FILENAME

   echo "sysevent set ppp_status up 2>&1 > /dev/console" >> $IP_UP_FILENAME
   echo "ulog ip-up event \"sysevent set ppp_status up\"" >> $IP_UP_FILENAME
   echo "echo \"[utopia][pppd ip-up] sysevent set ppp_status up <\`date\`>\" > /dev/console" >> $IP_UP_FILENAME

   chmod 777 $IP_UP_FILENAME

}
#------------------------------------------------------------------
# script to be run when a ppp link is available for snd/rcv ipv6 packets
#------------------------------------------------------------------
prepare_pppd_ipv6_up_script() {
   IPV6_UP_FILENAME=/etc/ppp/ipv6-up
   IPV6_LOG_FILE=/var/log/ipv6/log

   # When the ppp link comes up, this script is called with the following
   # parameters
   #       $1      the interface name used by pppd (e.g. ppp3)
   #       $2      the tty device name
   #       $3      the tty device speed
   #       $4      the local IP address for the interface
   #       $5      the remote IP address
   #       $6      the parameter specified by the 'ipparam' option to pppd
   echo -n > $IPV6_UP_FILENAME
   echo "#!/bin/sh" >> $IPV6_UP_FILENAME
   echo "echo \"[utopia][pppd ipv6-up] Congratulations PPPoE is up\" >> $IPV6_LOG_FILE" >> $IPV6_UP_FILENAME
   echo "echo \"[utopia][pppd ipv6-up] Interface: \$1 tty device name: \$2 tty device speed: \$3\" >> $IPV6_LOG_FILE" >> $IPV6_UP_FILENAME
   echo "echo \"[utopia][pppd ipv6-up] Local link local address: \$4 Remote link local address: \$5 ipparam value: \$6\" >> $IPV6_LOG_FILE" >> $IPV6_UP_FILENAME

   cat << EOM >> $IPV6_UP_FILENAME
IPV6_ROUTER_ADV=\`syscfg get router_adv_provisioning_enable\`
if [ "1" = "\$IPV6_ROUTER_ADV" ] ; then
   echo 2 > /proc/sys/net/ipv6/conf/\$1/accept_ra    # Accept RA even when forwarding is enabled
   echo 1 > /proc/sys/net/ipv6/conf/\$1/accept_ra_dfrtr # Accept default router (metric 1024)
   echo 1 > /proc/sys/net/ipv6/conf/\$1/accept_ra_pinfo # Accept prefix information for SLAAC
   echo 1 > /proc/sys/net/ipv6/conf/\$1/autoconf     # Do SLAAC
fi
EOM

   chmod 777 $IPV6_UP_FILENAME
   sysevent set current_wan_ipv6address_ll "$4"
}

#------------------------------------------------------------------
# script to be run when a ppp link goes down
#------------------------------------------------------------------
prepare_pppd_ip_down_script() {
   IP_DOWN_FILENAME=/etc/ppp/ip-down

   echo -n > $IP_DOWN_FILENAME
   echo "#!/bin/sh" >> $IP_DOWN_FILENAME
   echo "source /etc/utopia/service.d/ulog_functions.sh" >> $IP_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ip-down] Notice PPP is down\" > /dev/console" >> $IP_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ip-down] Parameter 1: \$1 Parameter 2: \$2 Parameter 3: \$3\" > /dev/console" >> $IP_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ip-down] Parameter 4: \$4 Parameter 5: \$5 Parameter 6: \$6\" > /dev/console" >> $IP_DOWN_FILENAME

   echo "echo \"[utopia][pppd ip-down] unset wan_ppp_ifname \" > /dev/console" >> $IP_DOWN_FILENAME
   echo "sysevent set wan_ppp_ifname" >> $IP_DOWN_FILENAME

   echo "sysevent set ppp_status down" >> $IP_DOWN_FILENAME
   echo "ulog ip-down event \"sysevent set ppp_status down\"" >> $IP_DOWN_FILENAME
   echo "echo \"[utopia][pppd ip-down] <\`date\`>\" > /dev/console" >> $IP_DOWN_FILENAME

   chmod 777 $IP_DOWN_FILENAME
}

#------------------------------------------------------------------
# script to be run when a ppp link goes down
#------------------------------------------------------------------
prepare_pppd_ipv6_down_script() {
   IPV6_DOWN_FILENAME=/etc/ppp/ipv6-down

   echo -n > $IPV6_DOWN_FILENAME
   echo "#!/bin/sh" >> $IPV6_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ipv6-down] Notice PPP is down\" > /dev/console" >> $IPV6_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ipv6-down] Parameter 1: \$1 Parameter 2: \$2 Parameter 3: \$3\" > /dev/console" >> $IPV6_DOWN_FILENAME
   # echo "echo \"[utopia][pppd ipv6-down] Parameter 4: \$4 Parameter 5: \$5 Parameter 6: \$6\" > /dev/console" >> $IPV6_DOWN_FILENAME

   chmod 777 $IPV6_DOWN_FILENAME
}

#------------------------------------------------------------------
# prepare_pppd_options
#------------------------------------------------------------------
prepare_pppd_options() {
   PPP_CONN_METHOD=`syscfg get ppp_conn_method`
   PPP_IDLE_TIME=`syscfg get ppp_idle_time`
   PPP_KEEPALIVE=`syscfg get ppp_keepalive_interval`
   WAN_PROTO=`syscfg get wan_proto`
   CLIENT=`syscfg get wan_proto_username`
   WAN_MTU=`syscfg get wan_mtu`

   echo -n > $PPP_OPTIONS_FILE
   if [ "demand" = "$PPP_CONN_METHOD" ]; then
     if [ "l2tp" != "$WAN_PROTO" ]; then
       echo "demand" >> $PPP_OPTIONS_FILE
     fi
     echo "idle $PPP_IDLE_TIME" >> $PPP_OPTIONS_FILE
     echo "ipcp-accept-remote" >> $PPP_OPTIONS_FILE
     echo "ipcp-accept-local" >> $PPP_OPTIONS_FILE
     echo "connect true" >> $PPP_OPTIONS_FILE
     echo "noipdefault" >> $PPP_OPTIONS_FILE
     echo "ktune" >> $PPP_OPTIONS_FILE
     echo "lcp-echo-interval 0" >> $PPP_OPTIONS_FILE
   else
     if [ "l2tp" != "$WAN_PROTO" ]; then
       echo "persist" >> $PPP_OPTIONS_FILE
     fi
     if [ "pppoe" = "$WAN_PROTO" ]; then
       echo "lcp-echo-failure 1" >> $PPP_OPTIONS_FILE
     fi
     if [ -z "$PPP_KEEPALIVE" ] || [ "0" = "$PPP_KEEPALIVE" ]; then
       echo "lcp-echo-interval 30" >> $PPP_OPTIONS_FILE
     else
       echo "lcp-echo-interval $PPP_KEEPALIVE" >> $PPP_OPTIONS_FILE
     fi

   # The comma in the ipv6 line is mandatory, don't remove it (it separates two default fields)
     echo "ipv6 ," >> $PPP_OPTIONS_FILE
   fi
   echo "defaultroute" >> $PPP_OPTIONS_FILE
   echo "usepeerdns" >> $PPP_OPTIONS_FILE
   echo "user $CLIENT" >> $PPP_OPTIONS_FILE
   if [ -z "$WAN_MTU" ] || [ "0" = "$WAN_MTU" ]; then
     case "$WAN_PROTO" in
       pppoe)
       echo "mtu 1492" >> $PPP_OPTIONS_FILE
       ;;
       pptp | l2tp)
       echo "mtu 1460" >> $PPP_OPTIONS_FILE
       ;;
     esac
   else
      echo "mtu $WAN_MTU" >> $PPP_OPTIONS_FILE
   fi
   echo "default-asyncmap" >> $PPP_OPTIONS_FILE
   echo "nopcomp" >> $PPP_OPTIONS_FILE
   echo "noaccomp" >> $PPP_OPTIONS_FILE
   echo "noccp" >> $PPP_OPTIONS_FILE
   echo "novj" >> $PPP_OPTIONS_FILE
   echo "nobsdcomp" >> $PPP_OPTIONS_FILE
   echo "nodeflate" >> $PPP_OPTIONS_FILE
   # echo "lcp-echo-failure 6" >> $PPP_OPTIONS_FILE
   echo "lock" >> $PPP_OPTIONS_FILE
   echo "noauth" >> $PPP_OPTIONS_FILE
   echo "debug" >> $PPP_OPTIONS_FILE
   echo "logfile /tmp/ppp.log" >> $PPP_OPTIONS_FILE
   # echo "dump" >> $PPP_OPTIONS_FILE
   # echo "dryrun" >> $PPP_OPTIONS_FILE
}

#------------------------------------------------------------------
# prepare_pppd_secrets
#------------------------------------------------------------------
prepare_pppd_secrets() {
   echo -n > $PPP_PAP_SECRETS_FILE
   echo -n > $PPP_CHAP_SECRETS_FILE
   CLIENT=`syscfg get wan_proto_username`
   SECRET=`syscfg get wan_proto_password`
   REMOTE_NAME=`syscfg get wan_proto_remote_name`

   if [ -z "$REMOTE_NAME" ] ; then
      REMOTE_NAME=*
   fi

   DOMAIN=`syscfg get wan_domain`
   if [ -z "$DOMAIN" ] ; then
      # client   server   secret   IP addresses
      LINE="$CLIENT $REMOTE_NAME $SECRET *"
   else
      LINE="$DOMAIN\\\\$CLIENT $REMOTE_NAME $SECRET *"
   fi
   echo "$LINE" >> $PPP_PAP_SECRETS_FILE
   echo "$LINE" >> $PPP_CHAP_SECRETS_FILE
   chmod 600 $PPP_PAP_SECRETS_FILE
   chmod 600 $PPP_CHAP_SECRETS_FILE
}

#------------------------------------------------------------------
# do_stop_wan_monitor
#------------------------------------------------------------------
do_stop_wan_monitor() {
   pidof wmon > /dev/null
   if [ $? -eq 0 ] ; then
      killall -SIGQUIT wmon

      # wait for wmon to exit
     LOOP=1
     while [ "10" -gt "$LOOP" ] ; do 
        pidof wmon > /dev/null
        if [ $? -eq 0 ] ; then
           sleep 1
           LOOP=`expr $LOOP + 1`
        else
           return 0
        fi
     done
   fi
}

#------------------------------------------------------------------
# do_start_wan_monitor
#------------------------------------------------------------------
do_start_wan_monitor() {
   do_stop_wan_monitor
   /sbin/wmon "$LAN_IFNAME"
}
