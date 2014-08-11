#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2009,2010 by Cisco Systems, Inc. All Rights Reserved.
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
# This script is used to start the routing daemons (zebra and ripd)
# $1 is the calling event (current_wan_state  current_lan_state  ipv6_prefix)
#------------------------------------------------------------------

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="routed"

ZEBRA_PID_FILE=/var/zebra.pid
RIPD_PID_FILE=/var/ripd.pid
RIPNGD_PID_FILE=/var/ripngd.pid

ZEBRA_CONF_FILE=/etc/zebra.conf
RIPD_CONF_FILE=/etc/ripd.conf
RIPNGD_CONF_FILE=/etc/ripngd.conf
#ZEBRA_BIN_NAME=/usr/sbin/zebra
#RIPD_BIN_NAME=/usr/sbin/ripd
ZEBRA_BIN_NAME=zebra
RIPD_BIN_NAME=ripd
RIPNGD_BIN_NAME=ripngd

QUAGGA_UNAME=quagga
QUAGGA_GROUP=quagga

#----------------------------------------------------------------------
# utctx_batch_get
# Takes one parameter which is the utctx_cmd argument list
#----------------------------------------------------------------------
utctx_batch_get() 
{
    SYSCFG_FAILED='false'
    eval `utctx_cmd get $1`
    if [ $SYSCFG_FAILED = 'true' ] ; then
        echo "Call failed"
        return 1
    fi
}

#----------------------------------------------------------------------
# get_one_static_route_group
# Takes one parameter which is the namespace of a static route group
# Sets DEST, MASK, INTERFACE, GW
#----------------------------------------------------------------------
get_one_static_route_group()
{
   utctx_batch_get "$1"
  
   eval NS='$'SYSCFG_$1
      ARGS="\
      $NS::dest \
      $NS::interface \
      $NS::netmask \
      $NS::gw"

   # Do a batch get
   utctx_batch_get "$ARGS"
   if [ "${?}" -ne "0" ] ; then
      return 1
   fi

   eval `echo DEST='$'SYSCFG_${NS}_dest`
   eval `echo MASK='$'SYSCFG_${NS}_netmask`
   eval `echo INTERFACE='$'SYSCFG_${NS}_interface`
   eval `echo GW='$'SYSCFG_${NS}_gw`
}

