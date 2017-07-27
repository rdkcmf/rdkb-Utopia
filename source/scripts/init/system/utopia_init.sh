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

#------------------------------------------------------------------
#   This file contains the code to initialize the board
#------------------------------------------------------------------
UTOPIA_PATH=/etc/utopia/service.d
SWITCH_HANDLER=$UTOPIA_PATH/service_multinet/handle_sw.sh
INIT_DIR=/etc/utopia/registration.d

echo "*******************************************************************"
echo "*                                                                  "
echo "* Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.   "
echo "*                                                                  "
echo "*******************************************************************"

source $UTOPIA_PATH/log_capture_path.sh
if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi
dmesg -n 5

TR69TLVFILE="/nvram/TLVData.bin"
REVERTFLAG="/nvram/reverted"
# determine the distro type (GAP or GNP)
if [ -n "$(grep TPG /etc/drg_version.txt)" ]; then
    distro=GAP
else
    distro=GNP
fi

# determine the build type (debug or production)
if [ -f /etc/debug_build ] ; then
    debug_build=1
else
    debug_build=0
fi

if [ $BOX_TYPE == "XB3" ];then
	if [ $MFG_NAME == "Arris" ]; then
	       firmware_name=`cat /version.txt | grep ^imagename= | cut -d "=" -f 2`
 	else
       		firmware_name=`cat /version.txt | grep ^imagename: | cut -d ":" -f 2`
 	fi
else
	firmware_name=`cat /version.txt | grep ^imagename= | cut -d "=" -f 2`
fi
echo_t "DEVICE_INIT:$firmware_name"


echo "*******************************************************************"
echo "*                                                                  "
echo_t "[utopia][init] P-UNIT status"
cat /proc/P-UNIT/status
echo "*                                                                  "
echo "*******************************************************************"

echo "[utopia][init] Tweaking network parameters" > /dev/console
echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout_stream
echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_syn_sent
echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_generic_timeout
echo "10" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_time_wait
echo "10" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close
echo "20" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close_wait
echo "1800" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established
echo "8192" > /proc/sys/net/ipv4/netfilter/ip_conntrack_max

echo "400" > /proc/sys/net/netfilter/nf_conntrack_expect_max

#RDKB-9061 Fix
#echo 4096 > /proc/sys/net/ipv6/neigh/default/gc_thresh1
#echo 8192 > /proc/sys/net/ipv6/neigh/default/gc_thresh2
#echo 8192 > /proc/sys/net/ipv6/neigh/default/gc_thresh3

# starting the rpcserver
echo "[utopia][init] Starting rpcserver from arm" > /dev/console
if [ -f /usr/bin/rpcserver ];then
    /usr/bin/rpcserver &
fi

#echo "[utopia][init] Loading drivers"
#MODULE_PATH=/fss/gw/lib/modules/`uname -r`/
#insmod $MODULE_PATH/drivers/net/erouter_ni.ko netdevname=erouter0

#if [ "$distro" = "GAP" ]; then
#    #
#    # ---- GAP: boot sequence (TPG)
#    #
#
#    sh /etc/rcS.d/11platform-init.sh
#
#    echo "*******************************************************************"
#    echo "*                                                                  "
#    echo "* Booting Cisco DRG `getFlashValue model -d`                       "
#    echo "* Hardware ID: `getFlashValue hwid -d` Hardware Version: `getFlashValue hwversion -d`"
#    echo "* Unit Serial Number: `getFlashValue unitsn`                       "
#    echo "* Board Serial Number: `getFlashValue boardsn`                     "
#    echo "* Manufacture Date: `getFlashValue mfgdate -d`                     "
#    echo "* Software Version: `cat /etc/drg_version.txt`                     "
#    echo "*                                                                  "
#    echo "*******************************************************************"
#
#else
#    #
#    # ---- GNP: boot sequence (CNS)
#    #
#
#    echo "*******************************************************************"
#    echo "* Software Version: `cat /etc/drg_version.txt`                     "
#    echo "*******************************************************************"
#
#    insmod /lib/modules/`uname -r`/kernel/drivers/wifi/wl.ko
#    cp /etc/utopia/service.d/nvram.dat /tmp
#fi
echo_t "Starting log module.."
/fss/gw/usr/sbin/log_start.sh

