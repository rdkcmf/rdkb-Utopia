#!/bin/sh

#Configure LAN bridges for XB6 using vlan_util (which calls vlan_hal)
VLAN_UTIL="vlan_util"
QWCFG_TEST="qwcfg_test"
QCSAPI_PCIE="qcsapi_pcie"
IP="ip"
IFCONFIG="ifconfig"
NCPU_EXEC="ncpu_exec"
SYSEVENT="sysevent"
KILLALL="killall"
LOCKFILE=/var/tmp/vlan_util.pid

#Initial state variable values (will get reset later)
IF_LIST=""
CURRENT_IF_LIST=""
BRIDGE_MODE=0

#Debug settings to override binaries we call
#export LD_LIBRARY_PATH=/nvram/
#VLAN_UTIL="echo vlan_util"
#QWCFG_TEST="echo qwcfg_test"
#QCSAPI_PCIE="echo qcsapi_pcie"
#IP="echo ip"
#IFCONFIG="echo ifconfig"
#NCPU_EXEC="echo ncpu_exec"
#SYSEVENT="echo sysevent"
#KILLALL="echo killall"

#Wifi information
#Set this to 1 to set SSID name in this script to default
USE_DEFAULT_SSID=1
BASE_WIFI_IF="host0"
#Base vlan ID to use internally in QTN
QTN_VLAN_BASE="2000"
QTN_STATE="/var/run/.qtn_vlan_enabled"
UNIQUE_ID=`ncpu_exec -ep "cat /sys/class/net/wan0/address"|egrep -e ".*:.*:.*:.*:.*:.*"|cut -d ":" -f 3-6`
UNIQUE_ID=`echo "${UNIQUE_ID//:}"`
#GRE tunnel information
DEFAULT_GRE_TUNNEL="gretap0"
#Dummy MAC address to assign to GRE tunnels so we can add them to bridges
DUMMY_MAC="02:03:DE:AD:BE:EF"

#Map athX style index X to Quantenna index
#Takes as input parameter interface name (such as ath0)
map_ath_to_qtn(){
    ATH_INDEX=`echo "$1"|sed 's/\(ath\)\([0-9]*\)/\2/'`
    if [ `expr $ATH_INDEX % 2` -eq 0 ]
    then
        #2.4GHz
        VAP_INDEX=`expr $ATH_INDEX / 2`
        QTN_INDEX=`expr $VAP_INDEX + 16`
        RADIO=2
    else
        #5Ghz
        QTN_INDEX=`expr $ATH_INDEX / 2`
        VAP_INDEX=$QTN_INDEX
        RADIO=0
    fi
    #Generate a unique internal vlan ID for QTN to use
    QTN_VLAN=`expr $ATH_INDEX + $QTN_VLAN_BASE`
    VAP_NAME="wifi${RADIO}_${VAP_INDEX}"
    
    #Print results to see if they are correct
}

wait_qtn(){
    while [ ! -e /sys/class/net/$BASE_WIFI_IF ] ; do
        echo "Waiting for interface $BASE_WIFI_IF..."
        sleep 1
    done
}

