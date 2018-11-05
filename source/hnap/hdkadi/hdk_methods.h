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

#ifndef __HDK_METHODS_H__
#define __HDK_METHODS_H__

#include "hdk_mod.h"
#include "hdk_xml.h"


/*
 * Macro to control public exports
 */
#define HDK_ADI_EXPORT_PREFIX extern
#ifdef HDK_ADI_BUILD
# define HDK_ADI_EXPORT HDK_ADI_EXPORT_PREFIX __attribute__ ((visibility("default")))
#else
# define HDK_ADI_EXPORT HDK_ADI_EXPORT_PREFIX
#endif

/* Export the HNAP12 FirmwareUpload method */
HDK_ADI_EXPORT void HNAP12_Method_PN_FirmwareUpload(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);

#endif  /* __HDK_METHODS_H__ */
