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

#source /etc/utopia/service.d/interface_functions.sh
#source /etc/utopia/service.d/ulog_functions.sh
#source /etc/utopia/service.d/service_lan/wlan.sh
#source /etc/utopia/service.d/event_handler_functions.sh
#source /etc/utopia/service.d/service_lan/lan_hooks.sh
#source /etc/utopia/service.d/brcm_ethernet_helper.sh

source /etc/utopia/service.d/ut_plat.sh
source /etc/utopia/service.d/log_capture_path.sh

THIS=/etc/utopia/service.d/lan_handler.sh
SERVICE_NAME="lan_handler"

#args: router IP, subnet mask
ap_addr() {
    if [ "$2" ]; then
        NM="$2"
    else
        NM="255.255.255.0"
    fi
    if [ "$1" ]; then
        IP="$1"
    else
        IP="255.253.252.100"
    fi
    #
    n="${NM%.*}";m="${NM##*.}"
    l="${IP%.*}";r="${IP##*.}";c=""
    if [ "$m" = "0" ]; then
        c=".254"
        m="${n##*.}";n="${n%.*}"
        r="${l##*.}";l="${l%.*}"
        if [ "$m" = "0" ]; then
            c=".255$c"
            m="${n##*.}";n="${n%.*}"
            r="${l##*.}";l="${l%.*}"
            if [ "$m" = "0" ]; then
                c=".255$c"
                m=$n
                r=$l;l=""
            fi
        fi
    fi
    let s=256-$m
    let r=$r/$s*$s
    let r=$r+$s-1
    if [ "$l" ]; then
        SNW="$l.$r$c"
    else
        SNW="$r$c"
    fi

    echo $SNW
}


#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------


#service_init 
echo "RDKB_SYSTEM_BOOT_UP_LOG : lan_handler called with $1 $2"
#echo "lan_handler called with $1 $2" > /dev/console

