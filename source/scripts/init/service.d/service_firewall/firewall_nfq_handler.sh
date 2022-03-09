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


STARTED_FLG=`sysevent get parcon_nfq_status`

if [ "$STARTED_FLG" = "started" ]; then
    exit 0;
fi 

sysevent set parcon_nfq_status started

DNS_RESPONSE_QUEUE_START=`sysevent get parcon_dnsr_start`
DNS_RESPONSE_QUEUE_END=`sysevent get parcon_dnsr_end`
HTTP_GET_QUEUE_START=`sysevent get parcon_httpget_start`
HTTP_GET_QUEUE_END=`sysevent get parcon_httpget_end`
DNSV6_RESPONSE_QUEUE_START=`sysevent get parcon_dnsv6r_start`
DNSV6_RESPONSE_QUEUE_END=`sysevent get parcon_dnsv6r_end`
HTTPV6_GET_QUEUE_START=`sysevent get parcon_httpv6get_start`
HTTPV6_GET_QUEUE_END=`sysevent get parcon_httpv6get_end`

for i in $(seq "$DNS_RESPONSE_QUEUE_START" "$DNS_RESPONSE_QUEUE_END"); do
    echo "starting nfq_handler for dns response $i"
    ( ( nfq_handler "$i" dns_response & ) & )
    sleep 1
done

for i in $(seq "$DNSV6_RESPONSE_QUEUE_START" "$DNSV6_RESPONSE_QUEUE_END"); do
    echo "starting nfq_handler for dns response $i"
    ( ( nfq_handler "$i" dnsv6_response & ) & )
    sleep 1
done

BRLAN0_MAC=`ifconfig brlan0 | grep HWaddr | awk '{print $5}'`

for i in $(seq "$HTTP_GET_QUEUE_START" "$HTTP_GET_QUEUE_END"); do
    echo "starting nfq_handler for http get $i"
    ( ( nfq_handler "$i" http_get $BRLAN0_MAC & ) & )
    sleep 1
done

for i in $(seq "$HTTPV6_GET_QUEUE_START" "$HTTPV6_GET_QUEUE_END"); do
    echo "starting nfq_handler for http get $i"
    ( ( nfq_handler "$i" httpv6_get $BRLAN0_MAC & ) & )
    sleep 1
done
