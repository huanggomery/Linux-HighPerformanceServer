// 利用epoll实现IO复用，同时处理TCP和UDP消息的回声服务器
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>

#define BUF_SIZE 50
#define MAX_EVENT_NUM 1024
char buf[BUF_SIZE];

int main(int argc, char *argv[])
{
    int port;
    if (argc > 2)
        error_handle("arguments number error");
    else if (argc == 2)
        port = atoi(argv[1]);
    else
        port = 9190;
    
    // 设置地址
    sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    // 创建TCP和UDP的socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (listenfd == -1 || udpfd == -1)
        error_handle("socket error");
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));

    // 绑定地址
    if (bind(listenfd, (sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
        error_handle("bind error");
    if (bind(udpfd, (sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
        error_handle("bind error");
    
    // 开始TCP监听
    if (listen(listenfd, 5) == -1)
        error_handle("listen error");
    
    // 创建epoll内核注册表
    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(5);
    if (epollfd == -1)
        error_handle("epoll_create error");
    addfd(epollfd, listenfd, true);
    addfd(epollfd, udpfd, true);

    while (1)
    {
        int event_nums = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if (event_nums < 0)
            error_handle("epoll failure");
        for (int i = 0; i < event_nums; ++i)
        {
            int sockfd = events[i].data.fd;

            // 有新的TCP连接
            if (sockfd == listenfd)
            {
                sockaddr_in cln_addr;
                socklen_t addr_len = sizeof(cln_addr);
                int connfd = accept(listenfd, (sockaddr *)&cln_addr, &addr_len);
                if (connfd == -1)
                    continue;
                addfd(epollfd, connfd, true);
                printf("new tcp connection: %d\n", connfd);
            }
            // 新的UDP数据
            else if (sockfd == udpfd)
            {
                memset(buf, 0, BUF_SIZE);
                sockaddr_in cln_addr;
                socklen_t addr_len = sizeof(cln_addr);
                while (1)
                {
                    int str_len = recvfrom(udpfd, buf, BUF_SIZE, 0, (sockaddr *)&cln_addr, &addr_len);
                    if (str_len < 0)  // 数据已经读完，或者发生错误
                        break;
                    printf("get udp message: %s\n", buf);
                    sendto(udpfd, buf, str_len, 0, (sockaddr *)&cln_addr, addr_len);
                }
            }
            // 新的TCP数据
            else if (events[i].events & EPOLLIN)
            {
                memset(buf, 0, BUF_SIZE);
                while (1)
                {
                    int str_len = recv(sockfd, buf, BUF_SIZE, 0);
                    if (str_len < 0)  // 数据已经读完，或者发生错误
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        else
                        {
                            close(sockfd);
                            break;
                        }
                    }
                    else if (str_len == 0)  // 客户端断开连接
                    {
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
                        close(sockfd);
                        printf("tcp disconnetion: %d\n", sockfd);
                        break;
                    }
                    else  // 正常读取
                    {
                        printf("get tcp message: %s\n", buf);
                        send(sockfd, buf, str_len, 0);
                    }
                }
            }
            else
                printf("something else happened\n");
        }
    }
}