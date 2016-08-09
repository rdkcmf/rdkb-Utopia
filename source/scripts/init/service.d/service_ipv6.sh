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


#------------------------------------------------------------------
# name of this service
# This name MUST correspond to the registration of this service.
# Usually the registration occurs in /etc/utopia/registration.d
# The registration code will refer to the path/name of this handler
# to be activated upon default events (and other events)
#------------------------------------------------------------------
SERVICE_NAME="ipv6"



#----------------------------------------------------------------------------------------
#                     Default Event Handlers
#
# Each service has three default events that it should handle
#    ${SERVICE_NAME}-start
#    ${SERVICE_NAME}-stop
#    ${SERVICE_NAME}-restart
#
# For each case:
#   - Clear the service's errinfo
#   - Set the service's status 
#   - Do the work
#   - Check the error code (check_err will set service's status and service's errinfo upon error)
#   - If no error then set the service's status
#----------------------------------------------------------------------------------------

case "$1" in
   ${SERVICE_NAME}-start)
      service_ipv6 start
      ;;
   ${SERVICE_NAME}-stop)
      service_ipv6 stop
      ;;
   ${SERVICE_NAME}-restart)
      service_ipv6 restart
      ;;
   #----------------------------------------------------------------------------------
   # Add other event entry points here
   #----------------------------------------------------------------------------------

   #currently, leverage this event to represent the enabled lan interfaces
   multinet-instances)
      if [ x`sysevent get ipv6_prefix` != x"" ]; then
          service_ipv6 restart
          service_routed route-unset
          service_routed route-set
      fi
      ;;
   multinet_1-status)
      multinet_status=`sysevent get multinet_1-status`
      if [ "ready" = "$multinet_status" ] ; then
          if [ x`sysevent get ipv6_prefix` != x"" ]; then
              service_ipv6 restart multinet
              if [ "$?" = "0" ] ; then
                  service_routed route-unset
                  service_routed route-set
              fi
          fi
      fi
      ;;
   erouter_topology-mode)
      if [ x`sysevent get ipv6_prefix` != x"" ]; then
          service_ipv6 restart
      fi
      ;;

   ipv6_prefix)
      #service_ipv6 restart
     if [ x`sysevent get ipv6_prefix` != x"" ]; then
         multinet_status=`sysevent get multinet_1-status`
         if [ "ready" = "$multinet_status" ] ; then
           service_ipv6 restart
         fi
     fi
     ;;

   *)
      echo "Usage: $SERVICE_NAME [ ${SERVICE_NAME}-start | ${SERVICE_NAME}-stop | ${SERVICE_NAME}-restart]" > /dev/console
      exit 3
      ;;
esac