echo_t "[utopia][init] Starting udev.."

# Spawn telnet daemon only for production images
#if [ $debug_build -ne 0 ]; then
    #echo "[utopia][init] Starting telnetd"
    #service telnet start
    #utelnetd -d
#fi

#echo "[utopia][init]  Starting syslogd"
#/sbin/syslogd && /sbin/klogd

# echo "[utopia][init] Provisioning loopback interface"
#ip addr add 127.0.0.1/255.0.0.0 dev lo
#ip link set lo up
#ip route add 127.0.0.0/8 dev lo

# create our passwd/shadow/group files
#mkdir -p /tmp/etc/.root
#chmod 711 /tmp/etc/.root

#chmod 644 /tmp/etc/.root/passwd
#chmod 600 /tmp/etc/.root/shadow
#chmod 600 /tmp/etc/.root/group

# create the default profile. This is linked to by /etc/profile 
#echo "export setenv PATH=/bin:/sbin:/usr/sbin:/usr/bin:/opt/sbin:/opt/bin" > /tmp/profile
#echo "export setenv LD_LIBRARY_PATH=/lib:/usr/lib:/opt/lib" >> /tmp/profile
#echo "if [ \$(tty) != \"/dev/console\"  -a  \${USER} != \"root\" ]; then cd /usr/cosa; ./cli_start.sh; fi" >> /tmp/profile

# create other files that are linked to by etc
#echo -n > /tmp/hosts
#echo -n > /tmp/hostname
#echo -n > /tmp/resolv.conf
#echo -n > /tmp/igmpproxy.conf
#echo -n > /tmp/ez-ipupdate.conf
#echo -n > /tmp/ez-ipupdate.out
#echo -n > /tmp/TZ
#echo -n > /tmp/.htpasswd
#echo -n > /tmp/dnsmasq.conf
#echo -n > /tmp/dhcp_options
#echo -n > /tmp/dhcp_static_hosts
#echo -n > /tmp/dnsmasq.leases
#echo -n > /tmp/zebra.conf
#echo -n > /tmp/ripd.conf
#echo -n > /tmp/dhcp6c.conf

mkdir -p /tmp/cron

BUTTON_THRESHOLD=15
FACTORY_RESET_KEY=factory_reset
FACTORY_RESET_RGWIFI=y
FACTORY_RESET_WIFI=w
SYSCFG_MOUNT=/nvram
SYSCFG_FILE=$SYSCFG_MOUNT/syscfg.db
PSM_CUR_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_cur_cfg.xml"
PSM_BAK_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_bak_cfg.xml"
PSM_TMP_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_tmp_cfg.xml"
XDNS_DNSMASQ_SERVERS_CONFIG_FILE_NAME="$SYSCFG_MOUNT/dnsmasq_servers.conf"
FACTORY_RESET_REASON=false  

#syscfg_check -d $MTD_DEVICE
#if [ $? = 0 ]; then
#   echo "[utopia][init] Starting syscfg subsystem using flash partition $MTD_DEVICE"
#   /sbin/syscfg_create -d $MTD_DEVICE
#else
#   echo "[utopia][init] Formating flash partition $MTD_DEVICE for syscfg use"
#   syscfg_format -d $MTD_DEVICE
#   if [ $? = 0 ]; then
#      echo "[utopia][init] Starting syscfg subsystem using flash partition $MTD_DEVICE with default settings"
#      /sbin/syscfg_create -d $MTD_DEVICE
#   else
#      echo "[utopia][init] FAILURE: formatting flash partition $MTD_DEVICE for syscfg use"
#      echo "[utopia][init] Starting syscfg with default settings using file store ($SYSCFG_FILE)"
#      echo "" > $SYSCFG_FILE
#      /sbin/syscfg_create -f $SYSCFG_FILE
#   fi
#fi

echo_t "[utopia][init] Starting syscfg using file store ($SYSCFG_FILE)"
if [ -f $SYSCFG_FILE ]; then 
   syscfg_create -f $SYSCFG_FILE
