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

source /etc/utopia/service.d/ulog_functions.sh
PID="($$)"

UDHCPC_PID_FILE=/var/run/udhcpc.pid
UDHCPC_SCRIPT=/etc/utopia/service.d/service_bridge/dhcp_link.sh
RESOLV_CONF="/etc/resolv.conf"
RESOLV_CONF_TMP="/tmp/resolv_tmp.conf"
LOG_FILE="/tmp/udhcp.log"

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   FOO=`utctx_cmd get lan_ifname hostname`
   eval "$FOO"

  if [ -z "$SYSCFG_hostname" ] ; then
     SYSCFG_hostname="Utopia"
  fi
}

#------------------------------------------------------------------
# do_stop_dhcp
#------------------------------------------------------------------
do_stop_dhcp() {
   ulog dhcp_link status "stopping dhcp client on bridge"
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

   if [ ! -f "$UDHCPC_PID_FILE" ] ; then
      ulog dhcp_link status "starting dhcp client on bridge ($WAN_IFNAME)"
      service_init

      udhcpc -S -b -i "$SYSCFG_lan_ifname" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT
   elif [ ! "${UDPCP_PID}" ] ; then
      ulog dhcp_link status "dhcp client `cat $UDHCPC_PID_FILE` died"
      do_stop_dhcp
      ulog dhcp_link status "starting dhcp client on bridge ($SYSCFG_lan_ifname)"
      udhcpc -S -b -i "$SYSCFG_lan_ifname" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT
   else
      ulog dhcp_link status "dhcp client is already active on bridge ($SYSCFG_lan_ifname) as `cat $UDHCPC_PID_FILE`"
   fi
}


#------------------------------------------------------------------
# do_release_dhcp
#------------------------------------------------------------------
do_release_dhcp() {
   ulog dhcp_link status "releasing dhcp lease on bridge"
   service_init
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      kill -SIGUSR2 "`cat $UDHCPC_PID_FILE`"
   fi
   ip -4 addr flush dev "$SYSCFG_lan_ifname"
}
#------------------------------------------------------------------
# do_renew_dhcp
#------------------------------------------------------------------
do_renew_dhcp() {
   ulog dhcp_link status "renewing dhcp lease on bridge"
    if [ -f "$UDHCPC_PID_FILE" ] ; then
        kill -SIGUSR1 "`cat $UDHCPC_PID_FILE`"
    else
        ulog dhcp_link status "restarting dhcp client on bridge"
        udhcpc -S -b -i "$SYSCFG_lan_ifname" -h $SYSCFG_hostname -p $UDHCPC_PID_FILE --arping -s $UDHCPC_SCRIPT
   fi
}

#####################################################################################

