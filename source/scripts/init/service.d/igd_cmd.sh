# !/bin/sh

CCSP_CLI="ccsp_bus_client_tool"
CCSP_SUBSYS="eRT"
CCSP_TYPE="bool"
CCSP_DM="Device.UPnP.Device.UPnPIGD"

SELF_NAME="`basename $0`"


igd_start() {
  $CCSP_CLI $CCSP_SUBSYS setv $CCSP_DM $CCSP_TYPE true > /dev/null 2>&1
}

igd_stop() {
  $CCSP_CLI $CCSP_SUBSYS setv $CCSP_DM $CCSP_TYPE false > /dev/null 2>&1
}


case "$1" in
  start)
    igd_start;
    ;;  
  stop)
    igd_stop;
    ;;
  *)
    echo "Usage $SELF_NAME [start|stop]" >&2
    exit 3
    ;;
esac