else
   echo -n > $SYSCFG_FILE
   syscfg_create -f $SYSCFG_FILE
   #>>zqiu
   echo_t "[utopia][init] need to reset wifi when ($SYSCFG_FILE) is not avaliable (for 1st time boot up)"
   syscfg set $FACTORY_RESET_KEY $FACTORY_RESET_WIFI
   #<<zqiu

   # Put value 204 into networkresponse.txt file so that
   # all LAN services start with a configuration which will
   # redirect everything to Gateway IP.
   # This value again will be modified from network_response.sh 
   echo_t "[utopia][init] Echoing network response during Factory reset"
   echo 204 > /var/tmp/networkresponse.txt
fi

# Read reset duration to check if the unit was rebooted by pressing the HW reset button
if cat /proc/P-UNIT/status | grep -q "Reset duration from shadow register"; then
   # Note: Only new P-UNIT firmwares and Linux drivers (>= 1.1.x) support this.
   PUNIT_RESET_DURATION=`cat /proc/P-UNIT/status|grep "Reset duration from shadow register"|awk -F '[ |\.]' '{ print $9 }'`
   # Clear the Reset duration from shadow register value
   # echo "1" > /proc/P-UNIT/clr_reset_duration_shadow
    touch /var/tmp/utopia_cleared_shadow_reg.txt
   clean_reset_duration;
elif cat /proc/P-UNIT/status | grep -q "Last reset duration"; then
   PUNIT_RESET_DURATION=`cat /proc/P-UNIT/status|grep "Last reset duration"|awk -F '[ |\.]' '{ print $7 }'`
else
   echo_t "[utopia][init] Cannot read the reset duration value from /proc/P-UNIT/status"
fi

# Set the factory reset key if it was pressed for longer than our threshold
if test "$BUTTON_THRESHOLD" -le "$PUNIT_RESET_DURATION"; then
   syscfg set $FACTORY_RESET_KEY $FACTORY_RESET_RGWIFI && BUTTON_FR="1"
fi

SYSCFG_FR_VAL="`syscfg get $FACTORY_RESET_KEY`"

if [ "x$FACTORY_RESET_RGWIFI" = "x$SYSCFG_FR_VAL" ]; then
   echo_t "[utopia][init] Performing factory reset"
# Remove log file first because it need get log file path from syscfg   
   /fss/gw/usr/sbin/log_handle.sh reset
   echo -e "\n" | syscfg_destroy 
#   umount $SYSCFG_MOUNT
#   SYSDATA_MTD=`grep SysData /proc/mtd | awk -F: '{print $1}'`
#   if [ -n $SYSDATA_MTD ]; then
#      echo "[utopia][init] wiping system data flash"
#      flash_eraseall -j /dev/$SYSDATA_MTD
#      echo "[utopia][init] remounting system data flash"
#      mount -t jffs2 mtd:SysData $SYSCFG_MOUNT
#      echo -n > $SYSCFG_FILE
#   fi

# Remove syscfg and PSM storage files

#mark the factory reset flag 'on'
   FACTORY_RESET_REASON=true 
   
   rm -f $SYSCFG_FILE
   rm -f $PSM_CUR_XML_CONFIG_FILE_NAME
   rm -f $PSM_BAK_XML_CONFIG_FILE_NAME
   rm -f $PSM_TMP_XML_CONFIG_FILE_NAME
   rm -f $TR69TLVFILE
   rm -f $REVERTFLAG
   rm -f $XDNS_DNSMASQ_SERVERS_CONFIG_FILE_NAME
   # Remove DHCP lease file
   rm -f /nvram/dnsmasq.leases
   rm -f /nvram/server-IfaceMgr.xml
   rm -f /nvram/server-AddrMgr.xml
   rm -f /nvram/server-CfgMgr.xml
   rm -f /nvram/server-TransMgr.xml
   rm -f /nvram/server-cache.xml
   rm -f /nvram/server-duid

   #>>zqiu
   create_wifi_default
   #<<zqiu
   echo_t "[utopia][init] Retarting syscfg using file store ($SYSCFG_FILE)"
   syscfg_create -f $SYSCFG_FILE
#>>zqiu
   # Put value 204 into networkresponse.txt file so that
   # all LAN services start with a configuration which will
   # redirect everything to Gateway IP.
   # This value again will be modified from network_response.sh 
   echo_t "[utopia][init] Echoing network response during Factory reset"
   echo 204 > /var/tmp/networkresponse.txt
    

