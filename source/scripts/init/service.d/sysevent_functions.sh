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

# sysevent_np
# This function faciliate remote sysevent to NP's linked local IPv6 address
# should only be called from AP
sysevent_np() {
   iRouter_ll_ipaddr=`sysevent get iRouter_ll_ipaddr`
   echo sysevent --ip "${iRouter_ll_ipaddr//'%'/'%%'}" --port 36367 "$@"
   sysevent --ip "${iRouter_ll_ipaddr//'%'/'%%'}" --port 36367 "$@"
}

# sysevent_ap
# This function faciliate remote sysevent to AP's linked local IPv6 address
# should only be called from NP
sysevent_ap() {
   eMG_ll_ipaddr=`sysevent get eMG_ll_ipaddr`
   echo sysevent --ip "${eMG_ll_ipaddr//'%'/'%%'}" --port 36367 "$@"
   sysevent --ip "${eMG_ll_ipaddr//'%'/'%%'}" --port 36367 "$@"
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

   asyncid=`sysevent async "$EVENT_NAME" "$HANDLER"`
   if [ -n "$FLAG" ] ; then
      sysevent setoptions "$EVENT_NAME" "$FLAG"
   fi
   sysevent set "${SERVICE_NAME}"_"${EVENT_NAME}"_asyncid "$asyncid"
}

# unregister_sysevent_handler
# This function unregisters a sysevent handler for a service using
# previous saved async id
unregister_sysevent_handler()
{
   SERVICE_NAME=$1
   EVENT_NAME=$2

   asyncid="`sysevent get "${SERVICE_NAME}"_"${EVENT_NAME}"_asyncid`"
   if [ -z "$asyncid" ] ; then              #using -z where -n counts new line char too RDKB-43982
      sysevent rm_async "$asyncid"
      sysevent set "${SERVICE_NAME}"_"${EVENT_NAME}"_asyncid
   fi
}