#----------------------------------------------------------------------
# make_zebra_conf_file
#----------------------------------------------------------------------
make_zebra_conf_file() {
   echo "hostname zebra" >> $ZEBRA_CONF_FILE
   echo "!password zebra" >> $ZEBRA_CONF_FILE
   echo "!enable password admin" >> $ZEBRA_CONF_FILE
   echo "!log stdout" >> $ZEBRA_CONF_FILE
   echo "log syslog" >> $ZEBRA_CONF_FILE
   echo "log file /var/log/zebra.log" >> $ZEBRA_CONF_FILE

   #RT use local in policy routing route list
   echo "table 255" >> $ZEBRA_CONF_FILE

   # default route
   # -------------
   # use ppp0 if it exists
# MENR   echo "ip route 0.0.0.0   0.0.0.0   ppp0" >> $ZEBRA_CONF_FILE
# MENR   DEFAULT_ROUTER=`sysevent get default_router`
# MENR   if [ "" != "$DEFAULT_ROUTER" ] && [ "0.0.0.0" != "$DEFAULT_ROUTER" ] ; then
# MENR      # otherwise use default route with metric of 2
# MENR      echo "ip route 0.0.0.0   0.0.0.0   $DEFAULT_ROUTER " >> $ZEBRA_CONF_FILE
# MENR   fi

   # static routes
   # -------------
   if [ "" != "$SYSCFG_StaticRouteCount" ] && [ "0" != "$SYSCFG_StaticRouteCount" ] ; then
      WAN_IFNAME=`sysevent get current_wan_ifname`

      for ct in `seq 1 $SYSCFG_StaticRouteCount`
      do
         SR=StaticRoute_${ct}
         get_one_static_route_group $SR
         if [ "${?}" -ne "0" ] ; then
            ulog routed status "Failure in extracting static route info for $SR"
         else
            if [ "" = "$DEST" ] || [ "" = "$MASK" ] || [ "" = "$INTERFACE" ] ; then
               ulog routed status "Bad parameter for $NS"
            elif [ "lan" = "$INTERFACE" ] ; then
               if  [ "" = "$GW" ] ; then
                  ulog routed status "Bad parameter for $NS on $INTERFACE"
               else
                  echo "ip route $DEST $MASK $GW" >> $ZEBRA_CONF_FILE 
               fi
            elif [ "wan" = "$INTERFACE" ] ; then
               if [ "" = "$GW" ] || [ "0.0.0.0" = "$GW" ] ; then
                  echo "ip route $DEST $MASK $WAN_IFNAME" >> $ZEBRA_CONF_FILE 
               else
                  echo "ip route $DEST $MASK $GW" >> $ZEBRA_CONF_FILE 
               fi
            fi
         fi
      done
   fi

   # router advertisement
   # --------------------
   if [ "" != "$SYSCFG_router_adv_enable" ] && [ "0" != "$SYSCFG_router_adv_enable" ] ; then
      PREFIX=`sysevent get ipv6_prefix`
      OLD_PREFIX=`sysevent get previous_ipv6_prefix`
      CURRENT_LAN_IPV6ADDRESS=`sysevent get current_lan_ipv6address`
echo "# Based on prefix=$PREFIX, old_previous=$OLD_PREFIX, LAN IPv6 address=$CURRENT_LAN_IPV6ADDRESS" >> $ZEBRA_CONF_FILE
      if [ "" != "$PREFIX" -o "" != "$OLD_PREFIX" ] 
      then
         echo "interface $SYSCFG_lan_ifname" >> $ZEBRA_CONF_FILE
         echo "   no ipv6 nd suppress-ra" >> $ZEBRA_CONF_FILE
	 if [ ! -z "$PREFIX" ]
	 then
           echo "   ipv6 nd prefix $PREFIX 300 300" >> $ZEBRA_CONF_FILE
 	 fi
	 if [ ! -z "$OLD_PREFIX" ] # Say old IPv6 prefix is no more preferred
	 then
           echo "   ipv6 nd prefix $OLD_PREFIX 300 0" >> $ZEBRA_CONF_FILE
 	 fi
         echo "   ipv6 nd ra-interval 60" >> $ZEBRA_CONF_FILE
         echo "   ipv6 nd ra-lifetime 180" >> $ZEBRA_CONF_FILE
	 #if [ "$SYSCFG_dhcpv6s_enable" = "1" ]

	 if [ "`syscfg get router_managed_flag`" = "1" ]
	 then
           echo "   ipv6 nd managed-config-flag" >> $ZEBRA_CONF_FILE
         fi

	 if [ "`syscfg get router_other_flag`" = "1" ]
         then
           echo "   ipv6 nd other-config-flag" >> $ZEBRA_CONF_FILE
	 fi

         echo "   ipv6 nd router-preference medium" >> $ZEBRA_CONF_FILE
         # Use RFC 5006 and send all recursive DNS servers than we know of (including ourself)
         if [ ! -z "$CURRENT_LAN_IPV6ADDRESS" ]
         then
            echo "   ipv6 nd rdnss $CURRENT_LAN_IPV6ADDRESS 300" >> $ZEBRA_CONF_FILE
         fi
         for rdnss in `sysevent get ipv6_nameserver`
         do
            echo "   ipv6 nd rdnss $rdnss 300" >> $ZEBRA_CONF_FILE
         done
      fi
   fi

   # IRDP - IPv4 Router Advertisement (RFC1256)
   if [ "" != "$SYSCFG_router_adv_enable" ] && [ "0" != "$SYSCFG_router_adv_enable" ] ; then
      echo "interface $SYSCFG_lan_ifname" >> $ZEBRA_CONF_FILE
      echo "   ip irdp multicast" >> $ZEBRA_CONF_FILE
   fi
}

