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
# This script is responsible for bringing up lan connectivity
# using dhcp.
# 
# It is responsible for provisioning the interface IP Address, and the
# routing table. And also /etc/resolv.conf
#
# This script is called by sysevent when certain events are received.
# It is called when:
#    the value of <desired_ipv4_link_state, *> changes,
#    the value of <phylink_wan_state, *> changes
#    dhcp_client-restart event
#    dhcp_client-release event
#    dhcp_client-renew   event
# desired_ipv4_link_state is one of:
#    up                     - The system wants to bring the wan ipv4 link up
#    down                   - The system wants to bring the wan ipv4 link down
# phylink_wan_state is one of:
#    up                     - The physical ethernet port has link
#    down                   - The physical ethernet port has no link
#
# It is also called by udhcpc process when certain dhcp events are received.
#    deconfig
#    renew
#    bound
#
# Upon success it must set:
#    sysevent ipv4_wan_ipaddr
#    sysevent ipv4_wan_subnet  
#    sysevent current_ipv4_link_state
#
# current_ipv4_link_state is used by wan protocols to determine whether they
# have connectivity to the wan (or to wan servers for example in the case
# of pptp or l2tp). 
# current_ipv4_link_state is one of:
#   up             - connectivity with the wan/wan servers is established
#   down           - connectivity with the wan/wan servers is disabled
#
#------------------------------------------------------------------
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /lib/rdk/t2Shared_api.sh
PID="($$)"

UDHCPC_PID_FILE=/var/run/udhcpc_lan.pid
UDHCPC_SCRIPT=/etc/utopia/service.d/service_lan/dhcp_lan.sh
#UDHCPC_OPTIONS="-O vendorspecific"
#UDHCPC_OPTIONS="-V GW-eMG"
#RESOLV_CONF="/etc/resolv.conf"
LOG_FILE="/tmp/udhcp_lan.log"
IPV6_LOG_FILE="/var/log/ipv6_lan.log"

LAN_IFNAME=`syscfg get lan_ifname`
#WAN_PROTOCOL=`syscfg get wan_proto`

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   FOO=`utctx_cmd get hostname`
   eval "$FOO"

  if [ -z "$SYSCFG_hostname" ] ; then
     SYSCFG_hostname="Utopia"
  fi
  UDHCPC_OPTIONS=`syscfg get udhcpc_options_lan`
  ulog dhcp_link status "initing dhcp UDHCPC_OPTIONS: $UDHCPC_OPTIONS"  
}

#------------------------------------------------------------------
# do_stop_dhcp
#------------------------------------------------------------------
do_stop_dhcp() {
   ulog dhcp_link status "stopping dhcp client on lan"
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      kill -USR2 "`cat $UDHCPC_PID_FILE`" && kill "`cat $UDHCPC_PID_FILE`"
      rm -f $UDHCPC_PID_FILE
   else
      killall -USR2 udhcpc && killall udhcpc
   fi
   rm -f $LOG_FILE
}

#------------------------------------------------------------------
# do_start_dhcp
#------------------------------------------------------------------
do_start_dhcp() {
#   if [ "pppoe" != "$WAN_PROTOCOL" ] ; then
      #This is a hardcoded reference to the wan udhcpc.pid file.
      #Needs to be more robust. Sysevent variable?
      if [ -f "/var/run/udhcpc.pid" ] ; then
         WAN_UDHCPC_PID_OMIT="-o "`cat /var/run/udhcpc.pid`
      else
         WAN_UDHCPC_PID_OMIT=""
      fi
      UDHCP_PID=`pidof udhcpc "$WAN_UDHCPC_PID_OMIT"`

      service_init
      if [ ! -f "$UDHCPC_PID_FILE" ] ; then
         ulog dhcp_link status "starting dhcp client on lan ($LAN_IFNAME)"
         udhcpc -S -b -i "$LAN_IFNAME" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT "$UDHCPC_OPTIONS"
      elif [ ! "${UDPCP_PID}" ] ; then
         ulog dhcp_link status "dhcp client `cat $UDHCPC_PID_FILE` died"
         do_stop_dhcp
         ulog dhcp_link status "starting dhcp client on lan ($LAN_IFNAME)"
         udhcpc -S -b -i "$LAN_IFNAME" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT "$UDHCPC_OPTIONS"
      else
         ulog dhcp_link status "dhcp client is already active on lan ($LAN_IFNAME) as `cat $UDHCPC_PID_FILE`"
      fi
#   fi
}


