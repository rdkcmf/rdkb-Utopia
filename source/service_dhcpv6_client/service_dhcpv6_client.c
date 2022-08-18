/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysevent/sysevent.h>
#include <syscfg/syscfg.h>
#include <errno.h>
#include <service_dhcpv6_client.h>
#include <sys/sysinfo.h>
#include <util.h>
#include <safec_lib_common.h>
#include <secure_wrapper.h>
#include <libnet.h>

#define SERVICE_NAME    "dhcpv6_client"
#define DHCPV6_CONF_FILE    "/etc/dhcp6c.conf"
#define DHCPV6_REGISTER_FILE    "/tmp/dhcpv6_registered_events"
#define DHCP6C_PROGRESS_FILE    "/tmp/dhcpv6c_inprogress"
#define DIBBLER_DEBUG_DIR   "/var/log/dibbler"
#define DIBBLER_INFO_DIR    "/tmp/.dibbler-info"
#define TRIGGER_LOC     "/usr/bin/service_dhcp"

#define BUFF_LEN_8    8
#define BUFF_LEN_16    16
#define BUFF_LEN_32    32
#define BUFF_LEN_64    64
#define BUFF_LEN_128    128
#define BUFF_LEN_256    256

int g_iSyseventfd;
token_t g_tSysevent_token;

static async_id_t l_sAsyncID_eve[4] = {0};
static async_id_t l_sAsyncID= (async_id_t){0};

const char* const g_cComponent_id = "ccsp.servicedhcpv6client";
void* g_vBus_handle = NULL;
FILE* g_fArmConsoleLog = NULL;

#if defined (_XB6_PRODUCT_REQ_) || defined(_CBR_PRODUCT_REQ_) || defined (_XB7_PRODUCT_REQ_)
#define CONSOLE_LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#else
#define CONSOLE_LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#endif

#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
#define DHCPV6_BINARY   "/sbin/ti_dhcp6c"
#define DHCPV6_PID_FILE "/var/run/erouter_dhcp6c.pid"
#else
#define DHCPV6_BINARY   "dibbler-client"
#define DHCPV6_PID_FILE "/tmp/dibbler/client.pid"
#endif

