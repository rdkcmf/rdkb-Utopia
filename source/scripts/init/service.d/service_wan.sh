#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2008,2010 by Cisco Systems, Inc. All Rights Reserved.
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
# This script sets up the wan handlers.
# Upon completion of bringing up the appropriate wan handler bringing up the wan,
# it is expected that the wan handler will set the wan-status up.
#
# All wan protocols must set the following sysevent tuples
#   current_wan_ifname
#   current_wan_ipaddr
#   current_wan_subnet
#   current_wan_state
#   wan-status
#
# Wan protocols are responsible for setting up:
#   /etc/resolv.conf
# Wan protocols are also responsible for restarting the dhcp server if
# the wan domain or dns servers change
#------------------------------------------------------------------
#--------------------------------------------------------------
#
# This script set the handlers for 2 events:
#    desired_ipv4_link_state
#    desired_ipv4_wan_state
# and then as appropriate sets these events.
# The desired_ipv4_link_state handler establishes ipv4 connectivity to
# the isp (currently dhcp or static).
# The desired_ipv4_wan_state handler uses the connectivity to the isp to
# set up a connection that is used as the ipv4 wan. This connection
# may be dhcp, static, pppoe, l2tp, pptp or in the future other protocols
#--------------------------------------------------------------


source /etc/utopia/service.d/interface_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
source /etc/utopia/service.d/service_wan/ppp_helpers.sh
source /etc/utopia/service.d/brcm_ethernet_helper.sh
source /etc/utopia/service.d/sysevent_functions.sh

SERVICE_NAME="wan"
WAN_HANDLER=/etc/utopia/service.d/service_wan.sh

PID="($$)"

#------------------------------------------------------------------
# setup_ethernet_switch
# Set the hardware specific registers to create the vlans per port
#
# Port 0 is vlan 2
# Port 1 is vlan 1
# Port 2 is vlan 1
# Port 3 is vlan 1
# Port 4 is vlan 1
# Port 5 is vlan 1 (but it isnt connected)
# Port 6 is unused (vlan 0)
# Port 7 is unused (vlan 0)
# Port 8 is the internal port to the SoC and is vlan 1
# Note that even though these ports are setup at boot, we do this 
# again here just in case we are switching from bridging mode to
# router mode
#------------------------------------------------------------------
setup_ethernet_switch() {
	ulog switch status "TODO: ethernet switch boot time setup."
}

