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

/*
 * Copyright (c) 2010 by Cisco Systems, Inc. All Rights Reserved.
 *
 * This work is subject to U.S. and international copyright laws and
 * treaties. No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged, condensed,
 * expanded, collected, compiled, linked, recast, transformed or adapted
 * without the prior written consent of Cisco Systems, Inc. Any use or
 * exploitation of this work without authorization could subject the
 * perpetrator to criminal and civil liability.
 */

#include <stdio.h>
#include "srvmgr.h"
#include "syscfg/syscfg.h"


void do_start(void) {
   syscfg_init();

   FILE *fp = fopen("/etc/shadow", "w"); 
   if (NULL != fp) {
      char passwd[256];
      passwd[0] = '\0';
      syscfg_get(NULL, "root_pw", passwd, sizeof(passwd));
      if ('\0' == passwd[0]) {
         // default is admin
         fprintf(fp,"%s\n", "root:$1$okk8R3sJ$nOWjEHqTyMwLHT7puB6VM1:14073:0:99999:7:::");
      } else {
         fprintf(fp,"%s%s%s\n", "root:", passwd, ":14073:0:99999:7:::");
      }
      fprintf(fp,"%s\n", "nobody:*:9797:0:::::");
      fprintf(fp,"%s\n", "sshd:!:14047:0:99999:7:::");
      passwd[0] = '\0';
      syscfg_get(NULL, "admin_pw", passwd, sizeof(passwd));
      if ('\0' == passwd[0]) {
         // default is admin
         fprintf(fp,"%s\n", "admin:$1$1r2FkUle$1d/iIcRrgCgk5FrA36Yul/:14278:0:99999:7:::");
      }  else {
         fprintf(fp,"%s%s%s\n", "admin:", passwd, ":14278:0:99999:7:::");
      }
      fprintf(fp,"%s\n", "quagga:$1$PV.f8jCW$HVFrIVniSJJsJ9HroN5Fw/:10957:0:99999:7:::");
      fprintf(fp,"%s\n", "firewall:$1$PV.f8jCW$HVFrIVniSJJsJ9HroN5Fw/:10957:0:99999:7:::");
      fclose(fp);
   }

   fp = fopen("/etc/passwd", "w");
   if (NULL != fp) {
      fprintf(fp, "%s\n", "root:x:0:0::/:/bin/sh");
      fprintf(fp, "%s\n", "nobody:x:99:99:Nobody:/:/bin/nologin");
      fprintf(fp, "%s\n", "sshd:x:22:22::/var/empty:/sbin/nologin");
      fprintf(fp, "%s\n", "admin:x:1000:1000:Admin User:/tmp/home/admin:/bin/sh");
      fprintf(fp, "%s\n", "quagga:x:1001:1001:Quagga:/var/empty:/bin/nologin");
      fprintf(fp, "%s\n", "firewall:x:1002:1002:Firewall:/var/empty:/bin/nologin");
      fprintf(fp, "%s\n", "msi:x:1003:1003::/var/empty:/bin/nologin");
      fclose(fp);
   }

   fp = fopen("/etc/group", "w");
   if (NULL != fp) {
      fprintf(fp, "%s\n", "root:x:0:root");
      fprintf(fp, "%s\n", "nobody:x:99:");
      fprintf(fp, "%s\n", "sshd:x:22:");
      fprintf(fp, "%s\n", "admin:x:1000:");
      fprintf(fp, "%s\n", "quagga:x:1001:");
      fprintf(fp, "%s\n", "firewall:x:1002:");
      fprintf(fp, "%s\n", "msi:x:1003:");
      fclose(fp);
   }
}

int main(int argc, char **argv)
{
   cmd_type_t choice = parse_cmd_line(argc, argv);
   
   switch(choice) {
      case(nochoice):
      case(start):
         do_start();
         break;
      case(stop):
         break;
      case(restart):
         do_start();
         break;
      default:
         printf("%s called with invalid parameter (%s)\n", argv[0], 1==argc ? "" : argv[1]);
   }   
   return(0);
}