elif [ "x$FACTORY_RESET_WIFI" = "x$SYSCFG_FR_VAL" ]; then
    echo_t "[utopia][init] Performing wifi reset"
    create_wifi_default
    syscfg unset $FACTORY_RESET_KEY
#<<zqiu
fi
#echo "[utopia][init] Cleaning up vendor nvram"
# /etc/utopia/service.d/nvram_cleanup.sh

echo_t "*** HTTPS root certificate for TR69 ***"

if [ ! -f /etc/cacert.pem ]; then
	echo_t "HTTPS root certificate for TR69 is missing..."

else
	echo_t "Copying HTTPS root certificate for TR69"
	if [ -f /nvram/cacert.pem ]; then
		rm -f /nvram/cacert.pem
	fi
	cp -f /etc/cacert.pem /nvram/
fi

echo_t "[utopia][init] Starting system logging"
$UTOPIA_PATH/service_syslog.sh syslog-start

echo_t "[utopia][init] Starting sysevent subsystem"
#syseventd --threads 18
syseventd

# we want plugged in usb devices to propagate events to sysevent
#echo "[utopia][init] Late loading usb drivers"
#MODULE_PATH=/lib/modules/`uname -r`/
#insmod $MODULE_PATH/usbcore.ko
#insmod $MODULE_PATH/ehci-hcd.ko
#insmod $MODULE_PATH/scsi_mod.ko
#insmod $MODULE_PATH/sd_mod.ko
#insmod $MODULE_PATH/libusual.ko
#insmod $MODULE_PATH/usb-storage.ko
#insmod $MODULE_PATH/nls_cp437.ko
#insmod $MODULE_PATH/nls_iso8859-1.ko
#insmod $MODULE_PATH/fat.ko
#insmod $MODULE_PATH/vfat.ko

echo_t "[utopia][init] Setting any unset system values to default"
apply_system_defaults

# Get the syscfg value which indicates whether unit is activated or not.
# This value is set from network_response.sh based on the return code received.
activated=`syscfg get unit_activated`
echo_t "[utopia][init] Value of unit_activated got is : $activated"
if [ "$activated" = "1" ]
then
    echo_t "[utopia][init] Echoing network response during Reboot"
    echo 204 > /var/tmp/networkresponse.txt
fi 

echo_t "[utopia][init] Applying iptables settings"

lan_ifname=`syscfg get lan_ifname`
cmdiag_ifname=`syscfg get cmdiag_ifname`
ecm_wan_ifname=`syscfg get ecm_wan_ifname`
wan_ifname=`sysevent get wan_ifname`

#disable telnet / ssh ports
iptables -A INPUT -i $lan_ifname -p tcp --dport 23 -j DROP
iptables -A INPUT -i $lan_ifname -p tcp --dport 22 -j DROP
iptables -A INPUT -i $cmdiag_ifname -p tcp --dport 23 -j DROP
iptables -A INPUT -i $cmdiag_ifname -p tcp --dport 22 -j DROP

ip6tables -A INPUT -i $lan_ifname -p tcp --dport 23 -j DROP
ip6tables -A INPUT -i $lan_ifname -p tcp --dport 22 -j DROP
ip6tables -A INPUT -i $cmdiag_ifname -p tcp --dport 23 -j DROP
ip6tables -A INPUT -i $cmdiag_ifname -p tcp --dport 22 -j DROP

#protect from IPv6 NS flooding
ip6tables -t mangle -A PREROUTING -i $ecm_wan_ifname -d ff00::/8 -p ipv6-icmp -m icmp6 --icmpv6-type 135 -j DROP
ip6tables -t mangle -A PREROUTING -i $wan_ifname -d ff00::/8 -p ipv6-icmp -m icmp6 --icmpv6-type 135 -j DROP

/fss/gw/sbin/ulogd -c /fss/gw/etc/ulogd.conf -d

#echo "[utopia][init] Starting telnetd"
#TELNET_ENABLE=`syscfg get mgmt_wan_telnetaccess`
#if [ "$TELNET_ENABLE" = "1" ]; then
#    if [ -e /bin/login ]; then
#        /usr/sbin/telnetd -l /bin/login
#    else
#        /usr/sbin/telnetd
#    fi
#fi