#------------------------------------------------------------------
# do_release_dhcp
#------------------------------------------------------------------
do_release_dhcp() {
   ulog dhcp_link status "releasing dhcp lease on lan"
      LAN_STATE=`sysevent get lan-status`
      if [ "$LAN_STATE" = "started" ] ; then
         sysevent set lan-status stopped
         ulog dhcp_link status "setting lan status stopped"
      fi
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      kill -SIGUSR2 "`cat $UDHCPC_PID_FILE`"
      ip -4 addr flush dev "$LAN_IFNAME"
   fi
}

#------------------------------------------------------------------
# do_renew_dhcp
#------------------------------------------------------------------
do_renew_dhcp() {
    ulog dhcp_link status "renewing dhcp lease on lan"
    if [ -f "$UDHCPC_PID_FILE" ] ; then
        kill -SIGUSR1 "`cat $UDHCPC_PID_FILE`"
#        LAN_STATE=`sysevent get lan-status`
#        if [ "$LAN_STATE" = "administrative_down" ] ; then
#           sysevent set current_wan_state up
#           sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
#        fi
    else
        ulog dhcp_link status "restarting dhcp client on lan"
        service_init
        udhcpc -S -b -i "$LAN_IFNAME" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT $UDHCPC_OPTIONS
    fi
}


CURRENT_STATE=`sysevent get lan-status`
DESIRED_STATE=`sysevent get desired_lan_dhcp_link`
#PHYLINK_STATE=`sysevent get phylink_wan_state`

[ -z "$1" ] && ulog dhcp_lan status "$PID called with no parameters. Ignoring call" && exit 1


if [ -n "$broadcast" ] ; then
   BROADCAST="broadcast $broadcast"
else
   BROADCAST="broadcast +"
fi
[ -n "$subnet" ] && NETMASK="/$subnet"


case "$1" in

   ############################################################################################
   # Calls from sysevent
   ############################################################################################
   dhcp_lan-restart)
      do_start_dhcp
      ;;

   dhcp_lan-release)
      do_release_dhcp
      ;;

   dhcp_lan-renew)
      do_renew_dhcp
      ;;

      desired_lan_dhcp_link)
         ulog dhcp_link status "$PID $DESIRED_STATE requested"

         if [ "up" = "$DESIRED_STATE" ] ; then
            do_start_dhcp
            exit 0
         elif [ "down" = "$DESIRED_STATE" ] ; then
            do_stop_dhcp
#            sysevent set dhcpc_ntp_server1
#            sysevent set dhcpc_ntp_server2
#            sysevent set dhcpc_ntp_server3
            sysevent set current_lan_ipaddr 0.0.0.0
            #Subnet is assumed to be 255.255.255.0
            sysevent set ipv4_lan_subnet 0.0.0.0
            sysevent set default_router
            #TODO: implement this variable
            sysevent set lan_dhcp_lease
#            sysevent set lan_dhcp_dns
            sysevent set lan-status stopped
	    ulog dhcp_link status "setting lan status stopped"
            exit 0
         fi
         ;;
 
   ############################################################################################
   # Calls from udhcpc process
   ############################################################################################
   deconfig)
      ulog dhcp_link status "udhcpc $PID - cmd $1 interface $interface ip $ip broadcast $broadcast subnet $subnet router $router" 
      # we get wan_dhcp_lease_expired whenever udhcpc deconfig is called
      # this happens if the dhcp lease is lost, but it also happens
      # before trying to get an ip address via dhcp.
      # in the latter case just ignore it
      if [ "up" = "$DESIRED_STATE" ] && [ "started" = "$CURRENT_STATE" ] ; then
         ulog dhcp_link status "$PID lan dhcp lease has expired"
         rm -f $LOG_FILE
         sysevent set lan-status stopped
	 ulog dhcp_link status "setting lan status stopped"
#         sysevent set dhcpc_ntp_server1
#         sysevent set dhcpc_ntp_server2
#         sysevent set dhcpc_ntp_server3
         sysevent set current_lan_ipaddr 0.0.0.0
         sysevent set ipv4_lan_subnet 0.0.0.0
         sysevent set default_router
         sysevent set lan_dhcp_lease