#Generate and set a default SSID for wifi AP
#Assumes you have already set the variables $AP_NAME and $VAP_NAME
qtn_set_default_ssid(){
    if [ $USE_DEFAULT_SSID -ne 0 -a "$BRIDGE_NAME" != "brlan0" ]
    then
        
        SSID_CATEGORY=""
        #Captive portal will take care of setting the private network ssid and passphrase
        #if [ $ATH_INDEX -eq 0 -o $ATH_INDEX -eq 1 ]
        #then
        #	SSID_CATEGORY="Private"
        if [ $ATH_INDEX -eq 2 -o $ATH_INDEX -eq 3 ]
        then
            SSID_CATEGORY="XHS"
            
            #ADD MAC ADDRESS here
            SSID="${SSID_CATEGORY}-$UNIQUE_ID"
            #THEN SET SSID XHS-MAC (same for both 2.4 and 5G)
            $QWCFG_TEST push $QTN_INDEX ssid "$SSID"
        elif [ $ATH_INDEX -eq 4 -o $ATH_INDEX -eq 5 ]
        then
            SSID_CATEGORY="xfinitywifi"
            SSID="${SSID_CATEGORY}"
            
            #THEN SET SSID "xfinitywifi"   (same for both 2.4 and 5G)
            $QWCFG_TEST push $QTN_INDEX ssid "$SSID"
        elif [ $ATH_INDEX -eq 6 -o $ATH_INDEX -eq 7 ]
        then
            echo "SETTING LOST & FOUND CONF"
            echo "SETTING LOST & FOUND CONF"
            $QWCFG_TEST set $QTN_INDEX ssid_bcast 1
            $QWCFG_TEST set $QTN_INDEX ssid D375C1D9F8B041E2A1995B784064977B
        fi
        
        #RADIO_DESC=""
        #if [ "$RADIO" -eq "0" ]
        #then
        #	RADIO_DESC="5GHz"
        #elif [ "$RADIO" -eq "2" ]
        #then
        #RADIO_DESC="2.4GHz"
        #fi
        ##SSID="${SSID_PREFIX} ${SSID_CATEGORY} ${RADIO_DESC} $UNIQUE_ID"
        #NEXT 2 lines - CANNOT DO THIS HERE. THIS WILL ALSO SET Private SSID
        ##SSID="${SSID_CATEGORY}-$UNIQUE_ID"
        ##$QWCFG_TEST push $QTN_INDEX ssid "$SSID"
    fi
}

#Setup QTN instance
#Syntax: setup_qtn [start|stop] athX
#Parameters: [start|stop] name(as in athX),
setup_qtn(){
    QTN_MODE="$1"
    AP_NAME="$2"
    
    qtn_init
    
    #First we need to map index to QTN internal indexand vlan ID
    map_ath_to_qtn $AP_NAME
    
    if [ "$QTN_MODE" = "start" ]
    then
        #Create VAP
        $QWCFG_TEST push $QTN_INDEX vap_emerged 1
        
        #Bind internal interface to vlan we can use
        $QWCFG_TEST set $QTN_INDEX bind_vlan $QTN_VLAN
        
        #Configure vlan on pcie0 interface
        $QWCFG_TEST set $QTN_INDEX trunk_vlan $QTN_VLAN
        
        #Create vlan atop base wifi interface
        $IP link add $AP_NAME link $BASE_WIFI_IF type vlan id $QTN_VLAN
        
        #Base interface and vlan interface must be up
        $IP link set $AP_NAME up
        
        #If configured to do so, set a default SSID name here
	# Quantenna layer will set the default SSID, etc.
        # qtn_set_default_ssid
    else
        #Remove vlan interface
        $IP link set $AP_NAME down
        $IP link del $AP_NAME
        
        #Configure vlan on pcie0 interface
        $QWCFG_TEST set $QTN_INDEX untrunk_vlan $QTN_VLAN
        
        #Bind internal interface to vlan we can use
        $QWCFG_TEST set $QTN_INDEX unbind_vlan $QTN_VLAN
        
        #Create VAP
        $QWCFG_TEST push $QTN_INDEX vap_emerged 0
    fi
}

qtn_init(){
    #Only run once, at boot
    if [ ! -f $QTN_STATE ]
    then
        echo > $QTN_STATE
        $QWCFG_TEST set 0 enable_vlan 1
        $IP link set $BASE_WIFI_IF up
    fi
}

#Optionally you could set up all wifi AP's at once, here
#Parameters: start | stop
qtn_setup_all(){
    qtn_init
    setup_qtn $1 ath0
    setup_qtn $1 ath1
    setup_qtn $1 ath2
    setup_qtn $1 ath3
    setup_qtn $1 ath4
    setup_qtn $1 ath5
    setup_qtn $1 ath6
    setup_qtn $1 ath7
}

wait_for_gre_ready(){
    while [ "`$SYSEVENT get if_${LAN_GRE_TUNNEL}-status`" != "ready" ] ; do
        echo "Waiting for $LAN_GRE_TUNNEL to be ready..."
        sleep 1
    done
}

