#!/bin/sh

source /fss/gw/etc/utopia/service.d/log_env_var.sh

if [ ! -d "$LOG_PATH" ]; then
    mkdir $LOG_PATH
fi


exec 3>&1 4>&2 >>$CONSOLEFILE 2>&1

