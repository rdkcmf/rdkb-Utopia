#!/bin/sh

if [ ! -d "/var/tmp/logs" ]; then
    mkdir /var/tmp/logs
fi

CONSOLEFILE="/var/tmp/logs/ArmConsolelog.txt.0"
exec 3>&1 4>&2 >>$CONSOLEFILE 2>&1

