/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2015] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/
#ifndef MNET_NV_H
#define MNET_NV_H

#define MNET_NV_PRIMARY_L2_INST_KEY "dmsb.MultiLAN.PrimaryLAN_l2net"
#define MNET_NV_PRIMARY_L2_INST_FORMAT "%3d"

#include "service_multinet_base.h"


int nv_get_members(PL2Net net, PMember memberList, int numMembers);
int nv_get_bridge(int l2netInst, PL2Net net);
int nv_get_primary_l2_inst(void);
#if defined(MESH_ETH_BHAUL)
int nv_toggle_ethbhaul_ports(BOOL onOff);
#endif

#endif