#Do any gretap setup needed here, or call events, whatever
#Also returns LAN_GRE_TUNNEL so we know which to use for the base tunnel
#Syntax: setup_gretap [start|stop] bridge_name group_number
setup_gretap(){
    GRE_MODE="$1"
    LAN="$2"
    LAN_VLAN="$3"
    
    #For now, both use the same tunnel
    LAN_GRE_TUNNEL="$DEFAULT_GRE_TUNNEL"
    
    if [ "$GRE_MODE" = "start" ]
    then
        #Wait until gre got created
        wait_for_gre_ready
        vconfig add $LAN_GRE_TUNNEL $LAN_VLAN
        ifconfig $LAN_GRE_TUNNEL up
        ifconfig ${LAN_GRE_TUNNEL}.$LAN_VLAN up
    else
        vconfig rem ${LAN_GRE_TUNNEL}.${LAN_VLAN}
    fi
}

#Gets a space-separated list of interfaces currently in a group
#Returns the list in CURRENT_IF_LIST
get_current_if_list() {
    CURRENT_IF_NAMES=`$VLAN_UTIL list_interfaces $BRIDGE_NAME`
    CMD_STATUS=$?
    CURRENT_IF_LIST=""
    
    if [ $CMD_STATUS -ne 0 ]
    then
        echo "$0 error: couldn't get interface list for group $BRIDGE_VLAN"
    else
        for LINE in $CURRENT_IF_NAMES; do
            if [ "$CURRENT_IF_LIST" = "" ]
            then
                CURRENT_IF_LIST="$LINE"
            else
                CURRENT_IF_LIST="$CURRENT_IF_LIST $LINE"
            fi
        done
    fi
    echo -e "Group $BRIDGE_NAME vlan $BRIDGE_VLAN now includes:\t$CURRENT_IF_LIST"
}

#Calls dmcli to check whether switch port 2 should be enabled in instance 1 (private LAN) or 2 (XFinity Home) networks
#If the setting is not yet available, waits for 1 second and tries again, for up to 30 seconds
#Returns "true" in variable $isport2enable if port 2 should be enabled in this group
check_port_2(){
    COUNTER=0
    MAXTRIES=30
    PORT_ERR="INIT"

    #Only supports instance 1 or 2 currently
    case $INSTANCE in
        #XFinity Private LAN
        1)
            PORT_CMD="dmcli eRT getv Device.Bridging.Bridge.1.Port.8.Enable"
        ;;
        #XFinity Home
        2)
            PORT_CMD="dmcli eRT getv Device.Bridging.Bridge.2.Port.2.Enable"        
        ;;    
    esac
        
    while [ "$PORT_ERR" != "" -a $COUNTER -lt $MAXTRIES ] ; do
        PORT_CHECK=`$PORT_CMD`
        PORT_ERR=`echo "$PORT_CHECK"|grep "Can't find"`
        
        if [ "$PORT_ERR" != "" ] ; then
            echo "Waiting for dmcli port 2 configuration to be available..."
            COUNTER=`expr $COUNTER + 1`
            isport2enable="false"
            sleep 1
        else
            isport2enable=`echo "$PORT_CHECK" | grep value | cut -f3 -d : | cut -f2 -d " "`
        fi
        
    done

    if [ $COUNTER -eq $MAXTRIES ] ; then
        echo "$0 error: dmcli configuration for instance $INSTANCE port 2 timed out!"
        isport2enable="false"
    fi
}

#Returns a space-separated list of interfaces that should be in this group in the current operating mode
#Returns the list in IF_LIST
get_expected_if_list() {
    
    case $INSTANCE in
        
        #XFinity Private LAN
        1)
            check_port_2
            
            if [ "$BRIDGE_MODE" -gt 0 ]
            then
                #Interface list for bridged mode
                if [ "$isport2enable" = "true" ]
                then
                    #Switch port 2 connected to XFinity Private LAN
                    IF_LIST="eth_0 eth_1 nmoca0 l${CMDIAG_IF} lbr0"
                else
                    IF_LIST="eth_0 nmoca0 l${CMDIAG_IF} lbr0"
                    #Switch port 2 connected to XFinity Home
                fi
            else
                #Interface list for routed mode
                if [ "$isport2enable" = "true" ]
                then
                    #Switch port 2 connected to XFinity Private LAN
                    IF_LIST="eth_0 eth_1 nmoca0 ath0 ath1"
                else
                    #Switch port 2 connected to XFinity Home
                    IF_LIST="eth_0 nmoca0 ath0 ath1"
                fi
            fi

            syscfg set lan_ethernet_physical_ifnames "$IF_LIST"
        ;;
        
        #XFinity Home
        2)
            check_port_2
            
            if [ "$isport2enable" = "true" ]
            then
                #Switch port 2 connected to XFinity Home
                IF_LIST="eth_1 nmoca0.${BRIDGE_VLAN} ath2 ath3"
            else
                #Switch port 2 connected to XFinity Private LAN
                IF_LIST="nmoca0.${BRIDGE_VLAN} ath2 ath3"
            fi
        ;;
        
        #XFinity Hostspot 2.4 GHz
        3)
            IF_LIST="nmoca0.${BRIDGE_VLAN} ${DEFAULT_GRE_TUNNEL}.${BRIDGE_VLAN} ath4"
        ;;
        
        #XFinity Hotspot 5 GHz
        4)
            IF_LIST="nmoca0.${BRIDGE_VLAN} ${DEFAULT_GRE_TUNNEL}.${BRIDGE_VLAN} ath5"
        ;;
        
        #XFinity IoT network
        6)
            IF_LIST="ath6 ath7"
        ;;
        
    esac
    
    echo -e "Group $BRIDGE_NAME vlan $BRIDGE_VLAN should include:\t$IF_LIST"
}

