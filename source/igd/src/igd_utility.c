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

/* ***************************************************************************
 * FileName:   igd_utility.c
 * Author:      
 * Date:         April-22-2009
 * Description: This file contains data structure definitions and function for linksys IGD utiles
 *****************************************************************************/
/*$Id: igd_utility.c,v 1.2 2009/05/15 05:43:00 jianxiao Exp $
 *
 *$Log: igd_utility.c,v $
 *Revision 1.2  2009/05/15 05:43:00  jianxiao
 *Update for integration
 *
 *Revision 1.1  2009/05/13 08:57:27  tahong
 *create orignal version
 *

 *
 **/
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h> 
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <utctx/utctx_api.h>
#include <utapi/utapi.h>

#include "pal_upnp_device.h"
#include "pal_upnp.h"
#include "pal_def.h"
#include "igd_utility.h"


//for timer
LOCAL struct timer_function_node *timer_function_list_run_once = NULL;
LOCAL struct timer_function_node *timer_function_list_cycle = NULL;
INT32 timer_init_count = 0;

LOCAL pthread_t timer_thread_id_run_once;
LOCAL pthread_t timer_thread_id_cycle;
LOCAL pthread_mutex_t timer_thread_mutex_run_once;
LOCAL pthread_mutex_t timer_thread_mutex_cycle;

/************************************************************
* Function: _timer_loop_run_once
*
*  Parameters: 
*      arg:             IN.  not used now.
* 
*  Description:
*     This function loops once a second to count the time and triggers the registered function
*     The registered function runs only once
*
*  Return Values: 
*   This function does not return
************************************************************/  
LOCAL VOID *_timer_loop_run_once(IN VOID *arg)
{
    (void) arg;
    struct timer_function_node *copy_timer_function_list = NULL;
    struct timer_function_node *temp_node_prev = NULL, *temp_node = NULL;

    //LOG_ENTER_FUNCTION;

    while(1)
    {
        sleep(1);

        pthread_mutex_lock(&timer_thread_mutex_run_once);
        temp_node_prev = temp_node = timer_function_list_run_once;
        copy_timer_function_list = NULL;
        while(temp_node != NULL)
        {
            temp_node->accumulate_second++;

            if (temp_node->trigger_second == temp_node->accumulate_second)
            {
                //take the temp_node
                if (temp_node == timer_function_list_run_once)//temp_node is the head
                {
                    temp_node_prev = timer_function_list_run_once = timer_function_list_run_once->next;
                    
                    temp_node->next = copy_timer_function_list;
                    copy_timer_function_list = temp_node;

                    temp_node = temp_node_prev;
                }
                else//temp_node is not the head
                {
                    temp_node_prev->next = temp_node->next;
                    
                    temp_node->next = copy_timer_function_list;
                    copy_timer_function_list = temp_node;

                    temp_node = temp_node_prev->next;                
                }
            }
            else
            {
                temp_node_prev = temp_node;
                temp_node = temp_node->next;
                continue;
            }
        }        
        pthread_mutex_unlock(&timer_thread_mutex_run_once);

        //run the timer functions
        temp_node = copy_timer_function_list;
        while(temp_node != NULL)
        {
            temp_node->timer_function(temp_node->upnp_device, temp_node->upnp_service);
            temp_node = temp_node->next;
        }
        //free the timer functions
        temp_node = copy_timer_function_list;
        while(temp_node != NULL)
        {
            copy_timer_function_list = copy_timer_function_list->next;
            free(temp_node);
            temp_node = copy_timer_function_list;
        }
    }
}