#         sysevent set wan_dhcp_dns
      else
         ulog dhcp_link status "$PID deconfig does not require handling"
      fi
      ;;

   renew|bound)
      ulog dhcp_link status "udhcpc $PID - cmd $1 interface $interface ip $ip broadcast $broadcast subnet $subnet router $router" 
      # write received dhcp options to a user accessible log
      echo "interface     : $interface" > $LOG_FILE
      echo "ip address    : $ip"        >> $LOG_FILE
      echo "subnet mask   : $subnet"    >> $LOG_FILE
      echo "broadcast     : $broadcast" >> $LOG_FILE
      echo "lease time    : $lease"     >> $LOG_FILE
      echo "router        : $router"    >> $LOG_FILE
      echo "hostname      : $hostname"  >> $LOG_FILE
      echo "domain        : $domain"    >> $LOG_FILE
      echo "next server   : $siaddr"    >> $LOG_FILE
      echo "server name   : $sname"     >> $LOG_FILE
      echo "server id     : $serverid"  >> $LOG_FILE
      echo "tftp server   : $tftp"      >> $LOG_FILE
      echo "timezone      : $timezone"  >> $LOG_FILE
      echo "time server   : $timesvr"   >> $LOG_FILE
      echo "name server   : $namesvr"   >> $LOG_FILE
      echo "ntp server    : $ntpsvr"    >> $LOG_FILE
      echo "dns server    : $dns"       >> $LOG_FILE
      echo "wins server   : $wins"      >> $LOG_FILE
      echo "log server    : $logsvr"    >> $LOG_FILE
      echo "cookie server : $cookiesvr" >> $LOG_FILE
      echo "print server  : $lprsvr"    >> $LOG_FILE
      echo "swap server   : $swapsvr"   >> $LOG_FILE
      echo "boot file     : $boot_file" >> $LOG_FILE
      echo "boot file name: $bootfile"  >> $LOG_FILE
      echo "bootsize      : $bootsize"  >> $LOG_FILE
      echo "root path     : $rootpath"  >> $LOG_FILE
      echo "ip ttl        : $ipttl"     >> $LOG_FILE
      echo "mtu           : $mtuipttl"  >> $LOG_FILE
      echo "vendorspecific: $vendorspecific"  >> $LOG_FILE

      if [ -n "$lease" ] ; then
         sysevent set lan_dhcp_lease "$lease" 
      fi
      if [ -n "$subnet" ] ; then
         sysevent set ipv4_lan_subnet "$subnet" 
      fi

      # did the assigned ip address change
      OLDIP=`/sbin/ip addr show dev "$interface"  | grep "inet " | awk '{split($2,foo, "/"); print(foo[1]);}'`
      if [ "$OLDIP" != "$ip" ] ; then
         RESULT=`arping -q -c 2 -w 3 -D -I "$interface" "$ip"`
         if [ -n "$RESULT" ] &&  [ "0" != "$RESULT" ] ; then
            echo "[utopia][lan dhcp client script] duplicate address detected $ip on $interface." > /dev/console
            echo "[utopia][lan dhcp client script] ignoring duplicate ... hoping for the best" > /dev/console
         fi

         # remove the old ip address and put in the new one
         # ip addr flush is too harsh since it also removes ipv6 addrs
         /sbin/ip -4 link set dev "$interface" down
         /sbin/ip -4 addr show dev "$interface" | grep "inet " | awk '{system("/sbin/ip addr del " $2 " dev $interface")}'
         /sbin/ip -4 addr add "$ip""$NETMASK" "$BROADCAST" dev "$interface" 
         /sbin/ip -4 link set dev "$interface" up
      fi

      # if the gateway router has changed then we need to flush routing cache
      if [ -n "$router" ] ; then
         OLD_DEFAULT_ROUTER=`sysevent get default_router`
         if [ "$router" != "$OLD_DEFAULT_ROUTER" ] ; then
#            while ip -4 route del default dev $interface ; do
            while ip -4 route del default ; do
               :
            done
            for i in $router ; do
               ip -4 route add default dev "$interface" via "$i"      
               sysevent set default_router "$i" 
            done
            ip -4 route flush cache
         fi
      fi

      # initialize ntp server found by dhcp to null