[ -z "$1" ] && ulog dhcp_link status "$PID called with no parameters. Ignoring call" && exit 1


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
   dhcp_client-restart)
      do_start_dhcp
      ;;

   dhcp_client-release)
      do_release_dhcp
      ;;

   dhcp_client-renew)
      do_renew_dhcp
      ;;

   ############################################################################################
   # Calls from udhcpc process
   ############################################################################################

   deconfig)
      ulog dhcp_link status "udhcpc $PID - cmd $1 interface $interface ip $ip broadcast $broadcast subnet $subnet router $router" 
      ulog dhcp_link status "$PID bridge dhcp lease has expired"
      rm -f $LOG_FILE
      sysevent set dhcpc_ntp_server1
      sysevent set dhcpc_ntp_server2
      sysevent set dhcpc_ntp_server3
      sysevent set bridge_ipv4_ipaddr 0.0.0.0
      sysevent set bridge_ipv4_subnet 0.0.0.0
      sysevent set bridge_default_router
      sysevent set bridge_dhcp_lease
      sysevent set bridge_dhcp_dns
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

      if [ -n "$lease" ] ; then
         sysevent set bridge_dhcp_lease "$lease" 
      fi
      if [ -n "$subnet" ] ; then
         sysevent set bridge_ipv4_subnet "$subnet" 
      fi

      # did the assigned ip address change
      OLDIP=`/sbin/ip addr show dev "$interface"  | grep "inet " | awk '{split($2,foo, "/"); print(foo[1]);}'`
      if [ "$OLDIP" != "$ip" ] ; then
         RESULT=`arping -q -c 2 -w 3 -D -I "$interface" "$ip"`
         if [ -n "$RESULT" ] &&  [ "0" != "$RESULT" ] ; then
            echo "[utopia][dhcp client script] duplicate address detected $ip on $interface." > /dev/console
            echo "[utopia][dhcp client script] ignoring duplicate ... hoping for the best" > /dev/console
         fi

         # remove the old ip address and put in the new one
         # ip addr flush is too harsh since it also removes ipv6 addrs
         /sbin/ip -4 link set dev "$interface" down
         /sbin/ip -4 addr show dev "$interface" | grep "inet " | awk '{system("/sbin/ip addr del " $2 " dev $interface")}'
         /sbin/ip -4 addr add "$ip""$NETMASK $BROADCAST" dev "$interface" 
         /sbin/ip -4 link set dev "$interface" up
      fi

      # if the gateway router has changed then we need to flush routing cache
      if [ -n "$router" ] ; then
         OLD_DEFAULT_ROUTER=`sysevent get bridge_default_router`
         if [ "$router" != "$OLD_DEFAULT_ROUTER" ] ; then
            while ip -4 route del default ; do
               :
            done
            for i in $router ; do
               ip -4 route add default dev "$interface" via "$i"      
               sysevent set bridge_default_router "$i" 
            done
            ip -4 route flush cache
         fi
      fi

      # initialize ntp server found by dhcp to null
      sysevent set dhcpc_ntp_server1 
      sysevent set dhcpc_ntp_server2 
      sysevent set dhcpc_ntp_server3 

      if [ -n "$domain" ] ; then
         sysevent set dhcp_domain "$domain"
      fi

      # Purge all IPv4 DNS servers from existing resolv.conf file
      # Keep all IPv6 DNS servers
      if [ -f $RESOLV_CONF ] ; then
         /bin/egrep -v '^search |^nameserver +[0123456789]+\.[0123456789]+\.[0123456789]+\.[0123456789]+' $RESOLV_CONF > /tmp/foo.$$
         # mv doesnt work because /etc is read-only so use cat
         cat /tmp/foo.$$ > $RESOLV_CONF
         rm -f /tmp/foo.$$
      else
           # Removing IPV4 DNS server config to retaing XDNS config entry rather than complete empty over writing of resolv.conf file 
           cp $RESOLV_CONF $RESOLV_CONF_TMP
           interface=`sysevent get wan_ifname`
           get_dns_number=`sysevent get ipv4_"${interface}"_dns_number`
           sed -i '/domain/d' "$RESOLV_CONF_TMP"
           sed -i '/nameserver 127.0.0.1/d' "$RESOLV_CONF_TMP"
                if [ -n "$get_dns_number" ]; then
                        echo "Removing old DNS IPV4 SERVER configuration from resolv.conf " >> $LOG_FILE
                        counter=0;
                        while [ $counter -lt "$get_dns_number" ]; do
                        get_old_dns_server=`sysevent get ipv4_"${interface}"_dns_$counter`
                        ipv4_dns_server="nameserver $get_old_dns_server"
                        sed -i "/$ipv4_dns_server/d" "$RESOLV_CONF_TMP"
                        let counter=counter+1
                        done
                fi


           N=""
           while read line; do
           N="${N}$line
"
          done < $RESOLV_CONF_TMP
          echo -n "$N" > "$RESOLV_CONF"
          rm -rf $RESOLV_CONF_TMP
      fi

      if [ -n "$domain" ] ; then
         echo "search $domain" >> $RESOLV_CONF
      fi

      WAN_DNS=
      # if there are any statically provisioned dns servers then add them in
      # to the dns resolver file
      NAMESERVER1=`syscfg get nameserver1`
      NAMESERVER2=`syscfg get nameserver2`
      NAMESERVER3=`syscfg get nameserver3`
      if [ "0.0.0.0" != "$NAMESERVER1" ] && [ -n "$NAMESERVER1" ] ; then
         echo nameserver "$NAMESERVER1" >> $RESOLV_CONF
         WAN_DNS=`echo "$WAN_DNS" "$NAMESERVER1"`
      fi
      if [ "0.0.0.0" != "$NAMESERVER2" ]  && [ -n "$NAMESERVER2" ]; then
         echo nameserver "$NAMESERVER2" >> $RESOLV_CONF
         WAN_DNS=`echo "$WAN_DNS" "$NAMESERVER2"`
      fi
      if [ "0.0.0.0" != "$NAMESERVER3" ]  && [ -n "$NAMESERVER3" ]; then
         echo nameserver "$NAMESERVER3" >> $RESOLV_CONF
         WAN_DNS=`echo "$WAN_DNS" "$NAMESERVER3"`
      fi

      if [ -n "$dns" ] ; then
         WAN_DNS=`echo "$WAN_DNS" "$dns"`
      fi

      # dns nameservers
      for i in $dns ; do
         echo nameserver "$i" >> $RESOLV_CONF
      done
      # and add an entry so that the router can also use itself as dns resolver
#      echo "nameserver 127.0.0.1" >> $RESOLV_CONF
      `sysevent set bridge_dhcp_dns "${WAN_DNS}"`
   
      # ntp servers
      # if any are found then provision sysevent so that they can be found by ntpclient
      NTPSERVER1=
      NTPSERVER2=
      for ii in $ntpsrv ; do
         if [ -z "$NTPSERVER1" ] ; then
            NTPSERVER1=$ii
            `sysevent set dhcpc_ntp_server1 "$NTPSERVER1"`
         elif [ -z "$NTPSERVER2" ] ; then
            NTPSERVER2=$ii
            `sysevent set dhcpc_ntp_server2 "$NTPSERVER2"`
         else
            `sysevent set dhcpc_ntp_server3 "$ii"`
         fi
      done


      sysevent set bridge_ipv4_ipaddr "$ip"
      ;;
   esac

exit 0
