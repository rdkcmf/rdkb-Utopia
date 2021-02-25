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
#   This file contains the code to initialize the board
#------------------------------------------------------------------

changeFilePermissions() {

	if [ -e $1 ]; then 
		filepermission=$(stat -c %a $1)
	
		if [ $filepermission -ne $2 ] 
		then
		
			chmod $2 $1
			echo "[utopia][init] Modified File Permission to $2 for file - $1"
		fi
	else
		echo "[utopia][init] changeFilePermissions: file $1 doesn't exist"
	fi
}

echo "*******************************************************************"
echo "*                                                                  "
echo "* Copyright 2014 Cisco Systems, Inc. 				 "
echo "* Licensed under the Apache License, Version 2.0                   "
echo "*                                                                  "
echo "*******************************************************************"

source /etc/utopia/service.d/log_capture_path.sh

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

dmesg -n 5

TR69TLVFILE="/nvram/TLVData.bin"
TR69KEYS="/nvram/.keys"
REVERTFLAG="/nvram/reverted"
MAINT_START="/nvram/.FirmwareUpgradeStartTime"
MAINT_END="/nvram/.FirmwareUpgradeEndTime"
# determine the distro type (GAP or GNP)
#distro not used
#if [ -n "$(grep TPG /etc/drg_version.txt)" ]; then
#    distro=GAP
#else
#    distro=GNP
#fi

# determine the build type (debug or production)
if [ -f /etc/debug_build ] ; then
    debug_build=1
else
    debug_build=0
fi

firmware_name=`cat /version.txt | grep ^imagename: | cut -d ":" -f 2`
utc_time=`date -u`
echo "[$utc_time] [utopia][init] DEVICE_INIT:$firmware_name"
if [ -e "/usr/bin/onboarding_log" ]; then
    /usr/bin/onboarding_log "[utopia][init] DEVICE_INIT:$firmware_name"
fi

echo "[utopia][init] Tweaking network parameters" > /dev/console

KERNEL_VERSION=`uname -r | cut -c 1`

if [ $KERNEL_VERSION -lt 4 ] ; then
	echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout_stream
	echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_syn_sent
	echo "60" > /proc/sys/net/ipv4/netfilter/ip_conntrack_generic_timeout
	echo "10" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_time_wait
	echo "10" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close
	echo "20" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close_wait
        if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ] ; then
		#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
		echo "16384" > /proc/sys/net/ipv4/netfilter/ip_conntrack_max
		echo "7440" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established
	else
		# TCCBR-1849 - don't override nf_conntrack_max here, this value is set at /lib/rdk/brcm.networking
		#echo "8192" > /proc/sys/net/ipv4/netfilter/ip_conntrack_max
		echo "[$utc_time] [utopia][init] don't override nf_conntrack_max here, value is set at /lib/rdk/brcm.networking"
		echo "7440" > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established
	fi

else
	echo "60" > /proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream
	echo "60" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_syn_sent
	echo "60" > /proc/sys/net/netfilter/nf_conntrack_generic_timeout
	echo "10" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_time_wait
	echo "10" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_close
	echo "20" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_close_wait
	if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ] ; then
		#Intel Proposed RDKB Generic Bug Fix from XB6 SDK
		echo "16384" > /proc/sys/net/netfilter/nf_conntrack_max
		echo "7440" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established
	else
		# TCCBR-1849 - don't override nf_conntrack_max here, this value is set at /lib/rdk/brcm.networking
		#echo "8192" > /proc/sys/net/netfilter/ip_conntrack_max
		echo "[$utc_time] [utopia][init] don't override nf_conntrack_max here, value is set at /lib/rdk/brcm.networking"
		echo "7440" > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established
	fi
fi

echo "400" > /proc/sys/net/netfilter/nf_conntrack_expect_max

# RDKB-26160
#echo 4096 > /proc/sys/net/ipv6/neigh/default/gc_thresh1
#echo 8192 > /proc/sys/net/ipv6/neigh/default/gc_thresh2
#echo 8192 > /proc/sys/net/ipv6/neigh/default/gc_thresh3

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
echo "Starting log module.."
/fss/gw/usr/sbin/log_start.sh