#----------------------------------------------------------------------
# make_ripd_conf_file
#----------------------------------------------------------------------
make_ripd_conf_file() {
   WAN_IFNAME=`sysevent get current_wan_ifname`

   echo "router rip" >> $RIPD_CONF_FILE

   # figure out whether rip is enabled on the wan/lan or both
   if [ "" = "$SYSCFG_rip_interface_wan" ] || [ "0" = "$SYSCFG_rip_interface_wan" ] ; then
      RIP_WAN_PREFIX="!"
   else
      RIP_WAN_PREFIX=
   fi
   if [ "" = "$SYSCFG_rip_interface_lan" ] || [ "0" = "$SYSCFG_rip_interface_lan" ] ; then
      RIP_LAN_PREFIX="!"
   else
      RIP_LAN_PREFIX=
   fi

   echo "version 2" >> $RIPD_CONF_FILE
   echo "redistribute kernel" >> $RIPD_CONF_FILE
   echo "redistribute static" >> $RIPD_CONF_FILE
   echo "${RIP_WAN_PREFIX} network $WAN_IFNAME" >> $RIPD_CONF_FILE
   echo "network $SYSCFG_lan_ifname" >> $RIPD_CONF_FILE
   echo "!password admin" >> $RIPD_CONF_FILE
   echo "!enable password admin" >> $RIPD_CONF_FILE
   echo "!log file /tmp/ripd.log" >> $RIPD_CONF_FILE
   echo "!debug rip packet" >> $RIPD_CONF_FILE
   echo "!debug rip events" >> $RIPD_CONF_FILE

   # if there is md5 authentication
   if [ "" != "$SYSCFG_rip_md5_passwd" ] ; then
      echo "key chain utopia" >> $RIPD_CONF_FILE
      echo "  key 1" >> $RIPD_CONF_FILE
      echo "     key-string $SYSCFG_rip_md5_passwd" >> $RIPD_CONF_FILE
   fi

   SPLIT_HORIZON=`syscfg get rip_no_split_horizon`
   RIP_WAN_SEND=`syscfg get rip_send_wan`
   RIP_WAN_RECV=`syscfg get rip_recv_wan`
   RIP_LAN_SEND=`syscfg get rip_send_lan`
   RIP_LAN_RECV=`syscfg get rip_recv_lan`
      
   echo "${RIP_WAN_PREFIX} interface $WAN_IFNAME" >> $RIPD_CONF_FILE
   # the next 2 statements are only required for rip version 1 support
   # echo "   ${RIP_WAN_PREFIX} ip rip send version 1 2" >> $RIPD_CONF_FILE 
   # echo "   ${RIP_WAN_PREFIX} ip rip receive version 1 2" >> $RIPD_CONF_FILE 
  
   if [ "1" = "$RIP_WAN_SEND" ] ; then
     echo "   ${RIP_WAN_PREFIX} ip rip send version 1 2" >> $RIPD_CONF_FILE 
   else     
     echo "   ${RIP_WAN_PREFIX} no ip rip send version 1 2" >> $RIPD_CONF_FILE 
   fi

   if [ "1" = "$RIP_WAN_RECV" ] ; then
     echo "   ${RIP_WAN_PREFIX} ip rip receive version 1 2" >> $RIPD_CONF_FILE 
   else     
     echo "   ${RIP_WAN_PREFIX} no ip rip receive version 1 2" >> $RIPD_CONF_FILE 
   fi

   if [ "" != "$SYSCFG_rip_md5_passwd" ] ; then
      echo "   ${RIP_WAN_PREFIX} ip rip authentication mode md5" >> $RIPD_CONF_FILE
      echo "   ${RIP_WAN_PREFIX} ip rip authentication key-chain utopia" >> $RIPD_CONF_FILE
   else
      if [ "" != "$SYSCFG_rip_text_passwd" ]; then
         echo "   ${RIP_WAN_PREFIX} ip rip authentication string $SYSCFG_rip_text_passwd" >> $RIPD_CONF_FILE
         echo "   ${RIP_WAN_PREFIX} ip rip authentication mode text" >> $RIPD_CONF_FILE
      else
        echo "   ${RIP_WAN_PREFIX} no ip rip authentication mode md5" >> $RIPD_CONF_FILE
        echo "   ${RIP_WAN_PREFIX} no ip rip authentication mode text" >> $RIPD_CONF_FILE
      fi
   fi
   if [ "1" = "$SPLIT_HORIZON" ] ; then
      echo "   ${RIP_WAN_PREFIX} no ip rip split-horizon" >> $RIPD_CONF_FILE
   else
      echo "   ${RIP_WAN_PREFIX} ip rip split-horizon" >> $RIPD_CONF_FILE
   fi

   echo "${RIP_LAN_PREFIX} interface $SYSCFG_lan_ifname" >> $RIPD_CONF_FILE
   # the next 2 statements are only required for rip version 1 support
   # echo "   ${RIP_WAN_PREFIX} ip rip send version 1 2" >> $RIPD_CONF_FILE 
   # echo "   ${RIP_WAN_PREFIX} ip rip receive version 1 2" >> $RIPD_CONF_FILE 

   if [ "1" = "$RIP_LAN_SEND" ] ; then
     echo "   ${RIP_LAN_PREFIX} ip rip send version 1 2" >> $RIPD_CONF_FILE 
   else     
     echo "   ${RIP_LAN_PREFIX} no ip rip send version 1 2" >> $RIPD_CONF_FILE 
   fi

   if [ "1" = "$RIP_LAN_RECV" ] ; then
     echo "   ${RIP_LAN_PREFIX} ip rip receive version 1 2" >> $RIPD_CONF_FILE 
   else     
     echo "   ${RIP_LAN_PREFIX} no ip rip receive version 1 2" >> $RIPD_CONF_FILE 
   fi
   
   echo "   ${RIP_LAN_PREFIX} no ip rip authentication mode text" >> $RIPD_CONF_FILE
   echo "   ${RIP_LAN_PREFIX} no ip rip authentication mode md5" >> $RIPD_CONF_FILE
   if [ "1" = "$SPLIT_HORIZON" ] ; then
      echo "   ${RIP_LAN_PREFIX} no ip rip split-horizon" >> $RIPD_CONF_FILE
   else
      echo "   ${RIP_LAN_PREFIX} ip rip split-horizon" >> $RIPD_CONF_FILE
   fi
#   echo "!   interface-passive $SYSCFG_lan_ifname" >> $RIPD_CONF_FILE
}

