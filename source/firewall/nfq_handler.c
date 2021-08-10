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
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <linux/ip.h>
#include <netinet/ether.h> 
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "secure_wrapper.h"
#include "safec_lib_common.h"

#define _NFQ_DEBUG_LEVEL 0
#define PARCON_IP_PATH "/var/parcon/"


typedef int (*nfq_cb)(struct nfq_q_handle *, struct nfgenmsg *, struct nfq_data *, void *);

typedef struct nfq_cfg
{
    char mode[32];
    unsigned char qnum_start;
    unsigned char qnum_end;
    nfq_cb callback;
} nfq_cfg;

void send_tcp_pkt(char *interface, int family, char *srcMac, char *dstMac, char *srcIp, char *dstIp, unsigned short srcPort, unsigned short dstPort, unsigned long seqNum, unsigned long ackNum, char *url, unsigned char fin);

char srcMac[20];

struct __attribute__((__packed__)) dns_header
{
    __u16 id;
    __u16 flags;
    __u16 questions;
    __u16 answerRR;
    __u16 authRR;
    __u16 extraRR;
};

struct __attribute__((__packed__)) dns_answer
{
    __u16 offset;   //TO-DO only support fully compressed name format
    __u16 type;
    __u16 class;
    __u32 ttl;
    __u16 len;
};

typedef enum {
    BLOCK_BY_URL,
    BLOCK_BY_KEYWD
} block_type;

static char *get_dns_header(char *payload)
{
    //Use kernel iphdr/udphdr struct to parse the packet
    struct iphdr *ipHdr = (struct iphdr *)(payload);
    struct udphdr *udpHdr;
    if(ipHdr->version == 4){
        udpHdr = (struct udphdr *)(payload + ((struct iphdr*)payload)->ihl * 4);
    }else{
        udpHdr = (struct udphdr *)(payload + sizeof(struct ip6_hdr));
    }
    return (char *)udpHdr + sizeof(struct udphdr);
}

//Get pointer to the start of url in the packet assuming there is only one question
//Note: Neither DjbDNS, BIND, nor MSDNS support queries where QDCOUNT > 1
#if _NFQ_DEBUG_LEVEL >= 1
static int get_query_url(char *url, char* dnsData)
{
    char *ptr = url;
    unsigned char l;
    int i, len = 0;

    //Convert the url hex data to string
    //03www05cisco03com -> www.cisco.com
    while(*dnsData != 0) {
        l = *dnsData;
        len += l+1;

        ++dnsData;

        for(i = 0; i < l; ++i, ++ptr, ++dnsData)
            *ptr = *dnsData;

        *ptr = '.';
        ++ptr;
    }
    *(--ptr) = '\0';

    return len + 1; //add the last 00 octet
}
#endif

static int get_query_url_length(char* dnsData)
{
    unsigned char l;
    int i, len = 0;

    while(*dnsData != 0) {
        l = *dnsData;
        len += l+1;

        ++dnsData;

        for(i = 0; i < l; ++i, ++dnsData)
            ;
    }

    return len + 1; //add the last 00 octet
}