/************************************************************
* Function: _timer_loop_cycle
*
*  Parameters: 
*      arg:             IN.  not used now.
* 
*  Description:
*     This function loops once a second to count the time and triggers the registered function
*     The registered function runs in cycle
*
*  Return Values: 
*   This function does not return
************************************************************/  
LOCAL VOID *_timer_loop_cycle(VOID *arg)
{
    (void) arg;
    struct timer_function_node *copy_timer_function_list = NULL;
    struct timer_function_node *temp_node = NULL, *temp_node_copy = NULL;

    //LOG_ENTER_FUNCTION;

    while(1)
    {
        sleep(1);

        pthread_mutex_lock(&timer_thread_mutex_cycle);

        temp_node = timer_function_list_cycle;
        copy_timer_function_list = NULL;
        while(temp_node != NULL)
        {
            temp_node->accumulate_second++;
            // copy the node
            if (temp_node->trigger_second == temp_node->accumulate_second)
            {
                temp_node->accumulate_second = 0;
                temp_node_copy = (struct timer_function_node *)calloc(1, sizeof(struct timer_function_node) );
                if (temp_node_copy != NULL)
                {
                    temp_node_copy->upnp_device = temp_node->upnp_device;
                    temp_node_copy->upnp_service = temp_node->upnp_service;
                    temp_node_copy->timer_function = temp_node->timer_function;
                    temp_node_copy->next = copy_timer_function_list;
                    copy_timer_function_list = temp_node_copy;
                }
                else
                {
                    ;//PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "malloc temp_node_copy fails");
                }
                temp_node = temp_node->next;
            }
            else
            {
                temp_node = temp_node->next;
                continue;
            }
        }        
        pthread_mutex_unlock(&timer_thread_mutex_cycle);

        //run the timer functions
        temp_node_copy = copy_timer_function_list;
        while(temp_node_copy != NULL)
        {
            temp_node_copy->timer_function(temp_node_copy->upnp_device, temp_node_copy->upnp_service);
            temp_node_copy = temp_node_copy->next;
        }
        //free the timer functions
        temp_node_copy = copy_timer_function_list;
        while(temp_node_copy != NULL)
        {
            copy_timer_function_list = copy_timer_function_list->next;
            free(temp_node_copy);
            temp_node_copy = copy_timer_function_list;
        }
    }
}

/************************************************************
* Function: IGD_timer_start
*
*  Parameters: none
* 
*  Description:
*     This function starts 2 thread(timer_thread_id_run_once and timer_thread_id_cycle)
*
*  Return Values: none
*
************************************************************/
VOID IGD_timer_start(VOID)
{
    //LOG_ENTER_FUNCTION;

    if (timer_init_count != 0)
    {
        //LOG_LEAVE_FUNCTION;
        return;
    }

    if ( pthread_mutex_init(&timer_thread_mutex_run_once, NULL))
    {
        //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "init timer_thread_mutex_run_once fail");
        return;
    }
    if ( pthread_mutex_init(&timer_thread_mutex_cycle, NULL))
    {
        //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "init timer_thread_mutex_cycle fail");
        pthread_mutex_destroy(&timer_thread_mutex_run_once);
        return;
    }

    if (pthread_create(&timer_thread_id_run_once, NULL, _timer_loop_run_once, NULL)) 
    {
        //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "init timer_thread_id_run_once fail");
        pthread_mutex_destroy(&timer_thread_mutex_run_once);
        pthread_mutex_destroy(&timer_thread_mutex_cycle);
        return;
    }
    pthread_detach(timer_thread_id_run_once);
    if (pthread_create(&timer_thread_id_cycle, NULL, _timer_loop_cycle, NULL)) 
    {
        //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "init timer_thread_id_cycle fail");
        pthread_mutex_destroy(&timer_thread_mutex_run_once);
        pthread_mutex_destroy(&timer_thread_mutex_cycle);
        
        pthread_cancel(timer_thread_id_run_once);
        return;
    }
    pthread_detach(timer_thread_id_cycle);

    timer_init_count++;
    //LOG_LEAVE_FUNCTION;
    return;
}

/************************************************************
* Function: IGD_timer_stop
*
*  Parameters: none
* 
*  Description:
*     This function stop the 2 thread(timer_thread_id_run_once and timer_thread_id_cycle)
*
*  Return Values: none
*
************************************************************/
VOID IGD_timer_stop(VOID)
{
    //LOG_ENTER_FUNCTION;

    if (timer_init_count != 1)
    {
        //LOG_LEAVE_FUNCTION;
        return;
    }

    pthread_mutex_destroy(&timer_thread_mutex_run_once);
    pthread_mutex_destroy(&timer_thread_mutex_cycle);

    pthread_cancel(timer_thread_id_run_once);
    pthread_cancel(timer_thread_id_cycle);

    timer_init_count--;
    //LOG_LEAVE_FUNCTION;
    return;
}

