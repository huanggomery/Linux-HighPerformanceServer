// 以多线程的方式，实现发送和接受数据的TCP客户端
// 需要输入服务器的IP地址和端口号，默认为127.0.0.1:9190

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "utils.h"

#define BUF_SIZE 50

void *send_routine(void *argv);
void *recv_routine(void *argv);

int main(int argc, char *argv[])
{
    char ip[20];
    int port;
    if (argc == 1)
    {
        strcpy(ip, "127.0.0.1");
        port = 9190;
    }
    else if (argc == 3)
    {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
    }
    else
        error_handle("arguments number error");
    
    // 设置地址
    sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(ip);
    srv_addr.sin_port = htons(port);

    // 创建客户端socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handle("socket error");

    // 连接服务器
    if (connect(sock, (sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
        error_handle("connect error");
    
    pthread_t send_tid, recv_tid;
    pthread_create(&send_tid, NULL, send_routine, (void *)&sock);
    pthread_create(&recv_tid, NULL, recv_routine, (void *)&sock);
    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);

    return 0;
}

void *send_routine(void *argv)
{
    int sock = *(int *)argv;
    char buf[BUF_SIZE];
    while (1)
    {
        memset(buf, 0, BUF_SIZE);
        fgets(buf, BUF_SIZE, stdin);
        if (strcmp(buf, "q\n") == 0)
        {
            shutdown(sock, SHUT_WR);
            break;
        }
        else
            send(sock, buf, strlen(buf), 0);
    }
    return NULL;
}

void *recv_routine(void *argv)
{
    int sock = *(int *)argv;
    char buf[BUF_SIZE];
    while (1)
    {
        memset(buf, 0, BUF_SIZE);
        int str_len = recv(sock, buf, BUF_SIZE, 0);
        if (str_len == 0)
        {
            close(sock);
            break;
        }
        else if (str_len < 0)
            continue;
        else
            printf("%s\n", buf);
    }
    return NULL;
}