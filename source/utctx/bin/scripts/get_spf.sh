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

if [ ! "$1" ] ; then
    echo "Usage: get_spf.sh <SPF number>"
    exit
fi

# Get the port foward we're after
SPF="SinglePortForward_$1"

do_utctx_get() # Accepts 1 parameter - the utctx_cmd argument list
{
    SYSCFG_FAILED='false'

    eval "`./utctx_cmd get "$1"`"

    if [ $SYSCFG_FAILED = 'true' ] ; then
        echo "Call failed"
        exit
    fi
}

# Get the namespace value for the SinglePortForward
do_utctx_get "$SPF"

eval NS='$'SYSCFG_"$SPF"
ARGS="\
$NS::enabled \
$NS::name \
$NS::protocol \
$NS::external_port \
$NS::internal_port \
$NS::to_ip"

# Do a batch get for the SinglePortForward
do_utctx_get "$ARGS"

#Display results
echo "$SPF:"
eval echo name = '$'SYSCFG_"${NS}"_name
eval echo enabled = '$'SYSCFG_"${NS}"_enabled
eval echo protocol = '$'SYSCFG_"${NS}"_protocol
eval echo external_port = '$'SYSCFG_"${NS}"_external_port
eval echo internal_port = '$'SYSCFG_"${NS}"_internal_port
eval echo to_ip = '$'SYSCFG_"${NS}"_to_ip