echo "[utopia][init] Starting udev.."

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
SYSCFG_TMP_LOCATION=/tmp
SYSCFG_FILE=$SYSCFG_TMP_LOCATION/syscfg.db
SYSCFG_BKUP_FILE=$SYSCFG_MOUNT/syscfg.db
SYSCFG_OLDBKUP_FILE=$SYSCFG_MOUNT/syscfg_bkup.db
SYSCFG_ENCRYPTED_PATH=/opt/secure/
SYSCFG_PERSISTENT_PATH=/opt/secure/data
SYSCFG_NEW_FILE=$SYSCFG_PERSISTENT_PATH/syscfg.db
SYSCFG_NEW_BKUP_FILE=$SYSCFG_PERSISTENT_PATH/syscfg_bkup.db
PSM_CUR_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_cur_cfg.xml"
PSM_BAK_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_bak_cfg.xml"
PSM_TMP_XML_CONFIG_FILE_NAME="$SYSCFG_MOUNT/bbhm_tmp_cfg.xml"
XDNS_DNSMASQ_SERVERS_CONFIG_FILE_NAME="$SYSCFG_MOUNT/dnsmasq_servers.conf"
FACTORY_RESET_REASON=false

if [ -d $SYSCFG_ENCRYPTED_PATH ]; then
       if [ ! -d $SYSCFG_PERSISTENT_PATH ]; then
               echo "$SYSCFG_PERSISTENT_PATH path not available creating directory and touching $SYSCFG_NEW_FILE file"
               mkdir $SYSCFG_PERSISTENT_PATH
               touch $SYSCFG_NEW_FILE
       fi
fi

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

CheckAndReCreateDB()
{
	NVRAMFullStatus=`df -h $SYSCFG_MOUNT | grep "100%"`
	if [ "$NVRAMFullStatus" != "" ]; then
		if [ -f "/rdklogger/rdkbLogMonitor.sh" ]
		then
			  #Remove Old backup files if there	
			  sh /rdklogger/rdkbLogMonitor.sh "remove_old_logbackup"		 

		  	  #Re-create syscfg create again
			  syscfg_create -f $SYSCFG_FILE
			  syscfg_oldDB=$?
			  if [ $syscfg_oldDB -ne 0 ]; then
				  NVRAMFullStatus=`df -h $SYSCFG_MOUNT | grep "100%"`
				  if [ "$NVRAMFullStatus" != "" ]; then
					 echo "[utopia][init] NVRAM Full(100%) and below is the dump"
					 du -h $SYSCFG_MOUNT 
					 ls -al $SYSCFG_MOUNT	 
				  fi
			  fi 
		fi
	fi 
}


if [ -f $SYSCFG_BKUP_FILE ]; then
        echo "[utopia][init] Starting syscfg using file store ($SYSCFG_BKUP_FILE)"
        if [ -d $SYSCFG_PERSISTENT_PATH ] && [ ! -f $SYSCFG_NEW_FILE ]; then
    	        cp $SYSCFG_BKUP_FILE $SYSCFG_NEW_FILE
        fi
        cp $SYSCFG_BKUP_FILE $SYSCFG_FILE
	syscfg_create -f $SYSCFG_FILE
        if [ $? != 0 ]; then
	     CheckAndReCreateDB
	fi
elif [ -s $SYSCFG_NEW_FILE ]; then
    echo "[utopia][init] Starting syscfg using file store ($SYSCFG_NEW_FILE)"
    SECURE_SYSCFG=`grep UpdateNvram $SYSCFG_NEW_FILE | cut -f2 -d=`
    echo "[utopia][init] UpdateNvram:$SECURE_SYSCFG"
    if [ "$SECURE_SYSCFG" = "false"  ]; then
          cp $SYSCFG_NEW_FILE $SYSCFG_FILE
    else
	  cp $SYSCFG_NEW_FILE $SYSCFG_BKUP_FILE
	  cp $SYSCFG_NEW_FILE $SYSCFG_FILE
    fi
    syscfg_create -f $SYSCFG_FILE
    if [ $? != 0 ]; then
         CheckAndReCreateDB
    fi     