//moniter dns query to get the IP of specific MAC
void handle_dns_query(struct nfq_data *pkt)
{
    char mac[64], ipAddr[32], saddr[32];
#if _NFQ_DEBUG_LEVEL == 1
    char cmd[256];
#endif
    char *payload;
    FILE *mac2Ip;
    
    struct nfqnl_msg_packet_hw *macAddr = nfq_get_packet_hw(pkt);
    int len = nfq_get_payload(pkt, (unsigned char **)&payload);
    struct iphdr *ipHdr = ((struct iphdr*)payload);
    unsigned char *srcIp = (unsigned char*)&ipHdr->saddr;
    
    uint32_t mark = nfq_get_nfmark(pkt);
    uint32_t insNum = mark;

    if(macAddr != NULL)
        snprintf(mac, sizeof(mac), PARCON_IP_PATH"%02x:%02x:%02x:%02x:%02x:%02x", \
                macAddr->hw_addr[0], macAddr->hw_addr[1], macAddr->hw_addr[2], \
                macAddr->hw_addr[3],macAddr->hw_addr[4], macAddr->hw_addr[5]);
    else {
        fprintf(stderr, "nfq_handler: no MAC address found in %s\n", __FUNCTION__);
        return;
    }

    if(len > 0) {
        snprintf(saddr, sizeof(saddr), "%u.%u.%u.%u", srcIp[0], srcIp[1], srcIp[2], srcIp[3]);

        if((mac2Ip = fopen(mac, "r")) != NULL) {
            fgets(ipAddr, sizeof(ipAddr), mac2Ip);
            if(strcmp(ipAddr, saddr) == 0) {
                fclose(mac2Ip);
                return;
            }
            fclose(mac2Ip); /*RDKB-7144, CID-33078, free resource after use*/
        }
#if _NFQ_DEBUG_LEVEL == 1
        printf("\nsyncing ip address of deivce_%u\n", insNum);
#endif
        if((mac2Ip = fopen(mac, "w")) != NULL) /*RDKB-7144, CID-33323, free resource after use*/
        {
            fprintf(mac2Ip, "%u.%u.%u.%u\n", srcIp[0], srcIp[1], srcIp[2], srcIp[3]);
            fclose(mac2Ip);
        }
#if _NFQ_DEBUG_LEVEL == 1
        printf("system: iptables -F pp_disabled_%u\n", insNum);
#endif
        v_secure_system("iptables -F pp_disabled_%u", insNum);

#if _NFQ_DEBUG_LEVEL == 1
        printf("system: iptables -A pp_disabled_%u -d %s -p tcp -m multiport --sports 80,443 -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir reply --connbytes-mode packets -j GWMETA --dis-pp\n", insNum, ipAddr);
#endif
        v_secure_system("iptables -A pp_disabled_%u -d %s -p tcp -m multiport --sports 80,443 -m state --state ESTABLISHED -m connbytes --connbytes 0:5 --connbytes-dir reply --connbytes-mode packets -j GWMETA --dis-pp", insNum, ipAddr);

#if _NFQ_DEBUG_LEVEL == 1
        printf("system: iptables -F device_%u_container\n", insNum);
#endif
        v_secure_system("iptables -F device_%u_container", insNum);
        
#if _NFQ_DEBUG_LEVEL == 1
        printf("system: iptables -A device_%u_container -d %s -j wan2lan_dnsr_nfqueue_%u\n", insNum, ipAddr, insNum);
#endif
        v_secure_system("iptables -A device_%u_container -d %s -j wan2lan_dnsr_nfqueue_%u", insNum, ipAddr, insNum);
    }
    else
        fprintf(stderr, "nfq_handler: error during nfq_get_payload() in %s\n", __FUNCTION__);
}

