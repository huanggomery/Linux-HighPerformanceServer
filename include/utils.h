// 一些常用的工具函数
#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>

// 错误处理函数
void error_handle(char msg[]);

// 将文件描述符设置成非阻塞的,返回原先的文件描述符选项
int setnoblocking(int fd);

// 将文件描述符fd注册到epollfd内核事件表中，监听事件为EPOLLIN，参数enable_et指定是否启用边缘触发
void addfd(int epollfd, int fd, bool enable_et);

#endif