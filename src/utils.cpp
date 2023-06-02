#include "utils.h"

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