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
#ifndef MNET_IFHANDLER_H
#define MNET_IFHANDLER_H

#include "service_multinet_base.h"

typedef struct l2net {
	int bEnabled;
	int vid;
	char name[16];
}L2Net, *PL2Net;

typedef struct iftype {
	char name[32];
	void* nvkey;
	struct ifTypeHandler handlers;
}IFType, *PIFType;

typedef struct netif {
	PIFType type;
	int pvid;
	int bTagging;
	char name[16];
}NetInterface, *PNetInterface;

typedef int (IFHandlerFunc*)(NetInterface* netif, L2Net* net);

typedef struct ifTypeHandler {
	IFHandlerFunc create;
	IFHandlerFunc destroy;
	IFHandlerFunc addVlan;
	IFHandlerFunc removeVlan;
	char* (getLocalName*)(PNetInterface netif);
} IFTypeHandler, *PIFTypeHandler;


	



#endif
