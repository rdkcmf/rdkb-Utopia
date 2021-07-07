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

#include<stdio.h>
#include<stdlib.h>
#include <sys/socket.h>
#include <features.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <sys/ioctl.h>
#include<net/if.h>
//#include<net/ethernet.h>
//#include<linux/ip.h>
#include <netinet/ip.h>
#include<netinet/ip6.h>
#include<linux/tcp.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/time.h>
#include<unistd.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>

#define HDRLEN 62
#define MAX_IPLEN INET6_ADDRSTRLEN 
//starts with "0x0a 0x0d"
static const unsigned char http_redirect_payload2[] = { \
    0x2f,0x48,0x6e,0x61,0x70,0x50,0x63,0x53,0x69,0x74, \
    0x65,0x42,0x6c,0x6f,0x63,0x6b,0x65,0x64,0x2e,0x70,0x68,0x70,0x3f,0x75,0x72,0x6c, 0x3d \
}; //ends with "url=", add "www.xxx.com"

static const unsigned char http_redirect_payload_bottom[] = { \
    0x0d,0x0a, \
    0x43,0x6f,0x6e,0x74,0x65,0x6e,0x74,0x2d,0x74,0x79,0x70,0x65,0x3a,0x20,0x74,0x65, \
    0x78,0x74,0x2f,0x68,0x74,0x6d,0x6c,0x0d,0x0a,0x43,0x6f,0x6e,0x74,0x65,0x6e,0x74, \
    0x2d,0x4c,0x65,0x6e,0x67,0x74,0x68,0x3a,0x20,0x30,0x0d,0x0a, \
    0x44,0x61,0x74,0x65,0x3a,0x20,0x4d,0x6f,0x6e,0x2c,0x20,0x31,0x36,0x20,0x53,0x65, \
    0x70,0x20,0x32,0x30,0x31,0x33,0x20,0x30,0x30,0x3a,0x33,0x33,0x3a,0x33,0x35,0x20, \
    0x47,0x4d,0x54,0x0d,0x0a, \
    0x53,0x65,0x72,0x76, 0x65,0x72,0x3a,0x20,0x6c,0x69,0x67,0x68,0x74,0x74,0x70,0x64, \
    0x0d,0x0a,0x0d,0x0a \
};

//assuming max url length is 256
static unsigned char http_redirect_payload[HDRLEN+MAX_IPLEN + 2 +sizeof(http_redirect_payload2)+256+sizeof(http_redirect_payload_bottom)] = { \
    0x48,0x54,0x54,0x50,0x2f,0x31,0x2e,0x31,0x20,0x33,0x30,0x32,0x20,0x46,0x6f,0x75, \
    0x6e,0x64,0x0d,0x0a,0x58,0x2d,0x50,0x6f,0x77,0x65,0x72,0x65,0x64,0x2d,0x42,0x79, \
    0x3a,0x20,0x50,0x48,0x50,0x2f,0x35,0x2e,0x33,0x2e,0x32,0x0d,0x0a,0x4c,0x6f,0x63, \
    0x61,0x74,0x69,0x6f,0x6e,0x3a,0x20,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f \
}; //ends with http://, add "192.168.0.1"

typedef struct PseudoHeader {
    u_int32_t source_ip;
    u_int32_t dest_ip;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t tcp_length;
} PseudoHeader;

typedef struct PseudoHeaderv6 {
    u_int8_t source_ipv6[16];
    u_int8_t dest_ipv6[16];
    u_int32_t up_len;
    u_int8_t reserved[3];
    u_int8_t next_hdr;
} PseudoHeaderv6;

static unsigned short tcp_checksum (unsigned short *buffer, int byte_count)
{
    register long word_sum;
    int word_count;
    int i;

    word_sum = 0;
    word_count = byte_count >> 1;

    for(i = 0; i < word_count ; i++) {
	word_sum += buffer[i];
    }

    if( byte_count & 1 ) {
	word_sum += *(unsigned char*)&buffer[i];
    }

    unsigned short carry = (unsigned short) (word_sum >> 16);
    
    while (0 != carry)
    {
        word_sum = (word_sum & 0xffff) + carry;
        carry = (unsigned short) (word_sum >> 16);
    }

    return (short)(~word_sum);

}

static int SendRawPacket (int rawsock, unsigned char *pkt, int pkt_len, char *dstIp, unsigned short dstPort)
{
    int sent= 0;
    struct sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(dstIp); // you can also use inet_aton()
    to.sin_port = htons(dstPort);
    memset(to.sin_zero, '\0', sizeof(to.sin_zero));

    if((sent = write(rawsock, pkt, pkt_len)) != pkt_len)
    {
        /* Error */
        printf("Could only send %d bytes of packet of length %d\n", sent, pkt_len);
        return 0;
    }

    return 1;
}