#------------------------------------------------------------------
# set_wan_mtu 
#------------------------------------------------------------------
set_wan_mtu() {

    # The up-limit for MTU size per protocol is:
    #  DHCP:   1500 bytes
    #  static: 1500 bytes
    #  PPPoE:  1492 bytes (reserve 8 bytes for pppoe header)
    #  PPTP:   1460 bytes (reserve 40 bytes for IP(20)+GRE(16)+PPP(4) headers)
    #  L2TP:   1460 bytes (reserver 40 bytes for IP(20)+UDP(8)+L2TP(12) headers)

    # The TCP MSS (Maximum Segment Size) varies with protocol:
    #  PPPoE:  1452 bytes (1492 - 40)
    #  PPTP:   1420 bytes (1460 - 40)
    #  L2TP:   1420 bytes (1460 - 40)
    #  Note that MSS is an option in TCP SYN which sets the maximum TCP payload size.
    #  40 bytes consists of 20 bytes IP header plus 20 bytes TCP header.

    # wan_mtu is for a particular wan protocol
    WAN_MTU=$SYSCFG_wan_mtu

    # if wan_mtu is not set, set it to appropriate default;
    # if wan_mtu is set to zero (indicates auto-mtu), set it to appropriate default; 
    # if wan_mtu is already set, check the up-limit
    if [ "" = "$WAN_MTU" ] || [ "0" = "$WAN_MTU" ] ; then
      case "$SYSCFG_wan_proto" in
        dhcp | static)
          WAN_MTU=1500
          DEF_WAN_MTU=$WAN_MTU
          ;;
        pppoe)
          WAN_MTU=1492
          DEF_WAN_MTU=`expr $WAN_MTU + 8`
          ;;
        pptp | l2tp)
          WAN_MTU=1460
          DEF_WAN_MTU=`expr $WAN_MTU + 40`
          ;;
        *)
          ulog wan status "$PID called with incorrect wan protocol $SYSCFG_wan_proto. Aborting"
          return 3
          ;;
        esac
    else
      case "$SYSCFG_wan_proto" in
        dhcp | static)
          if [ "$WAN_MTU" -gt 1500 ]; then
             WAN_MTU=1500
          fi
          DEF_WAN_MTU=$WAN_MTU
          ;;
        pppoe)
          if [ "$WAN_MTU" -gt 1492 ]; then
             WAN_MTU=1492
          fi
          DEF_WAN_MTU=`expr $WAN_MTU + 8`
          ;;
        pptp | l2tp)
          if [ "$WAN_MTU" -gt 1460 ]; then
             WAN_MTU=1460
          fi
          DEF_WAN_MTU=`expr $WAN_MTU + 40`
          ;;
        *)
          echo "[utopia] wanControl.sh error: called with incorrect wan protocol " $SYSCFG_wan_proto > /dev/console
          return 3
          ;;
      esac
    fi

    case "$SYSCFG_wan_proto" in
      pppoe | pptp | l2tp)
        WAN_BSS=`expr $WAN_MTU - 40`
        sysevent set ppp_clamp_mtu $WAN_BSS
        ;;
      *)
        sysevent set ppp_clamp_mtu 
        ;;
    esac

   ip -4 link set $SYSEVENT_wan_ifname mtu $DEF_WAN_MTU
}

#------------------------------------------------------------------
# clone_mac_addr
#------------------------------------------------------------------
clone_mac_addr ()
{
    if [ "$SYSCFG_def_hwaddr" != "" ] && [ "$SYSCFG_def_hwaddr" != "00:00:00:00:00:00" ]; then
      /sbin/macclone $SYSEVENT_wan_ifname $SYSCFG_def_hwaddr
    fi
}

#-------------------------------------------------------------
# Registration/Deregistration of dhcp client restart/release/renew handlers
# These are only needed if the dhcp is used
# Note that service_wan is creating the pseudo service dhcp_client
#-------------------------------------------------------------
HANDLER="/etc/utopia/service.d/service_wan/dhcp_link.sh"

unregister_dhcp_client_handlers() {
   # ulog wan status "$PID unregister_dhcp_client_handlers"
   asyncid1=`sysevent get ${SERVICE_NAME}_async_id_1`;
   if [ -n "$asyncid1" ] ; then
      sysevent rm_async $asyncid1
      sysevent set ${SERVICE_NAME}_async_id_1
   fi
   asyncid2=`sysevent get ${SERVICE_NAME}_async_id_2`;
   if [ -n "$asyncid2" ] ; then
      sysevent rm_async $asyncid2
      sysevent set ${SERVICE_NAME}_async_id_2
   fi
   asyncid3=`sysevent get ${SERVICE_NAME}_async_id_3`;
   if [ -n "$asyncid3" ] ; then
      sysevent rm_async $asyncid3
      sysevent set ${SERVICE_NAME}_async_id_3
   fi
}