echo_t "[utopia][init] Processing registration"
# run all executables in the sysevent registration directory
# echo "[utopia][init] Running registration using $INIT_DIR"
execute_dir $INIT_DIR&
#init_inter_subsystem&

#--------Set up private IPC vlan----------------
vconfig add l2sd0 500
if [ $BOX_TYPE == "XB3" ];then
	$UTOPIA_PATH/service_multinet_exec add_ipc_vlan &
else
	$SWITCH_HANDLER addVlan 0 500 sw_6
fi
ifconfig l2sd0.500 192.168.101.1

#start  ntpd server on ARM
NTP_CONF=/etc/ntp.conf
NTP_CONF_TMP=/tmp/ntp.conf
if [ "x$BOX_TYPE" = "xXB3" ]
then
	cp $NTP_CONF $NTP_CONF_TMP
	echo "interface ignore wildcard" >> $NTP_CONF_TMP
	echo "interface listen $ARM_INTERFACE_IP" >> $NTP_CONF_TMP
	ntpd -c $NTP_CONF_TMP 
fi
#--------Set up Radius vlan -------------------
vconfig add l2sd0 4090
if [ $BOX_TYPE == "XB3" ];then
	$UTOPIA_PATH/service_multinet_exec add_radius_vlan &
else
	$SWITCH_HANDLER addVlan 0 4090 sw_6 
fi
ifconfig l2sd0.4090 192.168.251.1 netmask 255.255.255.0 up
ip rule add from all iif l2sd0.4090 lookup erouter

#--------Marvell LAN-side egress flood mitigation----------------
echo_t "88E6172: Do not egress flood unicast with unknown DA"
swctl -c 11 -p 5 -r 4 -b 0x007b

# Creating IOT VLAN on ARM
swctl -c 16 -p 0 -v 106 -m 2 -q 1
swctl -c 16 -p 7 -v 106 -m 2 -q 1
vconfig add l2sd0 106
ifconfig l2sd0.106 192.168.106.1 netmask 255.255.255.0 up
ip rule add from all iif l2sd0.106 lookup erouter

# Creating MDC VLAN on ARM
if [ -e /usr/bin/Arm_Mdc ]; then

  echo "Starting mdc-vlan config"
  echo "Adding l2sd0.4040 for mdc"
  swctl -c 16 -p 0 -v 4040 -m 2 -q 1
  swctl -c 16 -p 7 -v 4040 -m 2 -q 1
  vconfig add l2sd0 4040
  echo "Added l2sd0.4040 for mdc"
  ifconfig l2sd0.4040 192.168.250.2 netmask 255.255.255.0 up
  ip rule add to 192.168.250.1 lookup all_mdc
  ip route add default via 192.168.250.1 dev l2sd0.4040 table all_mdc

fi

ip rule add to 192.168.254.254 lookup 2
ip route add default via 192.168.254.254 dev l2sd0.4093 table 2

# Check and set factory-reset as reboot reason 
if [ "$FACTORY_RESET_REASON" = "true" ]; then
   echo_t "[utopia][init] Detected last reboot reason as factory-reset"
   syscfg set X_RDKCENTRAL-COM_LastRebootReason "factory-reset"
   syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
else
   rebootReason=`syscfg get X_RDKCENTRAL-COM_LastRebootReason`
   echo_t "[utopia][init] X_RDKCENTRAL-COM_LastRebootReason ($rebootReason)"
   if [ "$rebootReason" = "factory-reset" ]; then
      echo_t "[utopia][init] Setting last reboot reason as unknown"
      syscfg set X_RDKCENTRAL-COM_LastRebootReason "unknown"
   else
      if [ -f /nvram/restore_reboot ];then
         syscfg set X_RDKCENTRAL-COM_LastRebootReason "restore-reboot"
         syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
         rm -f /nvram/restore_reboot
      fi
   fi
fi

#echo_t "[utopia][init] started dropbear process"
#$UTOPIA_PATH/service_sshd.sh sshd-start &

echo_t "[utopia][init] setting Multicast MAC before any switch configs"
$UTOPIA_PATH/service_multinet_exec set_multicast_mac &

echo_t "[utopia][init] completed creating utopia_inited flag"
touch /tmp/utopia_inited
