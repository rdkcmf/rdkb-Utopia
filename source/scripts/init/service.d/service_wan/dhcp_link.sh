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
# This script is responsible for bringing up connectivity with 
# the ISP using dhcp.
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
PID="($$)"

BIN=ti_udhcpc

UDHCPC_PID_FILE=/var/run/eRT_$BIN.pid
UDHCPC_SCRIPT=/etc/utopia/service.d/service_wan/dhcp_link.sh
#UDHCPC_OPTIONS="-O vendorspecific -V GW-iRouter"
RESOLV_CONF="/etc/resolv.conf"
RESOLV_CONF_TMP="/tmp/resolv_tmp.conf"
LOG_FILE="/tmp/udhcp.log"
IPV6_LOG_FILE="/var/log/ipv6.log"
PLUGIN="/lib/libert_dhcpv4_plugin.so"


WAN_IFNAME=`sysevent get wan_ifname`
if [ "1" = "`syscfg get byoi_enabled`" ] && [ "primary" = "`sysevent get current_hsd_mode`" ] ; then
   WAN_PROTOCOL=`syscfg get primary_wan_proto`
else
   WAN_PROTOCOL=`syscfg get wan_proto`
fi

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   FOO=`utctx_cmd get hostname`
   eval $FOO
  
   if [ -z "$SYSCFG_hostname" ] ; then
     SYSCFG_hostname="Utopia"
   fi 
  UDHCPC_OPTIONS=`syscfg get udhcpc_options_wan`
  ulog dhcp_link status "initing dhcp UDHCPC_OPTIONS: ${UDHCPC_OPTIONS}"
}

#------------------------------------------------------------------
# do_stop_dhcp
#------------------------------------------------------------------
do_stop_dhcp() {
   ulog dhcp_link status "stopping dhcp client on wan"
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      kill -USR2 `cat $UDHCPC_PID_FILE` && kill `cat $UDHCPC_PID_FILE`
      rm -f $UDHCPC_PID_FILE
   else
      PIDS=`pidof $BIN`
      for curpid in $PIDS 
      do
         case "`cat /proc/$curpid/cmdline`" in 
            *"$WAN_IFNAME"*) UDHCP_PID=$curpid;
            break;
            ;;
         esac
      done
      if [ x != x$UDHCP_PID ]; then 
          kill -USR2 $UDHCP_PID && kill $UDHCP_PID
      fi
   fi
   rm -f $LOG_FILE
}

#------------------------------------------------------------------
# do_start_dhcp
#------------------------------------------------------------------
do_start_dhcp() {
    ulog dhcp_link status "starting dhcp client on wan"    

   if [ "pppoe" != "$WAN_PROTOCOL" ] ; then

      PIDS=`pidof $BIN`
      for curpid in $PIDS 
      do
         case "`cat /proc/$curpid/cmdline`" in 
            *"$WAN_IFNAME"*) UDHCP_PID=$curpid;
            break;
            ;;
         esac
      done

      service_init
      
      if [ ! -f "$UDHCPC_PID_FILE" ] ; then
         ulog dhcp_link status "starting dhcp client on wan ($WAN_IFNAME)"
         $BIN -plugin $PLUGIN -i $WAN_IFNAME -H $SYSCFG_hostname -p $UDHCPC_PID_FILE -s $UDHCPC_SCRIPT -B
      elif [ ! "${UDHCP_PID}" ] ; then
         ulog dhcp_link status "dhcp client `cat $UDHCPC_PID_FILE` died"
         do_stop_dhcp
         ulog dhcp_link status "starting dhcp client on wan ($WAN_IFNAME)"
         $BIN -plugin $PLUGIN -i $WAN_IFNAME -H $SYSCFG_hostname -p $UDHCPC_PID_FILE -s $UDHCPC_SCRIPT -B
      else
         ulog dhcp_link status "dhcp client is already active on wan ($WAN_IFNAME) as `cat $UDHCPC_PID_FILE`"
      fi
   fi
}


#------------------------------------------------------------------
# do_release_dhcp
#------------------------------------------------------------------
do_release_dhcp() {
   ulog dhcp_link status "releasing dhcp lease on wan"
      WAN_STATE=`sysevent get current_wan_state`
      if [ "$WAN_STATE" = "up" ] ; then
         sysevent set current_wan_state administrative_down
      fi
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      kill -SIGUSR2 `cat $UDHCPC_PID_FILE`
      ip -4 addr flush dev $WAN_IFNAME
   fi
}

