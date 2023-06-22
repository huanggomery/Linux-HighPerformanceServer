// 一些常用的工具函数
#ifndef _UTILS_H
#define _UTILS_H
#include <signal.h>

// 错误处理函数
void error_handle(char msg[]);

// 将文件描述符设置成非阻塞的,返回原先的文件描述符选项
int setnoblocking(int fd);

// 将文件描述符fd注册到epollfd内核事件表中，参数ev是监听事件,默认是读
void addfd(int epollfd, int fd, int ev);

// 修改epollfd内核事件表
void modfd(int epollfd, int fd, int ev);

// 从epollfd内核事件表中删除某个文件描述符
void removefd(int epollfd, int fd);

// 创建客户端socket并连接服务器，服务器地址默认为127.0.0.1:9190，超时时间默认-1，不设置超时，成功则返回sockfd，否则返回-1
int connect_with_timeout(const char *ip = nullptr, int port = 0, int time = -1);

// 创建服务器socket，绑定地址，开始监听，服务器地址默认为127.0.0.1:9190
int create_and_listen(const char *ip = nullptr, int port = 0);

// 信号处理函数，仅仅通过pipe告知主函数，实际处理动作在主函数中完成
void sig_hander(int sig);

// 设置信号的信号处理函数，默认为sig_handler
void addsig(int sig, __sighandler_t handler = sig_hander);

#endif