#!/bin/sh

eval `./utctx_cmd get dhcp_nameserver_1 dhcp_nameserver_2 dhcp_nameserver_3`

echo DNS1 = $SYSCFG_dhcp_nameserver_1
echo DNS2 = $SYSCFG_dhcp_nameserver_2
echo DNS3 = $SYSCFG_dhcp_nameserver_3