static struct ethhdr *CreateEthernetHeader (char *src_mac, char *dst_mac, int protocol)
{
    struct ethhdr *ethernet_header;
    ethernet_header = (struct ethhdr *)malloc(sizeof(struct ethhdr));

    /* copy the Src mac addr */
    memcpy(ethernet_header->h_source, (void *)ether_aton(src_mac), 6);

    /* copy the Dst mac addr */
    memcpy(ethernet_header->h_dest, (void *)ether_aton(dst_mac), 6);

    /* copy the protocol */
    ethernet_header->h_proto = htons(protocol);

    /* done ...send the header back */
    return (ethernet_header);
}

/* ComputeChecksum() */
static unsigned short ComputeChecksum (void *data, unsigned long length)
{
	unsigned short	*tempUshort 	  = NULL,
					 UshortForPadding = 0;
	unsigned long	 checksum		  = 0;

	/*
	 * retrieve the shortcut pointer
	 */
	tempUshort = (unsigned short*)data;

	/*
	 * loop to calculate the check sum
	 */
	while ( length > 1 )
	{
		checksum += *tempUshort;
		tempUshort++;

		/*
		 * if high-order bit set, fold
		 */
		if ( checksum & 0x80000000 )
		{
			checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
		}

		/*
		 * modify length
		 */
		length -= 2;
	}

	/*
	 * take care of left over bytes.
	 * note: although it's impossible...
	 */
	if ( length )
	{
		UshortForPadding			= 0;
		*(unsigned char*)&UshortForPadding	= *(unsigned char*)tempUshort;
		checksum				   += UshortForPadding;
	}

	/*
	 * fold the result checksum
	 */
	while ( checksum >> 16 )
	{
		checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
	}

	/*
	 * return complement of checksum
	 */
	return	~((unsigned short)checksum);
}

static void *CreateIPHeader (int family, char *srcIp, char *dstIp, unsigned int dataSize)
{
    if(family == AF_INET6){
        struct ip6_hdr *ipv6Hdr = malloc(sizeof(struct ip6_hdr));
        if(ipv6Hdr == NULL)
            return NULL;
        memset(ipv6Hdr, 0, sizeof(struct ip6_hdr));
        ipv6Hdr->ip6_flow = 0x60000000;/* version = 6; flowlab = 0;triffic class = 0; */
        ipv6Hdr->ip6_plen = htons(sizeof(struct tcphdr) +  dataSize);
        ipv6Hdr->ip6_nxt = IPPROTO_TCP;
        ipv6Hdr->ip6_hops = 60;
        inet_pton(AF_INET6, srcIp, &(ipv6Hdr->ip6_src));
        inet_pton(AF_INET6, dstIp, &(ipv6Hdr->ip6_dst));
        return (ipv6Hdr);
    }else{
        struct iphdr *ip_header;

    ip_header = (struct iphdr *)malloc(sizeof(struct iphdr));

    ip_header->version = 4;
    ip_header->ihl = (sizeof(struct iphdr))/4 ;
    ip_header->tos = 0;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + dataSize);
    ip_header->id = htons(111);
    ip_header->frag_off = 0;
    ip_header->ttl = 111;
    ip_header->protocol = IPPROTO_TCP;
    ip_header->check = 0; /* We will calculate the checksum later */
    ip_header->saddr = inet_addr(srcIp);
    ip_header->daddr = inet_addr(dstIp);

    /* Calculate the IP checksum now :
       The IP Checksum is only over the IP header */
    ip_header->check = ComputeChecksum((unsigned char *)ip_header, ip_header->ihl*4);

        return (ip_header);
    }
}

static struct tcphdr *CreateTcpHeader(int family, unsigned short sport, unsigned short dport, unsigned long seqNum, unsigned long ackNum, unsigned char fin)
{
    struct tcphdr *tcp_header;

    /* Check /usr/include/linux/tcp.h for header definiation */
    tcp_header = (struct tcphdr *)malloc(sizeof(struct tcphdr));

    tcp_header->source = htons(sport);
    tcp_header->dest = htons(dport);
    tcp_header->seq = htonl(seqNum);
    tcp_header->ack_seq = htonl(ackNum);
    tcp_header->res1 = 0;
    tcp_header->doff = (sizeof(struct tcphdr))/4;
    tcp_header->psh = 1;
    tcp_header->fin = fin;
    tcp_header->ack = 1;
    tcp_header->window = htons(14608);
    tcp_header->check = 0; /* Will calculate the checksum with pseudo-header later */
    tcp_header->urg_ptr = 0;

    return (tcp_header);
}