remove_from_group() {
    IF_TO_REMOVE="$1"
    
    echo "Removing $IF_TO_REMOVE from group $BRIDGE_NAME"
    
    #Handle the case for wifi vlans
    if [ "`echo \"$IF_TO_REMOVE\"|egrep -e \"ath*\"`" != "" ]
    then
        setup_qtn stop $IF_TO_REMOVE
    elif [ "`echo \"$IF_TO_REMOVE\"|egrep -e \"host*\"`" != "" ]
    then
        setup_qtn stop $IF_TO_REMOVE
        #Handle the case for GRE tunnels
    elif [ "`echo \"$IF_TO_REMOVE\"|egrep -e \"${DEFAULT_GRE_TUNNEL}*\"`" != "" ]
    then
        setup_gretap stop $BRIDGE_NAME $BRIDGE_VLAN
    fi
    
    $VLAN_UTIL del_interface $BRIDGE_NAME $IF_TO_REMOVE
}

remove_from_all_groups(){
    IF_TO_REMOVE="$1"
    
    $VLAN_UTIL show|grep "Group:"|awk '{print $2}'|while read GROUP_NAME
    do
        $VLAN_UTIL list_interfaces $GROUP_NAME|while read IF_FOUND
        do
            if [ "$IF_TO_REMOVE" = "$IF_FOUND" -a "$GROUP_NAME" != "$BRIDGE_NAME" ]
            then
                echo "$IF_TO_REMOVE already belongs to $GROUP_NAME, removing"
                $VLAN_UTIL del_interface $GROUP_NAME $IF_FOUND
                break
            fi
        done
    done
}

add_to_group() {
    IF_TO_ADD="$1"
    VLAN_TO_ADD=""
    
    #If this interface is already in another group, remove it from there
    remove_from_all_groups $IF_TO_ADD $VLAN_TO_ADD
    
    #Check if the interface desired is a vlan interface
    echo "$IF_TO_ADD"|egrep -e ".*\..*"
    if [ $? -eq 0 ]
    then
        VLAN_TO_ADD="`echo \"$IF_TO_ADD\"|cut -d \".\" -f 2`"
        BASE_IF="`echo \"$IF_TO_ADD\"|cut -d \".\" -f 1`"
        IF_TO_ADD="$BASE_IF"
        echo "Adding ${IF_TO_ADD}.${VLAN_TO_ADD} to group $BRIDGE_NAME"
    else
        echo "Adding $IF_TO_ADD to group $BRIDGE_NAME"
    fi
    
    #Handle the case for wifi vlans
    if [ "`echo \"$IF_TO_ADD\"|egrep -e \"ath*\"`" != "" ]
    then
        setup_qtn start $IF_TO_ADD
    elif [ "`echo \"$IF_TO_ADD\"|egrep -e \"host*\"`" != "" ]
    then
        setup_qtn start $IF_TO_ADD
        #Handle the case for GRE tunnels
    elif [ "`echo \"$IF_TO_ADD\"|egrep -e \"${DEFAULT_GRE_TUNNEL}*\"`" != "" ]
    then
        sh /etc/utopia/service.d/service_multinet/handle_gre.sh create $INSTANCE $DEFAULT_GRE_TUNNEL
        setup_gretap start $BRIDGE_NAME $BRIDGE_VLAN
    fi
    
    $VLAN_UTIL add_interface $BRIDGE_NAME $IF_TO_ADD $VLAN_TO_ADD
}

