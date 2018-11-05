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
   Copyright [2014] [Cisco Systems, Inc.]
 
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

#ifndef __ACTUAL_DOM_H__
#define __ACTUAL_DOM_H__

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define ACTUAL_DOM_EXPORT_PREFIX extern "C"
#else
#  define ACTUAL_DOM_EXPORT_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllexport)
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef ACTUAL_DOM_BUILD
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX __attribute__ ((visibility("default")))
#    else
#      define ACTUAL_DOM_EXPORT ACTUAL_DOM_EXPORT_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */


/*
 * Elements
 */

typedef enum _ACTUAL_DOM_Element
{
    ACTUAL_DOM_Element_Cisco_CiscoStruct = 0,
    ACTUAL_DOM_Element_Cisco_CiscoStructToo = 1,
    ACTUAL_DOM_Element_Cisco_a = 2,
    ACTUAL_DOM_Element_Cisco_b = 3,
    ACTUAL_DOM_Element_Cisco_c = 4,
    ACTUAL_DOM_Element_Cisco_d = 5,
    ACTUAL_DOM_Element_Cisco_e = 6,
    ACTUAL_DOM_Element_Cisco_s = 7
} ACTUAL_DOM_Element;


/*
 * DOM Schemas
 */

ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_CiscoStruct();
ACTUAL_DOM_EXPORT const HDK_XML_Schema* ACTUAL_DOM_Schema_Cisco_CiscoStructToo();

#endif /* __ACTUAL_DOM_H__ */