static void CreatePseudoHeaderAndComputeTcpChecksum (int family, struct tcphdr *tcp_header, void *ip_header, unsigned char *data, unsigned int dataSize)
{
    unsigned char *hdr = NULL;
    int pseudo_offset = 0;
    int header_len;

    /*The TCP Checksum is calculated over the PseudoHeader + TCP header +Data*/
    if(family == AF_INET){
        struct iphdr *ipv4_header = ip_header;
        /* Find the size of the TCP Header + Data */
        int segment_len = ntohs(ipv4_header->tot_len) - ipv4_header->ihl*4;

        /* Total length over which TCP checksum will be computed */
        header_len = sizeof(PseudoHeader) + segment_len;

        /* Allocate the memory */
        hdr = (unsigned char *)malloc(header_len);
        if(hdr == NULL)
            return;
        pseudo_offset = sizeof(PseudoHeader);
        /* Fill in the pseudo header first */
        PseudoHeader *pseudo_header = (PseudoHeader *)hdr;

        pseudo_header->source_ip = ipv4_header->saddr;
        pseudo_header->dest_ip = ipv4_header->daddr;
        pseudo_header->reserved = 0;
        pseudo_header->protocol = ipv4_header->protocol;
        pseudo_header->tcp_length = htons(segment_len);

    }else{
        struct ip6_hdr *ipv6_header = ip_header;
        /* total len = pseudo header length + tcp length */
        header_len = sizeof(PseudoHeaderv6) + ntohs(ipv6_header->ip6_plen);
         /* Allocate the memory */
        hdr = (unsigned char *)malloc(header_len);
        if(hdr == NULL)
            return;
        pseudo_offset = sizeof(PseudoHeaderv6);
        PseudoHeaderv6 *pseudo_header = (PseudoHeaderv6 *)hdr;
        memcpy(pseudo_header->source_ipv6, &(ipv6_header->ip6_src), 16);
        memcpy(pseudo_header->dest_ipv6, &(ipv6_header->ip6_dst), 16);
        pseudo_header->up_len = ipv6_header->ip6_plen;
        memset(pseudo_header->reserved, 0, 3);
        pseudo_header->next_hdr = ipv6_header->ip6_nxt;
    }
    /* Now copy TCP */
    memcpy((hdr + pseudo_offset), (void *)tcp_header, tcp_header->doff*4);

    /* Now copy the Data */
    memcpy((hdr + pseudo_offset + tcp_header->doff*4), data, dataSize);

    /* Calculate the Checksum */
    tcp_header->check = tcp_checksum((unsigned short *)hdr, header_len);

    /* Free the PseudoHeader */
    free(hdr);

    return ;
}

static unsigned char *CreateData (int family, char *url, char *gwIp)
{
    unsigned char *data = http_redirect_payload;
    int offset = 0;
    if(family == AF_INET){
        offset = strlen(gwIp);
        memcpy(http_redirect_payload + HDRLEN, gwIp, offset);
    }else{
        offset = strlen(gwIp);
        memcpy(http_redirect_payload + HDRLEN, "[", 1);
        memcpy(http_redirect_payload + HDRLEN + 1, gwIp, offset);
        memcpy(http_redirect_payload + HDRLEN + offset + 1 , "]",1);
        offset += 2;
    }
    
    memcpy(http_redirect_payload + HDRLEN + offset, http_redirect_payload2, sizeof(http_redirect_payload2));
    memcpy(http_redirect_payload + HDRLEN + offset + sizeof(http_redirect_payload2), url, strlen(url));
    memcpy(http_redirect_payload + HDRLEN + offset + sizeof(http_redirect_payload2) + strlen(url), http_redirect_payload_bottom, sizeof(http_redirect_payload_bottom));

    return data;
}

