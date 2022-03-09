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

source /etc/utopia/service.d/ulog_functions.sh
source /etc/utopia/service.d/event_handler_functions.sh

SERVICE_NAME="bootstrap_dns"

# This service finds the dns forwarder for the system
# and registers us with it. If the dns forwarder is on
# a remote node,, then we register with that remote

#-----------------------------------------------------------------
remove_old_registrations ()
{
   DNS_NAME=$1
   REMOTE_SYSEVENT=$2

   iterator=`sysevent -i "$REMOTE_SYSEVENT" getiterator dns_a`
   while [ "4294967295" != "$iterator" ]
   do
      value=`sysevent -i "$REMOTE_SYSEVENT" getunique dns_a "$iterator"`
      if [ -n "$value" ] ; then
         name=`echo "$value" | cut -d, -f1`
         if [ "$name" = "$DNS_NAME" ] ; then
            sysevent -i "$REMOTE_SYSEVENT" delunique dns_a "$iterator"
         fi
      fi
      iterator=`sysevent -i "$REMOTE_SYSEVENT" getiterator dns_a "$iterator"`
   done

   iterator=`sysevent -i "$REMOTE_SYSEVENT" getiterator dns_aaaa`
   while [ "4294967295" != "$iterator" ]
   do
      value=`sysevent -i "$REMOTE_SYSEVENT" getunique dns_aaaa "$iterator"`
      if [ -n "$value" ] ; then
         name=`echo "$value" | cut -d, -f1`
         if [ "$name" = "$DNS_NAME" ] ; then
            sysevent -i "$REMOTE_SYSEVENT" delunique dns_aaaa "$iterator"
         fi
      fi
      iterator=`sysevent -i "$REMOTE_SYSEVENT" getiterator dns_aaaa "$iterator"`
   done

}

#-----------------------------------------------------------------
remove_local_registrations ()
{
   DNS_NAME=$1

   iterator=`sysevent getiterator dns_a`
   while [ "4294967295" != "$iterator" ]
   do
      value=`sysevent getunique dns_a "$iterator"`
      if [ -n "$value" ] ; then
         name=`echo "$value" | cut -d, -f1`
         if [ "$name" = "$DNS_NAME" ] ; then
            sysevent delunique dns_a "$iterator"
         fi
      fi
      iterator=`sysevent getiterator dns_a "$iterator"`
   done

   iterator=`sysevent getiterator dns_aaaa`
   while [ "4294967295" != "$iterator" ]
   do
      value=`sysevent getunique dns_aaaa "$iterator"`
      if [ -n "$value" ] ; then
         name=`echo "$value" | cut -d, -f1`
         if [ "$name" = "$DNS_NAME" ] ; then
            sysevent delunique dns_aaaa "$iterator"
         fi
      fi
      iterator=`sysevent getiterator dns_aaaa "$iterator"`
   done

}


#-----------------------------------------------------------------
service_init ()
{
   FOO=`utctx_cmd get lan_ifname local_syseventd bootstrap_syseventd`
   eval "$FOO"
   SCRIPT_FILE="/tmp/add_dns_entry.sh"
}

#-----------------------------------------------------------------

