#!/bin/sh

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
    nvram unset $i
done

# Cleanup all known, unwanted (!!) nvram vars
if [ -f $NVKNOWN_LIST_FILE ]; then
    for i in `cat $NVKNOWN_LIST_FILE`
    do
        NVVAL=`nvram get $i`
        if [ "set" != "set$NVVAL" ]; then
            NVDIRTY=1
            # echo "cmd - nvram unset $i"
            nvram unset $i
        fi
    done
fi

# Commit nvram on any unset
if [ "1" = $NVDIRTY ]; then
    echo "[utopia][init]Please wait, vendor nvram is being cleaned up"
    nvram commit
fi

# rm -f $NVTEMP_FILE
