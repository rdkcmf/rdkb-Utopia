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


#ifndef IGD_PLATFORM_DEPENDENT_INF_H
#define IGD_PLATFORM_DEPENDENT_INF_H

#if defined(CS_XB7)
#define ROOT_FRIENDLY_NAME          "Arris TG4482A"
#define MANUFACTURER                "Arris Group, Inc"
#define MANUFACTURER_URL            "http://www.arrisi.com/"
#define MODULE_DESCRIPTION          "DOCSIS 3.1 Cable Modem Gateway Device"
#define MODULE_NAME                 "TG4482A"
#define MODULE_NUMBER               "TG4482A"
#define MODULE_URL                  "http://www.comcast.com"
#define UPC                         "TG4482A"
#elif defined (_ARRIS_XB6_PRODUCT_REQ_)
#define ROOT_FRIENDLY_NAME         "Arris TG3482G"
#define MANUFACTURER                "Arris Group, Inc"
#define MANUFACTURER_URL            "http://www.arrisi.com/"
#define MODULE_DESCRIPTION          "DOCSIS 3.1 Cable Modem Gateway Device"
#define MODULE_NAME                 "TG3482G"
#define MODULE_NUMBER               "TG3482G"
#define MODULE_URL                  "http://www.comcast.com"
#define UPC                         "TG3482G"
#else
#define MANUFACTURER                "Cisco"
#define MANUFACTURER_URL            "http://www.cisco.com/"
#define MODULE_DESCRIPTION          "RDKB_ARM"
#define MODULE_NAME                 "RDKB_ARM"
#define MODULE_NUMBER               "RDKB_ARM"
#define MODULE_URL                  "http://www.cisco.com"
#define UPC                         "RDKB_ARM"
#endif

#if defined(_COSA_BCM_ARM_) && (defined(_CBR_PRODUCT_REQ_) || defined(_XB6_PRODUCT_REQ_))
#undef CONFIG_VENDOR_MODEL
    #if defined (_XB8_PRODUCT_REQ_)
      #define CONFIG_VENDOR_MODEL  "CGM4981COM"
    #elif defined (_XB7_PRODUCT_REQ_)
      #define CONFIG_VENDOR_MODEL  "CGM4331COM"
    #elif defined (_XB6_PRODUCT_REQ_)
      #define CONFIG_VENDOR_MODEL  "CGM4140COM"
    #elif defined (_CBR2_PRODUCT_REQ_)
      #define CONFIG_VENDOR_MODEL  "CGA4332COM"
    #elif defined (_CBR_PRODUCT_REQ_)
      #define CONFIG_VENDOR_MODEL  "CGA4131COM"
    #endif
#endif

#endif

