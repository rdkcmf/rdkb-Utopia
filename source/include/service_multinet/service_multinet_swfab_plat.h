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
#ifndef MNET_SWFAB_PLAT_H
#define MNET_SWFAB_PLAT_H

#include "service_multinet_base.h"
#include "service_multinet_swfab_deps.h"

/**The purpose of this function is to fill in the "map" member of iface
 * with a PPlatformPort pointer to a complete platform port, including
 * HAL references. It will be assumed that the pointers returned from this
 * function are internally managed by the mapping code, and will not be freed
 * by the framework. 
 */
int mapToPlat(PNetInterface iface);


/**
 * Return a pointer to a EntityPathDeps structure that defines a list of trunk ports
 * requiring configuration to connect the two specified entities. It is expected that 
 * this information can be provided for any combination of entity IDs. entityLow will
 * be less than entityHigh.
 * 
 * This mapping must remain static. The framework does not support re-mapping of trunk
 * ports for existing paths.
 */
PEntityPathDeps getPathDeps(int entityLow, int entityHigh);

/**
 * Return the platform port represented by its string ID, retrieved via its HAL's stringID
 * function. Primarily used when loading from nonvol or string based runtime data store.
 */
PPlatformPort plat_mapFromString(char* portIdString);


#endif
