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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <net/route.h>
#include "util.h"
#include "errno.h"

int vsystem(const char *fmt, ...)
{
    char cmd[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    if (n < 0 || n >= sizeof(cmd))
        return -1;

    fprintf(stderr, "%s: %s\n", __FUNCTION__, cmd);
    return system(cmd);
}

int sysctl_iface_set(const char *path, const char *ifname, const char *content)
{
    /* _sysctl(2) is not encouraged to use, write /proc/sys directly
     * also avoid to use sysctl(3) or echo for performance */
    FILE *fp;
    char file[256];

    if (ifname)
        snprintf(file, sizeof(file), path, ifname);
    else
        snprintf(file, sizeof(file), path);

    if (strncmp(file, "/proc/sys/", strlen("/proc/sys/") != 0)) {
        fprintf(stderr, "%s: not /proc/sys/ dir\n", __FUNCTION__);
        return -1;
    }

    if ((fp = fopen(file, "wb")) == NULL) {
        fprintf(stderr, "%s: cannot open file %s\n", __FUNCTION__, file);
        return -1;
    }

    if (fwrite(content, strlen(content), 1, fp) != 1) {
        fprintf(stderr, "%s: fail to write %s\n", __FUNCTION__, content);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    //fprintf(stderr, "%s: file %s content %s\n", __FUNCTION__, file, content);
    return 0;
}

int iface_get_hwaddr(const char *ifname, char *mac, size_t size)
{
    int sockfd;
    struct ifreq ifr;
    unsigned char *ptr;

    if (!ifname || !mac || size < sizeof("00:00:00:00:00:00"))
        return -1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) 
	{
		if (ENODEV == errno)
       	{
        	fprintf(stderr, "%s interface is not present, cannot get MAC Address\n", ifname);
        }
        else
        {
        	fprintf(stderr, "%s interface is present, but got an error:%d while getting MAC Address\n", 
					ifname, errno);
        }
        perror("ioctl");
        close(sockfd);
        return -1;
    }

    ptr = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    snprintf(mac, size, "%02x:%02x:%02x:%02x:%02x:%02x",
            ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

    close(sockfd);
    return 0;
}

int iface_get_ipv4addr (const char *ifname, char *ipv4Addr, size_t size)
{
	int l_iSock_Fd;
    struct ifreq l_sIfReq;

    if (!ifname || !ipv4Addr || size < sizeof("000.000.000.000"))
	{
		fprintf(stderr, "Invalid input parameters for iface_get_ipv4addr function !!!\n");
        return -1; 
	}

    if ((l_iSock_Fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		fprintf(stderr, "Error opening socket while getting the IPv4 Address of interface:%s\n", ifname);
        return -1; 
    }

    snprintf(l_sIfReq.ifr_name, sizeof(l_sIfReq.ifr_name), "%s", ifname);
    if (ioctl(l_iSock_Fd, SIOCGIFADDR, &l_sIfReq) == -1) 
    {   
        if (ENODEV == errno)
        {
            fprintf(stderr, "%s interface is not present, cannot get IPv4 Address\n", ifname);
        }   
        else
        {   
            fprintf(stderr, "%s interface is present, but got an error:%d while getting IPv4 Address\n", 
                    ifname, errno);
        }   
        close(l_iSock_Fd);
        return -1; 
    }   

	strncpy(ipv4Addr, inet_ntoa(((struct sockaddr_in *)&l_sIfReq.ifr_addr)->sin_addr), size);
    close(l_iSock_Fd);
    return 0;	
}

int is_iface_present(const char *ifname)
{
	int l_iSock_Fd;
    struct ifreq l_sIfReq;

    if (!ifname)
	{
		fprintf(stderr, "Interface name is empty !!!\n");
        return 0; 
	}

    if ((l_iSock_Fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)  
    {   
        fprintf(stderr, "Error opening socket while checking whether interface:%s is present or not\n", ifname);
        return 0; 
    }   

    snprintf(l_sIfReq.ifr_name, sizeof(l_sIfReq.ifr_name), "%s", ifname);
    if (ioctl(l_iSock_Fd, SIOCGIFADDR, &l_sIfReq) == -1) 
    {   
        if (ENODEV == errno)
        {
            fprintf(stderr, "%s interface is not present, cannot get IPv4 Address\n", ifname);
    		close(l_iSock_Fd);
			return 0;
        }
        else
        {
            fprintf(stderr, "%s interface is present, but got an error:%d while getting IPv4 Address\n", 
                    ifname, errno);
        }
        close(l_iSock_Fd);
        return 0; 
    }   
    close(l_iSock_Fd);
    return 1;	
}

int pid_of(const char *name, const char *keyword)
{
    DIR *dir;
    struct dirent *d;
    int pid;
    FILE *fp;
    char path[275];
    char line[257];
    char *cp;
    int n, i;

    if ((dir = opendir("/proc")) == NULL)
        return -1;

    while ((d = readdir(dir)) != NULL) {
        pid = atoi(d->d_name);
        if (pid <= 0)
            continue;

        snprintf(path, sizeof(path), "/proc/%s/comm", d->d_name);
        if ((fp = fopen(path, "rb")) == NULL)
            continue;
        if (fgets(line, sizeof(line), fp) == NULL) {
            fclose(fp);
            continue;
        }
        if ((cp = strrchr(line, '\n')) != NULL)
            *cp = '\0';
        if (strcmp(line, name) != 0) {
            fclose(fp);
            continue;
        }
        fclose(fp);

        if (keyword) {
            snprintf(path, sizeof(path), "/proc/%s/cmdline", d->d_name);
            if ((fp = fopen(path, "rb")) == NULL)
                continue;

            /* we assume command line is shorter then sizeof(line) */
            if ((n = fread(line, 1, sizeof(line) - 1, fp)) <= 0) {
                fclose(fp);
                continue;
            }
            fclose(fp);

            for (i = 0; i < n; i++)
                if (line[i] == '\0')
                    line[i] = ' ';
            line[n] = '\0';

            if (strstr(line, keyword) == NULL)
                continue;
        }

        closedir(dir);
        return pid;
    }

    closedir(dir);
    return -1;
}

int serv_can_start(int sefd, token_t setok, const char *servname)
{
    char st_name[64];
    char status[16];

    snprintf(st_name, sizeof(st_name), "%s-status", servname);
    sysevent_get(sefd, setok, st_name, status, sizeof(status));

    if (strcmp(status, "starting") == 0 || strcmp(status, "started") == 0) {
        fprintf(stderr, "%s: service %s has already %s !\n", __FUNCTION__, servname, status);
        return 0;
    } else if (strcmp(status, "stopping") == 0) {
        fprintf(stderr, "%s: service %s cannot start in status %s !\n", __FUNCTION__, servname, status);
        return 0;
    }

    return 1;
}

int serv_can_stop(int sefd, token_t setok, const char *servname)
{
    char st_name[64];
    char status[16];

    snprintf(st_name, sizeof(st_name), "%s-status", servname);
    sysevent_get(sefd, setok, st_name, status, sizeof(status));

    if (strcmp(status, "stopping") == 0 || strcmp(status, "stopped") == 0) {
        fprintf(stderr, "%s: service %s has already %s !\n", __FUNCTION__, servname, status);
        return 0;
    } else if (strcmp(status, "starting") == 0) {
        fprintf(stderr, "%s: service %s cannot stop in status %s !\n", __FUNCTION__, servname, status);
        return 0;
    }
 
    return 1;
}
