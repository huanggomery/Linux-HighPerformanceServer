// 使用主机名和服务名来访问目标服务器上的daytime服务
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <cstring>
#include <utils.h>

#define BUF_SIZE 50
char buf[BUF_SIZE];

int main(int argc, char *argv[])
{
    if (argc != 2)
        error_handle("need hostname");
    
    hostent *hostinfo = gethostbyname(argv[1]);
    assert(hostinfo);
    servent *servinfo = getservbyname("daytime","tcp");
    assert(servinfo);

    printf("host address: %s\n", inet_ntoa(*(in_addr *)*hostinfo->h_addr_list));
    printf("service port: %d\n", ntohs(servinfo->s_port));

    sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *(in_addr *)*(hostinfo->h_addr_list);
    addr.sin_port = servinfo->s_port;

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handle("socket error");
    
    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == -1)
        error_handle("connect error");
    
    recv(sock, buf, BUF_SIZE, 0);
    printf("message from server: %s\n", buf);
    close(sock);
    
    return 0;
}