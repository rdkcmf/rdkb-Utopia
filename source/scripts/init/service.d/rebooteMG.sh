#!/bin/sh

eMG_ll_ipaddr=`sysevent get eMG_ll_ipaddr`

if [ "$1" == "factorydefault" ]; then
    sysevent --ip ${eMG_ll_ipaddr//'%'/'%%'} --port 36367 set gwreset factorydefault 
else
    sysevent --ip ${eMG_ll_ipaddr//'%'/'%%'} --port 36367 set gwreset reboot
fi