service_start ()
{

   LAN_STATUS=`sysevent get lan-status`
   BRIDGE_STATUS=`sysevent get bridge-status`
   if [ "started" != "$LAN_STATUS" -a "started" != "$BRIDGE_STATUS" ] ; then
      ulog system status "$PID ${SERVICE_NAME} start requested when lan is down. Ignoring"
      return 0
   fi

   # Register sysevent daemon with DNS

   if [ -n "$SYSCFG_bootstrap_syseventd" -a -n "$SYSCFG_local_syseventd" ]  ; then
      if [ "$SYSCFG_local_syseventd" = "$SYSCFG_bootstrap_syseventd" ] ; then
         # this is the syseventd on the platform containing dns forwarder
         # the lan must be up
         SYSEVENT_IP=`/sbin/ip addr show dev "$SYSCFG_lan_ifname"  | grep "inet " | awk '{split($2,foo, "/"); print(foo[1]);}'`
         LAST_OCTET=`echo "$SYSEVENT_IP" | cut -d. -f4`


         # register ipv4 and ipv6 addresses
         if [ -n "$LAST_OCTET" ] ; then
            UNIQ=`sysevent setunique dns_a "$SYSCFG_local_syseventd,$LAST_OCTET"`
            ulog system status "$PID Registered $SYSCFG_local_syseventd as ipv4 bootstrap syseventd"
         fi
         SYSEVENT_IP=`/sbin/ip -6 addr show dev "$SYSCFG_lan_ifname"  | grep "inet6 " | grep "scope link" | awk '{split($2,foo, "/"); print(foo[1]);}'`
         if [ -n "$SYSEVENT_IP" ] ; then
            UNIQ=`sysevent setunique dns_aaaa "$SYSCFG_local_syseventd,$SYSEVENT_IP"`
            ulog system status "$PID Registered $SYSCFG_local_syseventd as ipv6 bootstrap syseventd"
         fi
         sysevent set dns-restart
         sysevent set ${SERVICE_NAME}-status started
      else
         # every other syseventd
         sysevent set ${SERVICE_NAME}-status starting
         echo "#!/bin/sh" > $SCRIPT_FILE
         echo "source /etc/utopia/service.d/ulog_functions.sh" >> $SCRIPT_FILE
         echo "TEST=\`nslookup $SYSCFG_bootstrap_syseventd\`" >> $SCRIPT_FILE
         echo "until [ \"\$?\" = \"0\" ]" >> $SCRIPT_FILE
         echo "do" >> $SCRIPT_FILE
         echo "   sleep 5" >> $SCRIPT_FILE
         echo "   TEST=\`nslookup $SYSCFG_bootstrap_syseventd\`" >> $SCRIPT_FILE
         echo "done" >> $SCRIPT_FILE
         echo "SYSCFG_lan_ifname=\`syscfg get lan_ifname\`" >> $SCRIPT_FILE
         echo "SYSEVENT_IP=\`/sbin/ip addr show dev \$SYSCFG_lan_ifname  | grep \"inet \" | awk '{split(\$2,foo, \"/\"); print(foo[1]);}'\`" >> $SCRIPT_FILE
         echo "LAST_OCTET=\`echo \$SYSEVENT_IP | cut -d. -f4\`" >> $SCRIPT_FILE

         # remove any old registrations 

         echo "iterator=\`sysevent -i $SYSCFG_bootstrap_syseventd getiterator dns_a\`" >> $SCRIPT_FILE
         echo "while [ \"4294967295\" != \"\$iterator\" ]" >> $SCRIPT_FILE
         echo "do" >> $SCRIPT_FILE 
         echo "   value=\`sysevent -i $SYSCFG_bootstrap_syseventd getunique dns_a \$iterator\`" >> $SCRIPT_FILE
         echo "   if [ -n \"\$value\" ] ; then" >> $SCRIPT_FILE
         echo "      name=\`echo \$value | cut -d, -f1\`" >> $SCRIPT_FILE
         echo "      if [ \"\$name\" = \"$SYSCFG_local_syseventd\" ] ; then" >> $SCRIPT_FILE
         echo "         sysevent -i $SYSCFG_bootstrap_syseventd delunique dns_a \$iterator" >> $SCRIPT_FILE
         echo "      fi" >> $SCRIPT_FILE
         echo "   fi" >> $SCRIPT_FILE
         echo "   iterator=\`sysevent -i $SYSCFG_bootstrap_syseventd getiterator dns_a \$iterator\`" >> $SCRIPT_FILE
         echo "done" >> $SCRIPT_FILE

         echo "iterator=\`sysevent -i $SYSCFG_bootstrap_syseventd getiterator dns_aaaa\`" >> $SCRIPT_FILE
         echo "while [ \"4294967295\" != \"\$iterator\" ]" >> $SCRIPT_FILE
         echo "do" >> $SCRIPT_FILE
         echo "   value=\`sysevent -i $SYSCFG_bootstrap_syseventd getunique dns_aaaa \$iterator\`" >> $SCRIPT_FILE
         echo "   if [ -n \"\$value\" ] ; then" >> $SCRIPT_FILE
         echo "      name=\`echo \$value | cut -d, -f1\`" >> $SCRIPT_FILE
         echo "      if [ \"\$name\" = \"$SYSCFG_local_syseventd\" ] ; then" >> $SCRIPT_FILE
         echo "         sysevent -i $SYSCFG_bootstrap_syseventd delunique dns_aaaa \$iterator" >> $SCRIPT_FILE
         echo "      fi" >> $SCRIPT_FILE
         echo "   fi" >> $SCRIPT_FILE
         echo "   iterator=\`sysevent -i $SYSCFG_bootstrap_syseventd getiterator dns_aaaa \$iterator\`" >> $SCRIPT_FILE
         echo "done" >> $SCRIPT_FILE

         echo "if [ -n \"\$LAST_OCTET\" ] ; then" >> $SCRIPT_FILE
         echo "   UNIQ=\`sysevent -i $SYSCFG_bootstrap_syseventd setunique dns_a \"$SYSCFG_local_syseventd,\$LAST_OCTET\"\`" >> $SCRIPT_FILE
         echo "   ulog system status \"$PID Registered $SYSCFG_local_syseventd as ipv4 secondary syseventd\"" >> $SCRIPT_FILE
         echo "fi" >> $SCRIPT_FILE
         echo "SYSEVENT_IP=\`/sbin/ip -6 addr show dev \$SYSCFG_lan_ifname  | grep \"inet6 \" | grep \"scope link\" | awk '{split(\$2,foo, \"/\"); print(foo[1]);}'\`" >> $SCRIPT_FILE
         echo "if [ -n \"\$SYSEVENT_IP\" ] ; then" >> $SCRIPT_FILE

         echo "   UNIQ=\`sysevent setunique -i $SYSCFG_bootstrap_syseventd dns_aaaa \"$SYSCFG_local_syseventd,\$SYSEVENT_IP\"\`" >> $SCRIPT_FILE
         echo "   ulog system status \"$PID Registered $SYSCFG_local_syseventd as ipv6 secondary syseventd\"" >> $SCRIPT_FILE
         echo "fi" >> $SCRIPT_FILE
         echo "sysevent -i $SYSCFG_bootstrap_syseventd set dns-restart" >> $SCRIPT_FILE
         echo "sysevent set ${SERVICE_NAME}-status started" >> $SCRIPT_FILE

         chmod 700 $SCRIPT_FILE
         $SCRIPT_FILE &
      fi
   fi
}

