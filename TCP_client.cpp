#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#define MAXBUF 256

int main()
{
    int fd, len;
    struct sockaddr_in server_addr;
    char buf[MAXBUF];
    
    if((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("socket error:");
        exit(1);
    }
    
    len = sizeof(server_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.2");
    server_addr.sin_port = htons(7758);
    
    if(connect(fd, (struct sockaddr *)&server_addr, len) < 0)
    {
        perror("connect error:");
        exit(1);
    }
    memset(buf, 0, MAXBUF);
    
    if(read(fd, buf, MAXBUF)<=0)
    {
        perror("read error:");
        exit(1);
    }
    
    close(fd);
    printf("\nread: %s\n", buf);
    return 0;
}
