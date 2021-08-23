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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "syscfg/syscfg.h"
#include "sysevent/sysevent.h"
#include "autoconf.h"
#include "safec_lib_common.h"
#define _XOPEN_SOURCE
#include <time.h>
//#define _NO_MMAP__
#ifndef CONFIG_CISCO_PARCON_WALLED_GARDEN
 #define CONFIG_CISCO_PARCON_WALLED_GARDEN
#endif

#define TIME_SIZE 25
#define DESP_SIZE 100
#define ACTION_SIZE 25
#define KEY_SIZE 32
#define LOG_FILE_COUNT_MAX 30
#define LOCK_ATTEMP_TIME 10

#define RULE_NUM 10 
#define LOG_TIME_SIZE 20 
#define TEMP_FILE   "/tmp/.ipt_rule"
#define TEMP_LOG_LIST "/tmp/.log_list"
#define IPT_COUNT_CMD "iptables -L -n -v "
#define IPT_NAT_COUNT_CMD "iptables -t nat -L -n -v "
#define IP6T_COUNT_CMD "ip6tables -L -n -v "
#define IP6T_NAT_COUNT_CMD "ip6tables -t nat -L -n -v "
//#define FIREWALL_LOG_DIR "/nvram/log/firewall"
char FIREWALL_LOG_DIR[50];
#define LOCK_FILE_NAME "/tmp/.fw_lock"
#define ORG_LOG_NAME_1  "/var/log/kernel"
#define ORG_LOG_NAME_2  "/var/log/kernel.0"
#define FW_ORG_LOG_NAME "/tmp/.fw_org" 

#define STRING_SITE "Site: "
#define STRING_KEYWORD "Keyword match: "
#define STRING_SERVICE_PORT "PORT:"
#define STRING_DEVICE "Device MAC:"

#define STRING_DEVICE_BLOCKED "LOG_DeviceBlocked_"
#define STRING_SITE_BLOCKED "LOG_SiteBlocked_"
#define STRING_SERVICE_BLOCKED "LOG_ServiceBlocked_"
#define STRING_DEVICE_BLOCKED_ALL "LOG_DeviceBlocked_DROP"
#define STRING_URL "host match "

char *strptime(const char *s, const char *format, struct tm *tm);

//#define NEED_CLEAN_LOG 1
const char SHORT_LOG_PREFIX[2][10] = {
    "LOG_",
    "UTOPIA:"
};

typedef struct rule_info {
    char time[TIME_SIZE];
    char desp[DESP_SIZE];
    char action[ACTION_SIZE];
    char key[KEY_SIZE];
    int count;
}rule_info_t; 

typedef struct ipt_rule{
    int pkt;
    int byte;
    char target[ACTION_SIZE];
    char prot[10];
    char opt[50];
    char in[50];
    char out[50];
    char src[50];
    char dest[50];
    char others[50];
}ipt_rule_t;

typedef struct action_info{
    char action_name[25];
    int (*get_desp)(char* line, char* desp, char *key);
}action_info_t;

struct tm *g_ptime;
time_t t;
rule_info_t *g_p_rule_tbl = NULL;
int g_rule_tlb_len = 0;

int _memcpy_2_graph(char *d, char *s, size_t count);
#define printf(x, argv...) 

int fLock(){
    int fd = -1;
    struct flock fl; 
    int i = 0; 
    fl.l_type = F_WRLCK;  
    fl.l_whence = SEEK_SET;  
    fl.l_start = 0;  
    fl.l_len = 0;  
    fl.l_pid = -1;  

    if((fd=open(LOCK_FILE_NAME,O_CREAT| O_RDWR, S_IRUSR | S_IWUSR)) != -1){ /*RDKB-7143, CID-33171; validate open */
        do{
            if( -1  != fcntl(fd,F_SETLK,&fl)){
                printf("GET W-LOCK SUCCESS\n");
                return fd;
            }
            usleep(500);
            i++;
            printf("try to lock file\n");
        }while(i < LOCK_ATTEMP_TIME);
    }
    if(i == LOCK_ATTEMP_TIME)
        close(fd);
    return -1;
}

