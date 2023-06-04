#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

// 错误处理函数
void error_handle(char msg[])
{
    printf("%s\n", msg);
    exit(-1);
}

// 将文件描述符设置成非阻塞的,返回原先的文件描述符选项
int setnoblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_opt | O_NONBLOCK);
    return old_opt;
}

// 将文件描述符fd注册到epollfd内核事件表中，监听事件为EPOLLIN，参数enable_et指定是否启用边缘触发
void addfd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et)
        event.events |= EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnoblocking(fd);
}

// 创建客户端socket并连接服务器，服务器地址默认为127.0.0.1:9190，超时时间默认5s，成功则返回sockfd，否则返回-1
int connect_with_timeout(const char *ip, int port, int time)
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