register_dhcp_client_handlers() {
   # ulog wan status "$PID register_dhcp_client_handlers"
   # Remove any prior notification requests
   unregister_dhcp_client_handlers

   # instantiate a request to be notified when the dhcp_client-restart changes
   # make it an event (TUPLE_FLAG_EVENT = $TUPLE_FLAG_EVENT)
   asyncid1=`sysevent async dhcp_client-restart /etc/utopia/service.d/service_wan/dhcp_link.sh`;
   sysevent setoptions dhcp_client-restart $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_1 "$asyncid1"

   # instantiate a request to be notified when the dhcp_client-release / renew changes
   # make it an event (TUPLE_FLAG_EVENT = $TUPLE_FLAG_EVENT)
   asyncid2=`sysevent async dhcp_client-release /etc/utopia/service.d/service_wan/dhcp_link.sh`;
   sysevent setoptions dhcp_client-release $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_2 "$asyncid2"

   asyncid3=`sysevent async dhcp_client-renew /etc/utopia/service.d/service_wan/dhcp_link.sh`;
   sysevent setoptions dhcp_client-renew $TUPLE_FLAG_EVENT
   sysevent set ${SERVICE_NAME}_async_id_3 "$asyncid3"
}

#-------------------------------------------------------------
# Registration/Deregistration of LINK and WAN handlers
#-------------------------------------------------------------
unregister_link_wan_handlers() {
   # ulog wan status "$PID unregister_link_wan_handlers"
   # if there is any event handler for phylink_wan_state, then remove it
   asyncid=`sysevent get ${SERVICE_NAME}_phylink_wan_state_asyncid`
   if [ -n "$asyncid" ] ; then
      sysevent rm_async $asyncid
      sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid
   fi
   # if there is any event handler for desired_ipv4_link_state, then remove it
   asyncid=`sysevent get ${SERVICE_NAME}_desired_ipv4_link_state_asyncid`
   if [ -n "$asyncid" ] ; then
      sysevent rm_async $asyncid
      sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid
   fi
   # if there is any event handler for desired_ipv4_wan_state, then remove it
   asyncid=`sysevent get ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid`
   if [ -n "$asyncid" ] ; then
      sysevent rm_async $asyncid
      sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid
   fi
   # if there is any event handler for current_ipv4_link_state, then remove it
   asyncid=`sysevent get ${SERVICE_NAME}_current_ipv4_link_state_asyncid`
   if [ -n "$asyncid" ] ; then
      sysevent rm_async $asyncid
      sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid
   fi
}