void fUnlock(int fd){
    struct flock fl;  
    fl.l_type = F_UNLCK;  
    fl.l_whence = SEEK_SET;  
    fl.l_start = 0;  
    fl.l_len = 0;  
    fl.l_pid = -1;  

    fcntl(fd,F_SETLK,&fl);
    close(fd);
    printf("UN-LOCK SUCCESS\n");
    return;    
}

int _get_key(char* line, char* key, char* prefix){
    char *action;
    int   count;
    if(NULL != (action = strstr(line, prefix))){
        count = _memcpy_2_graph(key, action, KEY_SIZE -1);
        key[count] = '\0';
        return 0;
    }else{
        return -1;
    }
}

int _memcpy_2_chr(char *d, char *s, size_t count, char c){
    char *end = strchr(s, c);
    if(end == NULL || end -s == 0 || end -s > count)
        return 0;
    memcpy(d, s, end - s);
    return (end-s);
}

int _memcpy_quotes(char *d, char *s, size_t count){
    char *start;
    char *end;
    start = strchr(s, '\"');
    if(start != NULL){
        end = strchr(start + 1, '\"');
        if(end != NULL && end -start <= count && end -start > 1){
            memcpy(d, start + 1, (end-start-1));
            return (end -start-1);
        }
    }
    return 0;
}

int _memcpy_2_graph(char *d, char *s, size_t count){
    unsigned int i = 0;
    while(isgraph(*s)){
        *(d++) = *(s++);
        i++;
        if(count <= i){
            break;
        }
    }
    return i;   
}

char* _move_space(char *line){
    while(isspace(*line))
        line++;
    return line;
}
char* rev_getline_head(char* end, int len)
{
    /* ignor the '\n' at the end of string */
    while(len-- > 0 && *(end--) == '\n');
    while(len-- > 0 && *(end--) != '\n');
    if(len <= -2)
        return NULL;
    if(*(++end) == '\n')
        return ++end;
    else
        return end;
}

char* strstr_len(char *haystack, int count, char *needle){
    int i, j, len;
    len = strlen(needle);
    count -= len;
    for(i = 0; i <= count  ;i++)
    {
        for(j = 0;;)
            if(haystack[i+j] != needle[j])break;
            else if(++j == len)return (haystack+i);
    }
    return NULL;
}

#if 0
int _anlz_ipt_rule(char* line, ipt_rule_t *ipt){

#define GET_IPT_RULE_ELEM(var, line)   {\
    int tmp; \
    line = _move_space(line);  \
    if(0 == (tmp = _memcpy_2_chr(var, line, sizeof(var), ' ')))  \
        return -1;  \
    line += tmp; \
}  

    char *endptr = NULL;
    ipt->pkt = strtol(line, &endptr, 0);
    if(endptr == line)
        return -1;
    line = endptr;
    endptr = NULL;
    ipt->pkt = strtol(line, &endptr, 0);
    if(endptr == line)
        return -1;
    line = endptr;
    
    GET_IPT_RULE_ELEM(ipt->target, line)
    GET_IPT_RULE_ELEM(ipt->prot, line)
    GET_IPT_RULE_ELEM(ipt->opt, line)
    GET_IPT_RULE_ELEM(ipt->in, line)
    GET_IPT_RULE_ELEM(ipt->out, line)
    GET_IPT_RULE_ELEM(ipt->src, line)
    GET_IPT_RULE_ELEM(ipt->dest, line)
    GET_IPT_RULE_ELEM(ipt->others, line)

}
#endif

int SiteBlocked_func(char *line, char *desp, char *key){
    char *c;
    int tmp;
    errno_t safec_rc = -1;

    if(-1 == _get_key(line, key, STRING_SITE_BLOCKED))
        return -1;

    if(NULL != (c = strstr(line, "STRING match"))){
        c += strlen("STRING match");
        // Here desp is pointer, It's pointing to the array size is DESP_SIZE
        safec_rc = strcpy_s(desp, DESP_SIZE, STRING_KEYWORD);
        ERR_CHK(safec_rc);
        tmp = _memcpy_quotes(desp + strlen(STRING_KEYWORD), c, DESP_SIZE - strlen(STRING_KEYWORD) - 1);
        if(tmp != 0){
            tmp += strlen(STRING_KEYWORD);
            desp[tmp] = '\0';
            return 0; 
        } 
    }
    else if(NULL != (c = strstr(line, STRING_URL))){
        int tmp;
        // Here desp is pointer, It's pointing to the array size is DESP_SIZE
        safec_rc = strcpy_s(desp, DESP_SIZE ,STRING_SITE);
        ERR_CHK(safec_rc);
        tmp = _memcpy_2_graph(desp + strlen(STRING_SITE), c + strlen(STRING_URL), DESP_SIZE - strlen(STRING_SITE) -1);
        if(tmp != 0){
            tmp += strlen(STRING_SITE);
            desp[tmp] = '\0';
            return 0;
        }
    }
    return -1;
}

