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
    

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

source /lib/rdk/t2Shared_api.sh

DHCP_CONF=/etc/dnsmasq.conf
DHCP_STATIC_HOSTS_FILE=/etc/dhcp_static_hosts
DHCP_OPTIONS_FILE=/var/dhcp_options
if [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ]; then
LOCAL_DHCP_CONF=/tmp/dnsmasq.conf
LOCAL_DHCP_STATIC_HOSTS_FILE=/tmp/dhcp_static_hosts
LOCAL_DHCP_OPTIONS_FILE=/tmp/dhcp_options
else
LOCAL_DHCP_CONF=/tmp/dnsmasq.conf$$
LOCAL_DHCP_STATIC_HOSTS_FILE=/tmp/dhcp_static_hosts$$
LOCAL_DHCP_OPTIONS_FILE=/tmp/dhcp_options$$
fi
RESOLV_CONF=/etc/resolv.conf

# Variables needed for captive portal mode : start
DEFAULT_RESOLV_CONF="/var/default/resolv.conf"
DEFAULT_CONF_DIR="/var/default"
XCONF_FILE=/etc/Xconf
STATIC_URLS_FILE="/etc/static_urls"
STATIC_DNS_URLS_FILE="/etc/static_dns_urls"
XCONF_DOWNLOAD_URL="/tmp/xconfdownloadurl"
XCONF_DEFAULT_URL="https://xconf.xcal.tv/xconf/swu/stb/"
XFINITY_DEFAULT_URL="http://xfinity.com"
SPEEDTEST_DEFAULT_URL="http://speedtest.comcast.net"
XFINITY_RED_DEFAULT_URL="http://my.xfinity.com"
COMCAST_DEFAULT_URL="www.comcast.com"
COMCAST_ACTIVATE_URL="https://activate.comcast.com"
COMCAST_ACTIVATE_URL_2="https://caap-pdca.sys.comcast.net"
COMCAST_HTTP_URL="http://comcast.com"
DEFAULT_FILE="/etc/utopia/system_defaults"

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
#DHCP_NUM=`syscfg get dhcp_num`
#if [ "" = "$DHCP_NUM" ] ; then
#   DHCP_NUM=0
#fi

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

isValidSubnetMask () { 
   echo $1 | grep -w -E -o '^(254|252|248|240|224|192|128)\.0\.0\.0|255\.(254|252|248|240|224|192|128|0)\.0\.0|255\.255\.(254|252|248|240|224|192|128|0)\.0|255\.255\.255\.(254|252|248|240|224|192|128|0)' > /dev/null
   if [ $? -eq 0 ]; then
      echo 1
   else
      echo 0
   fi
}

isValidLANIP () {
    ip="$1"
    if expr "$ip" : '[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$' >/dev/null; then
       OIFS=$IFS
       IFS='.'
       set -- $ip
       ip1=$1
       ip2=$2
       ip3=$3
       ip4=$4
       IFS=$OIFS
       if [[ $ip1 -gt 255 || $ip2 -gt 255 || $ip3 -gt 255 || $ip4 -gt 255 ]] ||
          [[ $ip1 -ne 10 && $ip1 -ne 172 && $ip1 -ne 192 ]] ||
          [[ $ip1 -eq 172 && $ip2 -lt 16 ]] ||
          [[ $ip1 -eq 172 && $ip2 -gt 31 ]] ||
          [[ $ip1 -eq 192 && $ip2 -ne 168 ]] ||
          [[ $ip1 -eq 192 && $ip3 -eq 147 ]]; then
           echo 0
       else
           echo 1
       fi
   else
       echo 0
   fi
}