#This function will create the group if it doesn't already exist,
#remove interfaces that shouldn't be connected, and add any
#interfaces that should be connected to this group.
#Takes a parameter "true" or "false" which controls whether we should
#update the multinet status.  Defaults to "true".
sync_group_settings() {
    RAISE_EVENTS="$1"

    [ "$RAISE_EVENTS" != "false" ] && $SYSEVENT set multinet_${INSTANCE}-localready 1
    
    #Check if bridge exists and if ID is correct.  If not, delete and re-create.
    CURRENT_VLAN=`$VLAN_UTIL show_vlan $BRIDGE_NAME`
    CMD_STATUS=$?
    
    if [ $CMD_STATUS -ne 0 -o "$CURRENT_VLAN" != "$BRIDGE_VLAN" ]
    then
        echo "Group $BRIDGE_NAME doesn't exist or has wrong VLAN ID, re-creating"
        #We need to destroy and re-create bridge
        $VLAN_UTIL del_group $BRIDGE_NAME
        $VLAN_UTIL add_group $BRIDGE_NAME $BRIDGE_VLAN
    fi

    [ "$RAISE_EVENTS" != "false" ] && $SYSEVENT set multinet_${INSTANCE}-status partial
    
    #Get the list of which interfaces should be added to this group
    get_expected_if_list
    
    #Remove any interfaces that don't belong in this group
    get_current_if_list
    for EXISTING_IF in $CURRENT_IF_LIST; do
        IF_BELONGS=0
        for EXPECTED_IF in $IF_LIST; do
            if [ "$EXISTING_IF" = "$EXPECTED_IF" ]
            then
                IF_BELONGS=1
            fi
        done
        
        if [ $IF_BELONGS -eq 0 ]
        then
            remove_from_group $EXISTING_IF
        fi
    done
    
    #Now add whatever is missing to this group
    get_current_if_list
    for NEEDED_IF in $IF_LIST; do
        IF_FOUND=0
        for EXISTING_IF in $CURRENT_IF_LIST; do
            if [ "$EXISTING_IF" = "$NEEDED_IF" ]
            then
                IF_FOUND=1
            fi
        done
        
        if [ $IF_FOUND -ne 1 ]
        then
            add_to_group $NEEDED_IF
        fi
    done

    #In bridge mode if we reconfigure the ports we need to flush DOCSIS CPE table
     if [ "$BRIDGE_MODE" -gt 0 ]
     then
         ncpu_exec -e "(echo \"LearnFrom=CPE_DYNAMIC\" > /proc/net/dbrctl/delalt)"
     fi

    [ "$RAISE_EVENTS" != "false" ] && $SYSEVENT set multinet_${INSTANCE}-status ready
}

#Remove any interfaces from this group
remove_all_from_group() {
    IF_LIST=""
    sync_group_settings false
}

print_syntax(){
    echo "Syntax: $0 [multinet-up|multinet-down|multinet-syncMembers] instance"
    echo -e "\t$0 [iot-up|iot-down]"
}

#Script execution begins here
while [ -e ${LOCKFILE} ] ; do
    #See if process is still running
    kill -0 `cat ${LOCKFILE}`
    if [ $? -ne 0 ]
    then
        break
    fi
    echo "Waiting for parallel instance of $0 to finish..."
    sleep 1
done

#make sure the lockfile is removed when we exit and then claim it
trap "rm -f ${LOCKFILE}; exit" INT TERM EXIT
echo $$ > ${LOCKFILE}

#Handle input parameters
#Temporary workaround: kill link monitor. No longer needed.
#$KILLALL $QWCFG_TEST 2> /dev/null

BRIDGE_MODE=`$SYSEVENT get bridge_mode`
CMDIAG_IF=`syscfg get cmdiag_ifname`

#Get event
EVENT="$1"
if [ "$EVENT" = "multinet-up" ]
then
    MODE="up"
elif [ "$EVENT" = "multinet-start" ]
then
    MODE="start"
elif [ "$EVENT" = "lnf-setup" ]
then
    MODE="lnf-start"
