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
# This script controls the decision to reinit the combined
# dhcp server and dns forwarder
# It is called with the parameters
#    lan-status stopped | started
#  or dhcp_server-restart  
#
#------------------------------------------------------------------

source /etc/utopia/service.d/ut_plat.sh
source /etc/utopia/service.d/service_dhcp_server/dhcp_server_functions.sh
source /etc/utopia/service.d/hostname_functions.sh
source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh
source /etc/utopia/service.d/log_capture_path.sh
source /etc/device.properties
#source /etc/utopia/service.d/sysevent_functions.sh
UTOPIA_PATH="/etc/utopia/service.d"

SERVICE_NAME="dhcp_server"

#DHCP_CONF=/etc/dnsmasq.conf
DHCP_CONF=/var/dnsmasq.conf
RESOLV_CONF=/etc/resolv.conf
BIN=dnsmasq
SERVER=${BIN}
PMON=/etc/utopia/service.d/pmon.sh
PID_FILE=/var/run/dnsmasq.pid
PID=$$

LXC_PID_FILE=/run/lxc/dnsmasq.pid
LXC_DHCP_CONF=/etc/dnsmasq.conf

XCONF_FILE="/etc/Xconf"
XCONF_DEFAULT_URL="https://xconf.xcal.tv/xconf/swu/stb/"
CALL_ARG="$1"

CURRENT_LAN_STATE=`sysevent get lan-status`

# For dhcp_server_slow_start we use cron to restart us
# Just in case it is active, remove those files
if [ -f "$DHCP_SLOW_START_1_FILE" ] ; then
   rm -f $DHCP_SLOW_START_1_FILE
fi
if [ -f "$DHCP_SLOW_START_2_FILE" ] ; then
   rm -f $DHCP_SLOW_START_2_FILE
fi
if [ -f "$DHCP_SLOW_START_3_FILE" ] ; then
   rm -f $DHCP_SLOW_START_3_FILE
fi

# for leasing file sync
if [ -f "/usr/bin/rpcclient" ] ; then
	DHCP_SCRIPT=""
else
	DHCP_SCRIPT=""
fi

#-----------------------------------------------------------------
#  dnsserver_start_lxc
#
#  Start dnsmasq for the container too whenever dnsmasq is restarted
#-----------------------------------------------------------------
dnsserver_start_lxc ()
{
   if [ -f /usr/bin/lxc-ls ]; then
        IS_CONTAINER_ACTIVE=`/usr/bin/lxc-ls --active`
        if [ "$IS_CONTAINER_ACTIVE" = "webui" ]; then
             $SERVER -u nobody --strict-order --bind-interfaces --pid-file=$LXC_PID_FILE --conf-file=$LXC_DHCP_CONF --listen-address 147.0.3.1 --dhcp-range 147.0.3.2,147.0.3.254 --dhcp-lease-max=253 --dhcp-no-override --except-interface=lo --interface=$LXC_BRIDGE_NAME --dhcp-leasefile=/tmp/dnsmasq.$LXC_BRIDGE_NAME.leases --dhcp-authoritative
        fi
   fi
}