#----------------------------------------------------------------------
# make_ripngd_conf_file
#----------------------------------------------------------------------
make_ripngd_conf_file() {
   WAN_IFNAME=`sysevent get current_wan_ifname`

   echo "hostname ripng" >> $RIPNGD_CONF_FILE
   echo "password admin" >> $RIPNGD_CONF_FILE
   echo "!enable password admin" >> $RIPNGD_CONF_FILE
   echo "!log file /tmp/ripngd.log" >> $RIPNGD_CONF_FILE

   echo "router ripng" >> $RIPNGD_CONF_FILE

   echo "network $WAN_IFNAME" >> $RIPNGD_CONF_FILE
   echo "network $SYSCFG_lan_ifname" >> $RIPNGD_CONF_FILE

   echo "route $IPV6_PREFIX" >> $RIPNGD_CONF_FILE
}

#----------------------------------------------------------------------
# do_stop_zebra
#----------------------------------------------------------------------
do_stop_zebra() {
   if [ -f "$ZEBRA_PID_FILE" ] ; then
      # echo "[utopia] Stopping routing daemon" > /dev/console
      kill -TERM `cat $ZEBRA_PID_FILE`
      rm -f $ZEBRA_PID_FILE
   fi
   echo -n > $ZEBRA_CONF_FILE
   ulog routed status "zebra stopped"
}

#----------------------------------------------------------------------
# do_start_zebra
#----------------------------------------------------------------------
do_start_zebra() {
   if [ -f "$ZEBRA_PID_FILE" ] ; then
      # zebra doesn't support SIGHUP for reloading its conf file
      # kill -HUP `cat $ZEBRA_PID_FILE` 
      do_stop_zebra
   fi

   make_zebra_conf_file
   touch $ZEBRA_PID_FILE
   chown $QUAGGA_UNAME:$QUAGGA_GROUP $ZEBRA_PID_FILE
   $ZEBRA_BIN_NAME -d -f $ZEBRA_CONF_FILE -u root
   ulog routed status "zebra started"
}

