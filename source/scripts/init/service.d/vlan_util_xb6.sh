#!/bin/sh
#Create / Destroy LAN bridges for XB6 using $VLAN_UTIL
#TODO:
#       Create / destroy gretap tunnel if necessary for public wifi
VLAN_UTIL="vlan_util"
QWCFG_TEST="qwcfg_test"
QCSAPI_PCIE="qcsapi_pcie"
IP="ip"
IFCONFIG="ifconfig"
NCPU_EXEC="ncpu_exec"
SYSEVENT="sysevent"
KILLALL="killall"

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

#FIREWALLRESTART=""

#Map athX style index X to Quantenna index                               
#Takes as input parameter interface name (such as ath0)                  
map_ath_to_qtn(){                                                        
        ATH_INDEX=`echo "$1"|sed 's/\(ath\)\([0-9]*\)/\2/'`              
        if [ `expr $ATH_INDEX % 2` -eq 0 ] ; then                        
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

#We have a wrapper for vlan_util del_group in case a bridge
#got created outside of our control 
del_group(){
    BRIDGE_NAME="$1"
    $VLAN_UTIL del_group $BRIDGE_NAME

    $IP link show $BRIDGE_NAME
    DELRESULT=$?

    #If bridge exists and delete didn't work, delete with brctl
    if [ $DELRESULT -eq 0 ] ; then
        if [ -e /sys/class/net/$BRIDGE_NAME/bridge ] ; then
            echo "Deleting bridge which was created outside of $VLAN_UTIL"
            $IP link set $BRIDGE_NAME down
            $IP link del $BRIDGE_NAME
        fi
    fi
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
	if [ $USE_DEFAULT_SSID -ne 0 -a "$BRIDGE_NAME" != "brlan0" ] ; then

		SSID_CATEGORY=""
		#Captive portal will take care of setting the private network ssid and passphrase
		#if [ $ATH_INDEX -eq 0 -o $ATH_INDEX -eq 1 ] ; then
		#	SSID_CATEGORY="Private"	
		if [ $ATH_INDEX -eq 2 -o $ATH_INDEX -eq 3 ] ; then
			SSID_CATEGORY="XHS"

			#ADD MAC ADDRESS here
			SSID="${SSID_CATEGORY}-$UNIQUE_ID"
			#THEN SET SSID XHS-MAC (same for both 2.4 and 5G)
			$QWCFG_TEST push $QTN_INDEX ssid "$SSID"
		elif [ $ATH_INDEX -eq 4 -o $ATH_INDEX -eq 5 ] ; then
			SSID_CATEGORY="xfinitywifi"
			SSID="${SSID_CATEGORY}"

			#THEN SET SSID "xfinitywifi"   (same for both 2.4 and 5G)
			$QWCFG_TEST push $QTN_INDEX ssid "$SSID"
		elif [ $ATH_INDEX -eq 6 -o $ATH_INDEX -eq 7 ] ; then
			SSID_CATEGORY="IOT"

			#ADD MAC ADDRESS here
			SSID="${SSID_CATEGORY}-$UNIQUE_ID"
			#THEN SET SSID IOT-MAC (same for both 2.4 and 5G)
			$QWCFG_TEST push $QTN_INDEX ssid "$SSID"
		fi

		#RADIO_DESC=""
		#if [ "$RADIO" -eq "0" ] ; then
		#	RADIO_DESC="5GHz"
		#elif [ "$RADIO" -eq "2" ] ; then
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

        if [ "$QTN_MODE" = "start" ] ; then
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
		qtn_set_default_ssid
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
        if [ ! -f $QTN_STATE ]; then
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

        if [ "$GRE_MODE" = "start" ] ; then
		#Wait until gre got created
		wait_for_gre_ready
		vconfig add $LAN_GRE_TUNNEL $LAN_VLAN
		ifconfig $LAN_GRE_TUNNEL up
		ifconfig ${LAN_GRE_TUNNEL}.$LAN_VLAN up
        else
		vconfig rem ${LAN_GRE_TUNNEL}.${LAN_VLAN}
        fi
}

#Create or Destroy 
setup_iot() {
IOT_MODE=$1
if [ "$IOT_MODE" = "start" ];then
	$VLAN_UTIL add_group $BRIDGE_NAME $BRIDGE_VLAN
	#Set up Quantenna wifi
	setup_qtn $IOT_MODE ath6
	setup_qtn $IOT_MODE ath7
        #Create private LAN if it doesn't exist
        $VLAN_UTIL add_interface $BRIDGE_NAME ath6
        $VLAN_UTIL add_interface $BRIDGE_NAME ath7	
	$VLAN_UTIL add_interface $BRIDGE_NAME eth0 $BRIDGE_VLAN
else
	del_group $BRIDGE_NAME
	setup_qtn $IOT_MODE ath6
	setup_qtn $IOT_MODE ath7
fi
}

#Create or destoy LAN bridges
#Syntax: setup_lans [start|stop] instance
setup_lans(){
        LAN_MODE="$1"

        if [ "$LAN_MODE" = "start" ] ; then

                #Create bridge
                $VLAN_UTIL add_group $BRIDGE_NAME $BRIDGE_VLAN

                $SYSEVENT set multinet_${INSTANCE}-localready 1
                $SYSEVENT set multinet_${INSTANCE}-status partial
                case $INSTANCE in
                1)
		#Set up Quantenna wifi
		setup_qtn $LAN_MODE ath0
		setup_qtn $LAN_MODE ath1

                #Create private LAN if it doesn't exist
                $VLAN_UTIL add_interface $BRIDGE_NAME eth_0
                $VLAN_UTIL add_interface $BRIDGE_NAME nmoca0
                #$VLAN_UTIL add_interface $BRIDGE_NAME nmoca0 $BRIDGE_VLAN
                $VLAN_UTIL add_interface $BRIDGE_NAME ath0
                $VLAN_UTIL add_interface $BRIDGE_NAME ath1
                ;;

                2)
		#Set up Quantenna wifi
		setup_qtn $LAN_MODE ath2
		setup_qtn $LAN_MODE ath3

                #Create Xfinity home network if it doesn't exist
                $VLAN_UTIL add_interface $BRIDGE_NAME eth_1
                $VLAN_UTIL add_interface $BRIDGE_NAME nmoca0 $BRIDGE_VLAN
                $VLAN_UTIL add_interface $BRIDGE_NAME ath2
                $VLAN_UTIL add_interface $BRIDGE_NAME ath3
                ;;

                3)
                #Create Xfinity public 2.4GHz network

		#Set up GRE and add it to this group
                sh /etc/utopia/service.d/service_multinet/handle_gre.sh create $INSTANCE $DEFAULT_GRE_TUNNEL 
                setup_gretap $LAN_MODE $BRIDGE_NAME $BRIDGE_VLAN

		#Set up Quantenna wifi
		setup_qtn $LAN_MODE ath4

		$VLAN_UTIL add_interface $BRIDGE_NAME nmoca0 $BRIDGE_VLAN	
                $VLAN_UTIL add_interface $BRIDGE_NAME $LAN_GRE_TUNNEL $BRIDGE_VLAN
                $VLAN_UTIL add_interface $BRIDGE_NAME ath4
                ;;

                4)
                #Create Xfinity public 5GHz network

		#Set up GRE and add it to this group
                sh /etc/utopia/service.d/service_multinet/handle_gre.sh create $INSTANCE $DEFAULT_GRE_TUNNEL 
                setup_gretap $LAN_MODE $BRIDGE_NAME $BRIDGE_VLAN

                #Set up Quantenna wifi
                setup_qtn $LAN_MODE ath5

		$VLAN_UTIL add_interface $BRIDGE_NAME nmoca0 $BRIDGE_VLAN
                $VLAN_UTIL add_interface $BRIDGE_NAME $LAN_GRE_TUNNEL $BRIDGE_VLAN
                $VLAN_UTIL add_interface $BRIDGE_NAME ath5
                ;;

                esac
        else 
                $SYSEVENT set multinet_${INSTANCE}-status stopping
                case $INSTANCE in
                1)
                #Destroy private LAN
                del_group $BRIDGE_NAME
		setup_qtn $LAN_MODE ath0
		setup_qtn $LAN_MODE ath1
                ;;

                2)
                #Destroy home LAN
                del_group $BRIDGE_NAME
		setup_qtn $LAN_MODE ath2
		setup_qtn $LAN_MODE ath3
                ;;

                3)
                #Destroy Xfinity public 2.4GHz network
                del_group $BRIDGE_NAME
		setup_qtn $LAN_MODE ath4
		setup_gretap $LAN_MODE $BRIDGE_NAME $BRIDGE_VLAN
                ;;

                4)
                #Destroy Xfinity public 5GHz network
                del_group $BRIDGE_NAME
		setup_qtn $LAN_MODE ath5
                setup_gretap $LAN_MODE $BRIDGE_NAME $BRIDGE_VLAN
                ;;
		
                esac
                $SYSEVENT set multinet_${INSTANCE}-status stopped
        fi

}

