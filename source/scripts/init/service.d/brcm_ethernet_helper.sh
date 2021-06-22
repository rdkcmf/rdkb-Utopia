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

#------------------------------------------------------------------------
# Utilities for the broadcom ethernet switch
# These utilities are to set vlan and/or CoS priority on the switch
#------------------------------------------------------------------------

#------------------------------------------------------------------------
# Translate a port number into the address for the broadcom ethernet switch
# $1 is port
#------------------------------------------------------------------------
get_ethernet_port ()
{
   if [ "$1" = "0" ] ; then
      PORT=0x10
   elif [ "$1" = "1" ] ; then
      PORT=0x12
   elif [ "$1" = "2" ] ; then
      PORT=0x14
   elif [ "$1" = "3" ] ; then
      PORT=0x16
   elif [ "$1" = "4" ] ; then
      PORT=0x18
   elif [ "$1" = "5" ] ; then
      PORT=0x1A
   elif [ "$1" = "6" ] ; then
      PORT=0x1C
   elif [ "$1" = "7" ] ; then
      PORT=0x1E
   elif [ "$1" = "8" ] ; then
      PORT=0x20
   else
      PORT=0 
   fi
}

#------------------------------------------------------------------------
# Enable/Disable vlans on the switch
# Page 34:Address 00 is the Global IEEE 802.1Q Register
# Bit 7 Enables/Disables 802.1Q vlan
# Bit 6:5 Controls vlan learning mode (we use Individual VLAN Learning mode)
#------------------------------------------------------------------------
enable_vlan_mode_on_ethernet_switch ()
{
   CURRENT=`et robord 0x34 0x00`
   HIGHBITS=`echo "$CURRENT" | awk '{print substr($0,3,2)}'`
   LOWBITS=`echo "$CURRENT" | awk '{print substr($0,6,1)}'`
   et robowr 0x34 0x00 0x"${HIGHBITS}"e"${LOWBITS}"
}

disable_vlan_mode_on_ethernet_switch ()
{
   CURRENT=`et robord 0x34 0x00`
   HIGHBITS=`echo "$CURRENT" | awk '{print substr($0,3,2)}'`
   LOWBITS=`echo "$CURRENT" | awk '{print substr($0,6,1)}'`
   et robowr 0x34 0x00 0x"${HIGHBITS}"0"${LOWBITS}"
}

#------------------------------------------------------------------------
# Enable/Disable qos on the switch
# Page 30:Address 00 is the QoS Global Control Register
# Bit 6 Enables/Disables Port Based Qos
# Bit 4,5,7 are 0
#
# Page 30:Address 62 is TC to CoS Mapping Register
# Set Prio 7,6 => TX Q3
#     Prio 5,4 => TX Q2
#     Prio 3,2 => TX Q1
#     Prio 1,0 => TX Q0
#
# Page 30:Address 80 is the TX Queue Control Register
# Set to Weighted Round Robin
#
# Page 30:Address 81-84 are TX Queue Weight Registers
# Set the number of packets transmitted per round per queue
# Q3=>48,Q2=>24,Q1=>12,Q0=>6
#
#------------------------------------------------------------------------
enable_port_qos_on_ethernet_switch ()
{
   et robowr 0x30 0x62 0xFA50
   et robowr 0x30 0x80 0x00
   et robowr 0x30 0x81 0x06
   et robowr 0x30 0x82 0x0C
   et robowr 0x30 0x83 0x18
   et robowr 0x30 0x84 0x30


   CURRENT=`et robord 0x30 0x00`
   HIGHBITS=`echo "$CURRENT" | awk '{print substr($0,3,2)}'`
   LOWBITS=`echo "$CURRENT" | awk '{print substr($0,6,1)}'`
   et robowr 0x30 0x00 0x"${HIGHBITS}"4"${LOWBITS}"
}
disable_port_qos_on_ethernet_switch ()
{
   CURRENT=`et robord 0x30 0x00`
   HIGHBITS=`echo "$CURRENT" | awk '{print substr($0,3,2)}'`
   LOWBITS=`echo "$CURRENT" | awk '{print substr($0,6,1)}'`
   et robowr 0x30 0x00 0x"${HIGHBITS}"0"${LOWBITS}"

   et robowr 0x30 0x80 0x00
   et robowr 0x30 0x62 0x0000
   et robowr 0x30 0x81 0x01
   et robowr 0x30 0x82 0x02
   et robowr 0x30 0x83 0x04
   et robowr 0x30 0x84 0x08
}

#------------------------------------------------------------------------
# $1 is port
# $2 is vlan id 
#------------------------------------------------------------------------
set_vlan_on_ethernet_port ()
{
   get_ethernet_port "$1"
   if [ "0" = "$PORT" ] ; then
      return 0
   fi
 

   # Get the value for the port as 0xXXXX
   TAG=`et robord 0x34 $PORT`

   # The priority bits are bit 13:15 of the $TAG (bit 12 is 0)
   PRIO=`echo "$TAG" | awk '{print substr($0,3,1)}'`

   et robowr 0x34 $PORT 0x"${PRIO}""`printf "%03x" "$2"`"
}

#------------------------------------------------------------------------
# $1 is port
# $2 is priority 
#------------------------------------------------------------------------
set_prio_on_ethernet_port ()
{
   get_ethernet_port "$1"
   if [ "0" = "$PORT" ] ; then
      return 0
   fi

   # Get the value for the port as 0xXXXX
   TAG=`et robord 0x34 $PORT`

   # The vlan id is bits 0:11
   VLAN=`echo "$TAG" | awk '{print substr($0,4,4)}'`

   et robowr 0x34 $PORT 0x"`printf "%01x" "$2"`""${VLAN}"
}

#------------------------------------------------------------------------
# Enable DOS LAND Attack prevention on the switch
#------------------------------------------------------------------------

vendor_block_dos_land_attack ()
{
    # Enable drop packets when IPDA = IPSA in an IPv4/IPv6 datagram 
    et robowr 0x36 0x00 0x0003
    # Enable 'disable learning' with DOS packets
    et robowr 0x36 0x10 0x01
}