elif [ "$EVENT" = "lnf-down" ]
then
    MODE="lnf-stop"
elif [ "$EVENT" = "multinet-down" ]
then
    MODE="stop"
elif [ "$EVENT" = "multinet-stop" ]
then
    MODE="stop"
elif [ "$EVENT" = "multinet-syncMembers" ]
then
    MODE="syncmembers"
else
    echo "$0 error: Unknown event: $1"
    print_syntax
    rm -f ${LOCKFILE}
    exit 1
fi

#Instance maps to brlan0, brlan1, etc.
INSTANCE="$2"

#Get lan bridge name from instance number
BRIDGE_NAME=""
BRIDGE_VLAN=0
case $INSTANCE in
    1)
        #Private LAN
        BRIDGE_NAME="brlan0"
        BRIDGE_VLAN=100
    ;;
    2)
        #Home LAN
        BRIDGE_NAME="brlan1"
        BRIDGE_VLAN=101
    ;;
    3)
        #Public wifi 2.4GHz
        BRIDGE_NAME="brlan2"
        BRIDGE_VLAN=102
    ;;
    4)
        #Public wifi 5GHz
        BRIDGE_NAME="brlan3"
        BRIDGE_VLAN=103
    ;;
    6)
        BRIDGE_NAME="br106"
        BRIDGE_VLAN=106
    ;;
    *)
        #Unknown / unsupported instance number
        echo "$0 error: Unknown instance $INSTANCE"
        print_syntax
        rm -f ${LOCKFILE}
        exit 1
    ;;
esac

#Set the interface name
$SYSEVENT set multinet_${INSTANCE}-name $BRIDGE_NAME
$SYSEVENT set multinet_${INSTANCE}-vid $BRIDGE_VLAN

if [ "$MODE" = "up" ]
then 
    #n-mux bridge must exist first on ARM
    MUXSTATUS=1
    while [ $MUXSTATUS -ne 0 ] ; do
        $NCPU_EXEC -e "$IFCONFIG n-mux"
        MUXSTATUS=$?
        if [ $MUXSTATUS -ne 0 ]
        then
            echo "$0 waiting for n-mux..."
            sleep 1
        fi
    done
    remove_all_from_group       
    #Sync the group interfaces and raise status events
    sync_group_settings
elif [ $MODE = "start" ]
then
    remove_all_from_group       
    #Sync the group interfaces and raise status events
    sync_group_settings
    
    #Restart the firewall after the network is set up
    echo "VLAN XB6 : Triggering RDKB_FIREWALL_RESTART from mode=start"
    $SYSEVENT set firewall-restart    
elif [ $MODE = "lnf-start" ]
then
    remove_all_from_group
    #Sync the group interfaces and raise status events
    sync_group_settings
    
    #Restart the firewall after setting up LnF
    echo "VLAN XB6 : Triggering RDKB_FIREWALL_RESTART from mode=Lnfstart"
    $SYSEVENT set firewall-restart
elif [ $MODE = "lnf-stop" ]
then
    remove_all_from_group
    echo "VLAN XB6 : Triggering RDKB_FIREWALL_RESTART from mode=Lnfstop"
    $SYSEVENT set firewall-restart
elif [ "$MODE" = "stop" ]
then
    #Indicate LAN is stopping
    $SYSEVENT set multinet_${INSTANCE}-status stopping
    remove_all_from_group
    #Send event that LAN is stopped
    $SYSEVENT set multinet_${INSTANCE}-status stopped
elif [ "$MODE" = "syncmembers" ]
then
    #Sync the group interfaces and raise status events
    sync_group_settings
elif [ "$MODE" = "restart" ]
then
    #Indicate LAN is restarting
    $SYSEVENT set multinet_${INSTANCE}-status restarting
    remove_all_from_group
    #Sync the group interfaces and raise status events
    sync_group_settings
    #Restart the firewall after the network is set up
    echo "VLAN XB6 : Triggering RDKB_FIREWALL_RESTART from mode=restart"
    $SYSEVENT set firewall-restart
else
    echo "Syntax: $0 [start | stop | restart]"
    rm -f ${LOCKFILE}
    exit 1
fi

#When finished, restart the link monitor
# No longer needed to enable/disable.
#/etc/rc.d/qtn.rc.link_monitor up

#Script finished, remove lock file
rm -f ${LOCKFILE}