#      sysevent set dhcpc_ntp_server1 
#      sysevent set dhcpc_ntp_server2 
#      sysevent set dhcpc_ntp_server3 

      # the dhcp server needs to be restarted if domain or dns servers changes
      RESTART_DHCP_SERVER=0

#      if [ -n "$domain" ] ; then
#         PROPAGATE=`syscfg get dhcp_server_propagate_wan_domain`
#         if [ "1" = "$PROPAGATE" ] ; then
#            OLD_DOMAIN=`sysevent get dhcp_domain`
#            if [ "$OLD_DOMAIN" != "$domain" ]; then
#               RESTART_DHCP_SERVER=1
#            fi
#         fi
#         sysevent set dhcp_domain $domain
#      fi


      sysevent set current_lan_ipaddr "$ip"
#      if [ "1" = "$RESTART_DHCP_SERVER" ] ; then
#         sysevent set dhcp_server-restart
#      fi

      # for renew we are already up, so no need to set current_ipv4_link_state
      LINK_STATE=`sysevent get lan-status`
      if [ "started" != "$LINK_STATE" ] ; then
         ulog dhcp_link status "$PID setting lan-status to up"
         sysevent set lan-status started
      fi

      # Is there any DHCP vendor specific extension?
      # Such as 0009 12 14:52:20:01:06:F8:14:68:F0:00:00:00:00:00:00:00:00:00:C0:A8:06:01 (for Cisco 6RD provisionning)
#      if [ ! -z "$vendorspecific" ] ; then
#         case "${vendorspecific:0:4}" in
#            0009)    # Cisco Enterprise Number
#               case "${vendorspecific:5:2}" in
#                  12)     # 6RD provisioning
#                     SIXRD_PROV=${vendorspecific:8}
#                     echo "Found 6RD provisioning: $SIXRD_PROV" >> $IPV6_LOG_FILE
#                     SIXRD_COMMON_PREFIX4=`echo $SIXRD_PROV | awk -F: '{printf("%d","0x"$1)}'`
#                     SIXRD_ZONE_LENGTH=`echo $SIXRD_PROV | awk -F: '{printf("%d","0x"$2)}'`
#                     SIXRD_ZONE=`echo $SIXRD_PROV | awk -F: '{printf("%s%s:%s%s:%s%s:%s%s:%s%s:%s%s:%s%s:%s%s",$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18)}'`
#                     SIXRD_RELAY=`echo $SIXRD_PROV | awk -F: '{printf("%d.%d.%d.%d","0x"$19,"0x"$20,"0x"$21,"0x"$22)}'`
#                     echo "SIXRD_ZONE           : $SIXRD_ZONE" >> $IPV6_LOG_FILE
#                     echo "SIXRD_ZONE_LENGTH    : $SIXRD_ZONE_LENGTH" >> $IPV6_LOG_FILE
#                     echo "SIXRD_COMMON_PREFIX4 : $SIXRD_COMMON_PREFIX4" >> $IPV6_LOG_FILE
#                     echo "SIXRD_RELAY          : $SIXRD_RELAY" >> $IPV6_LOG_FILE
#                     sysevent set 6rd_zone $SIXRD_ZONE
#                     sysevent set 6rd_zone_length $SIXRD_ZONE_LENGTH
#                     sysevent set 6rd_common_prefix4 $SIXRD_COMMON_PREFIX4
#                     sysevent set 6rd_relay $SIXRD_RELAY
#                     # And restart 6RD unconditionally (as this DHCPv4 information is fresher than any static or previous configuration)
#                     # TODO do we need to also set ipv6_connection_state ???
#                     echo "$0: got 6RD provisioning information from DHCPv4, trying to start 6RD" >> $IPV6_LOG_FILE
#                     sysevent set 6rd-start
#                    ;;
#                 *)
#                    echo "Unsupported Cisco vendor option (0x${vendorspecific:5:2})" >> $IPV6_LOG_FILE
#                    ;;
#               esac
#               ;;
#            *)
#               echo "Unsupported vendor option (0x${vendorspecific:0:4})" >> $IPV6_LOG_FILE
#               ;;
#         esac
#      fi


      ;;
   lan-status)
       echo "dhcp_lan Triggering RDKB_FIREWALL_RESTART"
       t2CountNotify "SYS_SH_RDKB_FIREWALL_RESTART"
      sysevent set firewall-restart
      ;;
   esac

exit 0
