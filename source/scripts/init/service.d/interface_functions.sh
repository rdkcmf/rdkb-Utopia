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
#                     interface_functions.sh 
#
# Helper routines for configuring/unconfiguring network interfaces
#------------------------------------------------------------------

#--------------------------------------------------------------
# get_network takes an IP address and assumes that the netmask is 255.255.255.0
# it returns the network portion of the ip address
#    parameter:
#       ip address from which to extract network portion
#--------------------------------------------------------------
get_network () {
   TEMP=""
   LAST=""
   SAVEIFS=$IFS
   IFS=.
   for p in $1
   do
       if [ -z "$LAST" ] ; then
          LAST=$TEMP
       else
          LAST=$LAST"."$TEMP
       fi
       TEMP=$p
   done
   IFS=$SAVEIFS
   echo $LAST
}

#--------------------------------------------------------------
# utility to get the mac address of an interface
# the parameter: 
#    the name of the interface to get
#--------------------------------------------------------------
get_mac () {
   OUR_MAC=`ip link show "$1" | grep link | awk '{print $2}'`
   echo "$OUR_MAC"
}

#--------------------------------------------------------------
# utility to increment a mac address by a given amount
# the parameters:
#     the mac address
#     the amount to increment by
#--------------------------------------------------------------
incr_mac () {                                                    
   COUNTER=$2                                                                
   LAST_BYTE=`echo "$1" | awk 'BEGIN { FS = ":" } ; { printf ("%d", "0x"$6) }'`
   while [ "$COUNTER" -gt 0 ]                                                  
   do                                                                        
      T_BYTE=`expr "$LAST_BYTE" + 1`                                           
      if [ "255" = "$T_BYTE" ] ; then                                        
         T_BYTE=00                                                           
      fi                                                                     
      LAST_BYTE=$T_BYTE              
      COUNTER=`expr "$COUNTER" - 1`    
   done                                                             
                                                                    
   INCREMENTED_BYTE=`echo $LAST_BYTE | awk '{printf ("%02X", $1) }'`
                                                                                
   TRUNCATED_MAC=`echo "$1" | \
       awk 'BEGIN { FS = ":" } ; { printf ("%02X:%02X:%02X:%02X:%02X:", "0x"$1, "0x"$2, "0x"$3, "0x"$4, "0x"$5) }'`
   REPLACED_MAC="$TRUNCATED_MAC""$INCREMENTED_BYTE"                             
   echo "$REPLACED_MAC"                                                         
}

#--------------------------------------------------------------
# utility to convert an ipv4 address into its ipv6 4octet equivalent
# the parameters:
#     the ipv4 address
#--------------------------------------------------------------
convert_v4_2_v6 () {                                                    
   V6=`echo "$1" | awk 'BEGIN { FS = "." } ; { printf ("%02x%02x:%02x%02x", $1, $2, $3, $4) }'`
   echo "$V6"                                                         
}


#--------------------------------------------------------------
# configure the ethernet interface into vlans
#
# config_vlan takes a physical interface name and a vlan number as parameters
# eg. config_vlan eth0 2  will add interface eth0 to vlan2
#--------------------------------------------------------------
config_vlan () {
#   echo "[utopia][interface] configuring vlan $2 on $1" > /dev/console
   ip link set "$1" up
   vconfig set_name_type VLAN_PLUS_VID_NO_PAD
   vconfig add "$1" "$2"
   vconfig set_ingress_map vlan"$2" 0 0
   vconfig set_ingress_map vlan"$2" 1 1
   vconfig set_ingress_map vlan"$2" 2 2
   vconfig set_ingress_map vlan"$2" 3 3
   vconfig set_ingress_map vlan"$2" 4 4
   vconfig set_ingress_map vlan"$2" 5 5
   vconfig set_ingress_map vlan"$2" 6 6
   vconfig set_ingress_map vlan"$2" 7 7
   ip link set vlan"$2" mtu 1500

   # vconfig is not ensuring that a vlan's mac address is unique
   # so we need to do it
   # if the vlan mac has already been specified
   # then we will use that value, otherwise
   # our approach is to consider vlan1 the normal mac
   # and to increment each subsequent vlan mac by 1
   REPLACEMENT=`sysevent get vlan"$2"_mac`
   if [ -n "$REPLACEMENT" ] ; then
       ip link set vlan"$2" addr "$REPLACEMENT"
#      echo "[utopia] [interface] Setting vlan$2 hw address to $REPLACEMENT" > /dev/console
   else
      if [ "1" != "$2" ] ; then
         ip link set vlan"$2" up
         INCR_AMOUNT=`expr "$2" - 1`
         OUR_MAC=`get_mac "vlan""$2"`
         REPLACEMENT=`incr_mac "$OUR_MAC" "$INCR_AMOUNT"`
         ip link set vlan"$2" down
         ip link set vlan"$2" addr "$REPLACEMENT"
#         echo "[utopia] [interface] Changing vlan$2 hw address to $REPLACEMENT" > /dev/console
         `sysevent set vlan"$2"_mac "$REPLACEMENT"`
      fi
   fi
}

#--------------------------------------------------------------
# unconfigure a vlan
#
# unconfig_vlan takes a vlan number as parameter
#--------------------------------------------------------------
unconfig_vlan () {
#   echo "[utopia][interface] unconfiguring vlan$1" > /dev/console
   ip link set vlan"$1" down
   vconfig rem vlan"$1"
}
