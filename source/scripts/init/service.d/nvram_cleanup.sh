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

# This is called by bootup script to clean up vendor (broadcom)
# nvram settings

NVDIRTY=0
NVKNOWN_LIST_FILE=/etc/nvram.cleanup.lst
NVTEMP_FILE=/tmp/.nvram.cleanup.tmp

rm -f $NVTEMP_FILE

# Cleanup all wl, wps nvram vars
nvram show 2> /dev/null | grep "^wl_"       | cut -d'=' -f1  >  $NVTEMP_FILE
nvram show 2> /dev/null | grep "^wl[0|1]_"  | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^wl[0|1]\." | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^wsc_"      | cut -d'=' -f1  >> $NVTEMP_FILE

nvram show 2> /dev/null | grep "^storage_"    | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^ftp_share_"  | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^MS_scan_"    | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^LOG_"        | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^filter_"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^ddns_"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^ppp"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^l2tp"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^qos_"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^get_pa2g"     | cut -d'=' -f1  >> $NVTEMP_FILE
nvram show 2> /dev/null | grep "^get_pa5g"     | cut -d'=' -f1  >> $NVTEMP_FILE

for i in `cat $NVTEMP_FILE`
do
    NVDIRTY=1
    # echo "cmd - nvram unset $i"
    nvram unset "$i"
done

# Cleanup all known, unwanted (!!) nvram vars
if [ -f $NVKNOWN_LIST_FILE ]; then
    for i in `cat $NVKNOWN_LIST_FILE`
    do
        NVVAL=`nvram get "$i"`
        if [ "set" != "set$NVVAL" ]; then
            NVDIRTY=1
            # echo "cmd - nvram unset $i"
            nvram unset "$i"
        fi
    done
fi

# Commit nvram on any unset
if [ "1" = $NVDIRTY ]; then
    echo "[utopia][init]Please wait, vendor nvram is being cleaned up"
    nvram commit
fi

# rm -f $NVTEMP_FILE
