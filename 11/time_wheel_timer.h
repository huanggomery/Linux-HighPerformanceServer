// 时间轮定时器，书本207页
#ifndef TIME_WHEEL_TIMER_H
#define TIME_WHEEL_TIMER_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <list>
using std::vector;
using std::list;
#define BUF_SIZE 50

struct tw_timer;   // 前向声明
using timer_itr = list<tw_timer>::iterator;

// 用户数据和缓冲区
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUF_SIZE];
    timer_itr timer;
};

// 定时器节点
struct tw_timer
{
    typedef void (*cb_func_t)(client_data *);
    tw_timer(int ro, int s):
        rotation(ro),slot(s),cb_func(nullptr),user_data(nullptr){}
    int rotation;
    int slot;
    cb_func_t cb_func;
    client_data *user_data;
};

// 时间轮
class time_wheel
{
public:
    // 默认构造函数，槽数N=60,槽间隔SI=1 
    time_wheel();

    // 有参构造函数，n为槽数，si为槽间隔
    time_wheel(int n, int si);

    ~time_wheel() = default;

    // 根据定时值timeout，插入定时器
    timer_itr add_timer(int timeout);

    // 删除定时器，参数为指向定时器的迭代器
    void del_timer(timer_itr timer);

    void tick();

private:
    const int N;  // 槽数
    const int SI; // 槽间隔
    vector<list<tw_timer>> slots;  // 时间轮的槽，每个槽中都是一个定时器链表
    int cur_slot;  // 当前槽
};


#endif