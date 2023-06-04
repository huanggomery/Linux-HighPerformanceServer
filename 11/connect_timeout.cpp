// 使用socket选项SO_SNDTIMEO,来为connect函数定时，并处理连接超时的情况
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <utils.h>
#include <errno.h>

int timeout_connect(const char *ip = nullptr, int port = 0, int time = 5)
{
    if (port == 0)
        port = 9190;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip ? ip : "127.0.0.1");
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket error\n");
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        printf("setsockopt error");
        return -1;
    }

    int ret = connect(sockfd, (sockaddr *)&address, sizeof(address));
    if (ret == -1)
    {
        // 连接超时
        if (errno == EINPROGRESS)
        {
            printf("connecting timeout\n");
            return -1;
        }
        printf("error occur when connecting to server\n");
        return -1;
    }
    return sockfd;
}


int main(int argc, char *argv[])
{
    int sockfd = timeout_connect();
    if (sockfd < 0)
        error_handle("connect error");
    printf("connect successful\n");
    close(sockfd);
    return 0;
}