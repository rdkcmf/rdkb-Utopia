#!/bin/bash
source /etc/utopia/service.d/service_wan/ppp_helpers.sh
prepare_pppd_ip_pre_up_script
prepare_pppd_ip_up_script
prepare_pppd_ipv6_up_script
prepare_pppd_ip_down_script
prepare_pppd_ipv6_down_script
prepare_pppd_options
prepare_pppd_secrets
PPP_CONFIG_FILE=/etc/ppp/pppoe.conf

echo -n > $PPP_CONFIG_FILE
VLAN_ID=`syscfg get Vlan_1_ID`
echo "ETH='eth0.${VLAN_ID}'" >> $PPP_CONFIG_FILE
CLIENT=`syscfg get wan_proto_username`
IPV6CP=`syscfg get IPV6CPEn`
IPCP=`syscfg get IPCPEn`
PPP_IDLE_TIME=`syscfg get ppp_idle_time`
MAXMRUSIZE=`syscfg get MaxMRUSize`
echo "MRU=$MAXMRUSIZE" >> $PPP_CONFIG_FILE
echo "USER=$CLIENT" >>$PPP_CONFIG_FILE
echo "IFNAME='erouter0'" >>$PPP_CONFIG_FILE
echo "DEMAND=no" >> $PPP_CONFIG_FILE
echo "DNSTYPE=SERVER" >> $PPP_CONFIG_FILE
echo "PEERDNS=yes" >> $PPP_CONFIG_FILE
echo "DNS1=" >> $PPP_CONFIG_FILE
echo "DNS2=" >> $PPP_CONFIG_FILE
echo "DEFAULTROUTE=yes" >> $PPP_CONFIG_FILE
echo "CONNECT_TIMEOUT=0" >> $PPP_CONFIG_FILE
echo "CONNECT_POLL=2" >> $PPP_CONFIG_FILE
CONCENTRATOR=`syscfg get wan_proto_acname`
echo "ACNAME=$CONCENTRATOR" >> $PPP_CONFIG_FILE
SERVICE=`syscfg get wan_proto_servicename`
echo "SERVICENAME=$SERVICE" >> $PPP_CONFIG_FILE
echo 'PING="."' >> $PPP_CONFIG_FILE
echo "CF_BASE=`basename $CONFIG`" >> $PPP_CONFIG_FILE
echo 'PIDFILE="/var/run/$CF_BASE-pppoe.pid"' >> $PPP_CONFIG_FILE
echo "SYNCHRONOUS=no" >> $PPP_CONFIG_FILE
echo "CLAMPMSS=1412" >> $PPP_CONFIG_FILE
echo "LCP_INTERVAL=20" >> $PPP_CONFIG_FILE
echo "LCP_FAILURE=3" >> $PPP_CONFIG_FILE
echo "PPPOE_TIMEOUT=80" >> $PPP_CONFIG_FILE
echo "FIREWALL=NONE" >> $PPP_CONFIG_FILE
echo 'LINUX_PLUGIN="/usr/lib/pppd/2.4.7/rp-pppoe.so"' >> $PPP_CONFIG_FILE
echo 'PPPOE_EXTRA=""' >> $PPP_CONFIG_FILE
echo 'PPPD_EXTRA=""' >> $PPP_CONFIG_FILE
