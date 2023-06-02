// 以多线程的方式，实现发送和接受数据的UDP客户端
// 需要输入服务器的IP地址和端口号，默认为127.0.0.1:9190

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "utils.h"

#define BUF_SIZE 50

struct Target
{
    int sock;
    sockaddr_in srv_addr;
};
void *send_routine(void *argv);
void *recv_routine(void *argv);
bool flag = true;  // 指示线程是否可以退出
pthread_mutex_t mutex;

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
    
    pthread_mutex_init(&mutex, NULL);
    Target target;

    // 设置地址
    memset(&target.srv_addr, 0, sizeof(target.srv_addr));
    target.srv_addr.sin_family = AF_INET;
    target.srv_addr.sin_addr.s_addr = inet_addr(ip);
    target.srv_addr.sin_port = htons(port);

    // 创建客户端socket
    target.sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (target.sock == -1)
        error_handle("socket error");
    setnoblocking(target.sock);
    
    pthread_t send_tid, recv_tid;
    pthread_create(&send_tid, NULL, send_routine, (void *)&target);
    pthread_create(&recv_tid, NULL, recv_routine, (void *)&target);
    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);

    return 0;
}

void *send_routine(void *argv)
{
    int sock = ((Target *)argv)->sock;
    sockaddr_in srv_adddr = ((Target *)argv)->srv_addr;
    char buf[BUF_SIZE];
    while (flag)
    {
        memset(buf, 0, BUF_SIZE);
        fgets(buf, BUF_SIZE, stdin);
        if (strcmp(buf, "q\n") == 0)
        {
            pthread_mutex_lock(&mutex);
            flag = false;
            pthread_mutex_unlock(&mutex);
            break;
        }
        else
            sendto(sock, buf, strlen(buf), 0, (sockaddr *)&srv_adddr, sizeof(srv_adddr));
    }
    return NULL;
}

void *recv_routine(void *argv)
{
    int sock = ((Target *)argv)->sock;
    sockaddr_in srv_adddr = ((Target *)argv)->srv_addr;
    char buf[BUF_SIZE];
    while (flag)
    {
        memset(buf, 0, BUF_SIZE);
        socklen_t addr_len = sizeof(srv_adddr);
        int str_len = recvfrom(sock, buf, BUF_SIZE, 0, (sockaddr *)&srv_adddr, &addr_len);
        if (str_len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
            {
                pthread_mutex_lock(&mutex);
                flag = false;
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        printf("%s\n", buf);
    }
    return NULL;
}