service_stop ()
{
   if [ "$SYSCFG_local_syseventd" = "$SYSCFG_bootstrap_syseventd" ] ; then
      remove_local_registrations "$SYSCFG_local_syseventd"
      sysevent set dns-restart
   else
      remove_old_registrations "$SYSCFG_local_syseventd" "$SYSCFG_bootstrap_syseventd"
      sysevent set -i "$SYSCFG_bootstrap_syseventd" dns-restart
   fi
   sysevent set ${SERVICE_NAME}-status stopped
}

service_init

case "$1" in
   "${SERVICE_NAME}-start")
      service_start
      ;;
   "${SERVICE_NAME}-stop")
      service_stop
      ;;
   "${SERVICE_NAME}-restart")
      service_stop
      service__start
      ;;
   lan-status)
      STATUS=`sysevent get lan-status`
      if [ "stopped" = "$STATUS" -o "error" = "$STATUS" ] ; then
         service_stop
      elif [ "started" = "$STATUS" ] ; then
         service_start
      fi
      ;;
   bridge-status)
      STATUS=`sysevent get bridge-status`
      if [ "stopped" = "$STATUS" -o "error" = "$STATUS" ] ; then
         service_stop
      elif [ "started" = "$STATUS" ] ; then
         service_start
      fi
      ;;
   remote_syseventd)
      STATUS=`sysevent get remote_syseventd`
      if [ "stopped" = "$STATUS" ] ; then
         service_start
      fi
      ;;
   *)
      echo "Usage: $SERVICE_NAME [start|stop|restart|lan-status]" >&2
      exit 3
      ;;
esac
