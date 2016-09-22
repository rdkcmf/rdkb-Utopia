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

#------------------------------------------------------------------
# Copyright (c) 2008 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

DHCP_CONF=/etc/dnsmasq.conf
DHCP_STATIC_HOSTS_FILE=/etc/dhcp_static_hosts
DHCP_OPTIONS_FILE=/var/dhcp_options
LOCAL_DHCP_CONF=/tmp/dnsmasq.conf$$
LOCAL_DHCP_STATIC_HOSTS_FILE=/tmp/dhcp_static_hosts$$
LOCAL_DHCP_OPTIONS_FILE=/tmp/dhcp_options$$
RESOLV_CONF=/etc/resolv.conf

# Variables needed for captive portal mode : start
DEFAULT_RESOLV_CONF="/var/default/resolv.conf"
DEFAULT_CONF_DIR="/var/default"
XCONF_FILE=/etc/Xconf
STATIC_URLS_FILE="/etc/static_urls"
XCONF_DOWNLOAD_URL="/tmp/xconfdownloadurl"
XCONF_DEFAULT_URL="https://xconf.xcal.tv/xconf/swu/stb/"
XFINITY_DEFAULT_URL="http://xfinity.com"
SPEEDTEST_DEFAULT_URL="http://speedtest.comcast.net"
XFINITY_RED_DEFAULT_URL="http://my.xfinity.com"
COMCAST_DEFAULT_URL="www.comcast.com"
COMCAST_ACTIVATE_URL="https://activate.comcast.com"
COMCAST_ACTIVATE_URL_2="https://caap-pdca.sys.comcast.net"
COMCAST_HTTP_URL="http://comcast.com"

# Variables needed for captive portal mode : end

DHCP_SLOW_START_1_FILE=/etc/cron/cron.everyminute/dhcp_slow_start.sh
DHCP_SLOW_START_2_FILE=/etc/cron/cron.every5minute/dhcp_slow_start.sh
DHCP_SLOW_START_3_FILE=/etc/cron/cron.every10minute/dhcp_slow_start.sh


DHCP_LEASE_FILE=/nvram/dnsmasq.leases
DHCP_ACTION_SCRIPT=/etc/utopia/service.d/service_dhcp_server/dnsmasq_dhcp.script
# DHCP_FIRST_OCTETS will contain the first 3 octets of the lan interface
DHCP_FIRST_OCTETS=0.0.0


# DHCP_START is the starting value of the last octet of the available
# dhcp address range
DHCP_START=`syscfg get dhcp_start`
# DHCP_NUM is the number of available dhcp address for the lan
DHCP_NUM=`syscfg get dhcp_num`
if [ "" = "$DHCP_NUM" ] ; then
   DHCP_NUM=0
fi

# are we propagating the nameserver learned from wan dhcp client to our lan clients
PROPAGATE_NS=`syscfg get dhcp_server_propagate_wan_nameserver`
# if Filter Internet NAT Redirection is enabled then we need to make sure that
# the nameservers are propogated, because the expected behavior of that feature
# is that all lan-to-lan communication is disallowed (including dns queries to router)
if [ "1" != "$PROPAGATE_NS" ] ; then
   PROPAGATE_NS=`syscfg get block_nat_redirection`
fi

