#!/bin/sh

is_private() {
   ip=$1
   private_lan_ip=0

   if [ "192.168." = ${ip:0:8} ]; then
      private_lan_ip=1
   elif [ "10." = ${ip:0:3} ] ; then
      private_lan_ip=1
   elif [ "172." = ${ip:0:4} ] ; then
      second_octet=${ip#*.}
      second_octet=${second_octet%%.*}
      second_octet=$((second_octet & 0xF0))
      if [ $second_octet = 16 ] ; then
         private_lan_ip=1
      fi
   fi

   return $private_lan_ip
} 
