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

#include <stdio.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include "DM_TR181.h"


int file_parse(char* file_name, param_node **head)
{
    FILE *fp = NULL;
    param_node *curNode = NULL;
    param_node *node = NULL;
    char *str = NULL;
    char *name = NULL;
    char *val = NULL;
    char line[LINE_SZ];
    char tok[] = ": \n";
    int  len   = 0;

    if (!file_name || !head) {
        sprintf(ulog_msg, "%s: Invalid Input Parameter !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_PARAM;
    }
    if((fp = fopen(file_name, "r"))== NULL ) {
        sprintf(ulog_msg, "%s: Error File Open !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_FILE_OPEN_FAIL;
    }
    while((fgets(line, sizeof(line), fp)) != NULL)
    {
        if(line[0] == ' ' || line[0] == '\n')
            continue;
        /*
        ** RDKB-7130, CID-33065, free unused mem.
        ** Change is made to make coverity complaint to plug resource leak
        */
        if(node)
        {
            free(node);
            node = NULL;
        }
        node = (param_node*)malloc(sizeof(param_node));
        if (!node) {
            fclose(fp);/*RDKB-7130, CID-33164, free resources before exit*/
            sprintf(ulog_msg, "%s: Memory Allocation Error !!!", __FUNCTION__);
            ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
            return ERR_INSUFFICIENT_MEM;
        }

        if((strstr(line, "MACAddress") != NULL) || (strstr(line, "BSSID") != NULL)){
            str   = strchr(line, ':');    
            name  = strtok(line, tok);
            if(name && str) { /*RDKB-7130, CID-33482, CID-33393, CID-32980; null check before use */
                if(!strcasecmp(name, "MACAddressFilterList")){
                    val   = str+1;
                    while(*(str++))
                        len++;
                    val[len-1] = '\0'; /* str will point to ':', hence strlen(val) = strlen(str)-1 */
                    sprintf(ulog_msg, "%s: MACAddressFilterList = %slen = %d \n", __FUNCTION__, val, strlen(val));
                    ulogf(ULOG_CONFIG, UL_UTAPI, ulog_msg);
                }else{
                    val   = str+1;
                    val[MAX_MAC_LEN] = '\0';
                }
            }
        }else{
            name = strtok(line, tok);
            val  = strtok(NULL, tok);
        }

        if(name&& val){
            strncpy(node->param_name, name, strlen(name));
            node->param_name[strlen(name)] = '\0';
            strncpy(node->param_val, val, strlen(val));
            node->param_val[strlen(val)] = '\0';
        }else{
            free(node);
	    node = NULL;
            continue;
        }
        node->next = NULL;
           
        if(*head == NULL){
            *head = curNode = node;
            node = NULL; /*RDKB-7130, CID-33065, make node NULL if used*/
        }else {
            if(curNode){
                curNode->next = node;
                curNode = curNode->next;
                node = NULL; /*RDKB-7130, CID-33065, make node NULL if used*/
            }
        }
        name = NULL;
        val = NULL;
        str = NULL;
    }
    if(fclose(fp) != SUCCESS){
        sprintf(ulog_msg, "%s: File Close Error !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_FILE_CLOSE_FAIL;
    }
    
    return SUCCESS;
}

void free_paramList(param_node *head){
    param_node *p = NULL;
    while(head){
        p = head->next;
        free(head);
        head = p;
    }
    head = NULL;
}

int getMac(char * macAddress, int len, unsigned char * mac)
{
    char byte[3];
    int i, index, mc;

    if (macAddress == NULL){
        sprintf(ulog_msg, "%s: Invalid Input Parameter !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_PARAM;
    }
    if ((int)strlen(macAddress) < MIN_MAC_LEN){
        sprintf(ulog_msg, "%s: Invalid MAC Address Length!!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_PARAM;
    }
    memset(mac, 0, MAC_SZ);
    byte[2] = '\0';
    i = index = mc = 0;

    while (macAddress[i] != '\0')
    {
	byte[0] = macAddress[i];
	byte[1] = macAddress[i+1];

	if (sscanf(byte, "%x", &mc) != 1){
            sprintf(ulog_msg, "%s: Invalid MAC Address!!!", __FUNCTION__);
	    ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	    return ERR_INVALID_PARAM;
        }
	mac[index] = mc;
	i+=2;
	index++;
	if (macAddress[i] == ':' || macAddress[i] == '-'  || macAddress[i] == '_')
            i++;
    }

    if ((len && index != MAC_SZ)||(!len && index > MAC_SZ)){
	sprintf(ulog_msg, "%s: Invalid Mac Length !!!", __FUNCTION__);
	ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
	return ERR_INVALID_PARAM;
    }

    return SUCCESS;
}

int getHex(char *hex_val, unsigned char *hexVal, int hexLen)
{
    char byte[3];
    char str[MAX_HEX_LEN+1] = {'\0'};
    int i, index, val, j;

    if (hex_val == NULL){
        sprintf(ulog_msg, "%s: Invalid Input Parameter !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return -1;
    }
    i = j = index = val = 0;
    memset(hexVal, 0, hexLen);
    if(hexLen == 8){
       if(strstr(hex_val, "0x")!= NULL)
           i = 2;
       if(strlen(hex_val+i)< MAX_HEX_LEN){
           j = MAX_HEX_LEN - strlen(hex_val+i);
           for( ; j>0; j--)
              str[j-1] = '0'; 
       }
       strncat(str, hex_val+i, strlen(hex_val)-i);
       strncpy(hex_val, str, strlen(str));
       hex_val[strlen(str)] = '\0';
    }
    i = 0;
    while (hex_val[i] != '\0')
    {
	byte[0] = hex_val[i];
	byte[1] = hex_val[i+1];
	byte[2] = '\0';
	if (sscanf(byte, "%x", &val) != 1)
	    break;
	hexVal[index] = val;
	i+=2;
	index++;
	if (hex_val[i] == ':' || hex_val[i] == '-'  || hex_val[i] == '_')
	    i++;
    }
    if(index != hexLen){
        sprintf(ulog_msg, "%s: Error in hex length!!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_PARAM;
    }

    return SUCCESS;
}

int getHexGeneric(char *hex_val, unsigned char *hexVal, int hexLen)
{
    char byte[3];
    char str[MAX_HEX_LEN+1] = {'\0'};
    int i, index, val;

    if (hex_val == NULL){
        sprintf(ulog_msg, "%s: Invalid Input Parameter !!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return -1;
    }
    i = index = val = 0;
    memset(hexVal, 0, hexLen);
    while (hex_val[i] != '\0')
    {
        byte[0] = hex_val[i];
        byte[1] = hex_val[i+1];
        byte[2] = '\0';
        if (sscanf(byte, "%x", &val) != 1)
            break;
        hexVal[index] = val;
        i+=2;
        index++;
        if (hex_val[i] == ':' || hex_val[i] == '-'  || hex_val[i] == '_')
            i++;
    }
    if(index != hexLen){
        sprintf(ulog_msg, "%s: Error in hex length!!!", __FUNCTION__);
        ulog_error(ULOG_CONFIG, UL_UTAPI, ulog_msg);
        return ERR_INVALID_PARAM;
    }

    return SUCCESS;
}