# are we propagating the domain name learned from wan dhcp client to our lan clients
PROPAGATE_DOM=`syscfg get dhcp_server_propagate_wan_domain`
# is dhcp slow start feature enabled
SLOW_START=`syscfg get dhcp_server_slow_start`
SYSCFG_byoi_enabled=`syscfg get byoi_enabled`
if [ "1" = "$PROPAGATE_NS" ] || [ "1" = "$PROPOGATE_DOM" ] || [ "1" = "$SYSCFG_byoi_enabled" ]; then
   if [ "1" = "$SLOW_START" ] ; then
      wan_ipaddr=`sysevent get current_wan_ipaddr`
      primary_temp_ip_prefix=`syscfg get primary_temp_ip_prefix`
      if [ "0.0.0.0" = "$wan_ipaddr" ] ; then
         DHCP_SLOW_START_NEEDED=1
      fi
      if [ "1" = "$SYSCFG_byoi_enabled" ] && [ "primary" = "`sysevent get current_hsd_mode`" ] &&
         [ "$primary_temp_ip_prefix" = ${wan_ipaddr:0:${#primary_temp_ip_prefix}} ] ; then
         DHCP_SLOW_START_NEEDED=1
      fi
   fi
fi
#Disable this to alway pick lease value from syscfg.db
DHCP_SLOW_START_NEEDED=0
#DHCP_LEASE_TIME is the number of seconds or minutes or hours to give as a lease
DHCP_LEASE_TIME=`syscfg get dhcp_lease_time`
if [ "1" = "$DHCP_SLOW_START_NEEDED" ] ; then
   SYSCFG_temp_dhcp_lease_length=`syscfg get temp_dhcp_lease_length`
   if [ "" = "$SYSCFG_temp_dhcp_lease_length" ]; then
      DHCP_SLOW_START_QUANTA=`sysevent get dhcp_slow_start_quanta`
   else
      DHCP_SLOW_START_QUANTA=$SYSCFG_temp_dhcp_lease_length
   fi
   if [ "" = "$DHCP_SLOW_START_QUANTA" ] ; then
      DHCP_SLOW_START_QUANTA=1
      TIME_FILE=$DHCP_SLOW_START_1_FILE
   elif [ "" = "$SYSCFG_temp_dhcp_lease_length" ]; then
      if [ "$DHCP_SLOW_START_QUANTA" -lt 5 ] ; then
         DHCP_SLOW_START_QUANTA=`expr $DHCP_SLOW_START_QUANTA + 1`
         TIME_FILE=$DHCP_SLOW_START_1_FILE
      elif [ "$DHCP_SLOW_START_QUANTA" -le 15 ] ; then
         DHCP_SLOW_START_QUANTA=`expr $DHCP_SLOW_START_QUANTA + 5`
         TIME_FILE=$DHCP_SLOW_START_2_FILE
      elif [ "$DHCP_SLOW_START_QUANTA" -le 100 ] ; then
         DHCP_SLOW_START_QUANTA=`expr $DHCP_SLOW_START_QUANTA \* 2`
         TIME_FILE=$DHCP_SLOW_START_3_FILE
      else
         DHCP_SLOW_START_QUANTA=$DHCP_LEASE_TIME
         TIME_FILE=$DHCP_SLOW_START_3_FILE
      fi
   fi
   if [ "" = "$SYSCFG_temp_dhcp_lease_length" ] && [ "$DHCP_SLOW_START_QUANTA" -gt 60 ] ; then
      DHCP_SLOW_START_QUANTA=60
   fi
   sysevent set dhcp_slow_start_quanta $DHCP_SLOW_START_QUANTA
   DHCP_LEASE_TIME=$DHCP_SLOW_START_QUANTA
else
   sysevent set dhcp_slow_start_quanta
fi

if [ "" = "$DHCP_LEASE_TIME" ] ; then
   DHCP_LEASE_TIME=24h
fi


#--------------------------------------------------------------
#  figure out the dhcp range to use
#   The lan interface ip address
#   The lan interface netmask
#--------------------------------------------------------------
calculate_dhcp_range () {
   # extract last octet of the lan netmask
   NETMASK_LAST_OCTET=""
   SAVEIFS=$IFS
   IFS=.
   for p in $2
   do
       NETMASK_LAST_OCTET=$p
   done
   IFS=$SAVEIFS

   # check if dhcp start is just a last octet
   OCTET_NUM=`echo $DHCP_START|wc -m`
   if [ "$OCTET_NUM" -gt "4" ] ; then # full ip address format
      DHCP_START_ADDR=$DHCP_START
      DHCP_END_ADDR=`syscfg get dhcp_end`
   else                               # just last octet
      if [ "$DHCP_START" -lt "2" ] ; then
         DHCP_START=2
      fi
      DHCP_END=`expr $DHCP_START + $DHCP_NUM`
      DHCP_END=`expr $DHCP_END - 1`

      # ensure that we are not allocating the broadcast address
      LAST_IP=`expr $NETMASK_LAST_OCTET + $DHCP_END`
      while [ "$LAST_IP" -gt "254" ]
      do
         DHCP_END=`expr $DHCP_END - 1`
         LAST_IP=`expr $NETMASK_LAST_OCTET + $DHCP_END`
         DHCP_NUM=`expr $DHCP_NUM - 1`
      done

      if [ "$DHCP_START" -gt "$DHCP_END" ] ; then
         DHCP_END=$DHCP_START
         DHCP_NUM=0
      fi

      # extract 1st 3 octets of the lan ip address and concatenate the
      # start and end octet for figuring out the dhcp address range
      TEMP=""
      LAST=""
      SAVEIFS=$IFS
      IFS=.
      for p in $1
      do
         if [ "" = "$LAST" ] ; then
            LAST=$TEMP
         else
            LAST=$LAST"."$TEMP
         fi
         TEMP=$p
      done
      IFS=$SAVEIFS

      DHCP_START_ADDR=$LAST"."$DHCP_START
      DHCP_END_ADDR=$LAST"."$DHCP_END
      # update syscfg dhcp_start, dhcp_end to not use just last octet
      syscfg set dhcp_start $DHCP_START_ADDR
      syscfg set dhcp_end $DHCP_END_ADDR
      syscfg commit
      DHCP_FIRST_OCTETS=$LAST
   fi
   
}

#--------------------------------------------------------------
# Prepare the dhcp server static hosts/ip file
#--------------------------------------------------------------
prepare_dhcp_conf_static_hosts() {
   NUM_STATIC_HOSTS=`syscfg get dhcp_num_static_hosts`
   echo -n > $LOCAL_DHCP_STATIC_HOSTS_FILE

   for N in $(seq 1 $NUM_STATIC_HOSTS)
   do
      HOST_LINE=`syscfg get dhcp_static_host_$N`
      if [ "none" != "$HOST_LINE" ] ; then
         #MAC=""
         #SAVEIFS=$IFS
         #IFS=,
         #set -- $HOST_LINE
         #MAC=$1
         #shift
         #IP=$DHCP_FIRST_OCTETS.$1
         #shift
         #FRIENDLY_NAME=$1
         #IFS=$SAVEIFS
         #echo "$MAC,$IP,$FRIENDLY_NAME" >> $LOCAL_DHCP_STATIC_HOSTS_FILE
         echo "$HOST_LINE,$DHCP_LEASE_TIME" >> $LOCAL_DHCP_STATIC_HOSTS_FILE
      fi
   done
   cat $LOCAL_DHCP_STATIC_HOSTS_FILE > $DHCP_STATIC_HOSTS_FILE
   rm -f $LOCAL_DHCP_STATIC_HOSTS_FILE
}

#--------------------------------------------------------------
# Prepare the dhcp options file
#--------------------------------------------------------------
prepare_dhcp_options() {
   echo -n > $LOCAL_DHCP_OPTIONS_FILE
   STATIC_NAMESERVER_ENABLED=`syscfg get staticdns_enable`
   DHCP_OPTION_STR=

    # nameservers come from 3 parts:
    # Static LAN DNS, Static WAN DNS, Dynamic WAN DNS
   
    # add Lan static DNS
    NAMESERVERENABLED=`syscfg get dhcp_nameserver_enabled`
    NAMESERVER1=`syscfg get dhcp_nameserver_1`
    NAMESERVER2=`syscfg get dhcp_nameserver_2`
    NAMESERVER3=`syscfg get dhcp_nameserver_3`
   
    if [ "1" = "$NAMESERVERENABLED" ] ; then
      if [ "0.0.0.0" != "$NAMESERVER1" ] && [ "" != "$NAMESERVER1" ] ; then
         if [ "" = "$DHCP_OPTION_STR" ] ; then
            DHCP_OPTION_STR="option:dns-server, "$NAMESERVER1
         else
            DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER1
         fi
      fi
    
      if [ "0.0.0.0" != "$NAMESERVER2" ]  && [ "" != "$NAMESERVER2" ]; then
         if [ "" = "$DHCP_OPTION_STR" ] ; then
            DHCP_OPTION_STR="option:dns-server, "$NAMESERVER2
         else
            DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER2
         fi
      fi
    
      if [ "0.0.0.0" != "$NAMESERVER3" ]  && [ "" != "$NAMESERVER3" ]; then
         if [ "" = "$DHCP_OPTION_STR" ] ; then
            DHCP_OPTION_STR="option:dns-server, "$NAMESERVER3
         else
            DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER3
         fi
      fi
    fi

    # Propagate Wan DNS
    if [ "1" = "$PROPAGATE_NS" ] ; then

      if [ "$STATIC_NAMESERVER_ENABLED" = "1" ] ; then

        #Wan static DNS
        NS=`syscfg get nameserver1`
        if [ "0.0.0.0" != "$NS" ]  && [ "" != "$NS" ] ; then
           if [ "" = "$DHCP_OPTION_STR" ] ; then
              DHCP_OPTION_STR="option:dns-server, "$NS
           else
              DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
           fi
        fi
    
        NS=`syscfg get nameserver2`
        if [ "0.0.0.0" != "$NS" ] && [ "" != "$NS" ] ; then
           if [ "" = "$DHCP_OPTION_STR" ] ; then
              DHCP_OPTION_STR="option:dns-server, "$NS
           else
              DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
           fi
        fi
    
        NS=`syscfg get nameserver3`
        if [ "0.0.0.0" != "$NS" ] && [ "" != "$NS" ] ; then
           if [ "" = "$DHCP_OPTION_STR" ] ; then
             DHCP_OPTION_STR="option:dns-server, "$NS
           else
              DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
           fi
        fi
        
      else   
        #Wan Dynamic DNS from dhcp protocol
        NS=`sysevent get wan_dhcp_dns`
        if [ "" != "$NS" ] ; then
           NS=`echo "$NS" | sed "s/ /,/g"`
           if [ "" = "$DHCP_OPTION_STR" ] ; then
              DHCP_OPTION_STR="option:dns-server, "$NS
           else
              DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
           fi
        fi
      fi
     
    fi

#/*USG needs to support DNS Passthrough, so don't add it as a dns svr. soyou*/
   # add utopia as nameserver of last resort
#   NS=` syscfg get lan_ipaddr `
#   if [ "" != "$NS" ] ; then
#      if [ "" = "$DHCP_OPTION_STR" ] ; then
#         DHCP_OPTION_STR="option:dns-server, "$NS
#      else
#         DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
#      fi
#   fi
   echo $DHCP_OPTION_STR >> $LOCAL_DHCP_OPTIONS_FILE

   # if we need to provision a wins server
   # we use netbios-ns instead of option 44
   WINS_SERVER=`syscfg get dhcp_wins_server`
   if [ "" != "$WINS_SERVER" ] && [ "0.0.0.0" != "$WINS_SERVER" ] ; then
      echo "option:netbios-ns,"$WINS_SERVER >> $LOCAL_DHCP_OPTIONS_FILE
   fi

   cat $LOCAL_DHCP_OPTIONS_FILE > $DHCP_OPTIONS_FILE
   rm -f $LOCAL_DHCP_OPTIONS_FILE

}

prepare_dhcp_options_wan_dns()
{
   echo -n > $LOCAL_DHCP_OPTIONS_FILE
   DHCP_OPTION_STR=

   # Propagate Wan DNS
   if [ "1" = "$PROPAGATE_NS" ] ; then
      #Wan Dynamic DNS from dhcp protocol
      NS=`sysevent get wan_dhcp_dns`
      if [ "" != "$NS" ] ; then
         NS=`echo "$NS" | sed "s/ /,/g"`
         if [ "" = "$DHCP_OPTION_STR" ] ; then
            DHCP_OPTION_STR="option:dns-server, "$NS
         else
            DHCP_OPTION_STR=$DHCP_OPTION_STR","$NS
         fi
      fi
   fi

   echo $DHCP_OPTION_STR >> $LOCAL_DHCP_OPTIONS_FILE
   cat $LOCAL_DHCP_OPTIONS_FILE > $DHCP_OPTIONS_FILE
   rm -f $LOCAL_DHCP_OPTIONS_FILE
}

# A generic function which can be used for any URL parsing
removehttp()
{
	urlToCheck=$1
	haveHttp=`echo $urlToCheck | grep //`
	if [ "$haveHttp" != "" ]
	then
		url=`echo $urlToCheck | cut -f2 -d":" | cut -f3 -d"/"`
		echo $url
	else
		echo $urlToCheck
	fi
		
}

# This function will whitelist URLs that are needed during cpative portal mode
prepare_whitelist_urls()
{
    #ACS_URL=""
	Redirection_URL=""
	CloudPersonalization_URL=""
	isIPv4=""
	isIPv6=""
	nServer4=""
	nServer6=""
    #EMS_URL=""
	

	# Redirection URL can be get from DML
	Redirection_URL=`syscfg get redirection_url`
	if [ "$Redirection_URL" != "" ]
	then
		Redirection_URL=`removehttp $Redirection_URL`
	fi

	# CloudPersonalization URL can be get from DML	
	CloudPersonalization_URL=`syscfg get CloudPersonalizationURL`
	if [ "$CloudPersonalization_URL" != "" ]
	then
		CloudPersonalization_URL=`removehttp $CloudPersonalization_URL`
	fi

	#Check in what mode erouter0 is in : ipv4/ipv6
	isIPv4=`ifconfig erouter0 | grep inet | grep -v inet6`
	if [ "$isIPv4" = "" ]
	then
		isIPv6=`ifconfig erouter0 | grep inet6`
		if [ "$isIPv6" != "" ]
		then
			nServer6=`cat $RESOLV_CONF | grep nameserver | grep ":" | head -n 1 | cut -d" " -f2`
		fi
	else	
			nServer4=`cat $RESOLV_CONF | grep nameserver | grep "\." | head -n 1 | cut -d" " -f2`
	fi
	
	#TODO: ipv6 DNS whitelisting in case of ipv6 only clients
	
	# Whitelist all server IPs with IPv4 DNS servers.
	if [ "$nServer4" != "" ]
	then

		if [ "$Redirection_URL" != "" ]; then
			echo "server=/$Redirection_URL/$nServer4" >> $1
		fi

		if [ "$CloudPersonalization_URL" != "" ]; then
			echo "server=/$CloudPersonalization_URL/$nServer4" >> $1
		fi

        if [ -f $STATIC_URLS_FILE ]; then
         STATIC_URL_LIST=`cat $STATIC_URLS_FILE`
         for whitelisting_url in $STATIC_URL_LIST
         do
            echo "server=/$whitelisting_url/$nServer4" >> $1
         done
      fi

	
	fi
}

#-----------------------------------------------------------------
# set the dhcp config file which is also the dns forwarders file
#  Parameters:
#     lan ip address      eg. 192.168.1.1
#     lan netmask         eg. 255.255.255.0
#     dns_only  (if no dhcp server is required)
#-----------------------------------------------------------------
prepare_dhcp_conf () {
   echo "DHCP SERVER : Prepare DHCP configuration"
   LAN_IFNAME=`syscfg get lan_ifname`

  echo -n > $DHCP_STATIC_HOSTS_FILE

   if [ "$3" = "dns_only" ] ; then
      PREFIX=#
   else 
      PREFIX=
   fi
   #calculate_dhcp_range $1 $2

   echo -n > $LOCAL_DHCP_CONF


   CAPTIVE_PORTAL_MODE="false"

   #Read the http response value
   NETWORKRESPONSESTATUS=`cat /var/tmp/networkresponse.txt`

    
   # If redirection flag is "true" that means we are in factory default condition
   CAPTIVEPORTAL_ENABLED=`syscfg get CaptivePortal_Enable`
   REDIRECTION_ON=`syscfg get redirection_flag`
   WIFI_NOT_CONFIGURED=`psmcli get eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges`

echo "DHCP SERVER : redirection_flag val is $REDIRECTION_ON"
iter=0
max_iter=2
while [ "$WIFI_NOT_CONFIGURED" = "" ] && [ "$iter" -le $max_iter ]
do
	iter=$((iter+1))
	echo "DHCP SERVER : Inside while $iter iteration"
	WIFI_NOT_CONFIGURED=`psmcli get eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges`
done

echo "DHCP SERVER : NotifyWiFiChanges is $WIFI_NOT_CONFIGURED"
echo "DHCP SERVER : CaptivePortal_Enabled is $CAPTIVEPORTAL_ENABLED"

if [ "$CAPTIVEPORTAL_ENABLED" == "true" ]
then
	   if [ "$NETWORKRESPONSESTATUS" = "204" ] && [ "$REDIRECTION_ON" = "true" ] && [ "$WIFI_NOT_CONFIGURED" = "true" ]
	   then
	      	CAPTIVE_PORTAL_MODE="true"
			echo "DHCP SERVER : WiFi SSID and Passphrase are not modified,set CAPTIVE_PORTAL_MODE"
			if [ -e "/nvram/reverted" ]
			then
				echo "DHCP SERVER : Removing reverted flag"
				rm -f /nvram/reverted
			fi
	   else
		CAPTIVE_PORTAL_MODE="false"
			echo "DHCP SERVER : WiFi SSID and Passphrase are already modified or no network response ,set CAPTIVE_PORTAL_MODE to false"
	   fi
fi
  
   echo "domain-needed" >> $LOCAL_DHCP_CONF
   echo "bogus-priv" >> $LOCAL_DHCP_CONF

   if [ "$CAPTIVE_PORTAL_MODE" = "true" ]
   then
	# Create a temporary resolv configuration file
	# Pass that as an option in DNSMASQ
	if [ ! -d $DEFAULT_CONF_DIR ]
	then
		mkdir $DEFAULT_CONF_DIR
	fi
	touch $DEFAULT_RESOLV_CONF
	echo "nameserver 127.0.0.1" > $DEFAULT_RESOLV_CONF
	echo "resolv-file=$DEFAULT_RESOLV_CONF" >> $LOCAL_DHCP_CONF
	#echo "address=/#/$addr" >> $DHCP_CONF
   else
	if [ -e $DEFAULT_RESOLV_CONF ]
	then
		rm -f $DEFAULT_RESOLV_CONF
	fi
	echo "resolv-file=$RESOLV_CONF" >> $LOCAL_DHCP_CONF
   fi

   #echo "interface=$LAN_IFNAME" >> $LOCAL_DHCP_CONF
   echo "expand-hosts" >> $LOCAL_DHCP_CONF

   # if we are provisioned to use the wan domain name, the we do so
   # otherwise we use the lan domain name
   if [ "1" = "$PROPAGATE_DOM" ] ; then
      LAN_DOMAIN=`sysevent get dhcp_domain`
   fi
   if [ "" = "$LAN_DOMAIN" ] ; then
      LAN_DOMAIN=`syscfg get lan_domain`
   fi
   if [ "" != "$LAN_DOMAIN" ] ; then
      echo "domain=$LAN_DOMAIN" >> $LOCAL_DHCP_CONF
   fi
   LOG_LEVEL=`syscfg get log_level`
   if [ "" = "$LOG_LEVEL" ] ; then
       LOG_LEVEL=1
   fi

   if [ "$3" = "dns_only" ] ; then
      echo "no-dhcp-interface=$LAN_IFNAME" >> $LOCAL_DHCP_CONF
   fi 
   #echo "$PREFIX""dhcp-range=$DHCP_START_ADDR,$DHCP_END_ADDR,$2,$DHCP_LEASE_TIME" >> $LOCAL_DHCP_CONF
   echo "$PREFIX""dhcp-leasefile=$DHCP_LEASE_FILE" >> $LOCAL_DHCP_CONF
  # echo "$PREFIX""dhcp-script=$DHCP_ACTION_SCRIPT" >> $LOCAL_DHCP_CONF
   echo "$PREFIX""dhcp-lease-max=$DHCP_NUM" >> $LOCAL_DHCP_CONF
   echo "$PREFIX""dhcp-hostsfile=$DHCP_STATIC_HOSTS_FILE" >> $LOCAL_DHCP_CONF

   if [ "$CAPTIVE_PORTAL_MODE" = "false" ]
   then
		echo "$PREFIX""dhcp-optsfile=$DHCP_OPTIONS_FILE" >> $LOCAL_DHCP_CONF
   fi


   #if [ "$LOG_LEVEL" -gt 1 ] ; then
    #  echo "$PREFIX""log-dhcp" >> $LOCAL_DHCP_CONF
   #fi

   if [ "dns_only" != "$3" ] ; then
      prepare_dhcp_conf_static_hosts
      #prepare_dhcp_options
	  prepare_dhcp_options_wan_dns	
   fi
   
   if [ "started" = $CURRENT_LAN_STATE ]; then
      calculate_dhcp_range $1 $2
      echo "interface=$LAN_IFNAME" >> $LOCAL_DHCP_CONF
	  if [ $DHCP_LEASE_TIME == -1 ]; then
	      echo "$PREFIX""dhcp-range=$DHCP_START_ADDR,$DHCP_END_ADDR,$2,infinite" >> $LOCAL_DHCP_CONF
	  else
  	      echo "$PREFIX""dhcp-range=$DHCP_START_ADDR,$DHCP_END_ADDR,$2,$DHCP_LEASE_TIME" >> $LOCAL_DHCP_CONF
	  fi	      
   fi
   
   # For boot itme optimization, run do_extra_pool only when brlan1 interface is available
   isBrlan1=`ifconfig | grep brlan1`
   if [ "$isBrlan1" != "" ]
   then
      do_extra_pools
   fi

   iotEnabled=`syscfg get lost_and_found_enable`
   if [ "$iotEnabled" = "true" ]
   then
        echo "IOT_LOG : DHCP server configuring for IOT"
        IOT_IFNAME=`syscfg get iot_ifname`

	IOT_START_ADDR=`syscfg get iot_dhcp_start`
	IOT_END_ADDR=`syscfg get iot_dhcp_end`
	IOT_NETMASK=`syscfg get iot_netmask`
	echo "interface=$IOT_IFNAME" >> $LOCAL_DHCP_CONF
	  if [ $DHCP_LEASE_TIME == -1 ]; then
		echo "$PREFIX""dhcp-range=$IOT_START_ADDR,$IOT_END_ADDR,$IOT_NETMASK,infinite" >> $LOCAL_DHCP_CONF
	  else
		echo "$PREFIX""dhcp-range=$IOT_START_ADDR,$IOT_END_ADDR,$IOT_NETMASK,$DHCP_LEASE_TIME" >> $LOCAL_DHCP_CONF
	  fi	      
   fi

   if [ "$CAPTIVE_PORTAL_MODE" = "true" ]
   then
        # In factory default condition, prepare whitelisting and redirection IP
	addr=`syscfg get lan_ipaddr`
	echo "address=/#/$addr" >> $LOCAL_DHCP_CONF
	echo "dhcp-option=252,\"\n\"" >> $LOCAL_DHCP_CONF
        prepare_whitelist_urls $LOCAL_DHCP_CONF
	sysevent set captiveportaldhcp completed
   fi
   cat $LOCAL_DHCP_CONF > $DHCP_CONF
   rm -f $LOCAL_DHCP_CONF

   echo "DHCP SERVER : Completed preparing DHCP configuration"
}

do_extra_pools () {
    POOLS="`sysevent get ${SERVICE_NAME}_current_pools`"
    #DEBUG
    # echo "Extra pools: $POOLS"
    
    for i in $POOLS; do 
        #DNS_S1 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S1_DM} DNS_S2 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S2_DM} DNS_S3 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S3_DM}
        ENABLED=`sysevent get ${SERVICE_NAME}_${i}_enabled`
        if [ x"TRUE" != x$ENABLED ]; then
            continue;
        fi
        
        IPV4_INST=`sysevent get ${SERVICE_NAME}_${i}_ipv4inst`
        if [ x$L3_UP_STATUS != x`sysevent get ipv4_${IPV4_INST}-status` ]; then
            continue
        fi
        
        m_DHCP_START_ADDR=`sysevent get ${SERVICE_NAME}_${i}_startaddr`
        m_DHCP_END_ADDR=`sysevent get ${SERVICE_NAME}_${i}_endaddr`
        
        m_LAN_SUBNET=`sysevent get ${SERVICE_NAME}_${i}_subnet`
        m_DHCP_LEASE_TIME=`sysevent get ${SERVICE_NAME}_${i}_leasetime`
        IFNAME=`sysevent get ipv4_${IPV4_INST}-ifname`
        
        echo "${PREFIX}""interface="${IFNAME} >> $LOCAL_DHCP_CONF
        echo "${PREFIX}""dhcp-range=set:$i,${m_DHCP_START_ADDR},${m_DHCP_END_ADDR},$m_LAN_SUBNET,${m_DHCP_LEASE_TIME}" >> $LOCAL_DHCP_CONF
    done
}

#-----------------------------------------------------------------
# delete the dhcp lease that matches the given IP address
#  Parameters:
#     client ip address to be deleted      eg. 192.168.1.123
#-----------------------------------------------------------------
TEMP_DHCP_LEASE_FILE=/tmp/.temp_dhcp_lease_file
delete_dhcp_lease() {
   IP_ADDR=$1
   sed "/ $IP_ADDR /d" $DHCP_LEASE_FILE > $TEMP_DHCP_LEASE_FILE
   cat $TEMP_DHCP_LEASE_FILE > $DHCP_LEASE_FILE
   rm $TEMP_DHCP_LEASE_FILE
}

#-----------------------------------------------------------------
# For dnsmasq if the leases file contains extraneous leases then
# it will count these as part of the available pool
# For example, if the number of leases allowed was dropped
# and a lease that had been allocated, but is now outside of the new
# range, and still exists in the leases file, then dnsmasq will believe
# that its pool is smaller by the number of extraneous leases
#-----------------------------------------------------------------
sanitize_leases_file() {
   if [ ! -f "$DHCP_LEASE_FILE" ] ; then
      return
   fi

   #SLF_OUTFILE_1="/tmp/sanitize_leases.${$}"
   #SLF_LAN_PREFIX_ADDR="`syscfg get lan_ipaddr | cut -f 1,2,3 -d '.'`."
   #SLF_DHCP_START=`syscfg get dhcp_start`
   #SLF_DHCP_NUM=`syscfg get dhcp_num`

   #if [ -z "$SLF_DHCP_NUM" ] || [ "0" = "$SLF_DHCP_NUM" ] ; then
   #   echo > $DHCP_LEASE_FILE
   #   return
   #fi

   #SLF_DHCP_END=`expr $SLF_DHCP_START + $SLF_DHCP_NUM`
   #if [ "255" -le "$SLF_DHCP_END" ] ; then
   #   SLF_DHCP_END=254
   #fi

   # extract all current leases that match the ip address prefix
   #cat $DHCP_LEASE_FILE | grep $SLF_LAN_PREFIX_ADDR | cut -f 3 -d ' ' > $SLF_OUTFILE_1

   #while read line ; do
   #  SLF_FOUND=`echo $line | cut -f 4 -d '.'`
   #  if [ "$SLF_FOUND" -gt "$SLF_DHCP_END" ] ; then
   #     delete_dhcp_lease $line
   #  fi
   #  if [ "$SLF_FOUND" -lt "$SLF_DHCP_START" ] ; then
   #     delete_dhcp_lease $line
   #  fi
  #done < $SLF_OUTFILE_1

  #rm -f $SLF_OUTFILE_1

}