int DeviceBlocked_func(char *line, char *desp, char *key){
    char *c;
    int tmp;
    errno_t safec_rc = -1;

    if(-1 == _get_key(line, key, STRING_DEVICE_BLOCKED))
        return -1;

    if( NULL != strstr(key, STRING_DEVICE_BLOCKED_ALL)){
        // Here desp is pointer, It's pointing to the array size is DESP_SIZE
        safec_rc = strcpy_s(desp, DESP_SIZE,"Blocked Unallowed Devices");
        ERR_CHK(safec_rc);
        safec_rc = strcpy_s(key, KEY_SIZE,STRING_DEVICE_BLOCKED_ALL);
        ERR_CHK(safec_rc);
        return 0;
    }else if(NULL != (c = strstr(line, "MAC "))){
        c += strlen("MAC ");
        memcpy(desp, STRING_DEVICE, sizeof(STRING_DEVICE) - 1);
        tmp = _memcpy_2_graph(desp + strlen(STRING_DEVICE), c, DESP_SIZE - strlen(STRING_DEVICE) -1);
        if(tmp != 0){
            tmp += strlen(STRING_DEVICE);
            desp[tmp] = '\0';
            return 0;
        }
    }
    return -1; 
}

int FW_block_func(char *line, char *desp, char *key){
    char *c;
    int tmp;
    char *kpos;
    char *fwpos;
    char *despos;
    char *end;
    
    if((NULL != (c = strstr(line, "prefix"))) && \
       (NULL != (kpos = strstr(c+sizeof("prefix"), "UTOPIA:"))) &&
       (NULL != (fwpos = strstr(kpos + sizeof("UTOPIA:"), "FW.")))){

        if( NULL != (despos = strstr(fwpos+sizeof("FW.")-1, "IPv6"))){
            if(NULL != (despos = strchr(despos+sizeof("IPv6")-1, '\"'))){
                end = despos;
            }else{
                return -1;
            }
        }else if( NULL != (despos = strstr(fwpos+sizeof("FW.")-1, "DROP"))){
            end = despos + sizeof("DROP") - 1;
        }else if( NULL != (despos = strstr(fwpos+sizeof("FW.")-1, "ACCEPT"))){
            end = despos + sizeof("ACCEPT") - 1;
        }else if( NULL != (despos = strstr(fwpos+sizeof("FW.")-1, "REJECT"))){
            end = despos + sizeof("REJECT") - 1;
        }else
            return -1;

        tmp = end - kpos;
        if(tmp < KEY_SIZE){
            memcpy(key, kpos, tmp);
            key[tmp] = '\0';
        }else
            return -1;

        tmp = end -fwpos;
        if(tmp < DESP_SIZE){
            memcpy(desp, fwpos, tmp);
            desp[tmp] = '\0';
            return 1;
        }else
            return -1;
    }
    return -1;    
}

int ServiceBlocked_func(char *line, char *desp, char *key){
    char *c;
    int tmp;

    if(-1 == _get_key(line, key, STRING_SERVICE_BLOCKED))
        return -1;

    if(NULL != (c = strstr(line, "dpts:"))){
        c += strlen("dpts:");
    }else if(NULL != (c = strstr(line, "dpt:"))){
        c += strlen("dpt:");
    }else{
        return -1;
    }
   
    memcpy(desp, STRING_SERVICE_PORT, strlen(STRING_SERVICE_PORT));
    tmp = _memcpy_2_graph(desp + strlen(STRING_SERVICE_PORT), c, DESP_SIZE - strlen(STRING_SERVICE_PORT) - 1);

    if(tmp != 0){
        tmp += strlen(STRING_SERVICE_PORT);
        desp[tmp] = '\0';
        return 0; 
    }

    return -1;
}