void handle_dns_response(char *payload, uint32_t mark)
{
    char *dnsHdr = get_dns_header(payload);
    char *dnsData = dnsHdr + sizeof(struct dns_header);
    char *dnsAns;
#if _NFQ_DEBUG_LEVEL == 1 || _NFQ_DEBUG_LEVEL == 2
    char cmd[256];
#endif
    char ipAddr[INET6_ADDRSTRLEN];
    int ansNum = 0, queryNameLen = 0, i;
    __u16 type, dataLen;

    uint32_t insNum = mark & 0xff;

#if _NFQ_DEBUG_LEVEL >= 1
    char url[512];
    queryNameLen = get_query_url(url, dnsData);
#else 
    queryNameLen = get_query_url_length(dnsData);
#endif

    type = ntohs(*(__u16 *)(dnsData + queryNameLen));

#if _NFQ_DEBUG_LEVEL == 2
    printf("type of query in answer is %d\n", type);
#endif

    //only handle DNS response with Answer RRs > 0 and query Type = A or Type = AAAA(0x001c)
    if((ansNum = ntohs(((struct dns_header*)dnsHdr)->answerRR)) > 0 && (type == 1 || type == 0x001c)) {

#if _NFQ_DEBUG_LEVEL == 2
        printf("dnsNum of query in answer is %d\n", ansNum);
#endif

        dnsAns = dnsData + queryNameLen + 2 + 2; //Type and Class fields are both 2-byte

#if _NFQ_DEBUG_LEVEL == 2
        printf("dnsAns is %x\n", *dnsAns & 0xff);
#endif

        for(i = 0; i < ansNum; i++) {

            type = ntohs(((struct dns_answer *)dnsAns)->type);
            dataLen = ntohs(((struct dns_answer *)dnsAns)->len);

            if(type == 1){
#if _NFQ_DEBUG_LEVEL == 2
                printf("type of answer in answer is %d\n", type);
                printf("dataLen of answer in answer is %x\n", dataLen);
#endif       

                unsigned char *ip = (unsigned char*)(dnsAns + sizeof(struct dns_answer));

                snprintf(ipAddr, sizeof(ipAddr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

#if _NFQ_DEBUG_LEVEL == 1
                printf("%s - %s\n", url, ipAddr);
                printf("system(\"ipset -! add %u %s\")\n", insNum, ipAddr);
#endif
                v_secure_system("ipset -! add %u %s", insNum, ipAddr);
            }else if (type == 0x001c){ /* Type AAAA*/
#if _NFQ_DEBUG_LEVEL == 2
                printf("type of answer in answer is %d\n", type);
                printf("dataLen of answer in answer is %x\n", dataLen);
#endif       
                unsigned char *ip = (unsigned char*)(dnsAns + sizeof(struct dns_answer));

                snprintf(ipAddr, sizeof(ipAddr), "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", \
                        ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14], ip[15]);
#if _NFQ_DEBUG_LEVEL == 2 
                printf("%s - %s\n", url, ipAddr);
                printf("system(\"ipset -! add %u_v6 %s\")\n", insNum, ipAddr);
#endif
                v_secure_system("ipset -! add %u_v6 %s", insNum, ipAddr);
            }

            dnsAns += sizeof(struct dns_answer) + dataLen;
        }
        return;
    }
}

//packet processing function
static int dns_query_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg, struct nfq_data *pkt, void *data)
{
    int id = 0x00; /*RDKB-7144, CID-33514, init before use */
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(pkt);

    if (ph)
        id = ntohl(ph->packet_id);

    handle_dns_query(pkt);

    return nfq_set_verdict(queueHandle, id, NF_ACCEPT, 0, NULL);
}

//packet processing function
static int dns_response_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg, struct nfq_data *pkt, void *data)
{
    int id = 0x00; /*RDKB-7144, CID-33468, init before use */
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(pkt);
    char *payload = NULL;
    int len = nfq_get_payload(pkt, (unsigned char **)&payload);

    if (ph)
        id = ntohl(ph->packet_id);

    if(len > 0) 
        handle_dns_response(payload, nfq_get_nfmark(pkt));
    else
        fprintf(stderr, "nfq_handler: error during nfq_get_payload() in %s\n", __FUNCTION__);

    return nfq_set_verdict(queueHandle, id, NF_ACCEPT, 0, NULL);
}