#------------------------------------------------------------------
# do_renew_dhcp
#------------------------------------------------------------------
do_renew_dhcp() {
   ulog dhcp_link status "renewing dhcp lease on wan"
    if [ -f "$UDHCPC_PID_FILE" ] ; then
        kill -SIGUSR1 `cat $UDHCPC_PID_FILE`
        WAN_STATE=`sysevent get current_wan_state`
        if [ "$WAN_STATE" = "administrative_down" ] ; then
           sysevent set current_wan_state up
           sysevent set wan_start_time $(cut -d. -f1 /proc/uptime)
        fi
    else
        ulog dhcp_link status "restarting dhcp client on wan"
        service_init
        $BIN -plugin $PLUGIN -i $WAN_IFNAME -H $SYSCFG_hostname -p $UDHCPC_PID_FILE -s $UDHCPC_SCRIPT -B
   fi
}

# -----------------------------------------------------------------
# update_dns_policy_route
# -----------------------------------------------------------------
update_dns_policy_route() {
    iface=$1
    servs="$2"
    ert_if=$(syscfg get wan_physical_ifname)

    echo "==@== updating policy route for DNS Servers: iface $iface servs $servs" >> /tmp/dns.policy.route.log

    # we have only one policy route table "erouter" (for device erouter0)
    if [ "$iface" != "$ert_if" ]; then
        echo "==@== only support $ert_if" >> /tmp/dns.policy.route.log
        return
    fi

    ip rule show >> /tmp/dns.policy.route.log

    # remove old policy routes for DNS Server IPs
    old_servs=$(awk '/^nameserver[ ]+[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/ {print $2}' $RESOLV_CONF)
    for s in $old_servs ; do
        if [ "$s" == "127.0.0.1" ]; then
            continue
        fi

        ip rule show | grep "from all to $s lookup erouter"
        if [ $? -eq 0 ]; then
            echo "==@== removing 'to $s' from policy route" >> /tmp/dns.policy.route.log
            ip rule del to $s table erouter
        fi
    done

    ip rule show >> /tmp/dns.policy.route.log

    # add new policy routes
    for s in $servs; do
        echo "==@== adding 'to $s' from policy route" >> /tmp/dns.policy.route.log
        ip rule add to $s table erouter
    done

    ip rule show >> /tmp/dns.policy.route.log
}

