#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define UNUSED(x) (void)(x)

#define HTTP_PROTO 0
#define HTTPS_PROTO 1

#define UDP_PACKET 0
#define TCP_PACKET 1

#define ACCEPT_BOOL false
#define DROP_BOOL true


//static inline bool tcp_synack_segment(struct tcphdr *tcphdr ) {
//	if (tcphdr->urg == 0 &&
//		tcphdr->ack == 1 &&
//		tcphdr->psh == 0 &&
//		tcphdr->rst == 0 &&
//		tcphdr->syn == 1 &&
//		tcphdr->fin == 0) {
//		return 1;
//	}
//	return 0;
//}

bool webGuard(u_short proto, char **host, char **uri, u_short udp_tcp, char **saddr, char **daddr,  u_short sport, u_short dport){
    char * cmd;
    cmd= malloc(strlen(*host)+strlen(*uri)+100);
    *cmd='\0';
    //cmd=strcat(cmd, "echo \"http://");
    //cmd=strcat(cmd, *host);
    //cmd=strcat(cmd, *uri);
    //cmd=strcat(cmd, " - - GET\" | /usr/bin/squidGuard -P");
    cmd=strcat(cmd, "/usr/bin/webGuard ");
    switch (proto)
    {
        case HTTP_PROTO:
            cmd=strcat(cmd, "\"http://");
            break;
    
        case HTTPS_PROTO:
            cmd=strcat(cmd, "\"https://");
            break;
        default:
            cmd=strcat(cmd, "\"");
    }
    cmd=strcat(cmd, *host);
    cmd=strcat(cmd, *uri);
    cmd=strcat(cmd, "\" ");
    cmd=strcat(cmd, *saddr);
    cmd=strcat(cmd, " ");
    cmd=strcat(cmd, *daddr);
    if (udp_tcp == UDP_PACKET){
	cmd=strcat(cmd, " UDP ");
    }else{
	cmd=strcat(cmd, " TCP ");
    }
    char ports[13];
    sprintf(ports,"%hu %hu",sport,dport);
    cmd=strcat(cmd, ports);
    //printf("Cmd: %s\n", cmd);
    FILE* file = popen(cmd, "r");
    char buffer[100];
    fscanf(file, "%100s", buffer);
    pclose(file);
    //printf("buffer is: %s\n", buffer);
    if (strstr(buffer, "ACCEPT") != NULL) {
        return ACCEPT_BOOL;
    }
    return DROP_BOOL;
}