action_info_t action_arry[] = {
    { "Device Blocked", DeviceBlocked_func},
    { "Service Blocked", ServiceBlocked_func},
    { "Site Blocked", SiteBlocked_func},
    { "Firewall Blocked", FW_block_func},
    { "", (void*)NULL}
};

int get_count(char* line){
    char *endptr;
    int count;
    count = strtol(line, &endptr, 0);
    if(endptr == line || count == 0){
        return 0;
    }else
        return count;
}

int anlz_rule(char* line, rule_info_t *info){
    int i = 0;
    int count;
    int ret;
    int get_count_flag = 0;
    static int flag = 0;
    static rule_info_t tmp;
    errno_t safec_rc = -1;
#define ARF_GET_NEXT_COUNT 1

    memset(info, 0, sizeof(rule_info_t));
    if(flag == ARF_GET_NEXT_COUNT){
        flag = 0;
        *info = tmp;
        get_count_flag = 1;
    }

    if(0 == ( count = get_count(line))){
        return -1;
    }

    if(get_count_flag){
        info->count = count;
        return 0;
    } 

    while(action_arry[i].action_name[i] != '\0'){
        if(action_arry[i].get_desp != NULL){
            ret = action_arry[i].get_desp(line, info->desp, info->key);
            if(0 == ret){
                safec_rc = strcpy_s(info->action, sizeof(info->action),action_arry[i].action_name);
                ERR_CHK(safec_rc);
                info->count = count;
                info->time[0] = '\0';
                return 0;
            }else if(1 == ret){
                /* sometime LOG rule will be limited, so the count is not corrcet */
                /* The DROP rule will be followed the LOG rule */
                /* So use the DROP rule count */
                flag = ARF_GET_NEXT_COUNT; // we need get next rule's count
                tmp = *info;
                safec_rc = strcpy_s(tmp.action, sizeof(tmp.action),action_arry[i].action_name);
                ERR_CHK(safec_rc);
                info->time[0] = '\0';
                return -1;    
            }
        }
        i++;
    }

    return -1;
}

void write_rule(FILE *fp, rule_info_t *info){
    fprintf(fp, "Count=\"%d\"; Time=\"%s\"; Action=\"%s\"; Desp=\"%s\";\n", info->count, info->time, info->action, info->desp);
    printf("Count=\"%d\"; Time=\"%s\"; Action=\"%s\"; Desp=\"%s\";\n", info->count, info->time, info->action, info->desp);
    return;
}

char* get_old(FILE *fp, char* name){
    int c;
    int flag = 0, index = 0;
    unsigned int old = 0xffffffff, cur;
    char fName[50] ;
    errno_t safec_rc = -1;
    while(EOF != (c = fgetc(fp)))
    {
        if(c == ' ' || c == '\n'){
            if(flag == 1){
                flag = 0;
                if(index != 0){
                    cur = atoi(fName);
                    old = (old > cur) ? cur : old;
                }
                index = 0;
            }
        }else{
            if(flag == 0){
                index = 0;
                flag = 1;
            }
            fName[index++] = c;
        }
    }
    if(index != 0){
        fName[index] = '\0';
        cur = atoi(fName);
        old = (old > cur) ? cur : old;
    }
    safec_rc = sprintf_s(name, 20,"%08d", old);
    if(safec_rc < EOK){
        ERR_CHK(safec_rc);
    }
    return name;
}              

#ifdef NEED_CLEAN_LOG
/* If firewall log files' count is more than LOG_FILE_COUNT_MAX, clean the oldest log */  
void clean_log(void){
    FILE *fp;
    int c;
    int count = 0;
    int flag = 0;
    char cmd[256];
    char name[20];
    system("ls /var/log/firewall >" TEMP_LOG_LIST);
    fp = fopen(TEMP_LOG_LIST, "r");
    if(fp == NULL)
        return;

    while(EOF != (c = fgetc(fp))){
        if(c == ' ' || c == '\n'){
            flag = 0;
        }else{
            if(flag == 0){
                flag = 1;
                count++;
            
                if(count > LOG_FILE_COUNT_MAX){
                    fseek(fp, 0, SEEK_SET);
                    safec_rc = sprintf_s(cmd, sizeof(cmd),"rm %s/%s", FIREWALL_LOG_DIR, get_old(fp,name));
                    if(safec_rc < EOK)
                    {
                      ERR_CHK(safec_rc);
                    }
                    printf("%s\n", cmd);
                    system(cmd);    
                    break;
                }
            }
        }
    }
    fclose(fp);
    return;
}
#else
#define clean_log()  
#endif