register_link_wan_handlers() {
   # make desired_ipv4_link_state and desired_ipv4_wan_state events
   sysevent setoptions desired_ipv4_link_state $TUPLE_FLAG_EVENT
   sysevent setoptions desired_ipv4_wan_state $TUPLE_FLAG_EVENT

   # set up the handlers for ipv4 link, and ipv4 wan state changes
   # link handlers provide the ipv4 connectivity to the isp
   # wan handlers provide the wan connectivity 
   # link handler react to events:
   #    desired_ipv4_link_state
   # wan handlers react to events:
   #    desired_ipv4_wan_state
   #    current_ipv4_link_state
   case "$SYSCFG_wan_proto" in
      dhcp)
         ulog wan status "$PID installing handlers for dhcp_link and dhcp_wan"
         register_dhcp_client_handlers
         asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
         sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/dhcp_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/dhcp_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
      static)
         ulog wan status "$PID installing handlers for static_link and static_wan"
         asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/static_link.sh`
         sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/static_link.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/static_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/static_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
      pppoe)
         ulog wan status "$PID installing handlers for pppoe_link and pppoe_wan"
         asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/pppoe_link.sh`
         sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/pppoe_link.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/pppoe_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/pppoe_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
      pptp)
         if [ "0" != "$SYSCFG_pptp_address_static" ] ; then
            ulog wan status "$PID installing handlers for static_link and pptp_wan"
            asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/static_link.sh`
            sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
            asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/static_link.sh`
            sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         else
            ulog wan status "$PID installing handlers for dhcp_link and pptp_wan"
            register_dhcp_client_handlers
            asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
            sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
            asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
            sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         fi
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/pptp_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/pptp_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
      l2tp)
         if [ "0" != "$SYSCFG_l2tp_address_static" ] ; then
            ulog wan status "$PID installing handlers for static_link and l2tp_wan"
            asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/static_link.sh`
            sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
            asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/static_link.sh`
            sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         else
            ulog wan status "$PID installing handlers for dhcp_link and l2tp_wan"
            register_dhcp_client_handlers
            asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
            sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
            asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/dhcp_link.sh`
            sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         fi
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/l2tp_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/l2tp_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
      telstra)
         ulog wan status "$PID installing handlers for dhcp_link and telstra_wan"
         register_dhcp_client_handlers
         asyncid=`sysevent async phylink_wan_state /etc/utopia/service.d/service_wan/static_link.sh`
         sysevent set ${SERVICE_NAME}_phylink_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_link_state /etc/utopia/service.d/service_wan/static_link.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_link_state_asyncid "$asyncid"
         asyncid=`sysevent async desired_ipv4_wan_state /etc/utopia/service.d/service_wan/telstra_wan.sh`
         sysevent set ${SERVICE_NAME}_desired_ipv4_wan_state_asyncid "$asyncid"
         asyncid=`sysevent async current_ipv4_link_state /etc/utopia/service.d/service_wan/telstra_wan.sh`
         sysevent set ${SERVICE_NAME}_current_ipv4_link_state_asyncid "$asyncid"
         ;;
   esac
}


#------------------------------------------------------------------
# bring the wan down in stages. First the ipv4 wan, then the ipv4 link
#------------------------------------------------------------------
ipv4_wan_down() {
   do_stop_wan_monitor

   sysevent set wan-status stopping
   sysevent set wan-errinfo
   ulog wan status "$PID tearing wan down"
   ulog wan status "$PID setting desired_ipv4_wan_state down"
   sysevent set desired_ipv4_wan_state down
   ulog wan status "$PID setting desired_ipv4_link_state down"
   sysevent set desired_ipv4_link_state down

   # the wan handler will set wan-status to stopped when it is done
   DONE=`sysevent get wan-status`
   while [ "stopped" != "$DONE" ]
   do
      sleep 1
      DONE=`sysevent get wan-status`
   done

   # unregister handlers
   unregister_dhcp_client_handlers
   unregister_link_wan_handlers

   # since we are taking down both wan protocol and wan link
   # we should make sure everything is down
   DONE=`sysevent get current_ipv4_wan_state`
   while [ "up" = "$DONE" ]
   do
      sleep 1
      DONE=`sysevent get current_ipv4_wan_state`
   done

   DONE=`sysevent get current_ipv4_link_state`
   while [ "up" = "$DONE" ]
   do
      sleep 1
      DONE=`sysevent get current_ipv4_link_state`
   done

   # now take down the link
   if [ "" != "$SYSEVENT_wan_ifname" ] ; then
      ip -4 addr flush dev $SYSEVENT_wan_ifname
   fi
}

#------------------------------------------------------------------
# bring the wan up in stages. The link and the wan will work concurrently
#------------------------------------------------------------------
ipv4_wan_up() {

   do_stop_wan_monitor

   sysevent set wan-status starting
   sysevent set wan-errinfo
   ulog wan status "$PID bringing wan ipv4 up"

   # if there are any dhcp client handlers then remove them
   unregister_dhcp_client_handlers
   # if there is any event handler for phylink_wan_state, then remove it
   unregister_link_wan_handlers

   # register the link and wan handlers appropriate for the
   # wan protocol we are using
   register_link_wan_handlers

   sysevent set ipv4_wan_ipaddr 0.0.0.0

   set_wan_mtu
   if [ "$?" -ne "0" ] ; then
      return 3 
   fi

   # start to bring up the wan by signalling for the ipv4 link to be established
   # and the wan link to be established
   # --------------------------------------------------------------------------
   # sysevent set firewall-restart
   ulog wan status "$PID setting desired_ipv4_link_state up"
   sysevent set desired_ipv4_link_state up
   ulog wan status "$PID setting desired_ipv4_wan_state up"
   sysevent set desired_ipv4_wan_state up
   sysevent set firewall_flush_conntrack 1
   # the wan handler will set the wan-status to up
}

#-------------------------------------
# Bring IPv4 service up
#-------------------------------------
bring_ipv4_up()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "started" != "$STATUS" -a "starting" != "$STATUS" ] ; then
      ipv4_wan_up
   fi
}

#-------------------------------------
# Take IPv4 service down
#-------------------------------------
take_ipv4_down()
{
   wait_till_end_state ${SERVICE_NAME}
   STATUS=`sysevent get ${SERVICE_NAME}-status`
   if [ "stopped" != "$STATUS" ]; then
      ipv4_wan_down
   fi
}

#------------------------------------------------------------------
# bring the wan L2 link up, not including IPv4/IPv6 (L3 Link)
#------------------------------------------------------------------
bring_wan_up() {
   # NOTE: This is hardware specific code
   #setup_ethernet_switch

   # determine the name of the wan interface.
   # Some products use a physical interface name,
   # and some use a virtual interface name.
   #if [ "" != "$SYSCFG_wan_virtual_ifnum" ] ; then
   #   ip -4 link set $SYSCFG_wan_physical_ifname up
   #   config_vlan $SYSCFG_wan_physical_ifname $SYSCFG_wan_virtual_ifnum
   #fi

   clone_mac_addr

   # are we accepting IPv6 router advertisements for provisioning wan
   # ----------------------------------------------------------------
   if [ "2" == "$SYSCFG_last_erouter_mode" ] || [ "3" == "$SYSCFG_last_erouter_mode" ]; then
       if [ "1" = "$SYSCFG_router_adv_provisioning_enable" ] ; then
          #  echo 2 > /proc/sys/net/ipv6/conf/default/accept_ra
          echo 2 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/accept_ra    # Accept RA even when forwarding is enabled
          echo 1 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/accept_ra_defrtr # Accept default router (metric 1024)
          #USGv2 Only, don't accept Prefix-Information option in RA
          echo 0 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/accept_ra_pinfo 
          echo 1 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/autoconf     # Do SLAAC
          echo 1 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/disable_ipv6
          echo 0 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/disable_ipv6
       else
          echo 0 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/accept_ra    # Never accept RA
          echo 0 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/autoconf     # Do not do SLAAC
       fi

       echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
       echo 1 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/forwarding

   else
       echo 0 > /proc/sys/net/ipv6/conf/${SYSEVENT_wan_ifname}/autoconf     # Do not do SLAAC
   fi
   
   echo 1 > /proc/sys/net/ipv4/conf/${SYSEVENT_wan_ifname}/arp_announce

   ip -4 link set $SYSEVENT_wan_ifname up

   # if IPv4 enabled
   if [ "1" == "$SYSCFG_last_erouter_mode" ] || [ "3" == "$SYSCFG_last_erouter_mode" ]; then
      bring_ipv4_up
   fi
}

#------------------------------------------------------------------
# bring the wan L2 link down
#------------------------------------------------------------------
take_wan_down() {
   take_ipv4_down
   # now take down the link
   if [ "" != "$SYSEVENT_wan_ifname" ] ; then
      ip -4 link set $SYSEVENT_wan_ifname down
   fi
}

#------------------------------------------------------------------
# Handle phylink_wan_state
#------------------------------------------------------------------
handle_wan_link_status() {
   if [ "up" == "$SYSEVENT_wan_link_status" ]; then
      ulog wan status "$PID wan link up"
      bring_wan_up
   else
      ulog wan status "$PID wan link down"
      take_wan_down
   fi
}

#------------------------------------------------------------------
# Handle erouter mode update
#------------------------------------------------------------------
handle_erouter_mode_update() {
   if [ "up" == "$SYSEVENT_wan_link_status" ]; then
      # if IPv4 only mode or dual-stack mode
      if [ "1" == "$SYSCFG_last_erouter_mode" ] || [ "3" == "$SYSCFG_last_erouter_mode" ]; then
         bring_ipv4_up
      else
         take_ipv4_down
      fi
   fi
}

#--------------------------------------------------------------
# register_wan_handlers
#--------------------------------------------------------------
register_wan_handlers ()
{
   register_sysevent_handler $SERVICE_NAME erouter_mode-updated $WAN_HANDLER
   register_sysevent_handler $SERVICE_NAME phylink_wan_state $WAN_HANDLER
}

#--------------------------------------------------------------
# unregister_wan_handlers
#--------------------------------------------------------------
unregister_wan_handlers ()
{
   unregister_sysevent_handler $SERVICE_NAME erouter_mode-updated
   unregister_sysevent_handler $SERVICE_NAME phylink_wan_state
}

#----------------------------------------------------------------------------
# standard functions
#----------------------------------------------------------------------------

#--------------------------------------------------------------
# service_init
#--------------------------------------------------------------
service_init ()
{
   # Get all provisioning data

   SYSCFG_FAILED='false'
   FOO=`utctx_cmd get wan_proto wan_mtu def_hwaddr pptp_address_static l2tp_address_static wan_physical_ifname  wan_virtual_ifnum router_adv_provisioning_enable primary_wan_proto byoi_enabled last_erouter_mode`
   eval $FOO
   if [ $SYSCFG_FAILED = 'true' ] ; then
      ulog wan status "$PID utctx failed to get some configuration data"
      ulog wan status "$PID WAN CANNOT BE CONTROLLED"
      exit
   fi
  
   SYSEVENT_wan_ifname=`sysevent get wan_ifname`
   SYSEVENT_wan_link_status=`sysevent get phylink_wan_state`
   SYSEVENT_current_hsd_mode=`sysevent get current_hsd_mode`
   if [ "1" = "$SYSCFG_byoi_enabled" ] && [ "primary" = "$SYSEVENT_current_hsd_mode" ]; then
      SYSCFG_wan_proto=$SYSCFG_primary_wan_proto
   fi
}

#--------------------------------------------------------------
# service_start
#--------------------------------------------------------------
service_start ()
{
   if [ "started" == "`sysevent get wan_service-status`" ]; then
      ulog wan status "$PID wan service already started"
      return
   fi
   ulog wan status "$PID bringing wan service up"
   register_wan_handlers
   sysevent set wan_service-status started
   if [ "up" != "$SYSEVENT_wan_link_status" ]; then
      ulog wan status "$PID wan physical link is not yet up"
      return
   fi
   bring_wan_up

   ip rule add iif $SYSCFG_wan_physical_ifname lookup all_lans
   ip rule add oif $SYSCFG_wan_physical_ifname lookup erouter
}

#--------------------------------------------------------------
# service_stop
#--------------------------------------------------------------
service_stop ()
{
   if [ "stopped" == "`sysevent get wan_service-status`" ]; then
      ulog wan status "$PID wan service already started"
      return
   fi
   unregister_wan_handlers
   take_wan_down
   sysevent set wan_service-status stopped

   ip rule del iif $SYSCFG_wan_physical_ifname lookup all_lans
   ip rule del oif $SYSCFG_wan_physical_ifname lookup erouter
}

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------

service_init
case "$1" in
   ${SERVICE_NAME}-start)
      service_start
      ;;
   ${SERVICE_NAME}-stop)
      service_stop
      ;;
   ${SERVICE_NAME}-restart)
 #     if [ x"unknown" != x"$SYSEVENT_current_hsd_mode" ]; then
         sysevent set wan-restarting 1
         service_stop
         service_start
         sysevent set wan-restarting 0
 #     fi
      ;;
   phylink_wan_state)
      handle_wan_link_status
      ;;
   erouter_mode-updated)
      handle_erouter_mode_update
      ;;
   *)
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
