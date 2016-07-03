#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

int main()
{
    int fd;
    struct sockaddr_in servaddr;

    fd = socket(PF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5001);
    servaddr.sin_addr.s_addr = inet_addr("192.168.0.2");

    char sendline[100];
    sprintf(sendline, "Hello, world!");

    sendto(fd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    close(fd);

    return 0;
}
