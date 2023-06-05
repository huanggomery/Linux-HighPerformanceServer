// 回声服务端，能够断开不活跃的用户连接
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include "utils.h"
#include "time_wheel_timer.h"

#define FD_LIMIT 1024           // 文件描述符限制数量
#define MAX_EVENT_NUMBER 1024   // 同时监听事件的最大数量
#define TIMEOUT 20              // 不活跃时间

int pipefd[2];
time_wheel wheel;

// 设定定时器
void alarm_set(int time = 1)
{
    alarm(time);
}

// 定时器触发后实际调用这个函数
void alarm_handle()
{
    alarm_set();
    wheel.tick();
}

// 断开与客户端的连接
void cb_func(client_data *user)
{
    printf("sockfd = %d has been closed\n", user->sockfd);
    send(user->sockfd, "connect over!\n", 14, 0);
    close(user->sockfd);
}

int main(int argc, char *argv[])
{
    if (argc > 2)
        error_handle("arguments error");

    // 获取socket，并开始监听
    int listenfd;
    if (argc == 2)
        listenfd = create_and_listen(nullptr, atoi(argv[1]));
    else
        listenfd = create_and_listen();

    // 设置epoll相关
    int epfd = epoll_create(5);
    epoll_event events[MAX_EVENT_NUMBER];
    addfd(epfd, listenfd);

    // 设置管道
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd) == -1)
        error_handle("socketpair error");
    setnoblocking(pipefd[1]);
    addfd(epfd, pipefd[0]);

    // 设置SIGALRM时钟信号的处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);
    addsig(SIGINT);

    client_data users[FD_LIMIT];
    bool stop_server = false;
    alarm_set();
    while (!stop_server)
    {
        int num = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if (num < 0 && errno != EINTR)
            error_handle("epoll error");
        
        bool timeout = false;
        for (int i = 0; i < num; ++i)
        {
            int sockfd = events[i].data.fd;
            // 有新的连接
            if (sockfd == listenfd)
            {
                sockaddr_in cln_addr;
                socklen_t addr_len = sizeof(cln_addr);
                int connfd = accept(listenfd, (sockaddr *)&cln_addr, &addr_len);
                if (connfd == -1)
                {
                    printf("connect failure");
                    continue;
                }
                addfd(epfd, connfd);
                auto itr = wheel.add_timer(TIMEOUT);
                users[connfd].address = cln_addr;
                users[connfd].sockfd = connfd;
                users[connfd].timer = itr;
                itr->user_data = &users[connfd];
                itr->cb_func = cb_func;
                printf("new tcp connection: %d\n", connfd);
            }
            // 收到信号
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
                    case SIGALRM:
                        timeout = true;
                        break;
                    case SIGTERM:
                    case SIGINT:
                        printf("server stop\n");
                        stop_server = true;
                        break;
                    }
                }
            }
            // 客户端发来数据
            else if (events[i].events & EPOLLIN)
            {
                int str_len;
                while (1)
                {
                    memset(users[sockfd].buf, 0, BUF_SIZE);
                    str_len = recv(sockfd, users[sockfd].buf, BUF_SIZE, 0);
                    // 客户端断开连接
                    if (str_len == 0)
                    {
                        cb_func(&users[sockfd]);
                        wheel.del_timer(users[sockfd].timer);
                        break;
                    }
                    else if (str_len == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        // 接收数据异常，断开连接
                        else
                        {
                            cb_func(&users[sockfd]);
                            wheel.del_timer(users[sockfd].timer);
                            break;
                        }
                    }
                    // 正常读取数据
                    else
                    {
                        printf("get tcp message: %s\n", users[sockfd].buf);
                        send(sockfd, users[sockfd].buf, str_len, 0);
                        // 重新设置定时器
                        wheel.del_timer(users[sockfd].timer);
                        auto itr = wheel.add_timer(TIMEOUT);
                        users[sockfd].timer = itr;
                        itr->cb_func = cb_func;
                        itr->user_data = &users[sockfd];
                    }
                }
            }
            else
            {
                /* do nothing */
            }
        }
        if (timeout)
            alarm_handle();
        
    }
    close(epfd);
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