#define ADRF_NO_CHECK 0x00000000
#define ADRF_KEY 0x00000001
#define ADRF_ACTION 0x00000002
#define ADRF_DESP 0x00000004

void add_rule(rule_info_t *rule, int *num, int flag){
    int i = 0;
    rule_info_t *tbl = g_p_rule_tbl;
#define _ADRF_CHECK_(flag, ADRF_value, cond)  (((flag) & (ADRF_value)) ? (cond) : 1)
    /* check exsist rule */
    while( i < *num ){
        if(     _ADRF_CHECK_(flag, ADRF_KEY, (!strcmp(rule->key, tbl[i].key))) &&
                _ADRF_CHECK_(flag, ADRF_ACTION, (!strcmp(rule->action, tbl[i].action))) &&
                _ADRF_CHECK_(flag, ADRF_DESP, (!strcmp(rule->desp, tbl[i].desp))) ){
            tbl[i].count += rule->count;
            return;
        }
        i++;
    }

    if(*num >= g_rule_tlb_len){
        printf("rule table memory not enough, realloc it !!\n");
        g_rule_tlb_len += RULE_NUM;
        tbl = (rule_info_t *)realloc(tbl, sizeof(rule_info_t) * g_rule_tlb_len);
        if(tbl == NULL){
            g_rule_tlb_len = 0;
            g_p_rule_tbl = NULL;
            return ;
        }else{
            g_p_rule_tbl = tbl;
            memset(tbl + g_rule_tlb_len - RULE_NUM, 0, sizeof(rule_info_t) * RULE_NUM);
        }
    }

    tbl[*num] = *rule;
    (*num)++;
    return;
}

void merger_rule(FILE* fd, int *num){
    char *line = NULL;
    int ret = 0;
    int size = 0;
    rule_info_t info = {{0},{0},{0},{0},0}; /*RDKB-7143, CID-33545; init before use */

    while(-1 != getline(&line, &size, fd)){
        ret = sscanf(line, "Count=\"%d\"; Time=\"%[^\"]\"; Action=\"%[^\"]\"; Desp=\"%[^\"]\";\n", &(info.count), info.time, info.action, info.desp);
        if(ret == 4){
            add_rule(&info, num, ADRF_ACTION | ADRF_DESP);
        }
        free(line);
        line = NULL;
    }
    fseek(fd, 0L, SEEK_SET);
}