#----------------------------------------------------------------------
# do_stop_ripd
#----------------------------------------------------------------------
do_stop_ripd() {
   if [ -f "$RIPD_PID_FILE" ] ; then
      kill -TERM `cat $RIPD_PID_FILE`
      rm -f $RIPD_PID_FILE
   fi
   #echo -n > $RIPD_CONF_FILE
   ulog rip status "ripd stopped"
}

#----------------------------------------------------------------------
# do_start_ripd
#----------------------------------------------------------------------
do_start_ripd() {
   if [ -f "$RIPD_PID_FILE" ] ; then
      # ripd does support SIGHUP for rereading the conf file
      # but we are going to restart it anyways
      do_stop_ripd
   #else
      #Yan>We make conf file only when it doesn't exist.
      #ripd.conf should be charged by middle layer routing/rip
      #zebra.conf should be charged by shell here.
      #make_ripd_conf_file
   fi

   #make_ripd_conf_file
   touch $RIPD_PID_FILE
   chown $QUAGGA_UNAME:$QUAGGA_GROUP $RIPD_PID_FILE
   $RIPD_BIN_NAME -d -f $RIPD_CONF_FILE -u root
   ulog rip status "ripd started"
}

#----------------------------------------------------------------------
# do_stop_ripngd
#----------------------------------------------------------------------
do_stop_ripngd() {
   if [ -f "$RIPNGD_PID_FILE" ] ; then
      kill -TERM `cat $RIPNGD_PID_FILE`
      rm -f $RIPNGD_PID_FILE
   fi
   echo -n > $RIPNGD_CONF_FILE
   ulog ripng status "ripngd stopped"
}

#----------------------------------------------------------------------
# do_start_ripngd
#----------------------------------------------------------------------
do_start_ripngd() {
   if [ -f "$RIPNGD_PID_FILE" ] ; then
      # ripngd does support SIGHUP for rereading the conf file
      # but we are going to restart it anyways
      do_stop_ripngd
   fi

   make_ripngd_conf_file
   touch $RIPNGD_PID_FILE
   chown $QUAGGA_UNAME:$QUAGGA_GROUP $RIPNGD_PID_FILE
   $RIPNGD_BIN_NAME -d -f $RIPNGD_CONF_FILE -u root
   ulog ripng status "ripngd started"
}

#----------------------------------------------------------------------
# calculate_services_to_start
# This component handles quite a few services.
# Based on configuration, and status figure out what needs to be started
#----------------------------------------------------------------------
calculate_services_to_start ()
{
   # assume firewall does not need to be restarted
   FW_RESTART_REQ=0
   # assume routing does not need to be started
   ROUTED_REQ=1
   # assume ripd does not need to be restarted
   RIPD_REQ=0
   # assume ripngd does not need to be restarted
   RIPNGD_REQ=0

   # if the lan and wan are both down then we dont start routing services
   CURRENT_WAN_STATE=`sysevent get wan-status`
   CURRENT_LAN_STATE=`sysevent get lan-status`
   if [ "stopped" = "$CURRENT_WAN_STATE" ] && [ "stopped" = "$CURRENT_LAN_STATE" ] ; then
      return
   elif [ "stopping" = "$CURRENT_WAN_STATE" ] || [ "starting" = "$CURRENT_WAN_STATE" ] ; then
      # the network is in flux, postpone any changes till it is ready
      return
   elif [ "stopping" = "$CURRENT_LAN_STATE" ] || [ "starting" = "$CURRENT_LAN_STATE" ] ; then
      return
   else
      # at least one interface is started, and the other is started or stopped
      ROUTED_REQ=1
   fi

   # add ripd if rip is enabled
   if [ "1" = "$SYSCFG_rip_enabled" ] ; then
      RIPD_REQ=1
      FW_RESTART_REQ=1
   fi

   # add router advertisment is it is required
   IPV6_PREFIX=`sysevent get ipv6_prefix`
   if [ "0" != "$SYSCFG_router_adv_enable" ] && [ "" != "$IPV6_PREFIX" ] && [ "0" != "$IPV6_PREFIX" ] && [ "0.0.0.0" != "$IPV6_PREFIX" ] ; then
      FW_RESTART_REQ=1
   fi

   if [ "1" = "$SYSCFG_ripng_enabled" -a "" != "$IPV6_PREFIX" ]; then
      RIPNGD_REQ=1
      FW_RESTART_REQ=1
   fi

   # if there are static routes we restart the firewall just in case
   if [ "" != "$SYSCFG_StaticRouteCount" ] && [ "0" != "$SYSCFG_StaticRouteCount" ] ; then
      FW_RESTART_REQ=1
   fi
}

