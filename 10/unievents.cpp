/*
 使用epoll，统一处理网络数据和各类信号
 参考书本184页
*/
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "utils.h"

#define MAX_EVENT_NUM 1024
int pipefd[2];

// 信号处理函数，仅仅通过pipe告知主函数，实际处理动作在主函数中完成
void sig_hander(int sig)
{
    // 保存errno，在函数最后恢复，以保证函数的可重入性
    int old_errno = errno;
    send(pipefd[1], &sig, 1, 0);
    errno = old_errno;
}

// 设置信号的信号处理函数
void addsig(int sig)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_flags |= SA_RESTART;
    sigfillset(&act.sa_mask);
    act.sa_handler = sig_hander;
    if (sigaction(sig, &act, NULL) == -1)
        error_handle("sigaction error");
}

int main(int argc, char *argv[])
{
    printf("pid: %d\n", getpid());
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
    if (listenfd == -1)
        error_handle("socket error");
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));

    // 绑定地址
    if (bind(listenfd, (sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
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

    // 使用socketpair创建双向管道，非阻塞写，且把读取端注册到epoll例程
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd) == -1)
        error_handle("socketpair error");
    setnoblocking(pipefd[1]);
    addfd(epollfd, pipefd[0], true);

    // 设置一些信号的处理函数
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);
    
    bool stop_server = false;  // 产生SIGINT信号时置为true，退出主循环
    while (!stop_server)
    {
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if (num < 0 && errno != EINTR)  // 产生信号时会中断epoll_wait函数，返回-1且设置errno为EINTR
            error_handle("epoll failure");
        
        for (int i = 0; i < num; ++i)
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
            // 有信号
            else if (sockfd == pipefd[0] && events[i].events & EPOLLIN)
            {
                char signals[1024];
                int ret = recv(pipefd[0], signals, 1024, 0);
                if (ret <= 0)
                    continue;
                for (int j = 0; j < ret; ++j)
                {
                    switch (signals[j])
                    {
                    case SIGHUP:
                    {
                        printf("get SIGHUP\n");
                        continue;
                    }
                    case SIGCHLD:
                    {
                        printf("get SIGCHLD\n");
                        continue;
                    }
                    case SIGTERM:
                    {
                        printf("get SIGTERM\n");
                        stop_server = true;
                        break;
                    }
                    case SIGINT:
                    {
                        printf("get SIGINT\n");
                        stop_server = true;
                        break;
                    }
                    }
                }
            }
            // 接受到TCP数据
            else
            {
                // 不做处理
            }
        }
    }
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}