#--------------------------------------------------------------
#  figure out the dhcp range to use
#   The lan interface ip address
#   The lan interface netmask
#--------------------------------------------------------------
calculate_dhcp_range () {
	LAN_SUBNET=`subnet $1 $2` 
   	# Do a sanity check to make sure we got start address from DB
   	if [ "$DHCP_START" = "" ]
   	then
    	echo "DHCP_SERVER: Start IP is NULL"
 	   	DHCP_START=`syscfg get dhcp_start`
   	fi  
   	echo "DHCP_SERVER: Start IP is $DHCP_START LAN Subnet is $LAN_SUBNET"
	echo "dhcp_start:$DHCP_START" 

   	# Validate DHCP start IP.
   	# Valid IP should have:
   	#          - three 3 "." in it ( Eg: 192.168.100.2, 10.0.0.3)
   	#          - Last octet should not be NULL ( to avoid cases as "10.0.0.")
   	#   
   	isStartIpValid=0
   	isEndIpValid=0
   	allIpsValid=0

   	# Validate starting address
   	OCTET_NUM=`echo $DHCP_START | grep -o "\." | wc -l`

   	# If total "."s are 3, then validate last octet
   	# Last octet should not be:
   	#          - NULL
   	#          - less than 2
   	#          - greater than 254
   	#   
   	if [ "$OCTET_NUM" -eq "3" ]
   	then
    	START_ADDR_LAST_OCTET=`echo "$DHCP_START" | awk -F '\.' '{print $NF}'`
        if [ "$START_ADDR_LAST_OCTET" = "" ]
        then
            echo "DHCP_SERVER: Last octet of start IP is NULL"
            isStartIpValid=0
        else
        	cnt=`echo "$START_ADDR_LAST_OCTET" | sed -e /\[0-9\]/!d -e /\[\.\,\:\]/d -e /\[a-zA-Z\]/d`
           	START_SUBNET=`subnet $DHCP_START $2`
           	echo "START_SUBNET is: $START_SUBNET, LAN_SUBNET=$LAN_SUBNET"
           	if [ -z $cnt ];
           	then
            	echo "DHCP_SERVER: Last octet of start IP is character"
             	isStartIpValid=0
           	else
            	if [ "$START_ADDR_LAST_OCTET" -gt "254" ]
             	then
                	echo "DHCP_SERVER: Last octet of start ip is greater than 254"
                 	isStartIpValid=0
             	elif [ "$START_ADDR_LAST_OCTET" -lt "2" ]
             	then
			 	 	echo "DHCP_SERVER: Last octet of start ip is less than 2"
                 	isStartIpValid=0
             	elif [ "$START_SUBNET" != "$LAN_SUBNET" ]
             	then
                	echo "DHCP_SERVER: DHCP Start Address:$DHCP_START is not in the LAN SUBNET:$LAN_SUBNET"
                 	isStartIpValid=0
             	else
                	echo "DHCP_SERVER: DHCP START Address:$DHCP_START is valid"
                 	isStartIpValid=1
                    DHCP_START_ADDR=$DHCP_START
             	fi
          	fi
       	fi
   	fi

   	if [ "$isStartIpValid" -eq "0" ]
   	then
    	echo "DHCP_SERVER: DHCP START Address:$DHCP_START is not valid re-calculating it"
      	# extract 1st 3 octets of the lan subnet and set the last octet to 2 for the start address
      	DHCP_START_ADDR=`echo $LAN_SUBNET | cut -d"." -f1-3`

      	DHCP_START=2
      	DHCP_START_ADDR="$DHCP_START_ADDR"".""$DHCP_START"
	  	echo "DHCP_SERVER: Start address to syscfg_db $DHCP_START_ADDR"
      	# update syscfg dhcp_start
      	syscfg set dhcp_start $DHCP_START_ADDR
      	syscfg commit
   	fi


   	# Get the ending address from syscfg DB
   	ENDING_ADDRESS=`syscfg get dhcp_end`
   	OCTET_NUM=`echo $ENDING_ADDRESS | grep -o "\." | wc -l`
        echo "dhcp_end:$ENDING_ADDRESS"

   	#
   	# If total "."s are 3, then validate last octet
   	# Last octet should not be:
   	#          - NULL
   	#          - less than 2
   	#          - greater than 254
   	#
	if [ "$OCTET_NUM" -eq "3" ]
   	then
    	END_ADDR_LAST_OCTET=`echo $ENDING_ADDRESS | awk -F '\\.' '{print $NF}'`
        if [ "$END_ADDR_LAST_OCTET" = "" ]
        then
            echo "DHCP_SERVER: Last octet of end IP is NULL"
            isEndIpValid=0
        else
        	cnt=`echo "$END_ADDR_LAST_OCTET" | sed -e /\[0-9\]/!d -e /\[\.\,\:\]/d -e /\[a-zA-Z\]/d`
   			END_SUBNET=`subnet $ENDING_ADDRESS $2`	   	        
   	       	if [ -z $cnt ];
           	then
            	echo "DHCP_SERVER: Last octet of end IP is character"
             	isEndIpValid=0
           	else
            	if [ "$END_ADDR_LAST_OCTET" -gt "254" ]
             	then
                	echo "DHCP_SERVER: Last octet of end ip is greater than 254"
                 	isEndIpValid=0
             	elif [ "$END_ADDR_LAST_OCTET" -lt "2" ]
             	then
                	echo "DHCP_SERVER: Last octet of end ip is less than 2"
                 	isEndIpValid=0
				elif [ "$END_SUBNET" != "$LAN_SUBNET" ]
            	then
            		echo "DHCP_SERVER: DHCP END Address:$ENDING_ADDRESS is not in the LAN SUBNET:$LAN_SUBNET"
                	isEndIpValid=0
	            else
    	            echo "DHCP_SERVER: DHCP END Address:$ENDING_ADDRESS is valid"
        	        isEndIpValid=1
                    DHCP_END_ADDR=$ENDING_ADDRESS
            	fi
          	fi
       	fi            
   	fi

    if [ "$isEndIpValid" -eq "0" ]
    then
    	echo "DHCP_SERVER: DHCP END Address:$ENDING_ADDRESS is not valid re-calculating it"
      	MASKBITS=`mask2cidr $2`
      	echo "MASKBITS is: $MASKBITS"

      	# extract 1st 3 octets of the lan subnet and set the last octet to 253 for the start address  
      	if [ "$MASKBITS" -eq "24" ]
      	then
          	DHCP_END_ADDR=`echo $LAN_SUBNET | cut -d"." -f1-3`
          	DHCP_END=253
          	DHCP_END_ADDR="$DHCP_END_ADDR"".""$DHCP_END"

          	echo "DHCP_SERVER: End address to syscfg_db $DHCP_END_ADDR"
      	# extract 1st 2 octets of the lan subnet and set the last remaining to 255.253 for the start address  
      	elif [ "$MASKBITS" -eq "16" ]
      	then
        	DHCP_END_ADDR=`echo $LAN_SUBNET | cut -d"." -f1-2`
          	DHCP_END="255.253"
          	DHCP_END_ADDR="$DHCP_END_ADDR"".""$DHCP_END"

          	echo "DHCP_SERVER: End address to syscfg_db $DHCP_END_ADDR"
      	# extract 1st octet of the lan subnet and set the last remaining octets to 255.255.253 for the start address  
      	elif [ "$MASKBITS" -eq "8" ]
      	then
          	DHCP_END_ADDR=`echo $LAN_SUBNET | cut -d"." -f1`
          	DHCP_END="255.255.253"
          	DHCP_END_ADDR="$DHCP_END_ADDR"".""$DHCP_END"

          	echo "DHCP_SERVER: End address to syscfg_db $DHCP_END_ADDR"
      	else
        	echo "DHCP_SERVER: Invalid subnet mask $2"   
      	fi
      	syscfg set dhcp_end $DHCP_END_ADDR
      	syscfg commit
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

get_dhcp_option_for_brlan0() {

   DHCP_OPTION_STR=

    # add Lan static DNS
    NAMESERVER1=`syscfg get dhcp_nameserver_1`
    NAMESERVER2=`syscfg get dhcp_nameserver_2`
    NAMESERVER3=`syscfg get dhcp_nameserver_3`
    CURR_LAN_IP=`sysevent get current_lan_ipaddr`
    SECUREWEBUI_ENABLED=`syscfg get SecureWebUI_Enable`
   
	  if [ "0.0.0.0" != "$NAMESERVER1" ] && [ "" != "$NAMESERVER1" ] ; then
	     if [ "" = "$DHCP_OPTION_STR" ] ; then
	        DHCP_OPTION_STR="dhcp-option=brlan0,6,"$NAMESERVER1
	     else
	        DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER1
	     fi
	  fi

	  if [ "0.0.0.0" != "$NAMESERVER2" ]  && [ "" != "$NAMESERVER2" ]; then
	     if [ "" = "$DHCP_OPTION_STR" ] ; then
	        DHCP_OPTION_STR="dhcp-option=brlan0,6,"$NAMESERVER2
	     else
	        DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER2
	     fi
	  fi

	  if [ "0.0.0.0" != "$NAMESERVER3" ]  && [ "" != "$NAMESERVER3" ]; then
	     if [ "" = "$DHCP_OPTION_STR" ] ; then
	        DHCP_OPTION_STR="dhcp-option=brlan0,6,"$NAMESERVER3
	     else
	        DHCP_OPTION_STR=$DHCP_OPTION_STR","$NAMESERVER3
	     fi
	  fi
          
          if [ "$SECUREWEBUI_ENABLED" = "true" ]; then
              NS=`sysevent get wan_dhcp_dns`
              if [ "" != "$NS" ] ; then
                  NS=`echo "$NS" | sed "s/ /,/g"`
              fi    
              if [ "" = "$DHCP_OPTION_STR" ] ; then
                 if [ "" != "$NS" ] ; then
                     DHCP_OPTION_STR="dhcp-option=brlan0,6,"$CURR_LAN_IP","$NS
                 else
                     DHCP_OPTION_STR="dhcp-option=brlan0,6,"$CURR_LAN_IP
                 fi
              else
                 if [ "" != "$NS" ] ; then
                     DHCP_OPTION_STR=$DHCP_OPTION_STR","$CURR_LAN_IP","$NS
                 else
                     DHCP_OPTION_STR=$DHCP_OPTION_STR","$CURR_LAN_IP
                 fi    
              fi
          fi

	  echo $DHCP_OPTION_STR
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

prepare_static_dns_urls()
{
  if [ -f $STATIC_DNS_URLS_FILE ]; then
     STATIC_DNS_URL_LIST=`cat $STATIC_DNS_URLS_FILE`
     for static_dns_url in $STATIC_DNS_URL_LIST
     do
        echo "server=/$static_dns_url/" >> $1
     done
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
   RF_CAPTIVE_PORTAL="false"
   SECWEBUI_ENABLED=`syscfg get SecureWebUI_Enable`
   if [ "$SECWEBUI_ENABLED" = "true" ]; then
       syscfg set dhcp_nameserver_enabled 1
       syscfg commit
   else
       NAMESERVERENABLED=`syscfg get dhcp_nameserver_enabled`
       NAMESERVER1=`syscfg get dhcp_nameserver_1`
       NAMESERVER2=`syscfg get dhcp_nameserver_2`
       if [ "1" = "$NAMESERVERENABLED" ] ; then
           if [ "0.0.0.0" == "$NAMESERVER1" ] || [ "" == "$NAMESERVER1" ] ; then
               if [ "0.0.0.0" == "$NAMESERVER2" ]  || [ "" == "$NAMESERVER2" ]; then
                   syscfg set dhcp_nameserver_enabled 0
                   syscfg commit
               fi
           fi
       fi
   fi
   LAN_IFNAME=`syscfg get lan_ifname`
   NAMESERVERENABLED=`syscfg get dhcp_nameserver_enabled`
   WAN_DHCP_NS=`sysevent get wan_dhcp_dns`
   if [ "" != "$WAN_DHCP_NS" ] ; then
		WAN_DHCP_NS=`echo "$WAN_DHCP_NS" | sed "s/ /,/g"`
   fi	 

  echo_t "DHCP_SERVER : NAMESERVERENABLED = $NAMESERVERENABLED"
  echo_t "DHCP_SERVER : WAN_DHCP_NS = $WAN_DHCP_NS"

  echo -n > $DHCP_STATIC_HOSTS_FILE

   if [ "$3" = "dns_only" ] ; then
      PREFIX=#
   else 
      PREFIX=
   fi

   LAN_IPADDR=$1
   LAN_NETMASK=$2
   if [[ `isValidLANIP $LAN_IPADDR` -eq 0 || `isValidSubnetMask $LAN_NETMASK` -eq 0 ]]; then
       echo "LAN IP Address OR LAN net mask is not in valid format, setting to default. lan_ipaddr:$LAN_IPADDR lan_netmask:$LAN_NETMASK"
       result=$(grep '$lan_ipaddr\|$lan_netmask\|$dhcp_start\|$dhcp_end' $DEFAULT_FILE | awk '/\$lan_ipaddr/ {split($1,ip, "=");} /\$lan_netmask/ {split($1,mask, "=");} /\$dhcp_start/ {split($1,start, "=");} /\$dhcp_end/ {split($1,end, "=");} END {print ip[2], mask[2], start[2], end[2]}')
       OIFS=$IFS
       IFS=' '
       set -- $result
       lanIP="$1"
       lanNetMask="$2"
       dhcpStart="$3"
       dhcpEnd="$4"
       IFS=$OIFS

       syscfg set lan_ipaddr $lanIP
       syscfg set lan_netmask $lanNetMask
       syscfg set dhcp_start $dhcpStart
       syscfg set dhcp_end  $dhcpEnd
       syscfg commit

       LAN_IPADDR=$lanIP
       LAN_NETMASK=$lanNetMask

       echo "lanIP:  $lanIP lanNetMask: $lanNetMask dhcpStart: $dhcpStart dhcpEnd: $dhcpEnd"

   fi

   #calculate_dhcp_range $1 $2

   echo -n > $LOCAL_DHCP_CONF


   CAPTIVE_PORTAL_MODE="false"

   #Read the http response value
   NETWORKRESPONSESTATUS=`cat /var/tmp/networkresponse.txt`

    
   # If redirection flag is "true" that means we are in factory default condition
   CAPTIVEPORTAL_ENABLED=`syscfg get CaptivePortal_Enable`
   REDIRECTION_ON=`syscfg get redirection_flag`
   RF_CP_FEATURE_EN=`syscfg get enableRFCaptivePortal`
   RF_CP_MODE=`syscfg get rf_captive_portal`
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
echo "DHCP SERVER : RF CP is $RF_CP_MODE, RF CP feature state is $RF_CP_FEATURE_EN"

if [ "$CAPTIVEPORTAL_ENABLED" == "true" ]
then
    noRf=0
    if [ "$BOX_TYPE" = "XB6" ]
    then
        if [ "$RF_CP_FEATURE_EN" = "true" ] && [ "$RF_CP_MODE" = "true" ]
        then 
            RF_SIGNAL_STATUS=`dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_CableRfSignalStatus | grep value | cut -f3 -d : | cut -f2 -d" "`
            if [ "$RF_SIGNAL_STATUS" = "false" ]
            then
               noRf=1
               RF_CAPTIVE_PORTAL="true"
               CAPTIVE_PORTAL_MODE="true"
            else
               echo "DHCP SERVER : RF Signal status $RF_SIGNAL_STATUS, resetting RF CP"
               syscfg set rf_captive_portal false
               syscfg commit
               noRf=0
            fi
        fi
    fi

    if [ "$noRf" = "0" ]
    then
        if [ "$NETWORKRESPONSESTATUS" = "204" ] && [ "$REDIRECTION_ON" = "true" ] && [ "$WIFI_NOT_CONFIGURED" = "true" ]
        then
            CAPTIVE_PORTAL_MODE="true"
            echo "DHCP SERVER : WiFi SSID and Passphrase are not modified,set CAPTIVE_PORTAL_MODE"
	    t2CountNotify "SYS_INFO_CaptivePortal"
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
fi

   localServerCnt=0
   isLocalDNSOnly=0
   localServerCnt=`wc -l < /etc/resolv.conf`
   if [ "$localServerCnt" = "" ]
   then
       localServerCnt=0
   fi
   isLocalHostPresent=`grep "127.0.0.1" /etc/resolv.conf`
   if [ $localServerCnt -lt 2 ] && [ "$isLocalHostPresent" != "" ]
   then
       isLocalDNSOnly=1
   fi

   if [ "$RF_CAPTIVE_PORTAL" != "true" ]
   then
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

        if [ "0" = "$NAMESERVERENABLED" ] && [ $isLocalDNSOnly -eq 0 ] ; then
            echo "resolv-file=$RESOLV_CONF" >> $LOCAL_DHCP_CONF
        fi

       fi

      # if we are provisioned to use the wan domain name, the we do so
      # otherwise we use the lan domain name
      if grep "domain=" $LOCAL_DHCP_CONF >/dev/null
      then
         echo "domain name is set "
      else
         if [ "1" = "$PROPAGATE_DOM" ] ; then
            LAN_DOMAIN=`sysevent get dhcp_domain`
            if [ "" = "$LAN_DOMAIN" ] ; then
               LAN_DOMAIN=`grep 'domain' /etc/resolv.conf | grep -v '#' | awk '{print $2}'`
            fi
         fi
         if [ "" = "$LAN_DOMAIN" ] ; then
            LAN_DOMAIN=`syscfg get lan_domain`
         fi
         if [ "" != "$LAN_DOMAIN" ] ; then
            echo "domain=$LAN_DOMAIN" >> $LOCAL_DHCP_CONF
         fi
       fi  
   else
       echo "no-resolv" >> $LOCAL_DHCP_CONF
   fi

   #echo "interface=$LAN_IFNAME" >> $LOCAL_DHCP_CONF
   echo "expand-hosts" >> $LOCAL_DHCP_CONF

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
  # echo "$PREFIX""dhcp-lease-max=$DHCP_NUM" >> $LOCAL_DHCP_CONF
   echo "$PREFIX""dhcp-hostsfile=$DHCP_STATIC_HOSTS_FILE" >> $LOCAL_DHCP_CONF

   if [ "$RF_CAPTIVE_PORTAL" != "true" ]
   then
       if [ "$CAPTIVE_PORTAL_MODE" = "false" ] && [ "0" = "$NAMESERVERENABLED" ]
       then
            if [ $isLocalDNSOnly -eq 0 ]
            then
               echo "$PREFIX""dhcp-optsfile=$DHCP_OPTIONS_FILE" >> $LOCAL_DHCP_CONF
            fi
       fi
   fi

   #if [ "$LOG_LEVEL" -gt 1 ] ; then
    #  echo "$PREFIX""log-dhcp" >> $LOCAL_DHCP_CONF
   #fi

   #Option for parsing plume vendor code
   if [ "$BOX_TYPE" = "XB6" ] || [ "$BOX_TYPE" = "HUB4" ]; then
     echo "dhcp-option=vendor:Plume,43,tag=123" >> $LOCAL_DHCP_CONF 
     echo "dhcp-option=vendor:PP203X,43,tag=123" >> $LOCAL_DHCP_CONF
     echo "dhcp-option=vendor:SE401,43,tag=123" >> $LOCAL_DHCP_CONF
     echo "dhcp-option=vendor:HIXE12AWR,43,tag=123" >> $LOCAL_DHCP_CONF
     echo "dhcp-option=vendor:WNXE12AWR,43,tag=123" >> $LOCAL_DHCP_CONF
   fi

   if [ "dns_only" != "$3" ] ; then
      prepare_dhcp_conf_static_hosts
      #prepare_dhcp_options
	  prepare_dhcp_options_wan_dns	
   fi
   
   if [ "x$BOX_TYPE" != "xHUB4" ] && [ "x$BOX_TYPE" != "xSR300" ]; then
      nameserver=`grep "nameserver" $RESOLV_CONF | awk '{print $2}'|grep -v ":"|tr '\n' ','| sed -e 's/,$//'`
      if [ "" != "$nameserver" ]; then
         echo "option:dns-server,$nameserver" >> $DHCP_OPTIONS_FILE
      fi
   fi
  
   if [ "$BOX_TYPE" = "rpi" ]; then                                       
	   LAN_STATUS=`sysevent get lan-status`
	   if [ "$LAN_STATUS" = "stopped" ]; then                
		   echo_t "DHCP_SERVER : Starting lan-status"
		   sysevent set lan-status started
	   fi                                                  
   fi   
   if [ "started" = $CURRENT_LAN_STATE ]; then
      calculate_dhcp_range $LAN_IPADDR $LAN_NETMASK
      echo "interface=$LAN_IFNAME" >> $LOCAL_DHCP_CONF
	  if [ $DHCP_LEASE_TIME == -1 ]; then
	      echo "$PREFIX""dhcp-range=$DHCP_START_ADDR,$DHCP_END_ADDR,$LAN_NETMASK,infinite" >> $LOCAL_DHCP_CONF
	  else
  	      echo "$PREFIX""dhcp-range=$DHCP_START_ADDR,$DHCP_END_ADDR,$LAN_NETMASK,$DHCP_LEASE_TIME" >> $LOCAL_DHCP_CONF
	  fi
	  if [ "1" = "$NAMESERVERENABLED" ]; then
		  DHCP_OPTION_FOR_LAN=`get_dhcp_option_for_brlan0`
		  echo "$PREFIX""$DHCP_OPTION_FOR_LAN" >> $LOCAL_DHCP_CONF
		  echo_t "DHCP_SERVER : $PREFIX$DHCP_OPTION_FOR_LAN"
                  SECUREWEBUI_ENABLED=`syscfg get SecureWebUI_Enable`
                  if [ "$SECUREWEBUI_ENABLED" = "true" ]; then
                      locaddr=`syscfg get lan_ipaddr`
                      LOCDOMAIN_NAME=`syscfg get SecureWebUI_LocalFqdn`
                      echo "address=/$LOCDOMAIN_NAME/$locaddr" >> $LOCAL_DHCP_CONF
                      echo "server=/$LOCDOMAIN_NAME/$locaddr" >> $LOCAL_DHCP_CONF
                      
                  fi 
          fi
               
   fi
   
   # For boot itme optimization, run do_extra_pool only when brlan1 interface is available
   isBrlan1=`ifconfig | grep brlan1`
   if [ "$isBrlan1" != "" ]
   then
      echo_t "DHCP_SERVER : brlan1 availble, creating dnsmasq entry "
      do_extra_pools $NAMESERVERENABLED $WAN_DHCP_NS
   else
       echo_t "DHCP_SERVER : brlan1 not available, cannot enter details in dnsmasq.conf"
   fi

   iotEnabled=`syscfg get lost_and_found_enable`
   if [ "$iotEnabled" = "true" ]
   then
        echo "IOT_LOG : DHCP server configuring for IOT"
        IOT_IFNAME=`syscfg get iot_ifname`
        if [ $IOT_IFNAME == "l2sd0.106" ]; then
         IOT_IFNAME=`syscfg get iot_brname`
        fi
	IOT_START_ADDR=`syscfg get iot_dhcp_start`
	IOT_END_ADDR=`syscfg get iot_dhcp_end`
	IOT_NETMASK=`syscfg get iot_netmask`
	echo "interface=$IOT_IFNAME" >> $LOCAL_DHCP_CONF
	  if [ $DHCP_LEASE_TIME == -1 ]; then
		echo "$PREFIX""dhcp-range=$IOT_START_ADDR,$IOT_END_ADDR,$IOT_NETMASK,infinite" >> $LOCAL_DHCP_CONF
	  else
		echo "$PREFIX""dhcp-range=$IOT_START_ADDR,$IOT_END_ADDR,$IOT_NETMASK,86400" >> $LOCAL_DHCP_CONF
	  fi
	  
	   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
		   echo "${PREFIX}""dhcp-option="${IOT_IFNAME}",6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
	   fi
   fi

   #zqiu:mesh >>
   #meshEnabled=`syscfg get mesh_enable`
   #if [ "$meshEnabled" = "true" ]
   #then
   echo "IOT_LOG : DHCP server configuring for Mesh"
   if [ -z ${BOX_TYPE+x} ]; then
       echo "BOX_TYPE not set in device.properties"
   else
       if [ "$BOX_TYPE" = "XB3" ]; then
           #for xb3/puma6
           echo "interface=l2sd0.112" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=l2sd0.112,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

           echo "interface=l2sd0.113" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=l2sd0.113,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

           #RDKB-15951 - Mesh Bhaul vlan address pool
           echo "interface=l2sd0.1060" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=192.168.245.2,192.168.245.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=l2sd0.1060,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

           echo "interface=l2sd0.4090" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=192.168.251.2,192.168.251.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=l2sd0.4090,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

        elif [ "$MODEL_NUM" = "CGM4331COM" ] || [ "$MODEL_NUM" = "CGM4981COM" ] || [ "$MODEL_NUM" = "TG4482A" ]; then
            echo "interface=brlan112" >> $LOCAL_DHCP_CONF
            echo "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

            if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                echo "${PREFIX}""dhcp-option=brlan112,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
            fi

            echo "interface=brlan113" >> $LOCAL_DHCP_CONF
            echo "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

            if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                echo "${PREFIX}""dhcp-option=brlan113,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
            fi
            
            if [ "$MODEL_NUM" != "TG4482A" ]; then
             echo "interface=brebhaul" >> $LOCAL_DHCP_CONF
             echo "dhcp-range=169.254.85.5,169.254.85.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF
            fi

            echo "interface=br403" >> $LOCAL_DHCP_CONF
            echo "dhcp-range=192.168.245.2,192.168.245.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

            if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                echo "${PREFIX}""dhcp-option=br403,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
            fi

       elif [ "$BOX_TYPE" = "XB6" ]; then
           echo "interface=ath12" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=ath12,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

           echo "interface=ath13" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=ath13,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi
          
           echo "interface=brebhaul" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.85.5,169.254.85.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF
 
	   echo "interface=br403" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=192.168.245.2,192.168.245.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

		   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			   echo "${PREFIX}""dhcp-option=br403,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		   fi

       elif [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ]; then
           echo "interface=brlan6" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.0.5,169.254.0.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

                   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                           echo "${PREFIX}""dhcp-option=brlan6,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
                   fi

           echo "interface=brlan7" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.1.5,169.254.1.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

                   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                           echo "${PREFIX}""dhcp-option=brlan7,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
                   fi

           echo "interface=brebhaul" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=169.254.85.5,169.254.85.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF
           
           echo "interface=br403" >> $LOCAL_DHCP_CONF
           echo "dhcp-range=192.168.245.2,192.168.245.253,255.255.255.0,infinite" >> $LOCAL_DHCP_CONF

                   if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
                           echo "${PREFIX}""dhcp-option=br403,6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
                   fi

           if [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ]; then

               #SKYH4-952: Sky selfheal support.
               #For Sky selfheal mode, prepare redirection IP for DSN redirection.
               #Here given a static IP returned in selfheal mode in the absense of WAN.
               #So instead of DNS refused, all the DNS queires resolved and returned this static IP.
               #Define static IPv4 and IPv6 address to resolve IPv4 and IPv6 hosts.
               #Also check /etc/resolv.conf contains either 127.0.0.1 or empty, then we add static ip configuration.
               resolv_conf_entry_cnt=`cat /etc/resolv.conf  | wc -l`
               isItLocalHost=`cat /etc/resolv.conf | grep "127.0.0.1" | cut -d " " -f2`
               if [ "$resolv_conf_entry_cnt" == "1" ] && [ "$isItLocalHost" == "127.0.0.1" ]
               then
                   echo "Adding static entries for selfheal"
                   echo "address=/#/10.10.10.10" >> $LOCAL_DHCP_CONF
                   echo "address=/#/a000::1" >> $LOCAL_DHCP_CONF
                   echo "dhcp-option=252,\"\n\"" >> $LOCAL_DHCP_CONF
               fi
           fi

       fi
   fi
   #fi
   #<<
   addr=`syscfg get lan_ipaddr`
   if [ "$CAPTIVE_PORTAL_MODE" = "true" ]
   then
        # In factory default condition, prepare whitelisting and redirection IP
        echo "address=/#/$addr" >> $LOCAL_DHCP_CONF

        if [ "$RF_CAPTIVE_PORTAL" != "true" ]
        then
            echo "dhcp-option=252,\"\n\"" >> $LOCAL_DHCP_CONF
            prepare_whitelist_urls $LOCAL_DHCP_CONF
        fi
        sysevent set captiveportaldhcp completed
   fi

	if [ "1" == "$NAMESERVERENABLED" ]; then
		prepare_static_dns_urls $LOCAL_DHCP_CONF
	fi

   cat $LOCAL_DHCP_CONF > $DHCP_CONF
   rm -f $LOCAL_DHCP_CONF

   echo "DHCP SERVER : Completed preparing DHCP configuration"
}

do_extra_pools () {
    POOLS="`sysevent get ${SERVICE_NAME}_current_pools`"
    if [ x"$POOLS" = x ]; then
        echo_t "DHCP_SERVER : dhcp_server pools not availble"
    fi

	NAMESERVERENABLED=$1
	WAN_DHCP_NS=$2

    #DEBUG
    # echo "Extra pools: $POOLS"
    
    for i in $POOLS; do 
        #DNS_S1 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S1_DM} DNS_S2 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S2_DM} DNS_S3 ${DHCPS_POOL_NVPREFIX}.${i}.${DNS_S3_DM}
        ENABLED=`sysevent get ${SERVICE_NAME}_${i}_enabled`
        if [ x"TRUE" != x$ENABLED ]; then
            echo_t "DHCP_SERVER : ${SERVICE_NAME}_${i} is not enabled"
            continue;
        fi
        
        IPV4_INST=`sysevent get ${SERVICE_NAME}_${i}_ipv4inst`
        if [ x$L3_UP_STATUS != x`sysevent get ipv4_${IPV4_INST}-status` ]; then
            echo_t "DHCP_SERVER : L3 is not up"
            continue
        fi
        
        m_DHCP_START_ADDR=`sysevent get ${SERVICE_NAME}_${i}_startaddr`
        if [ x$m_DHCP_START_ADDR = x ]; then
            echo_t "DHCP_SERVER : Start address for pool $i not availble"
        fi

        m_DHCP_END_ADDR=`sysevent get ${SERVICE_NAME}_${i}_endaddr`
        if [ x$m_DHCP_END_ADDR = x ]; then
            echo_t "DHCP_SERVER : End address for pool $i not availble"
        fi

        m_LAN_SUBNET=`sysevent get ${SERVICE_NAME}_${i}_subnet`
        if [ x$m_LAN_SUBNET = x ]; then
            echo_t "DHCP_SERVER : Subnet not available for pool $i"
        fi

        m_DHCP_LEASE_TIME=`sysevent get ${SERVICE_NAME}_${i}_leasetime`
        if [ x$m_DHCP_LEASE_TIME = x ]; then
            echo_t "DHCP_SERVER : Leasetime not available for pool $i"
        fi

        IFNAME=`sysevent get ipv4_${IPV4_INST}-ifname`
        
       if [ x"$m_DHCP_START_ADDR" != "x" ] && [ x"$m_DHCP_END_ADDR" != "x" ]
	then
		echo "${PREFIX}""interface="${IFNAME} >> $LOCAL_DHCP_CONF
		echo "${PREFIX}""dhcp-range=set:$i,${m_DHCP_START_ADDR},${m_DHCP_END_ADDR},$m_LAN_SUBNET,${m_DHCP_LEASE_TIME}" >> $LOCAL_DHCP_CONF
		echo_t "DHCP_SERVER : [BRLAN1] ${PREFIX}""dhcp-range=set:$i,${m_DHCP_START_ADDR},${m_DHCP_END_ADDR},$m_LAN_SUBNET,${m_DHCP_LEASE_TIME}"
	fi

	   	if [ "1" == "$NAMESERVERENABLED" ] && [ "$WAN_DHCP_NS" != "" ]; then
			echo "${PREFIX}""dhcp-option="${IFNAME}",6,$WAN_DHCP_NS" >> $LOCAL_DHCP_CONF
		fi
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
