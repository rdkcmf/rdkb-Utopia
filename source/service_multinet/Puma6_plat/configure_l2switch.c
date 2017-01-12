#if defined(_COSA_INTEL_USG_ARM_) && !defined(INTEL_PUMA7)
#include "sysevent/sysevent.h"
#include "service_multinet_base.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "swctl_hal.h"

#define BRLAN0_VLANID		100
#define BRLAN0_NETID		1
#define BRLAN0_LAN_PORTS 	"sw_1 sw_2 sw_3 sw_4"
#define BRLAN0_WIFI_PORTS 	"ath0 ath1"
#define BRLAN0_MOCA_PORT 	"sw_5"

#define BRLAN1_VLANID		101
#define BRLAN1_NETID		2
#define BRLAN1_LAN_PORTS 	"sw_1-t sw_2-t sw_3-t"
#define BRLAN1_WIFI_PORTS 	"ath2 ath3"
#define BRLAN1_MOCA_PORT 	"sw_5-t"

#define BRLAN2_VLANID		102
#define BRLAN2_NETID		3
#define BRLAN2_WIFI_PORT 	"ath4"

#define BRLAN3_VLANID		103
#define BRLAN3_NETID		4
#define BRLAN3_WIFI_PORT 	"ath5"

void configure_l2switch_brlan0()
{
    MNET_DEBUG("Inside configure_l2switch_brlan0 fn")
    addVlan(BRLAN0_NETID, BRLAN0_VLANID, BRLAN0_LAN_PORTS);
    addVlan(BRLAN0_NETID, BRLAN0_VLANID, BRLAN0_MOCA_PORT);
    addVlan(BRLAN0_NETID, BRLAN0_VLANID, BRLAN0_WIFI_PORTS);
}

void configure_l2switch_brlan1()
{
    MNET_DEBUG("Inside configure_l2switch_brlan1 fn")
    addVlan(BRLAN1_NETID, BRLAN1_VLANID, BRLAN1_MOCA_PORT);
    addVlan(BRLAN1_NETID, BRLAN1_VLANID, BRLAN1_LAN_PORTS);
    addVlan(BRLAN1_NETID, BRLAN1_VLANID, BRLAN1_WIFI_PORTS);
}
void configure_l2switch_brlan2()
{
    MNET_DEBUG("Inside configure_l2switch_brlan2 fn")
    addVlan(BRLAN2_NETID, BRLAN2_VLANID, BRLAN2_WIFI_PORT);
}
void configure_l2switch_brlan3()
{
    MNET_DEBUG("Inside configure_l2switch_brlan3 fn")
    addVlan(BRLAN3_NETID, BRLAN3_VLANID, BRLAN3_WIFI_PORT);
}
#endif
