// POSIX信号量、互斥锁、条件变量的封装 头文件
// 280页
#ifndef _LOCKER_H
#define _LOCKER_H

#include <semaphore.h>
#include <pthread.h>

class sem
{
public:
    sem(int n = 0);
    ~sem();
    int wait();
    int post();

private:
    sem_t m_sem;
};

class locker
{
public:
    locker();
    ~locker();
    int lock();
    int unlock();

private:
    pthread_mutex_t m_mutex;
};

class cond
{
public:
    cond();
    ~cond();
    int wait();
    int signal();

private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
};

#endif