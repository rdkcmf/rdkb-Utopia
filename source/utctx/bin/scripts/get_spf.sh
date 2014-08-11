#!/bin/sh

if [ ! $1 ] ; then
    echo "Usage: get_spf.sh <SPF number>"
    exit
fi

# Get the port foward we're after
SPF="SinglePortForward_$1"

do_utctx_get() # Accepts 1 parameter - the utctx_cmd argument list
{
    SYSCFG_FAILED='false'

    eval `./utctx_cmd get $1`

    if [ $SYSCFG_FAILED = 'true' ] ; then
        echo "Call failed"
        exit
    fi
}

# Get the namespace value for the SinglePortForward
do_utctx_get "$SPF"

eval NS='$'SYSCFG_$SPF
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
eval echo name = '$'SYSCFG_${NS}_name
eval echo enabled = '$'SYSCFG_${NS}_enabled
eval echo protocol = '$'SYSCFG_${NS}_protocol
eval echo external_port = '$'SYSCFG_${NS}_external_port
eval echo internal_port = '$'SYSCFG_${NS}_internal_port
eval echo to_ip = '$'SYSCFG_${NS}_to_ip