else
	 echo -n > $SYSCFG_FILE
	 echo -n > $SYSCFG_BKUP_FILE
         echo -n > $SYSCFG_NEW_FILE
   syscfg_create -f $SYSCFG_FILE
   if [ $? != 0 ]; then
	CheckAndReCreateDB
   fi
   #>>zqiu
   echo "[utopia][init] need to reset wifi when ($SYSCFG_BKUP_FILE) and ($SYSCFG_NEW_FILE) files are not available"
   syscfg set $FACTORY_RESET_KEY $FACTORY_RESET_WIFI
   syscfg commit
   #<<zqiu
   touch /nvram/.apply_partner_defaults
   # Put value 204 into networkresponse.txt file so that
   # all LAN services start with a configuration which will
   # redirect everything to Gateway IP.
   # This value again will be modified from network_response.sh 
   echo "[utopia][init] Echoing network response during Factory reset"
   echo 204 > /var/tmp/networkresponse.txt
fi

if [ -f $SYSCFG_OLDBKUP_FILE ];then
	rm -rf $SYSCFG_OLDBKUP_FILE
fi
if [ -f $SYSCFG_NEW_BKUP_FILE ]; then
	rm -rf $SYSCFG_NEW_BKUP_FILE
fi

SYSCFG_LAN_DOMAIN=`syscfg get lan_domain` 

if [ "$SYSCFG_LAN_DOMAIN" == "utopia.net" ]; then
   echo "[utopia][init] Setting lan domain to NULL"
   syscfg set lan_domain ""
   syscfg commit
fi

# Read reset duration to check if the unit was rebooted by pressing the HW reset button
if [ -s /sys/bus/acpi/devices/INT34DB:00/reset_btn_dur ]; then
   #Note: /sys/bus/acpi/devices/INT34DB:00/reset_btn_dur is an Arris XB6 File created by Arris and Intel by reading ARM
   PUNIT_RESET_DURATION=`cat /sys/bus/acpi/devices/INT34DB:00/reset_btn_dur`
else
   echo "[utopia][init] /sys/bus/acpi/devices/INT34DB:00/reset_btn_dur is empty or missing"
   PUNIT_RESET_DURATION=0
fi

#ForwardSSH log print

ForwardSSH=`syscfg get ForwardSSH`
Log_file="/rdklogs/logs/FirewallDebug.txt"
if $ForwardSSH;then
   echo "SSH: Forward SSH changed to enabled" >> $Log_file
else
   echo "SSH: Forward SSH changed to disabled" >> $Log_file
fi

# Set the factory reset key if it was pressed for longer than our threshold
if test "$BUTTON_THRESHOLD" -le "$PUNIT_RESET_DURATION"; then
   syscfg set $FACTORY_RESET_KEY $FACTORY_RESET_RGWIFI && BUTTON_FR="1"
   syscfg commit
fi

SYSCFG_FR_VAL="`syscfg get $FACTORY_RESET_KEY`"

if [ "x$FACTORY_RESET_RGWIFI" = "x$SYSCFG_FR_VAL" ]; then
   echo "[utopia][init] Performing factory reset"
   
SYSCFG_PARTNER_FR="`syscfg get PartnerID_FR`"
if [ "1" = "$SYSCFG_PARTNER_FR" ]; then
   echo_t "[utopia][init] Performing factory reset due to PartnerID change"
