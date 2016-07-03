#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <assert.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>

uint16_t in_checksum(const void* buf, int len)
{
    assert(len % 2 == 0);
    const uint16_t* data = static_cast<const uint16_t*>(buf);
    int sum = 0;
    for (int i = 0; i < len; i+=2)
    {
        sum += *data++;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    assert(sum <= 0xFFFF);
    return ~sum;
}

int tun_create(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    
    //The dev shouldn't be none
    assert(dev != NULL);
    
    //Open tun
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
        return fd;
    
    //Initialize ifr
    bzero(&ifr, sizeof(ifr));
    ifr.ifr_flags |= flags;
    
    //When the name had been specified
    if (*dev != '\0')
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    //Initialize struct ifr, get information
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
    {
        close(fd);
        return err;
    }
    
    strcpy(dev, ifr.ifr_name);
    return fd;
}

void icmp_input(int fd, const void* input, const void* payload, int len)
{
    const struct iphdr* iphdr = static_cast<const struct iphdr*>(input);
    const struct icmphdr* icmphdr = static_cast<const struct icmphdr*>(payload);
    // const int icmphdr_size = sizeof(*icmphdr);
    const int iphdr_len = iphdr->ihl*4;

    if (icmphdr->type == ICMP_ECHO)
    {
        char source[INET_ADDRSTRLEN];
        char dest[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &iphdr->saddr, source, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &iphdr->daddr, dest, INET_ADDRSTRLEN);
        printf("IP %s > %s: ", source, dest);
        printf("ICMP echo request, id %d, seq %d, length %d\n",
            ntohs(icmphdr->un.echo.id),
            ntohs(icmphdr->un.echo.sequence),
            len - iphdr_len);
    }
}

void tcp_input(int fd, const void* input, const void* payload, int tot_len)  
{  
    const struct iphdr* iphdr = static_cast<const struct iphdr*>(input);
    const struct tcphdr* tcphdr = static_cast<const struct tcphdr*>(payload);
    const int iphdr_len = iphdr->ihl*4;
    const int tcp_seg_len = tot_len - iphdr_len;
    const int tcphdr_size = sizeof(*tcphdr);
    if (tcp_seg_len >= tcphdr_size
        && tcp_seg_len >= tcphdr->doff*4)
    {
        const int tcphdr_len = tcphdr->doff*4;
        if (tcphdr->syn)
        {
            char source[INET_ADDRSTRLEN];
            char dest[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &iphdr->saddr, source, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &iphdr->daddr, dest, INET_ADDRSTRLEN);
            printf("IP %s:%d > %s:%d: ", source, ntohs(tcphdr->source), dest, ntohs(tcphdr->dest));
            printf("TCP request, seq %u , length %d\n", ntohl(tcphdr->seq), tot_len - iphdr_len - tcphdr_len);
        }
    }
}

void udp_input(int fd, const void* input, const void* payload, int len)
{
    const struct iphdr* iphdr = static_cast<const struct iphdr*>(input);
    const struct udphdr* udphdr = static_cast<const struct udphdr*>(payload);
    char source[INET_ADDRSTRLEN];
    char dest[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &iphdr->saddr, source, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &iphdr->daddr, dest, INET_ADDRSTRLEN);
    printf("IP %s:%d > %s:%d: ", source, ntohs(udphdr->source), dest, ntohs(udphdr->dest));
    printf("UDP request, length %d\n", ntohl(udphdr->len));
}

void igmp_input(int fd, const void* input, const void* payload, int len)
{
    const struct iphdr* iphdr = static_cast<const struct iphdr*>(input);
    const struct igmphdr* igmphdr = static_cast<const struct igmphdr*>(payload);
    char source[INET_ADDRSTRLEN];
    char dest[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &iphdr->saddr, source, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &iphdr->daddr, dest, INET_ADDRSTRLEN);
    printf("IP %s > %s ", source, dest);
    printf("IGMP request, length %d\n", len);
}

int main(int argc, char *argv[])
{
    int fd, ret;
    char tun_name[IFNAMSIZ];
    unsigned char buf[4096];

    tun_name[0] = '\0';
    
    //Create a tun device
    fd = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
    if (fd < 0)
    {
            perror("tun_create");
            return 1;
    }    
    printf("TUN name is %s\n", tun_name);

    //Recieve packets
    while (1)
    {
        union
        {
            unsigned char buf[ETH_FRAME_LEN];
            struct iphdr iphdr;
        };

        const int iphdr_size = sizeof iphdr;

        int nread = read(fd, buf, sizeof(buf));
        if (nread < 0)
        {
            perror("read");
            close(fd);
            exit(1);    
        }
        //printf("read %d bytes from tunnel interface %s.\n", nread, tun_name);

        const int iphdr_len = iphdr.ihl*4;
        if (nread >= iphdr_size
                && iphdr.version == 4
                && iphdr_len >= iphdr_size
                && iphdr_len <= nread
                && iphdr.tot_len == htons(nread)
                && in_checksum(buf, iphdr_len) == 0)
        {
            const void* payload = buf + iphdr_len;
            if (iphdr.protocol == IPPROTO_ICMP)
            {
                icmp_input(fd, buf, payload, nread);
            }
            else if (iphdr.protocol == IPPROTO_TCP)
            {
                tcp_input(fd, buf, payload, nread);
            }
            else if (iphdr.protocol == IPPROTO_UDP)
            {
                udp_input(fd, buf, payload, nread);
            }
        }
    }

    return 0;
}