CURRENT_STATE=`sysevent get current_ipv4_link_state`
DESIRED_STATE=`sysevent get desired_ipv4_link_state`
PHYLINK_STATE=`sysevent get phylink_wan_state`

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

   phylink_wan_state)
         ulog dhcp_link status "$PID physical link is $PHYLINK_STATE"
         if [ "up" != "$PHYLINK_STATE" ] ; then
            if [ "up" = "$CURRENT_STATE" ] ; then
               ulog dhcp_link status "$PID physical link is down. Setting link down."
               #sysevent set current_ipv4_link_state down
               exit 0
            else
               ulog dhcp_link status "$PID physical link is down. Link is already down."
               exit 0
            fi
         else
            if [ "up" = "CURRENT_STATE" ] ; then
               ulog dhcp_link status "$PID physical link is up. Link is already up."
            else
               if [ "up" = "$DESIRED_STATE" ] ; then
                  if [ -f "$DHCPC_PID_FILE" ] ; then
                     ulog dhcp_link status "$PID dhcp client is active"
                     IP=`sysevent get ipv4_wan_ipaddr`
                     if [ -n "$IP" ] ; then
                        if [ "0.0.0.0" = "$IP" ] ; then
                           ulog dhcp_link status "$PID dhcp client has not acquired an ip address yet. No change to link state"
                           exit 0
                        else
                           ulog dhcp_link status "$PID dhcp client has acquired an ip address ($IP). Setting link state up"
                           sysevent set current_ipv4_link_state up
                           exit 0
                        fi
                     else
                        ulog dhcp_link status "$PID dhcp client has no ip address yet. No change to link state"
                        exit 0
                     fi
                  else
                     ulog dhcp_link status "$PID starting dhcp client"
                     do_start_dhcp
                     exit 0
                  fi
               else
                  ulog dhcp_link status "$PID physical link is up, but desired link state is down."
                  exit 0;
               fi
            fi
         fi
         ;;
      desired_ipv4_link_state)
         ulog dhcp_link status "$PID $DESIRED_STATE requested"

         if [ "up" = "$DESIRED_STATE" ] ; then
            if [ "down" = "$PHYLINK_STATE" ] ; then
               ulog dhcp_link status "$PID up requested but physical link is still down"
               exit 0;
            fi

            do_start_dhcp
            exit 0
         elif [ "down" = "$DESIRED_STATE" ] ; then
            do_stop_dhcp
            sysevent set dhcpc_ntp_server1
            sysevent set dhcpc_ntp_server2
            sysevent set dhcpc_ntp_server3
            sysevent set ipv4_wan_ipaddr 0.0.0.0
            sysevent set ipv4_wan_subnet 0.0.0.0
            sysevent set default_router
            sysevent set wan_dhcp_lease
            sysevent set wan_dhcp_dns
            sysevent set dhcp_domain
            sysevent set current_ipv4_link_state down
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
      if [ "up" = "$DESIRED_STATE" ] && [ "up" = "$CURRENT_STATE" ] ; then
         ulog dhcp_link status "$PID wan dhcp lease has expired"
         rm -f $LOG_FILE
         sysevent set current_ipv4_link_state down
         sysevent set dhcpc_ntp_server1
         sysevent set dhcpc_ntp_server2
         sysevent set dhcpc_ntp_server3
         sysevent set ipv4_wan_ipaddr 0.0.0.0
         sysevent set ipv4_wan_subnet 0.0.0.0
         sysevent set default_router
         sysevent set wan_dhcp_lease
         sysevent set dhcp_domain
         sysevent set wan_dhcp_dns
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
         sysevent set wan_dhcp_lease $lease 
      fi
      if [ -n "$subnet" ] ; then
         sysevent set ipv4_wan_subnet $subnet 
      fi

      # did the assigned ip address change
      OLDIP=`ip addr show dev $interface label $interface | grep "inet " | awk '{split($2,foo, "/"); print(foo[1]);}'`
      if [ "$OLDIP" != "$ip" ] ; then
         RESULT=`arping -q -c 2 -w 3 -D -I $interface $ip`
         if [ -n "$RESULT" ] &&  [ "0" != "$RESULT" ] ; then
            echo "[utopia][dhcp client script] duplicate address detected $ip on $interface." > /dev/console
            echo "[utopia][dhcp client script] ignoring duplicate ... hoping for the best" > /dev/console
         fi

         # remove "erouter" routing table for packet from old erouter WAN IP
         if [ -n "$OLDIP" ]; then
            ip -4 rule del from $OLDIP lookup erouter
            ip -4 rule del from $OLDIP lookup all_lans
         fi

         # remove the old ip address and put in the new one
         # ip addr flush is too harsh since it also removes ipv6 addrs
         ip -4 link set dev $interface down
         ip -4 addr show dev $interface label $interface | grep "inet " | awk '{system("ip addr del " $2 " dev $interface")}'
         ip -4 addr add $ip$NETMASK $BROADCAST dev $interface 
         ip -4 link set dev $interface up

         # use "erouter" routing table for packet from erouter WAN IP
         ip -4 rule add from $ip lookup erouter
         ip -4 rule add from $ip lookup all_lans
      fi

      # if the gateway router has changed then we need to flush routing cache
      if [ -n "$router" ] ; then
         OLD_DEFAULT_ROUTER=`sysevent get default_router`
         if [ "$router" != "$OLD_DEFAULT_ROUTER" ] ; then