print_syntax(){
        echo "Syntax: $0 [multinet-up|multinet-down] instance"
}

#Script execution begins here
#Handle input parameters
#Temporary workaround: kill link monitor
$KILLALL $QWCFG_TEST

#Get event
EVENT="$1"
if [ "$EVENT" = "multinet-up" ] ; then
        MODE="up"
elif [ "$EVENT" = "multinet-start" ] ; then
        MODE="start"
elif [ "$EVENT" = "iot-up" ] ; then
        MODE="iot-start"
elif [ "$EVENT" = "iot-down" ] ; then                                             
        MODE="iot-stop"   
elif [ "$EVENT" = "multinet-down" ] ; then
        MODE="stop"
elif [ "$EVENT" = "multinet-stop" ] ; then
        MODE="stop"
elif [ "$EVENT" = "multinet-syncMembers" ] ; then
        MODE="syncmembers"
else
        echo "$0 error: Unknown event: $1"
        print_syntax
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
                exit 1
        ;;
esac

#Set the interfae name
$SYSEVENT set multinet_${INSTANCE}-name $BRIDGE_NAME
$SYSEVENT set multinet_${INSTANCE}-vid $BRIDGE_VLAN


if [ "$MODE" = "up" ] ; then
        #Start by setting status to stopped
        #$SYSEVENT set multinet_${INSTANCE}-status stopped

        #n-mux bridge must exist first on ARM
        MUXSTATUS=1
        while [ $MUXSTATUS -ne 0 ] ; do
                $NCPU_EXEC -e "$IFCONFIG n-mux"
                MUXSTATUS=$?
                if [ $MUXSTATUS -ne 0 ] ; then
                        echo "$0 waiting for n-mux..."
                        sleep 1
                fi
        done

        #If bridge already exists, tear it down first
        $IFCONFIG $BRIDGE_NAME 1> /dev/null 2> /dev/null
        if [ $? -eq 0 ] ; then
                #Bridge already exists, destroy it first then create
                setup_lans stop
        fi

        #Set up bridge and add interfaces
        setup_lans start
	#Send event saying we're ready
        $SYSEVENT set multinet_${INSTANCE}-status ready
