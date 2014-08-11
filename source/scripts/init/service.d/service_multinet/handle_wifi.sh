#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2013 by Cisco Systems, Inc. All Rights Reserved.
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
# ENTRY
#------------------------------------------------------------------

TYPE=WiFi


source /etc/utopia/service.d/ut_plat.sh



#service_init
case "$1" in
#  Synchronous calls
    #Args: netid, members
    create)
        echo ${TYPE}_READY=\"$3\"
        ;;
    destroy)
        service_stop
        ;;
    #Args: netid, netvid, members
    addVlan|delVlan)
     
        ${SW_HANDLER} $1 $2 $3 "$4"
    ;;
      
#  Sysevent calls
    *)
        exit 3
        ;;
esac