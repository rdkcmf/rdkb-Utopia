#!/bin/sh

# currently, rebooting Cavium requires rebooting Topanga (provisioning file)
if [ "$1" == "factorydefault" ]; then
    sysevent set gwreset factorydefault
else
    sysevent set gwreset reboot
fi