fi
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
   rm -f /nvram/.keys/*
   rm -f /nvram/ble-enabled
   touch /nvram/.apply_partner_defaults
   rm -f $SYSCFG_BKUP_FILE
   rm -f $SYSCFG_FILE
   rm -f $SYSCFG_NEW_FILE
   rm -f $PSM_CUR_XML_CONFIG_FILE_NAME
   rm -f $PSM_BAK_XML_CONFIG_FILE_NAME
   rm -f $PSM_TMP_XML_CONFIG_FILE_NAME
   rm -f $TR69TLVFILE
   rm -f $REVERTFLAG
   rm -f $XDNS_DNSMASQ_SERVERS_CONFIG_FILE_NAME
   rm -f $MAINT_START
   rm -f $MAINT_END
   # Remove DHCP lease file
   rm -f /nvram/dnsmasq.leases
   rm -f /nvram/server-IfaceMgr.xml
   rm -f /nvram/server-AddrMgr.xml
   rm -f /nvram/server-CfgMgr.xml
   rm -f /nvram/server-TransMgr.xml
   rm -f /nvram/server-cache.xml
   rm -f /nvram/server-duid
   rm -f /nvram/partners_defaults.json 
   rm -f /nvram/bootstrap.json
   rm -f /opt/secure/RFC/tr181store.json
   if [ -f /nvram/.CMchange_reboot_count ];then
      rm -f /nvram/.CMchange_reboot_count
   fi

   # Remove lxy L2 dir
   LOG_FILE=/rdklogs/logs/lxy.log
   echo_t "[FR] Removing lxy L2 Dir" >> $LOG_FILE
   if [ -f /etc/lxy.conf ];then
       L2="$(grep '^L2=' /etc/lxy.conf | sed -e 's/L2=//')"
   fi
   if [ -d "$L2" ]; then
       rm -rf $L2
   fi

   echo "[utopia][init] Retarting syscfg using file store ($SYSCFG_BKUP_FILE)"
   if [ -f /etc/ONBOARD_LOGGING_ENABLE ]; then
   	# Remove onboard files
   	rm -f /nvram/.device_onboarded
	rm -f /nvram/DISABLE_ONBOARD_LOGGING
   	rm -rf /nvram2/onboardlogs
   fi

   if [ -f /etc/WEBCONFIG_ENABLE ]; then
    # Remove webconfig_db.bin on factory reset on all RDKB platforms
    rm -f /nvram/webconfig_db.bin
   fi
   if [ -f /etc/AKER_ENABLE ]; then      	
      # Remove on factory reset, Aker schedule pcs.bin and pcs.bin.md5 on all RDKB platforms 
      rm -f /nvram/pcs.bin
      rm -f /nvram/pcs.bin.md5
   fi
   touch $SYSCFG_NEW_FILE
   touch $SYSCFG_BKUP_FILE
   touch $SYSCFG_FILE
   syscfg_create -f $SYSCFG_FILE
   syscfg_oldDB=$?
   if [ $syscfg_oldDB -ne 0 ];then
	 CheckAndReCreateDB
   fi
   
#>>zqiu
   # Put value 204 into networkresponse.txt file so that
   # all LAN services start with a configuration which will
   # redirect everything to Gateway IP.
   # This value again will be modified from network_response.sh 
   echo "[utopia][init] Echoing network response during Factory reset"
   echo 204 > /var/tmp/networkresponse.txt
    

elif [ "x$FACTORY_RESET_WIFI" = "x$SYSCFG_FR_VAL" ]; then
    echo "[utopia][init] Performing wifi reset"
    create_wifi_default
    syscfg unset $FACTORY_RESET_KEY
#<<zqiu
fi
#echo "[utopia][init] Cleaning up vendor nvram"
# /etc/utopia/service.d/nvram_cleanup.sh

echo "*** HTTPS root certificate for TR69 ***"

if [ ! -f /etc/cacert.pem ]; then
	echo "HTTPS root certificate for TR69 is missing..."

fi

if [ -f /nvram/cacert.pem ]; then
        echo "Remove HTTPS root certificate for TR69 if available in NVRAM to prevent updating cert"
	rm -f /nvram/cacert.pem
fi

#echo "[utopia][init] Starting system logging"
#/etc/utopia/service.d/service_syslog.sh syslog-start

# update max number of msg in queue based on system maximum queue memory.
# This update will be used for presence detection feature.
MSG_SIZE_MAX=`cat /proc/sys/fs/mqueue/msgsize_max`
MSG_MAX_SYS=`ulimit -q`
TOT_MSG_MAX=50
if [ "x$MSG_MAX_SYS" = "x" ]; then
echo "ulimit cmd not avail assign mq msg_max :$TOT_MSG_MAX"
else
TOT_MSG_MAX=$((MSG_MAX_SYS/MSG_SIZE_MAX))
echo "mq msg_max :$TOT_MSG_MAX"
fi
echo $TOT_MSG_MAX > /proc/sys/fs/mqueue/msg_max


echo "[utopia][init] Starting sysevent subsystem"
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

#ARRISXB6-1554: apply_system_defaults calls sysevent API. Logs showed binaries weren't fully started
attemptCounter=0

until [ -e "/tmp/syseventd_connection" ]; do
    
    if [ $attemptCounter -lt 3 ]
    then
       sleep 2
       let "attemptCounter++"
    else
       break
    fi
done

echo "[utopia][init] Setting any unset system values to default"
apply_system_defaults
#ARRISXB6-2998
changeFilePermissions $SYSCFG_BKUP_FILE 400
changeFilePermissions $SYSCFG_NEW_FILE 400

SYSCFG_DB_FILE="/nvram/syscfg.db"
SECURE_SYSCFG=`syscfg get UpdateNvram`
if [ "$SECURE_SYSCFG" = "false" ]; then
      SYSCFG_DB_FILE="/opt/secure/data/syscfg.db"
fi
echo "[utopia][init] SEC: Syscfg stored in $SYSCFG_DB_FILE"

# Get the syscfg value which indicates whether unit is activated or not.
# This value is set from network_response.sh based on the return code received.
activated=`syscfg get unit_activated`
echo "[utopia][init] Value of unit_activated got is : $activated"
if [ "$activated" = "1" ]
then
    echo "[utopia][init] Echoing network response during Reboot"
    echo 204 > /var/tmp/networkresponse.txt
fi 

echo "[utopia][init] Applying iptables settings"

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


#echo "[utopia][init] Starting telnetd"
#TELNET_ENABLE=`syscfg get mgmt_wan_telnetaccess`
#if [ "$TELNET_ENABLE" = "1" ]; then
#    if [ -e /bin/login ]; then
#        /usr/sbin/telnetd -l /bin/login
#    else
#        /usr/sbin/telnetd
#    fi
#fi


echo "[utopia][init] Processing registration"
INIT_DIR=/etc/utopia/registration.d
# run all executables in the sysevent registration directory
# echo "[utopia][init] Running registration using $INIT_DIR"
execute_dir $INIT_DIR&
#init_inter_subsystem&

#--------Set up private IPC vlan----------------
#SWITCH_HANDLER=/etc/utopia/service.d/service_multinet/handle_sw.sh
#vconfig add l2sd0 500
#$SWITCH_HANDLER addVlan 0 500 sw_6
#ifconfig l2sd0.500 192.168.101.1

#--------Set up Radius vlan -------------------
#vconfig add l2sd0 4090
#$SWITCH_HANDLER addVlan 0 4090 sw_6
#ifconfig l2sd0.4090 192.168.251.1 netmask 255.255.255.0 up
#ip rule add from all iif l2sd0.4090 lookup erouter

#--------Marvell LAN-side egress flood mitigation----------------
#echo "88E6172: Do not egress flood unicast with unknown DA"
#swctl -c 11 -p 5 -r 4 -b 0x007b

# Creating IOT VLAN on ARM
#swctl -c 16 -p 0 -v 106 -m 2 -q 1
#swctl -c 16 -p 7 -v 106 -m 2 -q 1
#vconfig add l2sd0 106
#ifconfig l2sd0.106 192.168.106.1 netmask 255.255.255.0 up
#ip rule add from all iif l2sd0.106 lookup erouter

cause_file="/sys/bus/acpi/devices/INT34DB:00/reset_cause"
type_file="/sys/bus/acpi/devices/INT34DB:00/reset_type"
COLD_REBOOT=false
if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ]; then
   if [ -e $cause_file && -e $type_file ];then
      value_c=$(cat $cause_file)
      value_t=$(cat $type_file)
      if [ "$value_t" == "0" && "$value_c" == "0" ];then
         COLD_REBOOT=true
      fi
   fi
fi


# Check and set factory-reset as reboot reason
if [ "$FACTORY_RESET_REASON" = "true" ]; then
   echo "[utopia][init] Detected last reboot reason as factory-reset"
   if [ -e "/usr/bin/onboarding_log" ]; then
       /usr/bin/onboarding_log "[utopia][init] Detected last reboot reason as factory-reset"
   fi

   if [ "$MODEL_NUM" = "TG3482G" ]; then
	rm -f /nvram/mesh_enabled
   fi
   syscfg set X_RDKCENTRAL-COM_LastRebootReason "factory-reset"
   syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
   if [ "$MODEL_NUM" = "CGM4331COM" ] || [ "$MODEL_NUM" = "CGM4140COM" ] || [ "$MODEL_NUM" = "CGA4332COM" ] || [  "$MODEL_NUM" = "TG4482A" ] || [ "$MODEL_NUM" = "INTEL_PUMA" ]; then
   # Enable AUTOWAN by default for XB7, change is made here so that it will take effect only after FR
      syscfg set selected_wan_mode "0"
   fi
elif [ "$PUNIT_RESET_DURATION" -gt "0" ]; then
   echo "[utopia][init] Detected last reboot reason as pin-reset"
   if [ -e "/usr/bin/onboarding_log" ]; then
       /usr/bin/onboarding_log "[utopia][init] Last reboot reason set as pin-reset"
   fi
   syscfg set X_RDKCENTRAL-COM_LastRebootReason "pin-reset"
   syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
elif [ -f /nvram/restore_reboot ]; then
     if [ -e "/usr/bin/onboarding_log" ]; then
             /usr/bin/onboarding_log "[utopia][init] Last reboot reason set as restore-reboot"
     fi
     syscfg set X_RDKCENTRAL-COM_LastRebootReason "restore-reboot"
     syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
     
     if [ "$BOX_TYPE" == "TCCBR" ];then
         if [ -f /nvram/bbhm_cur_cfg.xml-temp ]; then
              ##Work around: TCCBR-4087 Restored saved configuration is not restoring wan Static IP.
              ##after untar the new bbhm current config is overrriden/corrupted at times.
              ##Hence we are storing a backup and replacing it to current config upon such cases
              a=`md5sum /nvram/bbhm_cur_cfg.xml-temp`
              a=$(echo $a | cut -f 1 -d " ")
              b=`md5sum /nvram/bbhm_cur_cfg.xml`  
              b=$(echo $b | cut -f 1 -d " ")
              if [[ $a != $b ]]; then
                  cp /nvram/bbhm_cur_cfg.xml-temp /nvram/bbhm_cur_cfg.xml
              fi
              rm -f /nvram/bbhm_cur_cfg.xml-temp
         fi
     fi
     rm -f /nvram/restore_reboot
     rm -f /nvram/syscfg.db.prev
elif [ "$COLD_REBOOT" == "true" ]; then
     if [ -e "/usr/bin/onboarding_log" ]; then
         /usr/bin/onboarding_log "[utopia][init] Last reboot reason set as HW or Power-On Reset"
     fi
     syscfg set X_RDKCENTRAL-COM_LastRebootReason "HW or Power-On Reset"
     syscfg set X_RDKCENTRAL-COM_LastRebootCounter "1"
else
   rebootReason=`syscfg get X_RDKCENTRAL-COM_LastRebootReason` 
   reboot_counter=`syscfg get X_RDKCENTRAL-COM_LastRebootCounter`
   echo "[utopia][init] X_RDKCENTRAL-COM_LastRebootReason ($rebootReason)"
   #TCCBR-4220 To handle wrong reboot reason
   if ( [ "$rebootReason" = "factory-reset" ] ) || ( [ "$BOX_TYPE" = "TCCBR" ] && [ "$reboot_counter" -eq "0" ] ); then
      echo "[utopia][init] Setting last reboot reason as unknown"
      syscfg set X_RDKCENTRAL-COM_LastRebootReason "unknown"
      if [ -e "/usr/bin/onboarding_log" ]; then
          /usr/bin/onboarding_log "[utopia][init] Last reboot reason set as unknown"
      fi
   fi
fi
syscfg commit

#RDKB-24155 - TLVData.bin should not be used in EWAN mode
eth_wan_enable=`syscfg get eth_wan_enabled`
if [ "$eth_wan_enable" = "true" ] && [ -f $TR69TLVFILE ]; then
  rm -f $TR69TLVFILE
  #RDKB-30774 - Remove existing ACS server URL and passwords, when migrating from DOCSIS to EWAN
  #Default ACS URL from partners_defaults would be populated when booting in EWAN mode for the first time
  rm -rf $TR69KEYS
  sed -i '/eRT.com.cisco.spvtg.ccsp.tr069pa.Device.ManagementServer.URL.Value/d' $PSM_CUR_XML_CONFIG_FILE_NAME
fi
      
echo "[utopia][init] completed creating utopia_inited flag"
touch /tmp/utopia_inited