static int dbusInit( void )
{
    int ret = 0;
    char* pCfg = CCSP_MSG_BUS_CFG;
    if (g_vBus_handle == NULL)
    {
#ifdef DBUS_INIT_SYNC_MODE // Dbus connection init
        ret = CCSP_Message_Bus_Init_Synced(g_cComponent_id,
                                           pCfg,
                                           &g_vBus_handle,
                                           Ansc_AllocateMemory_Callback,
                                           Ansc_FreeMemory_Callback);
#else
        ret = CCSP_Message_Bus_Init((char *)g_cComponent_id,
                                    pCfg,
                                    &g_vBus_handle,
                                    (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,
                                    Ansc_FreeMemory_Callback);
#endif  /* DBUS_INIT_SYNC_MODE */
        if (ret == -1)
        {
            // Dbus connection error
            fprintf(stderr, "SERVICE_DHCP6C : DBUS connection error\n");
        }
    }
    return ret;
}

static inline void remove_file(char *tb_removed_file)
{
    int l_iRemove_Res;
    char file_name[BUFF_LEN_64] = {0};
    errno_t safec_rc = -1;
    safec_rc = strcpy_s(file_name, BUFF_LEN_64, tb_removed_file);
    ERR_CHK(safec_rc);

    if (access(file_name, F_OK) == 0)
    {
        l_iRemove_Res = remove(tb_removed_file);
        if (0 != l_iRemove_Res)
        {
            fprintf(stderr, "SERVICE_DHCP6C : remove of %s file is not successful error is:%d\n",
                    tb_removed_file, errno);
        }
    }
    else
    {
        fprintf(stderr, "SERVICE_DHCP6C : Requested File %s is not available\n", tb_removed_file);
    }

}

void copy_command_output(char *cmd, char *out, int len)
{
    FILE *l_fFp = NULL;
    errno_t safec_rc = -1;
    int buff_len;
    char l_cBuf[BUFF_LEN_256] = {0};
    l_fFp = popen(cmd, "r");
    if (l_fFp)
    {
        fgets(l_cBuf, sizeof(l_cBuf), l_fFp);
        /*we need to remove the \n char in buf*/
        if(l_cBuf[strlen(l_cBuf)-1] == '\n')
        {
            l_cBuf[strlen(l_cBuf)-1] = '\0';
        }
        buff_len = strlen(l_cBuf);
        if (buff_len > len)
        {
            fprintf(stderr, "SERVICE_DHCP6C : Warning Output will be truncated\n");
        }
        safec_rc = strcpy_s(out, len-1, l_cBuf);
        ERR_CHK(safec_rc);

        pclose(l_fFp);
    }
}

void dhcpv6_client_service_start ()
{
    fprintf(stderr, "SERVICE_DHCP6C : SERVICE START\n");

    char l_cLastErouterMode[BUFF_LEN_8] = {0}, l_cWanLinkStatus[BUFF_LEN_16] = {0}, l_cWanIfname[BUFF_LEN_16] = {0},
         l_cBridgeMode[BUFF_LEN_16] = {0}, l_cWanState[BUFF_LEN_16] = {0}, l_cDibblerEnable[BUFF_LEN_16] = {0},
         l_cPhylinkWanState[BUFF_LEN_16] = {0};

    if(0 != mkdir(DIBBLER_DEBUG_DIR, 0777))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Failed to create %s Directory\n",DIBBLER_DEBUG_DIR);
    }

    syscfg_get(NULL, "last_erouter_mode", l_cLastErouterMode, sizeof(l_cLastErouterMode));
    syscfg_get(NULL, "dibbler_client_enable", l_cDibblerEnable, sizeof(l_cDibblerEnable));
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "current_ipv4_link_state", l_cPhylinkWanState, sizeof(l_cPhylinkWanState));
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "wan_ifname", l_cWanIfname, sizeof(l_cWanIfname));
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "phylink_wan_state", l_cWanLinkStatus, sizeof(l_cWanLinkStatus));
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "bridge_mode", l_cBridgeMode, sizeof(l_cBridgeMode));
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "wan-status", l_cWanState, sizeof(l_cWanState));

    if ((strncmp(l_cLastErouterMode, "2", 1)) && (strncmp(l_cLastErouterMode, "3", 1)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Non IPv6 Mode, service_stop\n");
        dhcpv6_client_service_stop();
    }
    else if ((!strncmp(l_cWanLinkStatus, "down", 4)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : WAN LINK is Down, service_stop\n");
        dhcpv6_client_service_stop();
    }
    else if (l_cWanIfname == NULL)
    {
        fprintf(stderr, "SERVICE_DHCP6C : WAN Interface not configured, service_stop\n");
        dhcpv6_client_service_stop();
    }
    else if ((!strncmp(l_cBridgeMode, "1", 1)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : BridgeMode, service_stop\n");
        dhcpv6_client_service_stop();
    }
    else if ((!strncmp(l_cWanState, "stopped", 7)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : WAN state stopped, service_stop\n");
        dhcpv6_client_service_stop();
    }
    else if (access(DHCPV6_PID_FILE, F_OK) != 0)
    {
        if (access(DHCP6C_PROGRESS_FILE, F_OK) != 0)
        {
            FILE *fp;
            fp  = fopen (DHCP6C_PROGRESS_FILE, "w");
            fclose(fp);
            fprintf(stderr, "SERVICE_DHCP6C : Starting DHCPv6 Client from service_dhcpv6_client binary\n");
#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
            if (strncmp(l_cDibblerEnable, "true", 4))
            {
                fprintf(stderr, "SERVICE_DHCP6C : Starting ti_dhcp6c\n");
                v_secure_system("ti_dhcp6c -i %s -p %s -plugin /fss/gw/lib/libgw_dhcp6plg.so",l_cWanIfname,DHCPV6_PID_FILE);
            }
#else
            if(0 != mkdir(DIBBLER_INFO_DIR, 0777))
            {
                fprintf(stderr, "SERVICE_DHCP6C : Failed to create %s Directory\n",DIBBLER_INFO_DIR);
            }
            fprintf(stderr, "SERVICE_DHCP6C : Starting dibbler client\n");
            v_secure_system("sh /lib/rdk/dibbler-init.sh");
            sleep(1);
            v_secure_system("%s start",DHCPV6_BINARY);
#endif
            remove_file(DHCP6C_PROGRESS_FILE);
        }
        else
        {
           fprintf(stderr, "SERVICE_DHCP6C : DHCPv6 Client process start in progress, not starting one more\n");
        }
    }
}

void dhcpv6_client_service_stop ()
{
    fprintf(stderr, "SERVICE_DHCP6C : SERVICE STOP\n");
    char l_cDibblerEnable[BUFF_LEN_8] = {0}, l_cDSLiteEnable[BUFF_LEN_8] = {0};

    syscfg_get(NULL, "dibbler_client_enable", l_cDibblerEnable, sizeof(l_cDibblerEnable));
    syscfg_get(NULL, "dslite_enable", l_cDSLiteEnable, sizeof(l_cDSLiteEnable));

#if defined(_COSA_INTEL_XB3_ARM_) || defined(INTEL_PUMA7)
    if (strncmp(l_cDibblerEnable, "true", 4))
    {
        if (access(DHCPV6_PID_FILE, F_OK) == 0)
        {
            if (!strncmp(l_cDSLiteEnable, "1", 1))
            {
                // We need to make sure the erouter0 interface is UP when the DHCPv6 client process plan to send
                // the RELEASE message. Otherwise it will wait to send the message and get messed when another
                // DHCPv6 client process plan to start in service_start().
                char l_cCommand[BUFF_LEN_128] = {0}, l_cErouter0Status[BUFF_LEN_8] = {0};
                snprintf(l_cCommand, sizeof(l_cCommand),"ip -d link show erouter0 | grep state | awk '/erouter0/{print $9}'");
                copy_command_output(l_cCommand, l_cErouter0Status, sizeof(l_cErouter0Status));
                l_cErouter0Status[strlen(l_cErouter0Status)] = '\0';
                fprintf(stderr, "SERVICE_DHCP6C : l_cErouter0Status is %s\n",l_cErouter0Status);

                if (strncmp(l_cErouter0Status, "UP", 2))
                {
                   interface_up("erouter0");
                }
            }
            fprintf(stderr, "SERVICE_DHCP6C : Sending SIGTERM to %s\n",DHCPV6_PID_FILE);
            remove_file(DHCPV6_PID_FILE);

            if (!strncmp(l_cDSLiteEnable, "1", 1))
            {
                // After stop the DHCPv6 client, need to clear the sysevent tr_erouter0_dhcpv6_client_v6addr
                // So that it can be triggered again once DHCPv6 client got the same IPv6 address with the old address
                sysevent_set(g_iSyseventfd, g_tSysevent_token, "tr_erouter0_dhcpv6_client_v6addr", "", 0);
            }
        }
    }
#endif
    fprintf(stderr, "SERVICE_DHCP6C : Stopping dhcpv6 client\n");
    v_secure_system("%s stop",DHCPV6_BINARY);
    remove_file(DHCPV6_PID_FILE);
}

void dhcpv6_client_service_update()
{
    fprintf(stderr, "SERVICE_DHCP6C : Inside dhcpv6_client_service_update\n");

    char l_cDhcpv6cEnabled[BUFF_LEN_8] = {0};

    sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcpv6c_enabled", l_cDhcpv6cEnabled, sizeof(l_cDhcpv6cEnabled));

    if (!strncmp(l_cDhcpv6cEnabled, "1", 1))
    {
        dhcpv6_client_service_start();
    }
    else
    {
        dhcpv6_client_service_stop();
    }
}

void register_sysevent_handler(char *service_name, char *event_name, char *handler, char *flag)
{
    fprintf(stderr, "SERVICE_DHCP6C : Inside register_sysevent_handler\n");

    l_sAsyncID = (async_id_t){0};

    char l_sEvent_Name[BUFF_LEN_32] = {0}, l_sHandler_Name[BUFF_LEN_128] = {0},
         l_cAsyncId[BUFF_LEN_16] = {0}, l_sService_Name[BUFF_LEN_64] = {0}, l_sAsyncSysevent[BUFF_LEN_128] = {0};

    snprintf(l_sEvent_Name, sizeof(l_sEvent_Name), "%s", event_name);
    snprintf(l_sHandler_Name, sizeof(l_sHandler_Name), "%s", handler);
    snprintf(l_sService_Name, sizeof(l_sService_Name), "%s", service_name);

    if (flag != NULL)
    {
        sysevent_set_options(g_iSyseventfd, g_tSysevent_token, l_sEvent_Name, TUPLE_FLAG_EVENT);
        sysevent_setnotification(g_iSyseventfd, g_tSysevent_token, l_sEvent_Name,  &l_sAsyncID);
    }

    snprintf(l_cAsyncId, sizeof(l_cAsyncId), "%d %d", l_sAsyncID.action_id, l_sAsyncID.trigger_id);
    snprintf(l_sAsyncSysevent, sizeof(l_sAsyncSysevent), "%s_%s_asyncid", service_name, l_sEvent_Name);
    sysevent_set(g_iSyseventfd, g_tSysevent_token, l_sAsyncSysevent, l_cAsyncId, 0);

    if ((!strncmp(event_name, "erouter_mode-updated", 20)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Registering erouter_mode-updated\n");
        l_sAsyncID_eve[0] = l_sAsyncID;
    }
    else if ((!strncmp(event_name, "phylink_wan_state", 17)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Registering phylink_wan_state\n");
        l_sAsyncID_eve[1] = l_sAsyncID;
    }
    else if ((!strncmp(event_name, "current_wan_ifname", 18)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Registering current_wan_ifname\n");
        l_sAsyncID_eve[2] = l_sAsyncID;
    }
    else if ((!strncmp(event_name, "bridge_mode", 11)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Registering bridge_mode\n");
        l_sAsyncID_eve[3] = l_sAsyncID;
    }

}

void unregister_sysevent_handler(char *service_name, char *event_name)
{
    fprintf(stderr, "SERVICE_DHCP6C : Inside unregister_sysevent_handler\n");

    char l_sEvent_Name[BUFF_LEN_32] = {0}, l_cAsyncId[BUFF_LEN_16] = {0}, l_sService_Name[BUFF_LEN_64] = {0}, 
         l_sAsyncSysevent[BUFF_LEN_128] = {0};

    snprintf(l_sEvent_Name, sizeof(l_sEvent_Name), "%s", event_name);
    snprintf(l_sService_Name, sizeof(l_sService_Name), "%s", service_name);
    snprintf(l_sAsyncSysevent, sizeof(l_sAsyncSysevent), "%s_%s_asyncid", service_name, l_sEvent_Name);

    if ((!strncmp(event_name, "erouter_mode-updated", 20)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Unregistering erouter_mode-updated\n");
        l_sAsyncID = l_sAsyncID_eve[0];
    }
    else if ((!strncmp(event_name, "phylink_wan_state", 17)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Unregistering phylink_wan_state\n");
        l_sAsyncID = l_sAsyncID_eve[1];
    }
    else if ((!strncmp(event_name, "current_wan_ifname", 18)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Unregistering current_wan_ifname\n");
        l_sAsyncID = l_sAsyncID_eve[2];
    }
    else if ((!strncmp(event_name, "bridge_mode", 11)))
    {
        fprintf(stderr, "SERVICE_DHCP6C : Unregistering bridge_mode\n");
        l_sAsyncID = l_sAsyncID_eve[3];
    }

    sysevent_get(g_iSyseventfd, g_tSysevent_token, l_sAsyncSysevent,
                 l_cAsyncId, sizeof(l_cAsyncId));

    if ( l_cAsyncId != NULL)
    {
        sysevent_rmnotification(g_iSyseventfd, g_tSysevent_token, l_sAsyncID);
        sysevent_set(g_iSyseventfd, g_tSysevent_token, l_sAsyncSysevent, "", 0);
    }
}

void register_dhcpv6_client_handler()
{
    fprintf(stderr, "SERVICE_DHCP6C : Inside register_dhcpv6_client_handler\n");

    register_sysevent_handler(SERVICE_NAME, "erouter_mode-updated", TRIGGER_LOC, "");
    register_sysevent_handler(SERVICE_NAME, "phylink_wan_state", TRIGGER_LOC, "");
    register_sysevent_handler(SERVICE_NAME, "current_wan_ifname", TRIGGER_LOC, "");
    register_sysevent_handler(SERVICE_NAME, "bridge_mode", TRIGGER_LOC, "");

    FILE *fp;
    fp  = fopen (DHCPV6_REGISTER_FILE, "w");
    fclose(fp);
}

void unregister_dhcpv6_client_handler()
{
    fprintf(stderr, "SERVICE_DHCP6C : Inside unregister_dhcpv6_client_handler\n");

    unregister_sysevent_handler(SERVICE_NAME, "erouter_mode-updated");
    unregister_sysevent_handler(SERVICE_NAME, "phylink_wan_state");
    unregister_sysevent_handler(SERVICE_NAME, "current_wan_ifname");
    unregister_sysevent_handler(SERVICE_NAME, "bridge_mode");

    remove_file(DHCPV6_REGISTER_FILE);
}

void dhcpv6_client_service_enable ()
{
    fprintf(stderr, "SERVICE_DHCP6C : SERVICE ENABLE\n");

    char l_cDhcpv6cEnabled[BUFF_LEN_8] = {0};
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcpv6c_enabled", l_cDhcpv6cEnabled, sizeof(l_cDhcpv6cEnabled));

    if (!strncmp(l_cDhcpv6cEnabled, "1", 1))
    {
        fprintf(stderr, "SERVICE_DHCP6C : SERVICE ENABLE\n");
        if (access(DHCPV6_REGISTER_FILE, F_OK) != 0)
        {
            fprintf(stderr, "DHCPv6 Client is enabled but events are not registered, registering it now\n");
            register_dhcpv6_client_handler();
        }
        else
        {
            fprintf(stderr, "DHCPv6 Client is enabled and events are registered\n");
        }
        return;
    }

    dhcpv6_client_service_start();
    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcpv6c_enabled", "1", 0);
    register_dhcpv6_client_handler();
}

void dhcpv6_client_service_disable ()
{
    fprintf(stderr, "SERVICE_DHCP6C : SERVICE DISABLE\n");

    char l_cDhcpv6cEnabled[BUFF_LEN_8] = {0};
    sysevent_get(g_iSyseventfd, g_tSysevent_token, "dhcpv6c_enabled", l_cDhcpv6cEnabled, sizeof(l_cDhcpv6cEnabled));

    if (strncmp(l_cDhcpv6cEnabled, "1", 1))
    {
        fprintf(stderr, "SERVICE_DHCP6C : DHCPv6 Client is not enabled\n");
        return;
    }

    sysevent_set(g_iSyseventfd, g_tSysevent_token, "dhcpv6c_enabled", "0", 0);
    unregister_dhcpv6_client_handler();

    remove_file(DHCP6C_PROGRESS_FILE);
    dhcpv6_client_service_stop();
}

int sysevent_syscfg_init()
{
    g_iSyseventfd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION,
                                  "service_dhcpv6_client", &g_tSysevent_token);
    g_fArmConsoleLog = freopen(CONSOLE_LOG_FILE, "a+", stderr);
    if (NULL == g_fArmConsoleLog) //In error case not returning as it is ok to continue
    {
        fprintf(stderr, "SERVICE_DHCP6C : Error:%d while opening Log file:%s\n", errno, CONSOLE_LOG_FILE);
    }
    else
    {
        fprintf(stderr, "SERVICE_DHCP6C : Successful in opening Log file:%s\n", CONSOLE_LOG_FILE);
    }
    if (g_iSyseventfd < 0)
    {
        fprintf(stderr, "SERVICE_DHCP6C : service_dhcp::sysevent_open failed\n");
        return ERROR;
    }
    /* dbus init based on bus handle value */
    if(g_vBus_handle ==  NULL)
        dbusInit();
    if(g_vBus_handle == NULL)
    {
        fprintf(stderr, "SERVICE_DHCP6C : service_dhcp_init, DBUS init error\n");
        return ERROR;
    }
    return SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "SERVICE_DHCP6C : Insufficient number of args return\n");
        return 0;
    }
    if (0 == g_iSyseventfd)
        sysevent_syscfg_init();

    fprintf(stderr, "SERVICE_DHCP6C : %s case\n", argv[1]);
    if (!strncmp(argv[1], "erouter_mode-updated", 20))
    {
        dhcpv6_client_service_update();
    }
    else if(!strncmp(argv[1], "dhcpv6_client_service_enable", 28))
    {
        dhcpv6_client_service_enable();
    }
    else if(!strncmp(argv[1], "dhcpv6_client_service_disable", 29))
    {
        dhcpv6_client_service_disable();
    }
    else if(!strncmp(argv[1], "dhcpv6_client-start", 19))
    {
        //dhcpv6_client_service_enable();
    }
    else if(!strncmp(argv[1], "dhcpv6_client-stop", 18))
    {
        //dhcpv6_client_service_disable();
    }
    else if(!strncmp(argv[1], "dhcpv6_client-restart", 21))
    {
        /*dhcpv6_client_service_disable();
        dhcpv6_client_service_enable();*/
    }
    else if((!strncmp(argv[1], "phylink_wan_state", 17)) ||
            (!strncmp(argv[1], "current_wan_ifname", 18)) ||
            (!strncmp(argv[1], "current_ipv4_link_state", 23)) ||
            (!strncmp(argv[1], "lan-status", 10)) ||
            (!strncmp(argv[1], "bridge_mode", 11)))
    {
        dhcpv6_client_service_update();
    }
    fclose(g_fArmConsoleLog);
    sysevent_close(g_iSyseventfd, g_tSysevent_token);
    return 0;
}