elif [ $MODE = "start" ]; then
        #If bridge already exists, tear it down first
        $IFCONFIG $BRIDGE_NAME 1> /dev/null 2> /dev/null
        if [ $? -eq 0 ] ; then
                #Bridge already exists, destroy it first then create
                setup_lans stop
        fi

        #Set up bridge and add interfaces
        setup_lans start
	#Send event saying we're ready
        $SYSEVENT set multinet_${INSTANCE}-status ready
        #restart the firewall after the network is set up
        $SYSEVENT set firewall-restart
elif [ $MODE = "iot-start" ]; then
        #If bridge already exists, tear it down first
        $IFCONFIG $BRIDGE_NAME 1> /dev/null 2> /dev/null
        if [ $? -eq 0 ] ; then
                #Bridge already exists, destroy it first then create
                setup_iot stop
        fi

        #Set up bridge and add interfaces
        setup_iot start
        $SYSEVENT set firewall-restart
elif [ $MODE = "iot-stop" ]; then
	setup_iot stop                                                         
        $SYSEVENT set firewall-restart
elif [ "$MODE" = "stop" ] ; then
        #Indicate LAN is stopping
        $SYSEVENT set multinet_${INSTANCE}-status stopping
        setup_lans stop
        #Send event that LAN is stopped
        $SYSEVENT set multinet_${INSTANCE}-status stopped 
elif [ "$MODE" = "syncmembers" ] ; then
        echo "TO DO NOT SUPPORTED YET"
elif [ "$MODE" = "restart" ] ; then
        #Indicate LAN is stopping
        $SYSEVENT set multinet_${INSTANCE}-status stopping
        setup_lans stop
        #Indicate LAN is stopped
        $SYSEVENT set multinet_${INSTANCE}-status stopped
        setup_lans start
        #Send event that LAN is ready
        $SYSEVENT set multinet_${INSTANCE}-status ready
else
        echo "Syntax: $0 [start | stop | restart]"
        exit 1
fi

#When finished, restart the link monitor
/etc/rc.d/qtn.rc.link_monitor up
