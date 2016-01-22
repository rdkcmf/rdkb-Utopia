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
 
 #ifndef MNET_IFPLUG_H
 #define MNET_IFPLUG_H
 //Must define a header file called service_multinet_ifplugin_defs.h which
 //contains the macro or constant NUM_PLUGIN_IFTYPES. It could also be used
 //as a general header for the plugin.
//// #include "service_multinet_ifplugin_defs.h"
 #include "service_multinet_handler.h"
#include "service_multinet_base.h"
 
 int plat_addImplicitMembers(PL2Net nv_net, PMember memberBuf);
 
#ifdef MULTINET_IFHANDLER_PLUGIN
 /**Fill 'handlers' with all required platform specific handlers
  * for DM interfaces. See the MemberHandler definition for a 
  * list of functions that must be mapped for each handler.
  */
 int mnet_plugin_init(struct allIfHandlers* handlers);
#endif

 #endif
 