#------------------------------------------------------------------------
# start_all_required_services
# Calculate whether firewall, routing daemon, ripd are required
# and if so start whichever is needed
#------------------------------------------------------------------------
start_all_required_services () 
{
   calculate_services_to_start
   if [ "1" = "$FW_RESTART_REQ" ] ; then
      sysevent set firewall-restart
   fi
   if [ "1" = "$ROUTED_REQ" ] ; then
      do_start_zebra
      sysevent set ${SERVICE_NAME}-errinfo
      sysevent set ${SERVICE_NAME}-status started
   fi
   if [ "1" = "$RIPD_REQ" ] ; then
      do_start_ripd
      sysevent set rip-errinfo
      sysevent set rip-status started
   fi
   if [ "1" = "$RIPNGD_REQ" ] ; then
      do_start_ripngd
      sysevent set ripng-errinfo
      sysevent set ripng-status started
   fi
}

add_poli_route6()
{
    ip -6 rule add iif brlan0 table erouter
    gw=$(ip -6 route show default dev erouter0 | awk '/via/ {print $3}')
    if [ "$gw" != "" ]; then
        ip -6 route add default via $gw dev erouter0 table erouter
    fi
}

del_poli_route6()
{
    ip -6 route del default dev erouter0 table erouter
    ip -6 rule del iif brlan0 table erouter
}

#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   #if [ "started" != "$STATUS" ] ; then
      start_all_required_services
      # start_all_required_services will set the status
   #fi

   add_poli_route6
}

#--------------------------------------------------------------
# service_stop
#--------------------------------------------------------------
service_stop ()
{
   wait_till_end_state ${SERVICE_NAME}

   del_poli_route6

   STATUS=`sysevent get ${SERVICE_NAME}-status`
   #if [ "stopped" != "$STATUS" ] ; then
      STATUS=`sysevent get rip-status`
      if [ "stopped" != "$STATUS" ] ; then
         do_stop_ripd
         sysevent set rip-errinfo
         sysevent set rip-status stopped
      fi
      STATUS=`sysevent get ripng-status`
      if [ "stopped" != "$STATUS" ] ; then
         do_stop_ripngd
         sysevent set ripng-errinfo
         sysevent set ripng-status stopped
      fi
      do_stop_zebra
      sysevent set ${SERVICE_NAME}-status stopped
      sysevent set ${SERVICE_NAME}-errinfo

      # we might need to restart the firewall 
      start_all_required_services
      # start_all_required_services will set the status'
   #fi
}

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get hostname lan_ifname router_managed_flag router_other_flag router_adv_enable dhcpv6s_enable rip_enabled rip_interface_wan rip_interface_lan rip_md5_passwd get rip_text_passwd StaticRouteCount ripng_enabled`

   eval $FOO
  if [ $SYSCFG_FAILED = 'true' ] ; then
     ulog routed status "$PID utctx failed to get some configuration data"
     exit
  fi
}

#-------------------------------------------------------------------------------------------
# Entry
# The first parameter is the name of the event that caused this handler to be activated
#-------------------------------------------------------------------------------------------

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
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------
   wan-status)
      service_stop
      service_start
      ;;
   lan-status)
      service_stop
      service_start
      ;;
   ripd-restart)
      service_stop
      service_start
      ;;
   zebra-restart)
      service_stop
      service_start
      ;;
   staticroute-restart)
      service_stop
      service_start
      ;;
   ipv6_prefix|ipv6_nameserver)
      service_stop
      service_start
      ;;

   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