void get_rule_time(int count){
    char cmd[120];
    char *line = NULL;
    char today[32];
    int i = 0;
    rule_info_t *tbl = g_p_rule_tbl;
    errno_t safec_rc = -1;
#ifndef _NO_MMAP__
    int fd;
    struct stat statbuf;
    char *start, *end;
    int j = 0;
#else
    FILE *fd;
#endif
    strftime(today, sizeof(today), "%b %d", g_ptime);
    safec_rc = sprintf_s(cmd, sizeof(cmd),"grep -h -e \"%s\"  %s %s  > %s 2>/dev/null", today, ORG_LOG_NAME_2, ORG_LOG_NAME_1, FW_ORG_LOG_NAME);
    if(safec_rc < EOK)
    {
        ERR_CHK(safec_rc);
    }
//    printf("%s\n", cmd);
    system(cmd);
#ifdef _NO_MMAP__
    size_t size;
    fd = fopen(FW_ORG_LOG_NAME,"r");
    if(fd == NULL){
        remove(FW_ORG_LOG_NAME);
        return;
    }
    while(-1 != getline(&line, &size, fd)){
        i = 0;
        while(i < count){
            if(0 != strstr(line, tbl[i].key)){
                memcpy(tbl[i].time, line,LOG_TIME_SIZE);
                tbl[i].time[LOG_TIME_SIZE] = '\0';
                printf("Get [%s] time: [%s]\n", tbl[i].key, tbl[i].time);
            }
            i++;        
        }
        free(line);
        line = NULL; 
    }
#else
    fd = open(FW_ORG_LOG_NAME,O_RDONLY);
    if(fd < 0){
        remove(FW_ORG_LOG_NAME);
        return;
    }
    if(fstat(fd, &statbuf) < 0){
        printf("fstat error \n");
        goto END;
    }
    if((start = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED){
        printf("mmap error \n");
        goto END;
    }
    end = start + statbuf.st_size -1;
    line = end;
    j = 0;
    while(NULL != (line = rev_getline_head(line, line - start +1))){
        i = 0;
        while(i < count){
            if(0 != strstr_len(line, end - line + 1, tbl[i].key)){

               if(tbl[i].time[0] == 0){

                    // handle kernel timestamp in different formats
                    char string[TIME_SIZE];
                    char *cp;
                    struct tm result;

                    memset(tbl[i].time, '\0', TIME_SIZE);
                    memset(string, '\0', TIME_SIZE);
                    strncpy(string, line, LOG_TIME_SIZE);

                    // some kernel log has timestamp of Aug 23 18:37:42 2018
                    cp = (char *)strptime(string, "%b %d %T %Y", &result);
                    if (cp != NULL) {
                        // generate timestamp in standard format
                        strftime(tbl[i].time, TIME_SIZE, "%b %d %T %Y", &result);
                        fprintf(stderr, "Get [%s] time: [%s]\n", tbl[i].key, tbl[i].time);
                    }
                    else {
                        // some kernel log has timestamp of Aug 23 18:37:42.123456
                        // only take timestamp up to Aug 23 18:37:42
                        // then append today's year
                        cp = (char *)strptime(string, "%b %d %T", &result);
                        if (cp != NULL) {
                            time_t rawtime;
                            struct tm *tminfo;
                            time(&rawtime);
                            tminfo = localtime(&rawtime);
                            result.tm_year = tminfo->tm_year;
                            // generate timestamp in standard format
                            strftime(tbl[i].time, TIME_SIZE, "%b %d %T %Y", &result);
                            fprintf(stderr, "Get [%s] time: [%s] (year appended)\n", tbl[i].key, tbl[i].time);
                        }
                        else {
                            fprintf(stderr, "Get invalid time: [%s]\n", tbl[i].key);
                        }
                    }

                    if(++j >= count)       
                        goto OUT;
               }
               break;
            }
           i++; 
        }
        line--;
        end = line;
    }
OUT:
    munmap(start, statbuf.st_size);
//    close(fd);

#endif
END:
#ifdef _NO_MMAP__
    fclose(fd);
#else
    close(fd);
#endif
    remove(FW_ORG_LOG_NAME);
    return;
}
/* 
 *  -nz get the firewall log but not clear the iptables count
 *  -Z  get the firewall log and zero the iptables count
 *  -c  put iptabls count in to a file
 *  -gc get the firewall log from the file generated from flag -c 
 */
int main(int argc, char** argv){
    FILE *ipt;
    FILE *log;
    int  lock;
    char fName[80];
    char *line = NULL;
    size_t size;
    int opt = 0;
    rule_info_t rule;
    char *iptables_flage = "";
    char cmd[128];
    int i=0;
    char *fFlag = "w";
    char temp[100];
    int            sysevent_fd = -1;
    char          *sysevent_name = "GenFWLog";
    token_t        sysevent_token;
    unsigned short sysevent_port;
    char           sysevent_ip[19];
    errno_t        safec_rc = -1;

    t=time(NULL);
    g_ptime=localtime(&t);

    snprintf(sysevent_ip, sizeof(sysevent_ip), "127.0.0.1");
    sysevent_port = SE_SERVER_WELL_KNOWN_PORT;
    sysevent_fd =  sysevent_open(sysevent_ip, sysevent_port, SE_VERSION, sysevent_name, &sysevent_token);
    if(sysevent_fd  < 0){
        printf("GenFWLog: Init sysevent error\n");
        exit(1);
    }
     
    memset(FIREWALL_LOG_DIR, 0, sizeof(FIREWALL_LOG_DIR));
    sysevent_get(sysevent_fd, sysevent_token, "FW_LOG_FILE_PATH_V2", FIREWALL_LOG_DIR, sizeof(FIREWALL_LOG_DIR));
    if(FIREWALL_LOG_DIR[0] == '\0' ){
        syscfg_init();
        syscfg_get(NULL, "FW_LOG_FILE_PATH", FIREWALL_LOG_DIR, sizeof(FIREWALL_LOG_DIR));
        if(FIREWALL_LOG_DIR[0] == '\0' ){
           printf("Not get fw log path\n");
           exit(1);
        }
    }
    sysevent_close(sysevent_fd, sysevent_token);

    if(argc == 2 && !strcmp(argv[1], "-nz")){
        /* Don't clear iptables count */
        safec_rc = sprintf_s(fName, sizeof(fName),"%s/99999999", FIREWALL_LOG_DIR);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
		 opt = 3;
    }else if(argc == 2 && !strcmp(argv[1], "-c")){
        /* recoder iptabls rule and count in TEMP_FILE, but not generate log file */
        opt = 1;    
    }else{
        /* Clear iptables count */
        iptables_flage = "-Z";
        fFlag = "r+";
        safec_rc = sprintf_s(fName, sizeof(fName),"%s/%04d%02d%02d", FIREWALL_LOG_DIR, 1900 + g_ptime->tm_year, 1 + g_ptime->tm_mon, g_ptime->tm_mday);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        if(argc == 2 && !strcmp(argv[1], "-gc")){
            opt = 2;    
        }
    }

    if( -1 == access(FIREWALL_LOG_DIR, 0))
    {
        safec_rc = sprintf_s(temp, sizeof(temp),"mkdir -p %s", FIREWALL_LOG_DIR);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
         system(temp);
    }
    
    g_p_rule_tbl = NULL;
    g_rule_tlb_len = 0;

    lock = fLock();
    if(lock < 0 ){
        exit(0);
    }

    if(opt != 2){
        safec_rc = sprintf_s(cmd, sizeof(cmd),"%s%s > %s", IPT_COUNT_CMD, iptables_flage, TEMP_FILE);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        printf("%s\n",cmd);
        system(cmd);
#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
        safec_rc = sprintf_s(cmd, sizeof(cmd),"%s%s >> %s", IPT_NAT_COUNT_CMD , iptables_flage, TEMP_FILE);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        printf("%s\n",cmd);
        system(cmd);
#endif
        safec_rc = sprintf_s(cmd, sizeof(cmd),"%s%s >> %s", IP6T_COUNT_CMD, iptables_flage, TEMP_FILE);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        printf("%s\n",cmd);
        system(cmd);
#if defined (CONFIG_CISCO_PARCON_WALLED_GARDEN) && defined(_HUB4_PRODUCT_REQ_)
        safec_rc = sprintf_s(cmd, sizeof(cmd),"%s%s >> %s", IP6T_NAT_COUNT_CMD , iptables_flage, TEMP_FILE);
        if(safec_rc < EOK)
        {
            ERR_CHK(safec_rc);
        }
        printf("%s\n",cmd);
        system(cmd);
#endif
    }
    
    if(opt == 1){
        fUnlock(lock);
        exit(0);
    }

    ipt = fopen(TEMP_FILE, "r");
    if(ipt == NULL){
        fUnlock(lock);
        exit(1);
    }

      
    while(-1 != getline(&line, &size, ipt)){
        if(0 == anlz_rule(line, &rule)){
            add_rule(&rule, &i, ADRF_KEY);
        }
        free(line);
        line = NULL;
    }
    

    if( i == 0 ){
        fclose(ipt);
        remove(TEMP_FILE);
        fUnlock(lock);
        exit(0);
    }

    log = fopen(fName, fFlag);
    if(log == NULL){
        if(NULL == (log = fopen(fName, "w+"))){
            fclose(ipt);
            remove(TEMP_FILE);
            fUnlock(lock);
            exit(1);
        }
    }        

    get_rule_time(i);
    if(opt != 3){
        merger_rule(log, &i);
    }
    
    while((--i) >= 0){
        if(g_p_rule_tbl[i].time[0] == 0){
            /* If we can't find occur time use local time */
            strftime(g_p_rule_tbl[i].time, TIME_SIZE, "%b %d %T %Y", g_ptime); 
        }
        write_rule(log, &g_p_rule_tbl[i]);
    }


    fclose(log);
    fclose(ipt);
    clean_log();
    remove(TEMP_FILE);
    fUnlock(lock);
    if(g_rule_tlb_len != 0)
        free(g_p_rule_tbl);

    exit(0);
}

