// 并发模式：半同步/半反应堆式的线程池
// 书本301页
#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include "14/locker.h"
#include <pthread.h>
#include <list>
#include <vector>
#include <stdexcept>
using std::vector;
using std::list;

template <typename T>
class thread_pool
{
// 对外接口
public:
    thread_pool() = delete;
    thread_pool(int t_num = 20, int max_requests = 10000);
    ~thread_pool() = default;
    thread_pool(const thread_pool *tp) = delete;   // 阻止拷贝
    bool append(T *);  // 向工作队列添加任务
    void stop();  // 停止该线程池

// 私有方法
private:
    static void *worker(void *arg);   // 静态函数，用于创建线程时执行，arg是this指针
    void run();  // 线程实际工作的函数

// 私有变量
private:
    int m_threads_num;       // 线程数量
    int m_max_requests;      // 工作队列中最大请求数量
    vector<pthread_t> m_threads;   // 线程
    list<T*> m_workqueue;    // 工作队列
    sem m_queuestat;         // 信号量，表明工作队列中的任务数
    locker m_queuelocker;    // 保护工作队列的锁
    bool m_stop;             // 是否终止线程
};


// 构造函数，创建多个线程，让其运行worker()函数，并且detach
template <typename T>
thread_pool<T>::thread_pool(int t_num, int max_requests):
    m_threads_num(t_num), m_max_requests(max_requests), m_threads(t_num), m_queuestat(0), m_stop(false)
{
    if (t_num <= 0 || max_requests <= 0)
        throw std::runtime_error("需要大于0的值");
    for (int i = 0; i < m_threads_num; ++i)
    {
        if (pthread_create(&m_threads[i], NULL, worker, this) != 0)
            throw std::runtime_error("pthread_create failed");
        if (pthread_detach(m_threads[i]) != 0)
            throw std::runtime_error("pthread_detach failed");
    }
}

// 向工作队列中添加任务
template <typename T>
bool thread_pool<T>::append(T *task)
{
    m_queuelocker.lock();
    // 工作队列已满
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(task);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

// 静态函数，用于创建线程时执行，arg是this指针
template <typename T>
void *thread_pool<T>::worker(void *arg)
{
    thread_pool *self = (thread_pool *)arg;
    self->run();
    --self->m_threads_num;
    return nullptr;
}

// 线程实际工作的函数
template <typename T>
void thread_pool<T>::run()
{
    while (!m_stop)
    {
        m_queuestat.wait();  // 等待工作队列中有任务
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *task = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!task)
            continue;
        task->process();
    }
}

// 停止线程池
template <typename T>
void thread_pool<T>::stop()
{
    m_stop = true;
    while (m_threads_num > 0)
        m_queuestat.post();
}

#endif