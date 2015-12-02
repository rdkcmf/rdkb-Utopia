#!/bin/sh
# $1 should be 0 for disable and 1 for enable

ENABLE=$1

syscfg set mgmt_wan_telnetaccess $ENABLE
syscfg commit
sysevent set firewall-restart
