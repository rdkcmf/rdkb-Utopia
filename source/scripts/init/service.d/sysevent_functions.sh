#!/bin/sh

#------------------------------------------------------------------
# Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.
#
# This work is subject to U.S. and international copyright laws and
# treaties. No part of this work may be used, practiced, performed,
# copied, distributed, revised, modified, translated, abridged, condensed,
# expanded, collected, compiled, linked, recast, transformed or adapted
# without the prior written consent of Cisco Systems, Inc. Any use or
# exploitation of this work without authorization could subject the
# perpetrator to criminal and civil liability.
#------------------------------------------------------------------

# sysevent_np
# This function faciliate remote sysevent to NP's linked local IPv6 address
# should only be called from AP
sysevent_np() {
   iRouter_ll_ipaddr=`sysevent get iRouter_ll_ipaddr`
   echo sysevent --ip ${iRouter_ll_ipaddr//'%'/'%%'} --port 36367 $*
   sysevent --ip ${iRouter_ll_ipaddr//'%'/'%%'} --port 36367 $*
}

# sysevent_ap
# This function faciliate remote sysevent to AP's linked local IPv6 address
# should only be called from NP
sysevent_ap() {
   eMG_ll_ipaddr=`sysevent get eMG_ll_ipaddr`
   echo sysevent --ip ${eMG_ll_ipaddr//'%'/'%%'} --port 36367 $*
   sysevent --ip ${eMG_ll_ipaddr//'%'/'%%'} --port 36367 $*
}

# register_sysevent_handler
# This function registers a sysevent handler for a service and record
# the async id so it can be unregistered later
register_sysevent_handler()
{
   SERVICE_NAME=$1
   EVENT_NAME=$2
   HANDLER=$3
   FLAG=$4

   asyncid=`sysevent async $EVENT_NAME $HANDLER`
   if [ -n "$FLAG" ] ; then
      sysevent setoptions $EVENT_NAME $FLAG
   fi
   sysevent set ${SERVICE_NAME}_${EVENT_NAME}_asyncid "$asyncid"
}

# unregister_sysevent_handler
# This function unregisters a sysevent handler for a service using
# previous saved async id
unregister_sysevent_handler()
{
   SERVICE_NAME=$1
   EVENT_NAME=$2

   asyncid=`sysevent get ${SERVICE_NAME}_${EVENT_NAME}_asyncid`
   if [ -n "$asyncid" ] ; then
      sysevent rm_async $asyncid
      sysevent set ${SERVICE_NAME}_${EVENT_NAME}_asyncid
   fi
}