//packet processing function
static int http_get_callback(struct nfq_q_handle *queueHandle, struct nfgenmsg *nfmsg, struct nfq_data *pkt, void *data)
{
    int id = 0x00, ret = -1; /*RDKB-7144, CID-33291, init before use */
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(pkt);
    char *payload;
    char dstMac[64];
    int len = nfq_get_payload(pkt, (unsigned char **)&payload);
    
    uint32_t mark = nfq_get_nfmark(pkt);
    uint32_t insNum = mark;
    char dstIpAddr[INET6_ADDRSTRLEN], srcIpAddr[INET6_ADDRSTRLEN];
    struct tcphdr *tcpHdr;
    int family;

    struct nfqnl_msg_packet_hw *macAddr = nfq_get_packet_hw(pkt);
    if(macAddr != NULL)
        snprintf(dstMac, sizeof(dstMac), "%02x:%02x:%02x:%02x:%02x:%02x", \
                macAddr->hw_addr[0], macAddr->hw_addr[1], macAddr->hw_addr[2], \
                macAddr->hw_addr[3],macAddr->hw_addr[4], macAddr->hw_addr[5]);

    if (ph)
        id = ntohl(ph->packet_id);

    if(len > 0) {
        struct iphdr *ipHdr = (struct iphdr*)payload;
        if (ipHdr->version == 4){
            family = AF_INET;
            unsigned char *dstIp = (unsigned char*)&ipHdr->daddr;
            unsigned char *srcIp = (unsigned char*)&ipHdr->saddr;
            tcpHdr = (struct tcphdr *)(payload + ipHdr->ihl * 4);

        snprintf(dstIpAddr, sizeof(dstIpAddr), "%u.%u.%u.%u", dstIp[0], dstIp[1], dstIp[2], dstIp[3]);
        snprintf(srcIpAddr, sizeof(srcIpAddr), "%u.%u.%u.%u", srcIp[0], srcIp[1], srcIp[2], srcIp[3]);
        if(insNum != 0){ 
            v_secure_system("ipset -! add %u %s", insNum, dstIpAddr);
        }
        
#if _NFQ_DEBUG_LEVEL == 2
        printf("\nip tot len is %u\n", ntohs(ipHdr->tot_len));
        printf("tcp hdr len is %u\n", tcpHdr->doff * 4);
        printf("ip hdr len is %u\n", ipHdr->ihl * 4);
        printf("recv tcp seq is %u\n", ntohl(tcpHdr->seq));
        printf("recv tcp ack is %u\n", ntohl(tcpHdr->ack_seq));
#endif
        }else{/* IPv6 */
            family = AF_INET6;
            struct ip6_hdr *ipv6Hdr = (struct ip6_hdr*)payload;
            struct in6_addr *daddr = &(ipv6Hdr->ip6_dst);
            struct in6_addr *saddr = &(ipv6Hdr->ip6_src);
            tcpHdr = (struct tcphdr *)(payload + sizeof(struct ip6_hdr)); 
            inet_ntop(AF_INET6, daddr, dstIpAddr, sizeof(dstIpAddr));
            inet_ntop(AF_INET6, saddr, srcIpAddr, sizeof(srcIpAddr));
            v_secure_system("ipset -! add %u_v6 %s", insNum, dstIpAddr);
#if _NFQ_DEBUG_LEVEL == 2
            printf("\nip daddr is  %s\n", dstIpAddr);
            printf("tcp hdr len is %u\n", tcpHdr->doff * 4);
            //printf("ip hdr len is %u\n", ipHdr->ihl * 4);
            printf("recv tcp seq is %u\n", ntohl(tcpHdr->seq));
            printf("recv tcp ack is %u\n", ntohl(tcpHdr->ack_seq));
#endif
       }
        

        char *httpHdr = (char *)tcpHdr + tcpHdr->doff * 4;

        if(httpHdr[0] == 'G' && httpHdr[1] == 'E' && httpHdr[2] == 'T') {

            while (*httpHdr != 'H' || *(httpHdr+1) != 'o'|| *(httpHdr+2) != 's'|| *(httpHdr+3) != 't')
                httpHdr++;

            char *urlStart = httpHdr + strlen("Host: ");
            char *urlEnd = urlStart;
            int urlLen = 0;

            while (*urlEnd != 0x0d || *(urlEnd+1) != 0x0a)
                urlEnd++;

            urlLen = urlEnd - urlStart;

            char url[256];
            memcpy(url, urlStart, urlLen);
            url[urlLen] = '\0';
            
            unsigned long ackNum = ntohs(ipHdr->tot_len) - tcpHdr->doff * 4 - ipHdr->ihl * 4 + ntohl(tcpHdr->seq);

#if _NFQ_DEBUG_LEVEL == 1
            printf("sending pkt %s:%u ---> %s:%u\n", dstIpAddr, ntohs(tcpHdr->dest), srcIpAddr, ntohs(tcpHdr->source));
#endif
            /*snprintf(cmd, sizeof(cmd), "a.out brlan0 %s %s %s %u %u %lu %lu %s", \
                    mac, dstIpAddr, srcIpAddr, ntohs(tcpHdr->dest), ntohs(tcpHdr->source), ntohl(tcpHdr->ack_seq), ackNum, url);*/
            //printf("cmd is %s\n", cmd);
            //system(cmd);

            //reverse src/dst ip & port

	    // CID 66818: intentionally reversed src/dst ip & port, this CID was false positive
            send_tcp_pkt("brlan0", family, srcMac, dstMac, dstIpAddr, srcIpAddr, ntohs(tcpHdr->dest), ntohs(tcpHdr->source), ntohl(tcpHdr->ack_seq), ackNum, url, 1);

            ret = nfq_set_verdict(queueHandle, id, NF_DROP, 0, NULL);
        }
        else
            ret = nfq_set_verdict(queueHandle, id, NF_ACCEPT, 0, NULL);
    }
    else
        fprintf(stderr, "nfq_handler: error during nfq_get_payload() in %s\n", __FUNCTION__);

    return ret;
}
static void getIFMac(char *interface, char *mac){
    int s;
    struct ifreq buffer;
    int ret = -1;
    errno_t safec_rc = -1;
    do{
        s = socket(PF_INET, SOCK_DGRAM, 0);
	/* CID 65152: Argument cannot be negative */
	if(s < 0) {
	   printf("return value of socket can't be negative\n");
	   return;
        }
        memset(&buffer, 0x00, sizeof(buffer));
        safec_rc = strcpy_s(buffer.ifr_name, sizeof(buffer.ifr_name),interface);
        ERR_CHK(safec_rc);
        ret = ioctl(s, SIOCGIFHWADDR, &buffer);
        close(s);
        sleep(5);
    }while(ret != 0);
    // Here mac is pointer is pointing to srcMac[20] global array
    safec_rc = strcpy_s(mac, sizeof(srcMac),(void *)ether_ntoa((struct ether_addr *)(buffer.ifr_hwaddr.sa_data)));
    ERR_CHK(safec_rc);
}
//skeleton to connect to iptables NFQUEUE argv[1]
//argv[2] query:intercept dns query, response:intercept dns response
int main(int argc, char *argv[])
{
    struct nfq_handle *nfqHandle;
    struct nfq_q_handle *queueHandle;
    int fd, rv;
    char buf[4096];
    unsigned char i, j;
    u_int16_t family = atoi(argv[1]) == 4 ? AF_INET : AF_INET6;

#ifdef CONFIG_CISCO_PARCON_WALLED_GARDEN
    const nfq_cfg nfqCfg[] = {
        {"dns_response", 6, 8, dns_response_callback},
        {"http_get", 11, 12, http_get_callback}
    };

   const nfq_cfg nfqCfgV6[] = {
        {"dnsv6_response", 9, 10, dns_response_callback},
        {"httpv6_get", 13, 14, http_get_callback}
   };
#else
    const nfq_cfg nfqCfg[] = {
        {"dns_query", 5, 5, dns_query_callback},
        {"dns_response", 6, 8, dns_response_callback},
        {"http_get", 11, 12, http_get_callback}
    };

   const nfq_cfg nfqCfgV6[] = {
   };
#endif

   if(argc == 3)
   {
       /* CID 135501 : BUFFER_SIZE_WARNING */
       if (strlen(argv[2]) >= sizeof(srcMac))
       {
           fprintf(stderr, "nfq_handler: maxium length of srcMac %s\n", __FUNCTION__);
           exit(1);
       }
       strcpy(srcMac, argv[2]);
   }
   else{
       /* In ARES/XB3 brlan0 has not been created when program started
        * Get Mac by self */
       getIFMac("brlan0", srcMac);
   }

#if 0
    //int (*callback)(struct nfq_q_handle *, struct nfgenmsg *, struct nfq_data *, void *);
    //
    if(strcmp("dns_query", argv[2]) == 0)
        callback = dns_query_callback;
    else if(strcmp("dns_response", argv[2]) == 0)
        callback = dns_response_callback;
    else if(strcmp("dnsv6_response", argv[2]) == 0){
        callback = dns_response_callback;
        family = AF_INET6;
    }else if(strcmp("http_get", argv[2]) == 0) {
        callback = http_get_callback;
        if(argc == 4)
            strncpy(srcMac, argv[3], sizeof(srcMac));
        else{
            /* In ARES/XB3 brlan0 has not been created when program started
             * Get Mac by self */
            getIFMac("brlan0", srcMac);
        }
    }else if(strcmp("httpv6_get", argv[2]) == 0) {
        callback = http_get_callback;
        family = AF_INET6;
        callback = http_get_callback;
        if(argc == 4)
            strncpy(srcMac, argv[3], sizeof(srcMac));
        else{
            /* In ARES/XB3 brlan0 has not been created when program started
             * Get Mac by self */
            getIFMac("brlan0", srcMac);
        }

    }else {
        fprintf(stderr, "nfq_handler: error during nfq_create_queue()\n");
        exit(1);
    }
#endif

    nfqHandle = nfq_open();
    if (!nfqHandle) {
        fprintf(stderr, "nfq_handler: error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for %s (if any)\n", family == AF_INET ? "AF_INET" : "AF_INET6");
    if (nfq_unbind_pf(nfqHandle, family) < 0) {
        fprintf(stderr, "nfq_handler: error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for %s\n", family == AF_INET ? "AF_INET" : "AF_INET6");
    if (nfq_bind_pf(nfqHandle, family) < 0) {
        fprintf(stderr, "nfq_handler: error during nfq_bind_pf()\n");
        exit(1);
    }

    int numOfCfg;
    nfq_cfg *pNfqCfg;

    if(family == AF_INET) {
        numOfCfg = sizeof(nfqCfg) / sizeof(nfq_cfg);
        pNfqCfg = (nfq_cfg *)nfqCfg;
    } else {
        numOfCfg = sizeof(nfqCfgV6) / sizeof(nfq_cfg);
        pNfqCfg = (nfq_cfg *)nfqCfgV6;
    }

    for(i = 0; i < numOfCfg; i++) {
      
        for(j = pNfqCfg[i].qnum_start; j <= pNfqCfg[i].qnum_end; j++) {
     
            printf("binding this socket to queue %d in %s mode\n", j, pNfqCfg[i].mode);
            queueHandle = nfq_create_queue(nfqHandle, j, pNfqCfg[i].callback, NULL);
            if (!queueHandle) {
                fprintf(stderr, "nfq_handler: error during nfq_create_queue()\n");
                exit(1);
            }

            printf("setting copy_packet mode\n");
            if (nfq_set_mode(queueHandle, NFQNL_COPY_PACKET, 0xffff) < 0) {
                fprintf(stderr, "can't set packet_copy mode\n");
                exit(1);
            }
        }
    }

    fd = nfq_fd(nfqHandle);

    while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
#if _NFQ_DEBUG_LEVEL == 2
        printf("nfq_handler: %s packet received\n", argv[2]);
#endif
        nfq_handle_packet(nfqHandle, buf, rv);
    }

    return 0;
}