/************************************************************
* Function: IGD_timer_register
*
*  Parameters:
*               input_upnp_service:    IN.    The upnp_service which register a function
*               input_timer_function:   IN.    The function registered
*               input_trigger_second:  IN.    The alarm time or cycle time
*               input_mode:                IN.    timer_function_mode_run_once or timer_function_mode_cycle
*  Description:
*     This function register a function to thread timer_thread_id_run_once or timer_thread_id_cycle
*
*  Return Values: none
*
************************************************************/
VOID IGD_timer_register(IN struct upnp_device * input_upnp_device,
                           IN struct upnp_service * input_upnp_service,
                           IN timer_function_t input_timer_function,
                           IN INT32 input_trigger_second,
                           IN INT32 input_mode)
{
    struct timer_function_node *temp_node = NULL;

    //LOG_ENTER_FUNCTION;

    if ( (!input_upnp_device)
         ||(!input_upnp_service)
         || (!input_timer_function)
         ||(input_trigger_second <= 0)
         ||( (input_mode != timer_function_mode_run_once) && (input_mode != timer_function_mode_cycle) )
         )
    {
        //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_WARNING, "input parameter error");
        return;
    }

    if (input_mode == timer_function_mode_run_once)
    {
        pthread_mutex_lock(&timer_thread_mutex_run_once);
        temp_node = (struct timer_function_node *)calloc(1, sizeof(struct timer_function_node) );
        if (temp_node != NULL)
        {
            temp_node->upnp_device = input_upnp_device;
            temp_node->upnp_service = input_upnp_service;
            temp_node->timer_function = input_timer_function;
            temp_node->trigger_second = input_trigger_second;
            temp_node->mode = input_mode;
            temp_node->next = timer_function_list_run_once;
            timer_function_list_run_once = temp_node;
        }
        else
        {
            //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "malloc temp_node fails");
        }
        pthread_mutex_unlock(&timer_thread_mutex_run_once);
    }
    else// if (input_mode == timer_function_mode_cycle)
    {
        pthread_mutex_lock(&timer_thread_mutex_cycle);
        temp_node = (struct timer_function_node *)calloc(1, sizeof(struct timer_function_node) );
        if (temp_node != NULL)
        {
            temp_node->upnp_device = input_upnp_device;
            temp_node->upnp_service = input_upnp_service;
            temp_node->timer_function = input_timer_function;
            temp_node->trigger_second = input_trigger_second;
            temp_node->mode = input_mode;
            temp_node->next = timer_function_list_cycle;
            timer_function_list_cycle = temp_node;
        }
        else
        {
            //PAL_LOG(WAN_CONNECTION_DEVICE_LOG_NAME, PAL_LOG_LEVEL_FAILURE, "malloc temp_node fails");
        }
        pthread_mutex_unlock(&timer_thread_mutex_cycle);
    }

    //LOG_LEAVE_FUNCTION;
}

/*
 * IPV4 Address check functions
 */
/* addr is in network order */
int IPv4Addr_IsSameNetwork(uint32_t addr1, uint32_t addr2, uint32_t mask)
{
    return (addr1 & mask) == (addr2 & mask);
}
/* addr is in network order */
int IPv4Addr_IsBroadcast(uint32_t addr, uint32_t net, uint32_t mask)
{
    /* all ones or all zeros (old) */
    if (addr == 0xffffffff)
        return 1;

    /* on the same sub network and host bits are all ones */
    if (IPv4Addr_IsSameNetwork(addr, net, mask)
            && (addr & ~mask) == (0xffffffff & ~mask))
        return 1;

    return 0;
}
/* addr is in network order */
int IPv4Addr_IsNetworkAddr(uint32_t addr, uint32_t net, uint32_t mask)
{
    if (IPv4Addr_IsSameNetwork(addr, net, mask)
            && (addr & ~mask) == 0)
        return 1;

    return 0;
}

/** 
 * Check if internal client is valid. If valid, return true.
 *  
 * @param client 
 * 
 * @return BOOL 
 */
BOOL chkPortMappingClient(char* client)
{
    UtopiaContext Ctx;
    lanSetting_t lan;
    uint32_t ipaddr = 0xffffffff, netmask = 0xffffffff, clientaddr = 0xffffffff;
    BOOL ret;

    if (!Utopia_Init(&Ctx))
    {
        printf(("%s Error initializing context\n", __FUNCTION__));
        return FALSE;
    }

    Utopia_GetLanSettings(&Ctx, &lan);
    inet_pton(AF_INET, lan.ipaddr, &ipaddr);
    inet_pton(AF_INET, lan.netmask, &netmask);
    inet_pton(AF_INET, client, &clientaddr);
    if((clientaddr != ipaddr) &&
        !IPv4Addr_IsBroadcast(clientaddr, ipaddr, netmask) &&
        !IPv4Addr_IsNetworkAddr(clientaddr, ipaddr, netmask) &&
        IPv4Addr_IsSameNetwork(clientaddr, ipaddr, netmask) )
        ret =  TRUE; 
    else
        ret = FALSE;
   
    Utopia_Free(&Ctx, 0);
    return ret;
}
