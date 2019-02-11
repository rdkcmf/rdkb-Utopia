/****************************************************************************
  Copyright 2017 Intel Corporation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
******************************************************************************/

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
#ifndef P7_PLAT_MAP_H
#define P7_PLAT_MAP_H

typedef enum puma7entities {
    ENTITY_NP = 1,
    ENTITY_ESW,
    ENTITY_AP
} Puma6EntityID;

typedef enum puma7Hals {
    HAL_NOOP,
    HAL_WIFI,
    HAL_ESW,
    HAL_GRE,
    HAL_LINUX
} Puma6HalID;

int eventIDFromStringPortID (void* portID, char* stringbuf, int bufsize);

#define NUM_ENTITIES 4
#define NUM_HALS 6

#define PUMA7_WIFI_PREFIX_1 "ath"
#define PUMA7_WIFI_PREFIX_2 "wlan"
#define PUMA7_WIFI_UTIL "wifi_util.sh"

#endif