case "$1" in
   ${SERVICE_NAME}-start)
      service_start
      ;;
   ${SERVICE_NAME}-stop)
      service_stop
      ;;
   ${SERVICE_NAME}-restart)
      sysevent set lan-restarting 1
      service_stop
      service_start
      sysevent set lan-restarting 0
      ;;
   erouter_mode-updated)
      #last_erouter_mode code in ipv4-*-status) may be wrong, when erouter_mode-updated happens after ipv4_*-status event
      SYSCFG_last_erouter_mode=`syscfg get last_erouter_mode`
      #if below value is 1, we already used old last_erouter_mode in ipv4_4-status
      SYSEVENT_ipv4_4_status_configured=`sysevent get ipv4_4_status_configured`
      if [ "0" != "$SYSCFG_last_erouter_mode" ] && [ 1x = "${SYSEVENT_ipv4_4_status_configured}"x ]; then
          echo "lan_handler.sh: erouter_mode-updated, restart lan"
          LAN_INST=`sysevent get primary_lan_l3net`
          LAN_IFNAME=`sysevent get ipv4_${LAN_INST}-ifname`
          sysevent set ipv4-down $LAN_INST
          sysevent set ipv4-up $LAN_INST
      fi
      ;;
   ipv4_*-status)
        if [ x"up" = x${2} ]; then
            INST=${1#*_}
            INST=${INST%-*}
            RG_MODE=`syscfg get last_erouter_mode`
            sysevent set current_lan_ipaddr `sysevent get ipv4_${INST}-ipv4addr`

            if [ "$RG_MODE" = "2" -a x"ready" != x`sysevent get start-misc` ]; then
				echo "LAN HANDLER : Triggering DHCP server using LAN status based on RG_MODE:2"
                sysevent set lan-status started
                firewall
                execute_dir /etc/utopia/post.d/
            elif [ x"ready" != x`sysevent get start-misc` -a x != x`sysevent get current_wan_ipaddr` -a "0.0.0.0" != `sysevent get current_wan_ipaddr` ]; then
				echo "LAN HANDLER : Triggering DHCP server using LAN status based on start misc"
				sysevent set lan-status started
                STARTED_FLG=`sysevent get parcon_nfq_status`

                if [ x"$STARTED_FLG" != x"started" ]; then
                    BRLAN0_MAC=`ifconfig l2sd0 | grep HWaddr | awk '{print $5}'`
                    ((nfq_handler 4 $BRLAN0_MAC &)&)
                    ((nfq_handler 6 $BRLAN0_MAC &)&)
                    sysevent set parcon_nfq_status started
                fi
                isAvailablebrlan1=`ifconfig | grep brlan1`
                if [ "$isAvailablebrlan1" != "" ]
                then
                    echo "LAN HANDLER : Refreshing LAN from handler"
                    gw_lan_refresh&
                fi
               	firewall
                execute_dir /etc/utopia/post.d/
            else
				echo "LAN HANDLER : Triggering DHCP server using LAN status"
                sysevent set lan-status started
                sysevent set firewall-restart
            fi

            #sysevent set desired_moca_link_state up
            
            firewall_nfq_handler.sh &             

            sysevent set lan_start_time `cat /proc/uptime | cut -d'.' -f1`
            LAN_IFNAME=`sysevent get ipv4_${INST}-ifname`
            #if it's ipv4 only, not enable link local 
            SYSCFG_last_erouter_mode=`syscfg get last_erouter_mode`
            echo "lan_handler.sh last_erouter_mode: $SYSCFG_last_erouter_mode"
            
            if [ "4" = $INST ];then
                sysevent set ipv4_4_status_configured 1
            fi

            if [ "1" = "$SYSCFG_last_erouter_mode" ]; then
                echo 0 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/autoconf     # Do not do SLAAC
            else
                echo 1 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/autoconf
                echo 1 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/disable_ipv6
                echo 0 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/disable_ipv6
                echo 1 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/forwarding
            fi

            #disable dnsmasq when ipv6 only mode and DSlite is disabled
            DSLITE_ENABLED=`sysevent get dslite_enabled`
	    	DHCP_PROGRESS=`sysevent get dhcp_server-progress`
			echo "LAN HANDLER : DHCP configuration status got is : $DHCP_PROGRESS"
            if [ "2" = "$SYSCFG_last_erouter_mode" ] && [ "x1" != x$DSLITE_ENABLED ]; then
                sysevent set dhcp_server-stop		    
            elif [ "0" != "$SYSCFG_last_erouter_mode" ] && [ "$DHCP_PROGRESS" != "inprogress" ] ; then
				echo "LAN HANDLER : Triggering dhcp start based on last erouter mode"
                sysevent set dhcp_server-start
            fi

            LAN_IPV6_PREFIX=`sysevent get ipv6_prefix`
            if [ "$LAN_IPV6_PREFIX" != "" ] ; then
                    ip -6 route add $LAN_IPV6_PREFIX dev $LAN_IFNAME
            fi
        else
            if [ x"started" = x`sysevent get lan-status` ]; then
				#kill `pidof CcspHomeSecurity`
                sysevent set lan-status stopped
                #sysevent set desired_moca_link_state down
            fi
        fi
        sysevent set firewall-restart 
   ;;
   
   ipv4-resync)
        LAN_INST=`sysevent get primary_lan_l3net`
        if [ x"$2" = x"$LAN_INST" ]; then
            eval "`psmcli get -e LAN_IP ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_IP LAN_SUB ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_SUBNET`"
            AP_ADDR="`ap_addr $LAN_IP $LAN_SUB`"
            psmcli set dmsb.atom.l3net.${LAN_INST}.$IPV4_NV_IP $AP_ADDR dmsb.atom.l3net.${LAN_INST}.$IPV4_NV_SUBNET $LAN_SUB
            ccsp_bus_client_tool eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool 'true' 'true'
        fi
   ;;
   multinet-resync)
        ccsp_bus_client_tool eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool 'true' 'true'

   ;;
   
   pnm-status)
        if [ x = x"`sysevent get lan_handler_async`" ]; then
            eval `psmcli get -e INST dmsb.MultiLAN.PrimaryLAN_l3net L2INST dmsb.MultiLAN.PrimaryLAN_l2net BRPORT dmsb.MultiLAN.PrimaryLAN_brport HSINST dmsb.MultiLAN.HomeSecurity_l3net`
            if [ x != x$INST ]; then 
                async="`sysevent async ipv4_${INST}-status $THIS`"
                sysevent set lan_handler_async "$async"
                sysevent set primary_lan_l2net ${L2INST}
                sysevent set primary_lan_brport ${BRPORT}
                sysevent set homesecurity_lan_l3net ${HSINST}
                sysevent set primary_lan_l3net ${INST}
                
            fi
        fi
   ;;
   
   lan-restart)
        syscfg_lanip=`syscfg get lan_ipaddr`
        syscfg_lansub=`syscfg get lan_netmask`
        LAN_INST=`sysevent get primary_lan_l3net`
        eval "`psmcli get -e LAN_IP ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_IP LAN_SUB ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_SUBNET`"
        
        if [ x$syscfg_lanip != x$LAN_IP -o x$syscfg_lansub != x$LAN_SUB ]; then
            psmcli set ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_IP $syscfg_lanip ${IPV4_NV_PREFIX}.${LAN_INST}.$IPV4_NV_SUBNET $syscfg_lansub
            
            # TODO check for lan network being up ?
            sysevent set ipv4-resync $LAN_INST
        fi
 
        #handle ipv6 address on brlan0. Because it's difficult to add ipv6 operation in ipv4 process. So just put here as a temporary method
        SYSEVT_lan_ipaddr_v6_prev=`sysevent get lan_ipaddr_v6_prev`
        SYSEVT_lan_ipaddr_v6=`sysevent get lan_ipaddr_v6`
        SYSEVT_lan_prefix_v6=`sysevent get lan_prefix_v6`
        LAN_IFNAME=`sysevent get ipv4_${LAN_INST}-ifname`

        if [ x$SYSEVT_lan_ipaddr_v6_prev != x$SYSEVT_lan_ipaddr_v6 ]; then
            ip -6 addr del $SYSEVT_lan_ipaddr_v6_prev/64 dev $LAN_IFNAME valid_lft forever preferred_lft forever
            ip -6 addr add $SYSEVT_lan_ipaddr_v6/64 dev $LAN_IFNAME valid_lft forever preferred_lft forever
        fi

   ;;
   # TODO: register for lan-stop and lan-start
   lan-stop)
        LAN_INST=`sysevent get primary_lan_l3net`
        LAN_IFNAME=`sysevent get ipv4_${LAN_INST}-ifname`
        sysevent set ipv4-down $LAN_INST
        echo 1 > /proc/sys/net/ipv6/conf/$LAN_IFNAME/disable_ipv6
        
        SYSEVT_lan_ipaddr_v6_prev=`sysevent get lan_ipaddr_v6_prev`
        SYSEVT_lan_prefix_v6=`sysevent get lan_prefix_v6`

        ip -6 addr flush dev $LAN_IFNAME

        #we need to restart necessary application when lan restart
        #monitor will start dibbler
        dibbler-server stop
   ;;
   
   lan-start)
        # TODO call the restart routine
        sysevent set ipv4-up `sysevent get primary_lan_l3net`
   ;;
   *)   
      echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
