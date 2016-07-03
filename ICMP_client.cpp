#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>

void send_echo_req(int sockfd, struct sockaddr_in *dstaddr);
uint16_t in_cksum(uint16_t *addr, int len);

int main()  
{
    int sockfd;
    struct sockaddr_in dstaddr;  

    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        perror("socket");
        exit(1);  
    }

    bzero(&dstaddr, sizeof(dstaddr));
    dstaddr.sin_family = AF_INET;
    dstaddr.sin_port = htons(0);

    if (inet_pton(AF_INET, "192.168.0.2", &dstaddr.sin_addr) <= 0)
    {
        perror("inet_pton");
        exit(1);  
    }

    send_echo_req(sockfd, &dstaddr);
    exit(0);
}  

void send_echo_req(int sockfd, struct sockaddr_in *dstaddr)  
{
    char buf[100];
    size_t len = sizeof(struct icmp);
    struct icmp *icmp;
    socklen_t dstlen = sizeof(struct sockaddr_in);

    bzero(buf, sizeof(buf));
    icmp = (struct icmp *)buf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = getpid();
    icmp->icmp_seq = 1;
    icmp->icmp_cksum = in_cksum((uint16_t *) icmp, sizeof(struct icmp));
    if (sendto(sockfd, buf, len, 0, (struct sockaddr *)dstaddr, dstlen) == -1)
    {
        perror("send to");
        exit(1);  
    } 
}

uint16_t in_cksum(uint16_t *addr, int len)  
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return(answer);
}
