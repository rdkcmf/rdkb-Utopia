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

#include "dhcp_msg.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Compare two DHCP options
 * @return 0 if two options are same
 */
int compare_option(const struct dhcp_option *opt1, const struct dhcp_option *opt2)
{
   if (opt1->len < opt2->len) return -1;
   else if (opt1->len > opt2->len) return 1;
   else return memcmp(opt1->data, opt2->data, opt1->len);
}

/**
 * @brief Copy DHCP option, allocate new spaces
 */
void dhcp_copy_option(struct dhcp_option *dst, const struct dhcp_option *src)
{
   if (dst->data) free(dst->data);
   dst->code = src->code;
   dst->len = src->len;
   dst->data = (ui8*)malloc(dst->len);
   memcpy(dst->data, src->data, dst->len);
}

/**
 * @brief Cleanup DHCP option: can only be used to clean up copied DHCP option
 *        DHCP option from message parsing contains pointer to the option location
 *        in side the DHCP message and can not be cleaned using this function.
 *        This function does not free the option itself
 */
void dhcp_cleanup_option(struct dhcp_option *opt)
{
   if (opt->data)
   {
      free(opt->data);
   }
   memset(opt, 0, sizeof(struct dhcp_option));
}

/**
 * @brief Clear DHCP option info
 */
void dhcp_clear_option_info(struct dhcp_option_info *opt_info)
{
   struct dhcp_option *option_ptr, *next_option_ptr;

   for (option_ptr = opt_info->option_list; option_ptr; option_ptr = next_option_ptr)
   {
      next_option_ptr = option_ptr->next;
      free(option_ptr);
   }

   for (option_ptr = opt_info->file_option_list; option_ptr; option_ptr = next_option_ptr)
   {
      next_option_ptr = option_ptr->next;
      free(option_ptr);
   }

   for (option_ptr = opt_info->sname_option_list; option_ptr; option_ptr = next_option_ptr)
   {
      next_option_ptr = option_ptr->next;
      free(option_ptr);
   }

   memset(opt_info, 0, sizeof(struct dhcp_option_info));
}

/**
 * @brief Parse a list of DHCP options
 *        set overload flag is found
 * @return a link list of DHCP options
 */
struct dhcp_option *dhcp_parse_options(struct dhcp_option_info *opt_info,
                                       ui8* opt_data, size_t size)
{
  ui32 i;
  struct dhcp_option *opt_list = NULL, *last_opt=NULL, *curr_opt;
  for (i=0;i<size;)
  {
     ui8 opt_code = opt_data[i];
     if (opt_code == OPTION_PAD) i++;
     else if (opt_code == OPTION_END) break;
     else if (i+1 >= size || i+1+opt_data[i+1]>=size) break;
     else
     {
         curr_opt = (struct dhcp_option*)malloc(sizeof(struct dhcp_option));
         curr_opt->code = opt_code;
         curr_opt->len = opt_data[i+1];
         curr_opt->data = opt_data+i+2;
         i += 2 + curr_opt->len;
         curr_opt->next = NULL;
         if (last_opt) last_opt->next = curr_opt;
         else opt_list = curr_opt;
         last_opt = curr_opt;
         switch (opt_code)
         {
             case OPTION_OVERLOAD: opt_info->overload = curr_opt->data[0]; break;
             case OPTION_MSGTYPE: opt_info->msgtype = curr_opt->data[0]; break;
             case OPTION_SERVERID:
                   memcpy(&opt_info->server_ip, curr_opt->data, 4);
                   break;
             case OPTION_LEASETIME:
                   opt_info->leasetime = curr_opt->data[0] << 24 | curr_opt->data[1] << 16 | curr_opt->data[2] << 8 | curr_opt->data[3];
                   break;
             case OPTION_HOSTNAME: opt_info->opt_hostname = curr_opt; break;
             case OPTION_CLIENTID: opt_info->opt_clientid = curr_opt; break;
             case OPTION_ROUTER: opt_info->opt_router = curr_opt; break;
             case OPTION_DNS: opt_info->opt_dns = curr_opt; break;
         }
     }
  }
  return opt_list;
}

/**
 * @brief Parse DHCP message (mainly options in option field, file field and sname field)
 */
void dhcp_parse_msg(struct dhcp_option_info *opt_info, struct dhcp_msg *msg, size_t size)
{
  memset(opt_info, 0, sizeof(struct dhcp_option_info));
  opt_info->option_list = dhcp_parse_options(opt_info, msg->option_data, size-sizeof(struct dhcp_msg));
  if (opt_info->overload & OPTION_IN_FILE)
     opt_info->file_option_list = dhcp_parse_options(opt_info, msg->file, sizeof(msg->file));
  if (opt_info->overload & OPTION_IN_SNAME)
     opt_info->sname_option_list = dhcp_parse_options(opt_info, msg->sname, sizeof(msg->sname));
}

/**
 * @brief Validate DHCP message
 * @return 0 valid
 * @return -1 invalid
 */
int dhcp_validate_msg(const struct dhcp_msg *msg, const struct dhcp_option_info *opt_info)
{
   if (msg->hdr.op == BOOTREQUEST)
   {
      if (opt_info->msgtype != DHCPDISCOVER &&
          opt_info->msgtype != DHCPREQUEST &&
          opt_info->msgtype != DHCPDECLINE &&
          opt_info->msgtype != DHCPRELEASE &&
          opt_info->msgtype != DHCPINFORM) return -1;
   }
   else if (msg->hdr.op == BOOTREPLY)
   {
      if (opt_info->msgtype != DHCPOFFER &&
          opt_info->msgtype != DHCPACK &&
          opt_info->msgtype != DHCPNAK) return -1;
   }
   if (msg->hdr.hlen > 16) return -1;
   return 0;
}

/**
 * @brief add a single IP address option
 */
size_t dhcp_add_ipaddr_option(ui8 code, struct in_addr ipaddr, ui8* buf, size_t bufsize)
{
   buf[0] = code;
   buf[1] = 4;
   memcpy(buf+2, &ipaddr, 4);
   return 6;
}