void tcp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {

    register unsigned long sum = 0;
    unsigned short tcpLen = ntohs(pIph->tot_len) - (pIph->ihl<<2);
    struct tcphdr *tcphdrp = (struct tcphdr*)(ipPayload);
    sum += (pIph->saddr>>16)&0xFFFF;
    sum += (pIph->saddr)&0xFFFF;
    sum += (pIph->daddr>>16)&0xFFFF;
    sum += (pIph->daddr)&0xFFFF;
    sum += htons(IPPROTO_TCP);
    sum += htons(tcpLen);
    tcphdrp->check = 0;
    while (tcpLen > 1) {
        sum += * ipPayload++;
        tcpLen -= 2;
    }

    if(tcpLen > 0) {
        sum += ((*ipPayload)&htons(0xFF00));
    }
    while (sum>>16) {
          sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;
    tcphdrp->check = (unsigned short)sum;
}

//int rewrite_win_size( unsigned char *packet ) {
//
//	static uint16_t new_window = 40;
//
//	struct iphdr *iphdr = (struct iphdr *) packet;
//	struct tcphdr *tcphdr = (struct tcphdr *) (packet + (iphdr->ihl<<2));
//
//	tcphdr->window = htons(new_window);
//
//	tcp_checksum(iphdr, (unsigned short*)tcphdr);
//
//	return 0;
//}


bool check_packet_against_hostname(const unsigned char *packet)
{
        struct iphdr *iphdr = (struct iphdr *) packet;
	char *saddr, *daddr;
	saddr=malloc(sizeof(char)*strlen("255.255.255.255"));
	daddr=malloc(sizeof(char)*strlen("255.255.255.255"));        
	* saddr='\0';
	* daddr='\0';
	strcpy(saddr,inet_ntoa(*(struct in_addr*)&iphdr->saddr));
 	strcpy(daddr,inet_ntoa(*(struct in_addr*)&iphdr->daddr));
	
	if (iphdr->protocol == IPPROTO_TCP)
	{
		struct tcphdr *tcp_header = (struct tcphdr *) (packet + (iphdr->ihl<<2));
		u_int tcpdatalen = ntohs(iphdr->tot_len) - (tcp_header->doff * 4) - (iphdr->ihl * 4);
		if (tcpdatalen == 0){
			return ACCEPT_BOOL;
		}
		char *data;
	
		u_short sport=ntohs(tcp_header->th_sport);
		u_short dport=ntohs(tcp_header->th_dport);
		
		//u_int seq=(u_int) tcp_header->th_seq;
		//u_int ack=(u_int) tcp_header->th_ack;
	
		u_int16_t tls_header_len;
		u_int8_t handshake_protocol;
	
		data = (char *)((unsigned char *)tcp_header + (tcp_header->doff * 4));
		if (data[0] == 0x16)
		{
		//If this, it should be an TLS handshake
			tls_header_len = (data[3] << 8) + data[4] + 5;
			//printf("Data len %d, tcpdatalen %d, tls_header_len %d\n", tcpdatalen, tcpdatalen, tls_header_len);
			handshake_protocol = data[5];
		
			//Even if we don't have all the data, try matching anyway
			if (tls_header_len > tcpdatalen){
				//tls_header_len = tcpdatalen;
				tcpdatalen=tls_header_len+tcpdatalen;
			}
		
			if (tls_header_len > 4) {
				// Check only client hellos for now
				if (handshake_protocol == 0x01) {
					u_int offset, base_offset = 43, extension_offset = 2;
					u_int16_t session_id_len, cipher_len, compression_len, extensions_len;
		
					if (base_offset + 2 > tcpdatalen) {
						////printf("Data length is to small (%d)\n", (int)tcpdatalen);
						return ACCEPT_BOOL;
					}
		
					// Get the length of the session ID
					session_id_len = data[base_offset];
		
					////printf("Session ID length: %d\n", session_id_len);
					if ((session_id_len + base_offset + 2) > tls_header_len) {
						////printf("TLS header length is smaller than session_id_len + base_offset +2 (%d > %d)\n", (session_id_len + base_offset + 2), tls_header_len);
						return ACCEPT_BOOL;
					}
		
					// Get the length of the ciphers
					memcpy(&cipher_len, &data[base_offset + session_id_len + 1], 2);
					cipher_len = ntohs(cipher_len);
					offset = base_offset + session_id_len + cipher_len + 2;
					////printf("Cipher len: %d\n", cipher_len);
					////printf("Offset (1): %d\n", offset);
					if (offset > tls_header_len) {
						////printf("TLS header length is smaller than offset (%d > %d)\n", offset, tls_header_len);
						return ACCEPT_BOOL;
					}
		
					// Get the length of the compression types
					compression_len = data[offset + 1];
					offset += compression_len + 2;
					////printf("Compression length: %d\n", compression_len);
					////printf("Offset (2): %d\n", offset);
					if (offset > tls_header_len) {
						////printf("TLS header length is smaller than offset w/compression (%d > %d)\n", offset, tls_header_len);
						return ACCEPT_BOOL;
					}
		
					// Get the length of all the extensions
					memcpy(&extensions_len, &data[offset], 2);
					extensions_len = ntohs(extensions_len);
					////printf("Extensions length: %d\n", extensions_len);
		
					if ((extensions_len + offset) > tls_header_len) {
						////printf("TLS header length is smaller than offset w/extensions (%d > %d)\n", (extensions_len + offset), tls_header_len);
						return ACCEPT_BOOL;
					}
		
					// Loop through all the extensions to find the SNI extension
					while (extension_offset < extensions_len)
					{
						u_int16_t extension_id, extension_len;
		
						memcpy(&extension_id, &data[offset + extension_offset], 2);
						extension_offset += 2;
		
						memcpy(&extension_len, &data[offset + extension_offset], 2);
						extension_offset += 2;
		
						extension_id = ntohs(extension_id), extension_len = ntohs(extension_len);
		
						////printf("Extension ID: %d\n", extension_id);
						////printf("Extension length: %d\n", extension_len);
		
						if (extension_id == 0) {
							u_int16_t name_length;
							//u_int16_t name_type;
		
							// We don't need the server name list length, so skip that
							extension_offset += 2;
							// We don't really need name_type at the moment
							// as there's only one type in the RFC-spec.
							// However I'm leaving it in here for
							// debugging purposes.
							//name_type = data[offset + extension_offset];
							extension_offset += 1;
		
							memcpy(&name_length, &data[offset + extension_offset], 2);
							name_length = ntohs(name_length);
							extension_offset += 2;
		
							////printf("Name type: %d\n", name_type);
							////printf("Name length: %d\n", name_length);
							//// Allocate an extra byte for the null-terminator
							
							char *host = malloc(name_length + 1);
							strncpy(host, &data[offset + extension_offset], name_length);
							//// Make sure the string is always null-terminated.
							host[name_length] = 0;
							char *uri;
							uri=malloc(1);
							uri[0]='\0';
							printf("SNI: %s\n", host);
							return webGuard(HTTPS_PROTO,&host,&uri,TCP_PACKET,&saddr,&daddr,sport,dport);
						}
						extension_offset += extension_len;
					}
				}
			}
			return ACCEPT_BOOL;
		}
		else
		{
			char * uri;
			char * host;
			u_int i;
			if((data[0] == 'G' || data[0] == 'g') && (data[1] == 'E' || data[1] == 'e') && (data[2] == 'T'|| data[2] == 't') && data[3] == ' '){
				////printf("GET\n");
				for(i=4; i<tcpdatalen && data[i] != '\r' && data[i] != ' '; i++){}
				////printf("i %d\n",i);	
				uri=malloc(i-2);
				memcpy(uri, data+4,i-4);
				uri[i-4]='\0';
				////printf("URI %s\n",uri);
				for(i; i<tcpdatalen ; i++){	
					if((data[i] == 'H' || data[i] == 'h') && (data[i+1] == 'O' || data[i+1] == 'o') && (data[i+2] == 's'|| data[i+2] == 's') && (data[i+3] == 'T' || data[i+3] == 't') && data[i+4] == ':'){
						int j=i+6;
						for(i=i+6; i<tcpdatalen && data[i] != '\r' ; i++){}
						host=malloc(i-j+1);
						*host='\0';
						memcpy(host, data+j,i-j);
						host[i-j]='\0';
						printf("URL http://%s%s\n",host,uri);
						return webGuard(HTTP_PROTO,&host,&uri,TCP_PACKET,&saddr,&daddr,sport,dport);					
					}
				}
			}
		}
		return ACCEPT_BOOL;
	}
	else if (iphdr->protocol == IPPROTO_UDP)
	{
		// Base offset, skip to stream ID
		u_int16_t base_offset = 13;
		u_int16_t offset;
		struct udphdr *udp_header = (struct udphdr *) (packet + (iphdr->ihl<<2));
		u_int udpdatalen = ntohs(udp_header->len);
		//printf("LEN %d\n",udpdatalen);
	        if (udpdatalen != 1358)
			return ACCEPT_BOOL;	
		char *data;
		//udp_header = (struct udphdr *)skb_transport_header(skb);
		// The UDP header is a total of 8 bytes, so the data is at udp_header address + 8 bytes
		data = (char *)udp_header + 8;
	
		// Stream ID must be 1
		if ( (udpdatalen >base_offset) && (data[base_offset] != 1))
			return ACCEPT_BOOL;
		u_short sport=ntohs(udp_header->uh_sport);
		u_short dport=ntohs(udp_header->uh_dport);
	
		offset = base_offset + 17; // Skip data length
		// Only continue if this is a client hello
		if ((udpdatalen > (offset+(u_int)4)) && (strncmp(&data[offset], "CHLO", 4) == 0))
		{
			u_int32_t prev_end_offset = 0;
			u_int16_t tag_number;
			u_int tag_offset = 0;
			int i;
	
			offset += 4; // Size of tag
			if (udpdatalen > (offset+(u_int)2)){
			    memcpy(&tag_number, &data[offset], 2);
			}else{
			    return ACCEPT_BOOL;
			}
			ntohs(tag_number);
                        printf("TAG NUMBER%d\n",tag_number);

	
			offset += 4; // Size of tag number + padding
			base_offset=offset;	
			for (i = 0; i < tag_number ; i++)
			{
				u_int32_t tag_end_offset;
				int match;
				if (udpdatalen > (offset + tag_offset +4)){
				    match = strncmp("SNI", &data[offset + tag_offset], 4);
				}else{
				    return ACCEPT_BOOL;
				}
				tag_offset += 4;

				if (udpdatalen > (offset + tag_offset +4)){
				    memcpy(&tag_end_offset, &data[offset + tag_offset], 4);
				}else{
				    return ACCEPT_BOOL;
				}
				ntohs(tag_end_offset);
				printf("OFFSET%d\n",tag_end_offset);
				tag_offset += 4;
	
				if (match == 0)
				{
					int name_length = tag_end_offset - prev_end_offset;
	
					char *host = malloc(name_length + 1);
					if (udpdatalen > (base_offset+ tag_number*8+ tag_end_offset)){
					    strncpy(host, &data[base_offset+ tag_number*8+ prev_end_offset], name_length);;
					    //// Make sure the string is always null-terminated.
					    host[name_length] = 0;
					}else{
					    return ACCEPT_BOOL;
					}
					char *uri;
					uri=malloc(1);
					uri[0]='\0';
					printf("GQUIC-SNI: %s\n", host);
					return webGuard(HTTPS_PROTO,&host,&uri,UDP_PACKET,&saddr,&daddr,sport,dport);
				} else {
					prev_end_offset = tag_end_offset;
				}
			}
		}
		return ACCEPT_BOOL;
	}
	else{
		return ACCEPT_BOOL;
	}
}


int callback( struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa,
	void *data ) {

	UNUSED(nfmsg);
	UNUSED(data);

	struct iphdr *iphdr = NULL;
	//struct tcphdr *tcphdr = NULL;
	struct nfqnl_msg_packet_hdr *ph = NULL;
	unsigned char *packet= NULL;
	int id = 0;

	ph = nfq_get_msg_packet_hdr(nfa);
	if (ph) {
		id = ntohl(ph->packet_id);
	} else {
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	}

	if (nfq_get_payload(nfa, &packet) == -1) {
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	}

	iphdr = (struct iphdr *) packet;
	if ((iphdr->ihl < 5) || (iphdr->ihl > 15)) {
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	}

	//tcphdr = (struct tcphdr *) (packet + (iphdr->ihl<<2));
	//
	//if (tcp_synack_segment(tcphdr)) {
	//	if (rewrite_win_size(packet) != 0) {
	//		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	//	}
	//	return nfq_set_verdict(qh, id, NF_ACCEPT, ntohs(iphdr->tot_len), packet);
	//
	//} else {
                if (check_packet_against_hostname(packet)) {
                        printf ("Connection dropped\n");
			return nfq_set_verdict(qh, id, NF_ACCEPT, ntohs(iphdr->tot_len), packet);
		}else{
			return nfq_set_verdict(qh, id, NF_ACCEPT, ntohs(iphdr->tot_len), packet);
                }		
	//}
}

int init_libnfq( struct nfq_handle **h, struct nfq_q_handle **qh ) {

	*h = nfq_open();
	if (!(*h)) {
		return 1;
	}

	if (nfq_unbind_pf(*h, AF_INET) < 0) {
		return 1;
	}

	if (nfq_bind_pf(*h, AF_INET) < 0) {
		return 1;
	}

	*qh = nfq_create_queue(*h,  200, &callback, NULL);
	if (!(*qh)) {
		fprintf(stderr,"error during nfq_create_queue()\n");
		return 1;
	}

	if (nfq_set_mode(*qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr,"Error: can't set packet_copy mode\n");
		return 1;
	}

	return 0;
}

int main() {

	int fd = 0;
	int rv = 0;
	char buf[4096] __attribute__ ((aligned));
	struct nfq_handle *h;
	struct nfq_q_handle *qh;

	while(1){
		if (init_libnfq(&h, &qh) != 0) {
			return 1;
		}
	
		fd = nfq_fd(h);		
		while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
			
			nfq_handle_packet(h, buf, rv);
		}
		fprintf(stderr,"Restarting nfqueue handling\n");
		//printf("process ending\n");
		nfq_destroy_queue(qh);
		nfq_close(h);
	}
	return 0;
}
