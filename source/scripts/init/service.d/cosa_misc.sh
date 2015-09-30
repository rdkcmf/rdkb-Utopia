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


if [ "x"$1 = "xkill" ] || [ "x"$2 = "xkill" ]; then
    # this script will be invoked by ccspRecoveryManager, 
    # do not kill self.
    #killall ccspRecoveryManager
    sleep 3
    killall CcspTandDSsp
    #killall CcspDnsSsp
    killall CcspFuSsp
    killall CcspSsdSsp
    #killall CcspTr069PaSsp
    killall CcspRmSsp
    #killall CcspLmSsp
    killall Lm_hosts
fi

export LD_LIBRARY_PATH=$PWD:.:$PWD/lib:/usr/lib:$LD_LIBRARY_PATH
export DBUS_SYSTEM_BUS_ADDRESS=unix:path=/var/run/dbus/system_bus_socket

if [ -f /fss/gw/usr/ccsp/cp_subsys_ert ]; then
    Subsys="eRT."
else
    Subsys=""
fi

cd /fss/gw/usr/ccsp/pam
#double background to detach the script from the tty
((sh ./email_notification_monitor.sh 12 &) &)

cd ..

if [ -f ./cp_subsys_ert ]; then
#sleep 3
cd rm
    ./CcspRmSsp -subsys $Subsys
cd ../
fi

#cp ./cherokee/icons/yes.gif /usr/share/cherokee/icons
#cp ./cherokee/icons/add.gif /usr/share/cherokee/icons
#cp ./cherokee/icons/delete.gif /usr/share/cherokee/icons
#cherokee-worker -C ./cherokee/conf/cherokee.conf &

if [ "x"$1 = "xpam" ] || [ "x"$2 = "xpam" ]; then
  exit 0
fi

#cd ../avahi
#$PWD/avahi-daemon --file=$PWD/avahi-daemon.conf -D


# Tr069Pa, as well as SecureSoftwareDownload and FirmwareUpgrade
# if [ -f "/nvram/ccsp_tr069pa" ]; then

#    cd tr069pa
#    if [ "x"$Subsys = "x" ]; then
#        ./CcspTr069PaSsp
#    else
#        ./CcspTr069PaSsp -subsys $Subsys
#    fi
#    cd ..
    
    cd ssd
    sleep 1
    if [ "x"$Subsys = "x" ];then
        ./CcspSsdSsp
    else
        echo "./CcspSsdSsp -subsys $Subsys"
        ./CcspSsdSsp -subsys $Subsys
    fi
    cd ..

    cd fu
   sleep 1
    if [ "x"$Subsys = "x" ];then
        ./CcspFuSsp
    else
        echo "./CcspFuSsp -subsys $Subsys"
        ./CcspFuSsp -subsys $Subsys
    fi
    cd ..

    # add firewall rule to allow incoming packet for port 7547
    sysevent setunique GeneralPurposeFirewallRule " -A INPUT -i erouter0 -p tcp --dport=7547 -j ACCEPT "
    sysevent set firewall-restart
#fi


cd tad
#delay TaD in order to reduce CPU overload and make PAM ready early
sleep 3
if [ "x"$Subsys = "x" ];then
    ./CcspTandDSsp
else
    ./CcspTandDSsp -subsys $Subsys
fi
cd ..

sleep 1
if [ "x"$Subsys = "x" ];then
    ./ccspRecoveryManager &
else
    echo "./ccspRecoveryManager -subsys $Subsys &"
    ./ccspRecoveryManager -subsys $Subsys &
fi

#cd lm
#Lm need initialization after others running
#Sleep 120 is a temporary method
sleep 3
Lm_hosts &
#if [ "x"$Subsys = "x" ];then
#    ./CcspLmSsp
#else
#   echo "./CcspLmSsp -subsys $Subsys"
#   ./CcspLmSsp -subsys $Subsys
#fi
#cd ..

#start ntp time sync
if [ x"1" = x`syscfg get ntp_enabled` ] ; then
    dmcli eRT setv Device.Time.Enable bool 1 &
fi