#            while ip -4 route del default dev $interface ; do
            while ip -4 route del table erouter default ; do
               :
            done
            for i in $router ; do
               ip -4 route add table erouter default dev $interface via $i      
               sysevent set default_router $i 
            done
            ip -4 route flush cache
         fi
      fi

      # initialize ntp server found by dhcp to null
      sysevent set dhcpc_ntp_server1 
      sysevent set dhcpc_ntp_server2 
      sysevent set dhcpc_ntp_server3 

      # the dhcp server needs to be restarted if domain or dns servers changes
      RESTART_DHCP_SERVER=0

      if [ -n "$domain" ] ; then
         PROPAGATE=`syscfg get dhcp_server_propagate_wan_domain`
         if [ "1" = "$PROPAGATE" ] ; then
            OLD_DOMAIN=`sysevent get dhcp_domain`
            if [ "$OLD_DOMAIN" != "$domain" ]; then
               RESTART_DHCP_SERVER=1
            fi
         fi
         sysevent set dhcp_domain $domain
      fi

      # update resolv.conf only when the wan protocol is DHCP or Static;
      #  otherwise, let pppd do it.
      if [ "dhcp" = "$WAN_PROTOCOL" ] || [ "static" = "$WAN_PROTOCOL" ] ; then
          # must be invoke before update $RESOLV_CONF
          update_dns_policy_route $interface "$dns"

         # Purge all IPv4 DNS servers from existing resolv.conf file
         # Keep all IPv6 DNS servers
         if [ -f $RESOLV_CONF ] ; then
            /bin/egrep -v '^search |^nameserver +[0123456789]+\.[0123456789]+\.[0123456789]+\.[0123456789]+|^#' $RESOLV_CONF > /tmp/foo.$$
            # mv doesnt work because /etc is read-only so use cat
            cat /tmp/foo.$$ > $RESOLV_CONF
            rm -f /tmp/foo.$$
         else
           # Removing IPV4 DNS server config to retaing XDNS config entry rather than complete empty over writing of resolv.conf file 
	   cp $RESOLV_CONF $RESOLV_CONF_TMP 
	   interface=`sysevent get wan_ifname`	
	   get_dns_number=`sysevent get ipv4_${interface}_dns_number`
           sed -i '/domain/d' "$RESOLV_CONF_TMP"
           sed -i '/nameserver 127.0.0.1/d' "$RESOLV_CONF_TMP"
           	if [ -n "$get_dns_number" ]; then
        		echo "Removing old DNS IPV4 SERVER configuration from resolv.conf " >> $LOG_FILE
        		counter=0;
        		while [ $counter -lt $get_dns_number ]; do
                	get_old_dns_server=`sysevent get ipv4_${interface}_dns_$counter`
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
         STATIC_NAMESERVER_ENABLED=`syscfg get staticdns_enable`
         if [ "$STATIC_NAMESERVER_ENABLED" = "1" ] ; then
             NAMESERVER1=`syscfg get nameserver1`
             NAMESERVER2=`syscfg get nameserver2`
             NAMESERVER3=`syscfg get nameserver3`
             if [ "0.0.0.0" != "$NAMESERVER1" ] && [ -n "$NAMESERVER1" ] ; then
                 echo nameserver $NAMESERVER1 >> $RESOLV_CONF
                 WAN_DNS=`echo $WAN_DNS $NAMESERVER1`
             fi
             if [ "0.0.0.0" != "$NAMESERVER2" ]  && [ -n "$NAMESERVER2" ]; then
                 echo nameserver $NAMESERVER2 >> $RESOLV_CONF
                 WAN_DNS=`echo $WAN_DNS $NAMESERVER2`
             fi
             if [ "0.0.0.0" != "$NAMESERVER3" ]  && [ -n "$NAMESERVER3" ]; then
                 echo nameserver $NAMESERVER3 >> $RESOLV_CONF
                 WAN_DNS=`echo $WAN_DNS $NAMESERVER3`
             fi
         elif [ -n "$dns" ] ; then
             WAN_DNS=`echo $WAN_DNS $dns`
         fi

         # dns nameservers
         PROPAGATE=`syscfg get dhcp_server_propagate_wan_nameserver`
         for i in $dns ; do
             if [ "$STATIC_NAMESERVER_ENABLED" != "1" ] ; then
                 echo nameserver $i >> $RESOLV_CONF
             fi
             if [ "1" = "$PROPAGATE" ] ; then
                 TEST_NS=` sysevent get wan_dhcp_dns | grep $i`
                 if [ -z "$TEST_NS" ] ; then
                     RESTART_DHCP_SERVER=1
                 fi
             fi
         done
         # and add an entry so that the router can also use itself as dns resolver
         echo "nameserver 127.0.0.1" >> $RESOLV_CONF
         `sysevent set wan_dhcp_dns "${WAN_DNS}"`
      fi

      WAN_STATIC_DOMAIN=`syscfg get wan_domain`
      if [ -n "$WAN_STATIC_DOMAIN" ]; then
          echo "#overwrite domain name start" >> $RESOLV_CONF
          echo "search $WAN_STATIC_DOMAIN" >> $RESOLV_CONF
          echo "#overwrite domain name end" >> $RESOLV_CONF
          #sysevent set dhcp_domain $WAN_STATIC_DOMAIN
      fi
   
      # ntp servers
      # if any are found then provision sysevent so that they can be found by ntpclient
      NTPSERVER1=
      NTPSERVER2=
      for ii in $ntpsrv ; do
         if [ -z "$NTPSERVER1" ] ; then
            NTPSERVER1=$ii
            `sysevent set dhcpc_ntp_server1 $NTPSERVER1`
         elif [ -z "$NTPSERVER2" ] ; then
            NTPSERVER2=$ii
            `sysevent set dhcpc_ntp_server2 $NTPSERVER2`
         else
            `sysevent set dhcpc_ntp_server3 $ii`
         fi
      done


      sysevent set ipv4_wan_ipaddr $ip
      #if [ "1" = "$RESTART_DHCP_SERVER" ] ; then
      #   sysevent set dhcp_server-restart
      #fi

      # for renew we are already up, so no need to set current_ipv4_link_state
      LINK_STATE=`sysevent get current_ipv4_link_state`
      if [ "up" = "$LINK_STATE" ] ; then
         if [ "1" = "$RESTART_DHCP_SERVER" ] ; then
             touch /var/tmp/lan_not_restart
             sysevent set dhcp_server-restart
         fi
      else
         # lan dhcp slow start is decided by current_wan_ipaddr and it is modified on current_ipv4_link_state event
         ulog dhcp_link status "$PID setting current_ipv4_link_state to up"
         sysevent set current_ipv4_link_state up
      fi

      
      

      # Is there any DHCP vendor specific extension?
      # Such as 0009 12 14:52:20:01:06:F8:14:68:F0:00:00:00:00:00:00:00:00:00:C0:A8:06:01 (for Cisco 6RD provisionning)
      if [ ! -z "$vendorspecific" ] ; then
         case "${vendorspecific:0:4}" in
            0009)    # Cisco Enterprise Number
               case "${vendorspecific:5:2}" in
                  12)     # 6RD provisioning
                     SIXRD_PROV=${vendorspecific:8}
                     echo "Found 6RD provisioning: $SIXRD_PROV" >> $IPV6_LOG_FILE
                     SIXRD_COMMON_PREFIX4=`echo $SIXRD_PROV | awk -F: '{printf("%d","0x"$1)}'`
                     SIXRD_ZONE_LENGTH=`echo $SIXRD_PROV | awk -F: '{printf("%d","0x"$2)}'`
                     SIXRD_ZONE=`echo $SIXRD_PROV | awk -F: '{printf("%s%s:%s%s:%s%s:%s%s:%s%s:%s%s:%s%s:%s%s",$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18)}'`
                     SIXRD_RELAY=`echo $SIXRD_PROV | awk -F: '{printf("%d.%d.%d.%d","0x"$19,"0x"$20,"0x"$21,"0x"$22)}'`
                     echo "SIXRD_ZONE           : $SIXRD_ZONE" >> $IPV6_LOG_FILE
                     echo "SIXRD_ZONE_LENGTH    : $SIXRD_ZONE_LENGTH" >> $IPV6_LOG_FILE
                     echo "SIXRD_COMMON_PREFIX4 : $SIXRD_COMMON_PREFIX4" >> $IPV6_LOG_FILE
                     echo "SIXRD_RELAY          : $SIXRD_RELAY" >> $IPV6_LOG_FILE
                     sysevent set 6rd_zone $SIXRD_ZONE
                     sysevent set 6rd_zone_length $SIXRD_ZONE_LENGTH
                     sysevent set 6rd_common_prefix4 $SIXRD_COMMON_PREFIX4
                     sysevent set 6rd_relay $SIXRD_RELAY
                     # And restart 6RD unconditionally (as this DHCPv4 information is fresher than any static or previous configuration)
                     # TODO do we need to also set ipv6_connection_state ???
                     echo "$0: got 6RD provisioning information from DHCPv4, trying to start 6RD" >> $IPV6_LOG_FILE
                     sysevent set 6rd-start
                    ;;
                 *)
                    echo "Unsupported Cisco vendor option (0x${vendorspecific:5:2})" >> $IPV6_LOG_FILE
                    ;;
               esac
               ;;
            *)
               echo "Unsupported vendor option (0x${vendorspecific:0:4})" >> $IPV6_LOG_FILE
               ;;
         esac
      fi


      ;;
   esac

exit 0