void send_tcp_pkt (char *interface, int family, char *srcMac, char *dstMac, char *srcIp, char *dstIp, unsigned short srcPort, unsigned short dstPort, unsigned long seqNum, unsigned long ackNum, char *url, unsigned char rst)
{
    int raw;
    unsigned char *packet;
    struct sockaddr_ll socket_ll;
    struct ifreq inf_request;
    struct ethhdr* ethernet_header;
    void *ip_header;
    struct tcphdr  *tcp_header;
    unsigned char *data;
    int pkt_len;
    int ip_offset;
    unsigned int dataSize; 
    char gwIp[INET6_ADDRSTRLEN] = {'\0'};
    
    if(family == AF_INET){ 
        FILE *fp = fopen("/var/.gwip", "r");
        if(fp != NULL){
            fgets(gwIp, sizeof("255.255.255.255"), fp);
            fclose(fp);
        }
    }else{
        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == 0 ){
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL)
                    continue;
                if(0 == strcmp(ifa->ifa_name, interface) && \
                    ifa->ifa_addr->sa_family == AF_INET6){
                    struct sockaddr_in6 *addr = (struct sockaddr_in6 *)(ifa->ifa_addr);
                    inet_ntop(AF_INET6, &(addr->sin6_addr), gwIp, INET6_ADDRSTRLEN);
                    if(strncmp("fe80", gwIp, 4) != 0)
                        break;
                }
            }
            freeifaddrs(ifaddr);
        } 
    }
    if(gwIp[0] == '\0')
        return;
    
    dataSize = HDRLEN + strlen(gwIp) + sizeof(http_redirect_payload2) + strlen(url) + sizeof(http_redirect_payload_bottom);
    /* Create the raw socket */
    raw = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	
    if(-1 == raw)
    {
        perror("Error creating raw socket: ");
        exit(-1);
    }

    /* Bind raw socket to interface */
    bzero(&socket_ll, sizeof(socket_ll));
    bzero(&inf_request, sizeof(inf_request));

    /* First Get the Interface Index  */
    /*CID 135235 : BUFFER_SIZE_WARNING */
    strncpy((char *)inf_request.ifr_name, interface, sizeof(inf_request.ifr_name)-1);
    inf_request.ifr_name[sizeof(inf_request.ifr_name)-1] = '\0';
    if(-1 == (ioctl(raw, SIOCGIFINDEX, &inf_request)))
    {
        printf("Error getting Interface index !\n");
        exit(-1);
    }

    /* Bind our raw socket to this interface */
    socket_ll.sll_family   = AF_PACKET;
    socket_ll.sll_ifindex  = inf_request.ifr_ifindex;
    socket_ll.sll_protocol = htons(ETH_P_ALL);

    if(-1 == (bind(raw, (struct sockaddr *)&socket_ll, sizeof(socket_ll))))
    {
        perror("Error binding raw socket to interface\n");
        exit(-1);
    }
	
    /* create Ethernet header */
    ethernet_header = CreateEthernetHeader(srcMac, dstMac, family == AF_INET ? ETHERTYPE_IP : ETHERTYPE_IPV6);

    /* Create IP Header */
    ip_header = CreateIPHeader(family, srcIp, dstIp, dataSize);

    /* Create TCP Header */
    tcp_header = CreateTcpHeader(family, srcPort, dstPort, seqNum, ackNum, rst);

    /* Create Data */
    data = CreateData(family, url, gwIp);
    
    /* Create PseudoHeader and compute TCP Checksum  */
    CreatePseudoHeaderAndComputeTcpChecksum(family, tcp_header, ip_header, data, dataSize);
    
    /* Packet length = ETH + IP header + TCP header + Data*/
    if(family == AF_INET){
        struct iphdr *ip4_header = ip_header;
        pkt_len = sizeof(struct ethhdr) + ntohs(ip4_header->tot_len);
        ip_offset = ip4_header->ihl*4;
    }else{
        struct ip6_hdr *ipv6_header = ip_header;
        pkt_len = sizeof(struct ethhdr) + sizeof(struct ip6_hdr) + ntohs(ipv6_header->ip6_plen);
        ip_offset = sizeof(struct ip6_hdr);
    }

    /* Allocate memory */
    packet = (unsigned char *)malloc(pkt_len);

    /* Copy the Ethernet header first */
    memcpy(packet, ethernet_header, sizeof(struct ethhdr));

    /* Copy the IP header -- but after the ethernet header */
    memcpy((packet + sizeof(struct ethhdr)), ip_header, ip_offset);

    /* Copy the TCP header after the IP header */
    memcpy((packet + sizeof(struct ethhdr) + ip_offset),tcp_header, tcp_header->doff*4);

    /* Copy the Data after the TCP header */
    memcpy((packet + sizeof(struct ethhdr) + ip_offset + tcp_header->doff*4), data, dataSize);

    /* send the packet on the wire */
    if(!SendRawPacket(raw, packet, pkt_len, dstIp, dstPort)) {
        perror("Error sending packet");
    }
    else {
        //printf("Packet sent successfully\n");
    }

    /* Free the headers back to the heavenly heap */
    free(ethernet_header);
    free(ip_header);
    free(tcp_header);
    free(packet);

    close(raw);
    return;
}
#if 0
int main(int argc, char **argv)
{

//void send_tcp_pkt(char *interface,int family, char *srcMac, char *dstMac, char *srcIp, char *dstIp, unsigned short srcPort, unsigned short dstPort, unsigned long seqNum, unsigned long ackNum, char *url, unsigned char rst)
    send_tcp_pkt(argv[1], atoi(argv[2]), argv[3], argv[4], argv[5], argv[6],atol(argv[7]), atol(argv[8]), strtoul(argv[9], NULL, 10), strtoul(argv[10], NULL, 10), argv[11], 1);
    return 0;
}
#endif
