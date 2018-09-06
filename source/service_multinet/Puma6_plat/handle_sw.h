/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

const char TYPE[]="SW";

#define TAGGING_MODE 2
#define UNTAGGED_MODE 1
#define NATIVE_MODE 0

#define ADD     1
#define DELETE  0

const char SW_ALL_PORTS[]="sw_1 sw_2 sw_3 sw_4 sw_5 atom arm I2E E2I";
const char PORTMAP_sw_1[]="-c 0 -p 0";
const char PORTMAP_sw_2[]="-c 0 -p 1";
const char PORTMAP_sw_3[]="-c 0 -p 2";
const char PORTMAP_sw_4[]="-c 0 -p 3";
const char PORTMAP_sw_5[]="-c 16 -p 3";  //moca
const char PORTMAP_atom[]="-c 16 -p 0";
const char PORTMAP_arm[]="-c 16 -p 7";
const char PORTMAP_I2E[]="-c 16 -p 2";
const char PORTMAP_E2I[]="-c 0 -p 5";

const char PORTMAP_DEF_sw_1[]="-c 34 -p 0";
const char PORTMAP_DEF_sw_2[]="-c 34 -p 1";
const char PORTMAP_DEF_sw_3[]="-c 34 -p 2";
const char PORTMAP_DEF_sw_4[]="-c 34 -p 3";
const char PORTMAP_DEF_sw_5[]="-c 16 -p 3 -m 0 -q 1"; //moca
const char PORTMAP_DEF_atom[]="-c 16 -p 0 -m 0 -q 1";
const char PORTMAP_DEF_arm[]="-c 16 -p 7 -m 0 -q 1";
const char PORTMAP_DEF_I2E[]="-c 16 -p 2 -m 0 -q 1";
const char PORTMAP_DEF_E2I[]="-c 34 -p 5";

const char PORTMAP_REM_sw_1[]="-c 1 -p 0";
const char PORTMAP_REM_sw_2[]="-c 1 -p 1";
const char PORTMAP_REM_sw_3[]="-c 1 -p 2";
const char PORTMAP_REM_sw_4[]="-c 1 -p 3";
const char PORTMAP_REM_sw_5[]="-c 17 -p 3";  //moca
const char PORTMAP_REM_atom[]="-c 17 -p 0";
const char PORTMAP_REM_arm[]="-c 17 -p 7";
const char PORTMAP_REM_I2E[]="-c 17 -p 2";
const char PORTMAP_REM_E2I[]="-c 1 -p 5";

const char PORTMAP_VENABLE_sw_1[]="-c 4 -p 0";
const char PORTMAP_VENABLE_sw_2[]="-c 4 -p 1";
const char PORTMAP_VENABLE_sw_3[]="-c 4 -p 2";
const char PORTMAP_VENABLE_sw_4[]="-c 4 -p 3";
const char PORTMAP_VENABLE_sw_5[]="-c 20 -p 3";  //moca
const char PORTMAP_VDISABLE_sw_5[]="-c 21 -p 3"; //moca
const char PORTMAP_VENABLE_atom[]="-c 20 -p 0";
const char PORTMAP_VENABLE_arm[]="-c 20 -p 7";
const char PORTMAP_VENABLE_I2E[]="-c 20 -p 2";
const char PORTMAP_VENABLE_E2I[]="-c 4 -p 5";

const char EXT_DEP[]="I2E-t E2I-a";
const char ATOM_DEP[]="atom-t";

//const char MGMT_PORT_LINUX_IFNAME[]="l2sm0";

void addVlan(int, int, char*);
void setMulticastMac();
void addIpcVlan();
void addRadiusVlan();
void createMeshVlan();
void addMeshBhaulVlan(); // RDKB-15951
