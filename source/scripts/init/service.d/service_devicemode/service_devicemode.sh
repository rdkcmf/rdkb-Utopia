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
# This script sets up the device mode
#--------------------------------------------------------------

source /etc/utopia/service.d/log_capture_path.sh

echo_t "---------------------------------------------------------"
echo_t "-------------------- service_devicemode.sh ---------------------"
echo_t "---------------------------------------------------------"
set -x

SERVICE_NAME=devicemode

Ipv4_Gateway_Addr=""
Ipv6_Gateway_Addr=""
DEVICE_MODE=`syscfg get Device_Mode`
mesh_wan_ifname=$(psmcli get dmsb.Mesh.WAN.Interface.Name)

case "$1" in
    "${SERVICE_NAME}-start")
        service_devicemode start $2
        ;;
    "${SERVICE_NAME}-stop")
        service_devicemode stop $2
        ;;
    "${SERVICE_NAME}-restart")
        service_devicemode stop $2
        service_devicemode start $2
        ;;
    DeviceMode)
        service_devicemode DeviceMode $2
        cellular_ifname=`sysevent get cellular_ifname`

        if [ "$2" = "1" ];then
               	echo "Adding rule to resolve dns packets"
        	if_status=`ifconfig "$cellular_ifname" | grep UP`
                if [ "x$if_status" != "x" ];then
               	    ip rule add from all dport 53 lookup 12
                    ip rule add iif "$cellular_ifname" lookup 11

                   	Ipv4_Gateway_Addr=$(sysevent get cellular_wan_v4_gw)
                   	if [ "x$Ipv4_Gateway_Addr" != "x" ];then
                            ip route del default dev "$cellular_ifname" table 12 > /dev/null
                        	ip route add default via $Ipv4_Gateway_Addr dev "$cellular_ifname" table 12
                   	else
                        	ip route add default dev "$cellular_ifname" table 12
                   	fi

                   	ip -6 rule add from all dport 53 lookup 12
                        ip -6 rule add iif "$cellular_ifname" lookup 11

                   	Ipv6_Gateway_Addr=$(sysevent get cellular_wan_v6_gw)
                   	if [ "x$Ipv6_Gateway_Addr" != "x" ];then
                        	Ipv6_Gateway_Addr=$(echo $Ipv6_Gateway_Addr | cut -d "/" -f 1)
                            ip -6 route del default dev "$cellular_ifname" table 12 > /dev/null
                        	ip -6 route add default via $Ipv6_Gateway_Addr dev "$cellular_ifname" table 12
                   	else
                        	ip -6 route add default dev "$cellular_ifname" table 12
                    	fi

                fi
       	elif [ "$2" = "0" ];then
               	echo "Deleting rule to resolve dns packets"
                ip rule del from all dport 53 lookup 12
                ip rule del iif "$cellular_ifname" lookup 11
                Ipv4_Gateway_Addr=$(sysevent get cellular_wan_v4_gw)
                if [ "x$Ipv4_Gateway_Addr" != "x" ];then
                        ip route del default via $Ipv4_Gateway_Addr dev "$cellular_ifname" table 12
                else
             		ip route del default dev "$cellular_ifname" table 12
                fi

                ip -6 rule del from all dport 53 lookup 12
                ip -6 rule del iif "$cellular_ifname" lookup 11
                Ipv6_Gateway_Addr=$(sysevent get cellular_wan_v6_gw)
                if [ "x$Ipv6_Gateway_Addr" != "x" ];then
                	Ipv6_Gateway_Addr=$(echo $Ipv6_Gateway_Addr | cut -d "/" -f 1)
                        ip -6 route del default via $Ipv6_Gateway_Addr dev "$cellular_ifname" table 12
                else
                        ip -6 route del default dev "$cellular_ifname" table 12
                fi
   	fi
        ;;
    wan-status)

        if [ "1" = "$DEVICE_MODE" ] ; then
        	cellular_ifname=`sysevent get cellular_ifname`

            	if [ "$2" = "started" ];then
               		echo "Adding rule to resolve dns packets"
               		ip rule add from all dport 53 lookup 12
                    	ip rule add iif "$cellular_ifname" lookup 11
                   	Ipv4_Gateway_Addr=$(sysevent get cellular_wan_v4_gw)
                   	if [ "x$Ipv4_Gateway_Addr" != "x" ];then
                            ip route del default dev "$cellular_ifname" table 12 > /dev/null
                        	ip route add default via $Ipv4_Gateway_Addr dev "$cellular_ifname" table 12
                	else
                        	ip route add default dev "$cellular_ifname" table 12
                	fi
                	ip -6 rule add from all dport 53 lookup 12
                    	ip -6 rule add iif "$cellular_ifname" lookup 11
                	Ipv6_Gateway_Addr=$(sysevent get cellular_wan_v6_gw)
               		if [ "x$Ipv6_Gateway_Addr" != "x" ];then
                		Ipv6_Gateway_Addr=$(echo $Ipv6_Gateway_Addr | cut -d "/" -f 1)
                            ip -6 route del default dev "$cellular_ifname" table 12 > /dev/null
                        	ip -6 route add default via $Ipv6_Gateway_Addr dev "$cellular_ifname" table 12
                	else
                        	ip -6 route add default dev "$cellular_ifname" table 12
                	fi
                elif [ "$2" = "stopped" ];then
                	echo "Deleting rule to resolve dns packets"
                	ip rule del from all dport 53 lookup 12
                    	ip rule del iif "$cellular_ifname" lookup 11
                	Ipv4_Gateway_Addr=$(sysevent get cellular_wan_v4_gw)
                	if [ "x$Ipv4_Gateway_Addr" != "x" ];then
                        	ip route del default via $Ipv4_Gateway_Addr dev "$cellular_ifname" table 12
                	else
                    		ip route del default dev "$cellular_ifname" table 12
                	fi

                	ip -6 rule del from all dport 53 lookup 12
                    	ip -6 rule del iif "$cellular_ifname" lookup 11
                 	Ipv6_Gateway_Addr=$(sysevent get cellular_wan_v6_gw)

                   	if [ "x$Ipv6_Gateway_Addr" != "x" ];then
                        	Ipv6_Gateway_Addr=$(echo $Ipv6_Gateway_Addr | cut -d "/" -f 1)
                        	ip -6 route del default via $Ipv6_Gateway_Addr dev "$cellular_ifname" table 12
                   	else
                        	ip -6 route del default dev "$cellular_ifname" table 12
                	fi       
		fi
        fi
        ;;
    mesh_wan_linkstatus)

        if [ "1" = "$DEVICE_MODE" ] ; then

        sysevent set dhcp_server-restart
        sleep 2
        mesh_wan_ifname_ipaddr=$(ip -4 addr show dev "$mesh_wan_ifname" scope global | awk '/inet/{print $2}' | cut -d '/' -f1)
        if [ "x$mesh_wan_ifname_ipaddr" = "x" ];then
            mesh_wan_ifname_ipaddr="192.168.246.1"
        fi
            mesh_wan_ula_pref=$(sysevent get MeshWanInterface_UlaPref)
            mesh_wan_ula_ipv6=$(sysevent get MeshWANInterface_UlaAddr)
            mesh_remote_wan_ula_ipv6=$(sysevent get MeshRemoteWANInterface_UlaAddr)
            if [ "$2" = "up" ];then
            	ip rule add from all iif "$mesh_wan_ifname" lookup 12
                ip route add default via "$mesh_wan_ifname_ipaddr" dev "$mesh_wan_ifname" table 11
                ip -6 rule add from all iif "$mesh_wan_ifname" lookup 12
                ip -6 addr add "$mesh_wan_ula_ipv6" dev "$mesh_wan_ifname" 
                if [ "x$mesh_remote_wan_ula_ipv6" != "x" ];then
                    mesh_remote_wan_ula_ipv6=$(echo $mesh_remote_wan_ula_ipv6 | cut -d "/" -f 1)
                    ip -6 route add default via "$mesh_remote_wan_ula_ipv6" dev "$mesh_wan_ifname" table 11
                fi
                #ip -6 route add default dev "$mesh_wan_ifname" table 11

                sysevent set ipv6_prefix "$mesh_wan_ula_pref"
                sysevent set zebra-restart
            elif [ "$2" = "down" ];then
                ip rule del from all iif "$mesh_wan_ifname" lookup 12
                ip route del default via "$mesh_wan_ifname_ipaddr" dev "$mesh_wan_ifname" table 11
                ip -6 rule del from all iif "$mesh_wan_ifname" lookup 12
                if [ "x$mesh_remote_wan_ula_ipv6" != "x" ];then
                    mesh_remote_wan_ula_ipv6=$(echo $mesh_remote_wan_ula_ipv6 | cut -d "/" -f 1)
                    ip -6 route del default via "$mesh_remote_wan_ula_ipv6" dev "$mesh_wan_ifname" table 11
                fi
                ip -6 addr del "$mesh_wan_ula_ipv6" dev "$mesh_wan_ifname" 
                #ip -6 route del default dev "$mesh_wan_ifname" table 11
                sysevent set zebra-stop
            fi
            
            sysevent set firewall-restart
        fi

        ;;
    *)
        echo "Usage: service-${SERVICE_NAME} [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
        exit 3
        ;;
esac