dnsmasq_server_start ()
{
         if [ "$XDNS_ENABLE" = "true" ]; then
                SYSCFG_XDNS_FLAG=`syscfg get X_RDKCENTRAL-COM_XDNS`
                SYSCFG_DNSSEC_FLAG=`syscfg get XDNS_DNSSecEnable`
                SYSCFG_XDNSREFAC_FLAG=`syscfg get XDNS_RefacCodeEnable`
                if ([ "$MODEL_NUM" = "CGA4131COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ]) && [ "$SYSCFG_XDNS_FLAG" != "" ] && [ "$SYSCFG_XDNS_FLAG" = "1" ] && [ "$SYSCFG_DNSSEC_FLAG" = "1" ] ; then
                        if [ "$SYSCFG_XDNSREFAC_FLAG" = "1" ] ; then
                                $SERVER -u nobody -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C $DHCP_CONF $DNS_ADDITIONAL_OPTION --proxy-dnssec --cache-size=0 --xdns-refac-code  #--enable-dbus
                        else
                                $SERVER -u nobody -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C $DHCP_CONF $DNS_ADDITIONAL_OPTION --proxy-dnssec --cache-size=0  #--enable-dbus
                        fi

                else
                        if [ "$SYSCFG_XDNSREFAC_FLAG" = "1" ] ; then
                                $SERVER -u nobody -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C $DHCP_CONF $DNS_ADDITIONAL_OPTION --xdns-refac-code  #--enable-dbus
                        else
                                $SERVER -u nobody -q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C $DHCP_CONF $DNS_ADDITIONAL_OPTION  #--enable-dbus
                        fi
                fi
         else
                $SERVER -u nobody -P 4096 -C $DHCP_CONF  #--enable-dbus
         fi

}



#-----------------------------------------------------------------
#  lan_status_change
#
#  On a lan-status change we ignore if the dhcp_server has been manually
#  set to stop or start. We put the system in the state it should be given
#  lan-status and syscfg dhcp_server_enabled
#-----------------------------------------------------------------
lan_status_change ()
{

   echo_t "SERVICE DHCP : Inside lan status change with $1 and $2"
   echo_t "SERVICE DHCP : Current lan status is : $CURRENT_LAN_STATE"
#   if [ "stopped" = "$1" ] ; then
#         sysevent set dns-errinfo
#         sysevent set dhcp_server_errinfo
#         wait_till_end_state dns
#         wait_till_end_state dhcp_server
#         $PMON unsetproc dhcp_server
#         killall `basename $SERVER`
#         rm -f $PID_FILE
#         sysevent set dns-status stopped
#         sysevent set dhcp_server-status stopped
#   elif [ "started" = "$1" -a "started" = "$CURRENT_LAN_STATE" ] ; then
      if [ "0" = "$SYSCFG_dhcp_server_enabled" ] ; then
         # set hostname and /etc/hosts cause we are the dns forwarder
         prepare_hostname
         # also prepare dns part of dhcp conf cause we are the dhcp server too
         prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask dns_only
         echo_t "SERVICE DHCP : Start dhcp-server from lan status change"
         # Get the DNS strict order option
         DNSSTRICT_ORDER_ENABLE=`syscfg get DNSStrictOrder`

         DNS_ADDITIONAL_OPTION=""
         # Check for RFC Enable for DNS STRICT ORDER
         if [ "x$DNSSTRICT_ORDER_ENABLE" == "xtrue" ]; then
              DNS_ADDITIONAL_OPTION=" -o "
              echo "Starting dnsmasq with additional dns strict order option: $DNS_ADDITIONAL_OPTION"
         else
              echo "RFC DNSTRICT ORDER is not defined or Enabled"
         fi
	 dnsmasq_server_start
         sysevent set dns-status started
      else
	     sysevent set lan_status-dhcp started
	     echo_t "SERVICE DHCP :  Call start DHCP server from lan status change with $2"
         dhcp_server_start $2
      fi
#   fi
}

#-----------------------------------------------------------------
#  restart_request
#
#  On a restart_request we ignore if the dhcp_server has been manually
#  set to stop or start. We put the system in the state it should be given
#  lan-status and syscfg dhcp_server_enabled
#
#  The difference between this code and lan_status_change is that this code
#  will do not tear down the server unless some configuration change occured
#-----------------------------------------------------------------
restart_request ()
{
   if [ "started" != "`sysevent get dhcp_server-status`" ] ; then
      exit 0
   fi
   sysevent set dns-errinfo
   sysevent set dhcp_server_errinfo

   wait_till_end_state dns
   wait_till_end_state dhcp_server

   # save a copy of the dnsmasq conf file to help determine whether or not to 
   # kill the server
   DHCP_TMP_CONF="/tmp/dnsmasq.conf.orig"
   if [ -f $DHCP_CONF ];then
	   cp -f $DHCP_CONF $DHCP_TMP_CONF
   fi

   if [ "0" = "$SYSCFG_dhcp_server_enabled" ] ; then
      prepare_hostname
      prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask dns_only
   else
      prepare_hostname
      prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask
      # remove any extraneous leases
      sanitize_leases_file
   fi

   # we need to decide whether to completely restart the dhcp_server
   # or whether to just have it reread everything
   # SIGHUP is reread (except for dnsmasq.conf)
   RESTART=0
   FOO=""
   if [ -f $DHCP_CONF ] && [ -f $DHCP_TMP_CONF ];then
           FOO=`diff $DHCP_CONF $DHCP_TMP_CONF`
   fi
   if [ -n "$FOO" ] ; then
      RESTART=1
   fi
   CURRENT_PID=""
   if [ -f $PID_FILE ];then
	   CURRENT_PID=`cat $PID_FILE`
   fi  
   if [ -z "$CURRENT_PID" ] ; then
      RESTART=1
   else
      CURRENT_PIDS=`pidof dnsmasq`
      if [ -z "$CURRENT_PIDS" ] ; then
         RESTART=1
      else
         RUNNING_PIDS=`pidof dnsmasq`
         FOO=`echo $RUNNING_PIDS | grep $CURRENT_PID`
         if [ -z "$FOO" ] ; then
            RESTART=1
         fi
   
         	#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
         	#Check for the case where dnsmasq is running without config file
          	FOO=`cat /proc/${CURRENT_PID}/cmdline | grep "$DHCP_CONF"`
          	if [ -z "$FOO" ] ; then
            	RESTART=1
          	fi
         
      fi 
   fi
   rm -f $DHCP_TMP_CONF

   killall -HUP `basename $SERVER`
   if [ "0" = "$RESTART" ] ; then
      exit 0
   fi

   killall `basename $SERVER`
   rm -f $PID_FILE

   if [ "$CONTAINER_SUPPORT" = "1" ]; then
        dnsserver_start_lxc
   fi

   # Get the DNS strict order option
   DNSSTRICT_ORDER_ENABLE=`syscfg get DNSStrictOrder`

   DNS_ADDITIONAL_OPTION=""
   # Check for RFC Enable for DNS STRICT ORDER
   if [ "x$DNSSTRICT_ORDER_ENABLE" == "xtrue" ]; then
         DNS_ADDITIONAL_OPTION=" -o "
         echo "Starting dnsmasq with additional dns strict order option: $DNS_ADDITIONAL_OPTION"
   else
         echo "RFC DNSTRICT ORDER is not defined or Enabled"
   fi

   if [ "0" = "$SYSCFG_dhcp_server_enabled" ] ; then
        dnsmasq_server_start
	sysevent set dns-status started
   else
      # we use dhcp-authoritative flag to indicate that this is
      # the only dhcp server on the local network. This allows 
      # the dns server to give out a _requested_ lease even if
      # that lease is not found in the dnsmasq.leases file
      # Get the DNS strict order option
      dnsmasq_server_start

      if [ "1" = "$DHCP_SLOW_START_NEEDED" ] && [ -n "$TIME_FILE" ] ; then
         echo "#!/bin/sh" > $TIME_FILE
         echo "   sysevent set dhcp_server-restart" >> $TIME_FILE
         chmod 700 $TIME_FILE
      fi
      sysevent set dns-status started
      sysevent set dhcp_server-status started
      #sysevent_ap set lan-restart
   fi
}

#-----------------------------------------------------------------
service_init ()
{
    FOO=`utctx_cmd get lan_ipaddr lan_netmask dhcp_server_enabled`
    eval $FOO
}

#-----------------------------------------------------------------
#--args ["pool nums"]
resync_to_nonvol ()
{
    local REM_POOLS="$1"
    local CURRENT_POOLS="`sysevent get ${SERVICE_NAME}_current_pools`"
    local LOAD_POOLS
    local NV_INST="`psmcli getallinst ${DHCPS_POOL_NVPREFIX}.`"
    if [ x = x"$REM_POOLS" ]; then
        REM_POOLS="$CURRENT_POOLS"
        LOAD_POOLS="$NV_INST"
    else
        LOAD_POOLS="$REM_POOLS"
        
        for i in $LOAD_POOLS; do
            if [ 0 = `expr match "$NV_INST" '.*\b'${i}'\b.*'` ]; then
                LOAD_POOLS="`echo $LOAD_POOLS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
            fi
        done
        
        for i in $REM_POOLS; do
            if [ 0 = `expr match "$CURRENT_POOLS" '.*\b'${i}'\b.*'` ]; then
                REM_POOLS="`echo $REM_POOLS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
            fi
        done
        
    fi
    
    #first, construct the read string to perform a single read for all pools
    for i in  $LOAD_POOLS; do 
        REM_POOLS="`echo $REM_POOLS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
        
        REQ_STRING="${REQ_STRING} ENABLED_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${ENABLE_DM} IPV4_INST_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${IPV4_DM} DHCP_START_ADDR_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${START_ADDR_DM} DHCP_END_ADDR_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${END_ADDR_DM} SUBNET_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${SUBNET_DM}  DHCP_LEASE_TIME_${i} ${DHCPS_POOL_NVPREFIX}.${i}.${LEASE_DM}"
    done
    eval `psmcli get -e ${REQ_STRING}`
    for i in $LOAD_POOLS $REM_POOLS; do 
        CURRENT_POOLS="`echo $CURRENT_POOLS| sed 's/ *\<'$i'\>\( *\)/\1/g'`"
        CUR_IPV4=`sysevent get ${SERVICE_NAME}_${i}_ipv4inst`
        eval NEW_INST=\${IPV4_INST_${i}}
        if [ x$CUR_IPV4 != x$NEW_INST -a x != x$CUR_IPV4 ]; then
            async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
            sysevent rm_async $async
        fi
        eval sysevent set ${SERVICE_NAME}_${i}_startaddr \${DHCP_START_ADDR_${i}}
        eval sysevent set ${SERVICE_NAME}_${i}_endaddr \${DHCP_END_ADDR_${i}}
        eval sysevent set ${SERVICE_NAME}_${i}_ipv4inst \${IPV4_INST_${i}}
        eval sysevent set ${SERVICE_NAME}_${i}_subnet \${SUBNET_${i}}
        eval sysevent set ${SERVICE_NAME}_${i}_leasetime \${DHCP_LEASE_TIME_${i}} 
        eval sysevent set ${SERVICE_NAME}_${i}_enabled \${ENABLED_${i}} 
    done
    
    #for i in $REM_POOLS; do 
    #    async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
    #    sysevent rm_async $async
    #done
    
    for i in $LOAD_POOLS; do 
        async="`sysevent get ${SERVICE_NAME}_${i}-ipv4async`"
        if [ x = x"$async" ]; then
            eval async=\"\`sysevent async ipv4_\${IPV4_INST_${i}}-status ${UTOPIAROOT}/service_${SERVICE_NAME}.sh\`\"
            sysevent set ${SERVICE_NAME}_${i}-ipv4async "$async"
        fi
    done
    
    sysevent set ${SERVICE_NAME}_current_pools "$CURRENT_POOLS $LOAD_POOLS"
    
}

#-----------------------------------------------------------------
dhcp_server_start ()
{
   if [ "0" = "$SYSCFG_dhcp_server_enabled" ] ; then
      #when disable dhcp server in gui, we need remove the corresponding process in backend, or the dhcp server still work.
      dhcp_server_stop

      sysevent set dhcp_server-status error
      sysevent set dhcp_server-errinfo "dhcp server is disabled by configuration" 
      if [ "$IS_BCI" = "yes" ]; then
      echo_t "SERVICE DHCP : DHCPv4 Service is stopped"
      fi
      rm -f /var/tmp/lan_not_restart
	  return 0
   fi
   
   DHCP_STATE=`sysevent get lan_status-dhcp`
   #if [ "started" != "$CURRENT_LAN_STATE" ] ; then
   if [ "started" != "$DHCP_STATE" ] ; then
   	  if [ "$IS_BCI" = "yes" ] && [ -z "$DHCP_STATE" ] && [ "$CALL_ARG" = "dhcp_server-restart" ]; then
   	  	# If we are calling dhcp_server_start from a restart event, it's possible that lan_status-dhcp may have never
        # been set if DHCP was disabled on boot.
        echo_t "SERVICE DHCP : DHCPv4 Service state is empty, allowing restart"
   	  else
         rm -f /var/tmp/lan_not_restart
         exit 0
      fi
   fi

    dhcp_inprogress_wait_count=0
    while [ x"`sysevent get dhcp_server-progress`" == "xinprogress" ] && [ $dhcp_inprogress_wait_count -lt 5 ]; do
          echo_t "SERVICE DHCP : dhcp_server-progress is inprogress , waiting..."
          sleep 2
          dhcp_inprogress_wait_count=$((dhcp_inprogress_wait_count+1))
    done

   sysevent set dhcp_server-progress inprogress
   echo_t "SERVICE DHCP : dhcp_server-progress is set to inProgress from dhcp_server_start"
   sysevent set ${SERVICE_NAME}-errinfo
   #wait_till_end_state dhcp_server
   #wait_till_end_state dns

   # since dnsmasq acts as both dhcp server and dns forwarder
   # we need to decide whether to start dnsmasq or just sighup it
   # one criterea is whether the dnsmasq.conf file changes
   DHCP_TMP_CONF="/tmp/dnsmasq.conf.orig"
   if [ -f $DHCP_CONF ];then
   	cp -f $DHCP_CONF $DHCP_TMP_CONF
   fi
   # set hostname and /etc/hosts cause we are the dns forwarder
   prepare_hostname
   # also prepare dhcp conf cause we are the dhcp server too
   prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask
   # remove any extraneous leases
   sanitize_leases_file

   # we need to decide whether to completely restart the dns/dhcp_server
   # or whether to just have it reread everything
   # SIGHUP is reread (except for dnsmasq.conf)
   RESTART=0
   FOO=""
   if [ -f $DHCP_CONF ] && [ -f $DHCP_TMP_CONF ];then
	   FOO=`diff $DHCP_CONF $DHCP_TMP_CONF`
   fi
   if [ -n "$FOO" ] ; then
      RESTART=1
   fi
   
   CURRENT_PID=""
   if [ -f $PID_FILE ];then
           CURRENT_PID=`cat $PID_FILE`
   fi
   if [ -z "$CURRENT_PID" ] ; then
      RESTART=1
   else
      RUNNING_PIDS=`pidof dnsmasq`
      if [ -z "$RUNNING_PIDS" ] ; then
         RESTART=1
      else
         FOO=`echo $RUNNING_PIDS | grep $CURRENT_PID`
         if [ -z "$FOO" ] ; then
            RESTART=1
         fi
         
         	#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
         	#Check for the case where dnsmasq is running without config file
          	FOO=`cat /proc/${CURRENT_PID}/cmdline | grep "$DHCP_CONF"`
          	if [ -z "$FOO" ] ; then
            	RESTART=1
          	fi
         
      fi
   fi

   rm -f $DHCP_TMP_CONF

   killall -HUP `basename $SERVER`
   if [ "0" = "$RESTART" ] ; then
         sysevent set dhcp_server-status started
         sysevent set dhcp_server-progress completed
         rm -f /var/tmp/lan_not_restart
         return 0
   fi

   sysevent set dns-status stopped
   killall `basename $SERVER`
   rm -f $PID_FILE
   
   #Send SIGKILL to dnsmasq process if its not killed properly with SIGTERM
   if [ ! -z `pidof dnsmasq` ] ; then
         echo_t "SERVICE DHCP : dnsmasq process killed with SIGKILL "
         kill -KILL `pidof $SERVER`
   fi

   InterfaceInConf=""
   Bridge_Mode_t=`sysevent get bridge_mode`

   InterfaceInConf=`grep "interface=" $DHCP_CONF`

   if [ "x$InterfaceInConf" = "x" ] && [ "0" != "$Bridge_Mode_t" ] ; then
        echo "dnsmasq.conf interface info not found"
        $PMON unsetproc dhcp_server
        sysevent set dhcp_server-status stopped
        sysevent set dhcp_server-progress completed
        rm -f /var/tmp/lan_not_restart
        return 0
   fi

   if [ "$CONTAINER_SUPPORT" = "1" ]; then
      dnsserver_start_lxc
   fi

   # we use dhcp-authoritative flag to indicate that this is
   # the only dhcp server on the local network. This allows
   # the dns server to give out a _requested_ lease even if
   # that lease is not found in the dnsmasq.leases file

   # Get the DNS strict order option
   DNSSTRICT_ORDER_ENABLE=`syscfg get DNSStrictOrder`

   DNS_ADDITIONAL_OPTION=""
   # Check for RFC Enable for DNS STRICT ORDER
   if [ "x$DNSSTRICT_ORDER_ENABLE" == "xtrue" ]; then
         DNS_ADDITIONAL_OPTION=" -o "
         echo "Starting dnsmasq with additional dns strict order option: $DNS_ADDITIONAL_OPTION"
   else
         echo "RFC DNSTRICT ORDER is not defined or Enabled"
   fi
   
   echo_t "RDKB_SYSTEM_BOOT_UP_LOG : starting dhcp-server_from_dhcp_server_start:`uptime | cut -d "," -f1 | tr -d " \t\n\r"`"
   dnsmasq_server_start

   if [ $? -eq 0 ]; then
   	echo_t "$SERVER process started successfully"
   else
   	if [ "$BOX_TYPE" = "XB6" ] || [ "$BOX_TYPE" = "PUMA7_CGP" ]; then
   
        	COUNTER=0
        	while [ $COUNTER -lt 5 ]; do
   			echo_t "$SERVER process failed to start sleep for 5 sec and restart it"
                        sleep 5
			dnsmasq_server_start
                	if [ $? -eq 0 ]; then
				break
			fi
			COUNTER=$COUNTER+1
		done
	fi 
   fi

   if [ "1" = "$DHCP_SLOW_START_NEEDED" ] && [ -n "$TIME_FILE" ]; then
         echo "#!/bin/sh" > $TIME_FILE
         echo "   sysevent set dhcp_server-restart lan_not_restart" >> $TIME_FILE
         chmod 700 $TIME_FILE
   fi
   #sysevent_ap set lan-restart

   #USGv2: to refresh Ethernet ports/WiFI/MoCA
   PSM_MODE=`sysevent get system_psm_mode`
   if [ "$PSM_MODE" != "1" ]; then
       if [ ! -f "/var/tmp/lan_not_restart" ] && [ "$1" != "lan_not_restart" ]; then
           if [ x"ready" = x`sysevent get start-misc` ]; then
               echo_t "RDKB_SYSTEM_BOOT_UP_LOG : Call gw_lan_refresh_from_dhcpscript:`uptime | cut -d "," -f1 | tr -d " \t\n\r"`"
               gw_lan_refresh &
	       fi
       else
           rm -f /var/tmp/lan_not_restart
           echo_t "lan_not_restart found! Don't restart lan!"
       fi
   fi

   if [ ! -f "/tmp/dhcp_server_start" ]; then
       echo_t "dhcp_server_start is called for the first time private LAN initization is complete"
       print_uptime "boot_to_ETH_uptime"

       echo_t "LAN initization is complete notify SSID broadcast"
       if [ -f "/usr/bin/rpcclient" ] ; then
           rpcclient $ATOM_ARPING_IP "/bin/touch /tmp/.advertise_ssids"
       fi

       touch /tmp/dhcp_server_start
   fi

   # This function is called for brlan0 and brlan1
   # If XHS_INTERFACE is available then XHS service is available post all DHCP configuration   
   if [ -z "$XHS_INTERFACE" ]; then
       isAvailableXHS=""
   else
       isAvailableXHS=`ifconfig | grep $XHS_INTERFACE`
   fi

   if [ "$isAvailableXHS" != "" ]; then
       echo_t "Xfinityhome service is UP"
       if [ ! -f "/tmp/xhome_start" ]; then
           print_uptime "boot_to_XHOME_uptime"
           touch /tmp/xhome_start
       fi
   else
       echo_t "Xfinityhome service is not UP yet"
   fi
       	
   $PMON setproc dhcp_server $BIN $PID_FILE "/etc/utopia/service.d/service_dhcp_server.sh dhcp_server-restart" 
   sysevent set dns-status started
   sysevent set dhcp_server-status started
   sysevent set dhcp_server-progress completed
   if [ "$IS_BCI" = "yes" ]; then
   echo_t "DHCP SERVICE :DHCPv4 Service is started"
   fi
   echo_t "DHCP SERVICE :dhcp_server-progress_is_set_to_completed:`uptime | cut -d "," -f1 | tr -d " \t\n\r"`"

   echo_t "RDKB_DNS_INFO is : -------  resolv_conf_dump  -------"
   cat $RESOLV_CONF

   # tcxb6-6420, in eth-wan mode,
   # if DNS 127.0.0.0 is in resolv.conf, it means wan is not up, when we 
   # later get a valid DNS, call gw_lan_refresh to force clients to update 
   # its DNS
   if [ -f "/nvram/ETHWAN_ENABLE" ]
   then
       has_dns_127=`grep 127.0.0.1 $RESOLV_CONF`
       had_dns_127=`sysevent get clients-have-dns-127`
       if [ "$has_dns_127" != "" ]
       then
           echo_t "clients have DNS 127"
           sysevent set clients-have-dns-127 true
       elif [ "$had_dns_127" = "true" ]
       then
           echo_t "We had DNS 127, now have a valid DNS, do gw_lan_refresh"
           gw_lan_refresh
           sysevent set clients-have-dns-127 false
       fi
   fi

}

#-----------------------------------------------------------------
dhcp_server_stop ()
{
   wait_till_end_state dhcp_server
   DHCP_STATUS=`sysevent get dhcp_server-status`
   if [ "stopped" = "$DHCP_STATUS" ] ; then
      return 0
   fi
   
   #dns is always running
   prepare_hostname
   prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask dns_only
   $PMON unsetproc dhcp_server
   sysevent set dns-status stopped
   killall `basename $SERVER`
   rm -f $PID_FILE
   sysevent set dhcp_server-status stopped

   if [ "$CONTAINER_SUPPORT" = "1" ]; then
        dnsserver_start_lxc
   fi

   # Get the DNS strict order option
   DNSSTRICT_ORDER_ENABLE=`syscfg get DNSStrictOrder`

   DNS_ADDITIONAL_OPTION=""
   # Check for RFC Enable for DNS STRICT ORDER
   if [ "x$DNSSTRICT_ORDER_ENABLE" == "xtrue" ]; then
         DNS_ADDITIONAL_OPTION=" -o "
         echo "Starting dnsmasq with additional dns strict order option: $DNS_ADDITIONAL_OPTION"
   else
         echo "RFC DNSTRICT ORDER is not defined or Enabled"
   fi

   # restart the dns server
   dnsmasq_server_start
   sysevent set dns-status started
}

#-----------------------------------------------------------------
dns_stop ()
{
   $PMON unsetproc dhcp_server
   killall `basename $SERVER`
   rm -f $PID_FILE
   sysevent set dns-status stopped
   sysevent set dhcp_server-status stopped
}

#-----------------------------------------------------------------
dns_start ()
{
   wait_till_end_state dns
   wait_till_end_state dhcp_server
   DHCP_STATE=`sysevent get dhcp_server_status`
   byoi_bridge_mode=`sysevent get byoi_bridge_mode`
   if [ "0" = "$SYSCFG_dhcp_server_enabled" ] || [ "1" = "$byoi_bridge_mode" ] ; then
      DHCP_STATE=stopped
   fi

   # since sighup doesnt reread dnsmasq.conf, we have to stop the
   # dnsmasq if it is running 

   if [ "stopped" = "$DHCP_STATE" ] ; then
      # set hostname and /etc/hosts cause we are the dns forwarder
      prepare_hostname
      prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask dns_only
   else
      # set hostname and /etc/hosts cause we are the dns forwarder
      prepare_hostname
      # also prepare dhcp conf cause we are the dhcp server too
      prepare_dhcp_conf $SYSCFG_lan_ipaddr $SYSCFG_lan_netmask
      # remove any extraneous leases
      sanitize_leases_file
   fi

   if [ "stopped" != "$DHCP_STATE" ] ; then
      killall -HUP `basename $SERVER`
      sysevent set dhcp_server-status stopped
   fi
   killall `basename $SERVER`
   rm -f $PID_FILE

   # Get the DNS strict order option
   DNSSTRICT_ORDER_ENABLE=`syscfg get DNSStrictOrder`

   DNS_ADDITIONAL_OPTION=""
   # Check for RFC Enable for DNS STRICT ORDER
   if [ "x$DNSSTRICT_ORDER_ENABLE" == "xtrue" ]; then
         DNS_ADDITIONAL_OPTION=" -o "
         echo "Starting dnsmasq with additional dns strict order option: $DNS_ADDITIONAL_OPTION"
   else
         echo "RFC DNSTRICT ORDER is not defined or Enabled"
   fi

   # we use dhcp-authoritative flag to indicate that this is
   # the only dhcp server on the local network. This allows
   # the dns server to give out a _requested_ lease even if
   # that lease is not found in the dnsmasq.leases file
   if [ "stopped" = "$DHCP_STATE" ]; then
	dnsmasq_server_start
   else
        dnsmasq_server_start
	 if [ $? -eq 0 ]; then
   		echo_t "$SERVER process started successfully"
   	 else
   		if [ "$BOX_TYPE" = "XB6" ] || [ "$BOX_TYPE" = "PUMA7_CGP" ]; then
   
        		COUNTER=0
        		while [ $COUNTER -lt 5 ]; do
   				echo_t "$SERVER process failed to start sleep for 5 sec and restart it"
				sleep 5
				dnsmasq_server_start
                		if [ $? -eq 0 ]; then
					break
				fi
				COUNTER=$COUNTER+1
			done
		fi 
   	fi
   fi
   
   sysevent set dns-status started
   if [ "stopped" != "$DHCP_STATE" ] ; then
      sysevent set dhcp_server-status started
   fi

   if [ "1" = "$DHCP_SLOW_START_NEEDED" ] && [ -n "$TIME_FILE" ] ; then
         echo "#!/bin/sh" > $TIME_FILE
         echo "   touch /var/tmp/lan_not_restart" >> $TIME_FILE
         echo "   sysevent set dhcp_server-restart" >> $TIME_FILE
         chmod 700 $TIME_FILE
   fi

   $PMON setproc dhcp_server $BIN $PID_FILE "/etc/utopia/service.d/service_dhcp_server.sh dns-restart"
}

#-----------------------------------------------------------------------
#-----------------------------------------------------------------------

service_init

case "$1" in
   ${SERVICE_NAME}-start)
	  echo_t "SERVICE DHCP : Got start.. call dhcp_server_start"
      dhcp_server_start
      ;;
   ${SERVICE_NAME}-stop)
      echo_t "SERVICE DHCP : Got stop with $2.. Call dhcp_server_stop"
      dhcp_server_stop
      ;;
   ${SERVICE_NAME}-restart)
      #dhcp_server_stop
	  echo_t "SERVICE DHCP : Got restart with $2.. Call dhcp_server_start"
      dhcp_server_start $2
      ;;
   dns-start)
      dns_start
      ;;
   dns-stop)
      dns_stop
      ;;
   dns-restart)
      dns_start
      ;;
   lan-status)
	  echo_t "SERVICE DHCP : Got lan_status"
      lan_status_change $CURRENT_LAN_STATE
	  #if [ "$CURRENT_LAN_STATE" = "started" -a ! -f /tmp/fresh_start ]; then
	  #	  gw_lan_refresh&
	  #	  touch /tmp/fresh_start
	  #	  echo_t "Rstart LAN for first boot up"
	  # fi
      ;;
   syslog-status)
      STATUS=`sysevent get syslog-status`
      if [ "started" = "$STATUS" ] ; then
         restart_request
      fi
      ;;
   delete_lease)
      ulog dnsmasq status "($PID) Called because of lease deleted command"
      delete_dhcp_lease $2
      ;;
   ${SERVICE_NAME}-resync)
      if [ x"NULL" != x"$2" ]; then
        ARG="$2"
      fi
      resync_to_nonvol "$ARG"
      #dhcp_server_start
      ;;
    ipv4_*-status)
        if [ x"up" = x$2 ]; then
	        echo_t "SERVICE DHCP : Got ipv4 status"
            if [ "$BOX_TYPE" = "XB3" ]; then
		#setting lan_status-dhcp to started for handling dhcp_server-restart in brige mode
		sysevent set lan_status-dhcp started
		echo_t "SERVICE DHCP : $1, calling dhcp_server-restart lan_not_restart event"
		sysevent set dhcp_server-restart lan_not_restart
                #service_dhcp lan-status started lan_not_restart
            else
                lan_status_change started lan_not_restart
            fi
        fi
      ;;
   *)
      echo_t "Usage: $SERVICE_NAME [start|stop|restart]" >&2
      exit 3
      ;;
esac
