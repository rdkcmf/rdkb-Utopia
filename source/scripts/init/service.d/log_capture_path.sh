#!/bin/sh

source /etc/utopia/service.d/log_env_var.sh
source /etc/log_timestamp.sh

if [ ! -d "$LOG_PATH" ]; then
    mkdir $LOG_PATH
fi

exec 3>&1 4>&2 >>$CONSOLEFILE 2>&1

