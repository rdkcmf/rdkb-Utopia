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
#define ERROR	-1
#define SUCCESS	0
extern void* g_vBus_handle;

void dhcpv6_client_service_start ();
void dhcpv6_client_service_stop ();
void dhcpv6_client_service_update ();
void register_sysevent_handler(char *service_name, char *event_name, char *handler, char *flag);
void unregister_sysevent_handler(char *service_name, char *event_name);
void register_dhcpv6_client_handler();
void unregister_dhcpv6_client_handler();
void dhcpv6_client_service_enable ();
void dhcpv6_client_service_disable ();
