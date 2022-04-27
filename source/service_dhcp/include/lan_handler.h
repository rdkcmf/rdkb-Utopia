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

#include "ccsp_custom.h"
#include "ccsp_psm_helper.h"
#include "ccsp_base_api.h"
#include "ccsp_memory.h"

extern void* g_vBus_handle;

#define CCSP_SUBSYS     "eRT."
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(g_vBus_handle, CCSP_SUBSYS, name, NULL, &(str))
#define PSM_VALUE_SET_STRING(name, str) PSM_Set_Record_Value2(g_vBus_handle, CCSP_SUBSYS, name, ccsp_string, str)
#define PSM_VALUE_GET_INS(name, pIns, ppInsArry) PsmGetNextLevelInstances(g_vBus_handle, CCSP_SUBSYS, name, pIns, ppInsArry)

void bring_lan_up();
void ipv4_status(int, char *);
void lan_restart();
void lan_stop();
void teardown_instance(int l3_inst);
void resync_instance (int l3_inst);
void erouter_mode_updated();
void ipv4_resync(char *lan_inst);
