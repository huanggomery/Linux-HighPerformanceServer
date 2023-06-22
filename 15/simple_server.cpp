// 使用线程池，实现一个简单的服务器，把发来的字符串大小写互换，发回客户端
// 在主线程中读写IO，所以是模拟Proactor模式

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "15/thread_pool.h"
#include "utils.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
#define BUF_SIZE 2048

int sigpipe[2];
void sig_hander(int sig)
{
    // 保存errno，在函数最后恢复，以保证函数的可重入性
    int old_errno = errno;
    send(sigpipe[1], &sig, 1, 0);
    errno = old_errno;
}

class echo_conn
{
public:
    echo_conn():m_read_index(0),m_sock(-1){}
    void init(int sock, sockaddr_in addr);
    bool read();
    bool write();
    void process();
    void close_conn();

    // 静态数据，即所有对象共用一个epollfd
    static int m_epfd;
    static int user_count;

private:
    void clear_buf();   // 清空缓冲区

private:
    char m_buf[BUF_SIZE];
    int m_read_index;  // 记录下一次读到缓存中的哪个位置
    int m_sock;
    sockaddr_in m_addr;

};
int echo_conn::m_epfd = -1;
int echo_conn::user_count = 0;

void echo_conn::clear_buf()
{
    memset(m_buf, 0, BUF_SIZE);
    m_read_index = 0;
}

void echo_conn::init(int sock, sockaddr_in addr)
{
    m_sock = sock;
    m_addr = addr;
    addfd(m_epfd, sock, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    ++user_count;
    clear_buf();
}

// 从套接字读取数据
bool echo_conn::read()
{
    int str_len;
    while (1)
    {
        str_len = recv(m_sock, m_buf+m_read_index, BUF_SIZE-m_read_index, 0);
        if (str_len == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true;
            return false;
        }
        else if (str_len == 0)
            return false;
        else
            m_read_index += str_len;
    }
}

// 向套接字发送数据
bool echo_conn::write()
{
    int bytes_have_write = 0;  // 已经发送的字节数
    int str_len;
    while (bytes_have_write < m_read_index)
    {
        str_len = send(m_sock, m_buf+bytes_have_write, strlen(m_buf+bytes_have_write), 0);
        if (str_len == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                modfd(m_epfd, m_sock, EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
                return true;
            }
            return false;
        }
        else if (str_len == 0)
            return false;
        else
            bytes_have_write += str_len;
    }
    clear_buf();
    modfd(m_epfd, m_sock, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    return true;
}

// 业务逻辑，大写变小写，小写变大写，其他不变
void echo_conn::process()
{
    for (int i = 0; i < m_read_index; ++i)
    {
        if (m_buf[i] >= 'a' && m_buf[i] <= 'z')
            m_buf[i] += ('A'-'a');
        else if (m_buf[i] >= 'A' && m_buf[i] <= 'Z')
            m_buf[i] += ('a'-'A');
    }
    modfd(m_epfd, m_sock, EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
}

// 断开连接
void echo_conn::close_conn()
{
    // printf("disconnected\n");
    removefd(m_epfd, m_sock);
    close(m_sock);
    m_sock = -1;
    clear_buf();
    --user_count;
}

thread_pool<echo_conn> pool(20,10000);
int main(int argc, char *argv[])
{
    int listenfd;
    if ((listenfd = create_and_listen()) == -1)
        error_handle("create error");
    
    echo_conn *users = new echo_conn[MAX_FD];
    if (!users)
        error_handle("users error");
    
    epoll_event events[MAX_EVENT_NUMBER];
    int epfd = epoll_create(5);
    echo_conn::m_epfd = epfd;
    addfd(epfd, listenfd, EPOLLIN | EPOLLET);

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, sigpipe) != 0)
        error_handle("socketpair error");
    setnoblocking(sigpipe[1]);
    addfd(epfd, sigpipe[0], EPOLLIN | EPOLLET);

    addsig(SIGINT, sig_hander);
    addsig(SIGTERM, sig_hander);

    bool server_stop = false;
    while (!server_stop)
    {
        int num = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if (num < 0 && errno != EINTR)
        {
            printf("epoll failed\n");
            break;
        }

        for (int i = 0; i < num; ++i)
        {
            int sockfd = events[i].data.fd;
            // 新的连接请求
            if (sockfd == listenfd)
            {
                // printf("new client\n");
                sockaddr_in addr;
                socklen_t addr_len = sizeof(addr);
                int connfd = accept(listenfd, (sockaddr *)&addr, &addr_len);
                if (connfd < 0)
                    continue;
                if (echo_conn::user_count >= MAX_FD)
                {
                    send(connfd, "server busy", 12, 0);
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd, addr);
            }
            // 收到信号
            else if (sockfd == sigpipe[0])
            {
                char sigs[1024];
                memset(sigs, 0, 1024);
                int ret = recv(sigpipe[0], sigs, 1024, 0);
                for (int j = 0; j < ret; ++j)
                {
                    switch (sigs[j])
                    {
                    case SIGINT:
                    case SIGTERM:
                    {
                        // printf("stop server\n");
                        server_stop = true;
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
            // 收到新的数据
            else if (events[i].events & EPOLLIN)
            {
                // printf("get new message\n");
                if (users[sockfd].read())
                    pool.append(users+sockfd);
                else
                    users[sockfd].close_conn();
            }
            // 可以发送数据
            else if (events[i].events & EPOLLOUT)
            {
                if(!users[sockfd].write())
                    users[sockfd].close_conn();
            }
            // 断开连接或者错误
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
        }
    }
    close(listenfd);
    close(epfd);
    pool.stop();
    return 0;
}