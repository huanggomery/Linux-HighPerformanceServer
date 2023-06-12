// POSIX信号量、互斥锁、条件变量的封装 实现
#include "14/locker.h"
#include <exception>

sem::sem(int n)
{
    if (sem_init(&m_sem, 0, n) != 0)
        throw std::exception();
}

sem::~sem()
{ sem_destroy(&m_sem);}

int sem::wait()
{ return sem_wait(&m_sem);}

int sem::post()
{ return sem_post(&m_sem);}

locker::locker()
{
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
        throw std::exception();
}

locker::~locker()
{ pthread_mutex_destroy(&m_mutex);}

int locker::lock()
{ return pthread_mutex_lock(&m_mutex);}

int locker::unlock()
{ return pthread_mutex_unlock(&m_mutex);}

cond::cond()
{
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
        throw std::exception();
    if (pthread_cond_init(&m_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw std::exception();
    }
}

cond::~cond()
{ pthread_cond_destroy(&m_cond);}

int cond::wait()
{
    pthread_mutex_lock(&m_mutex);
    int ret = pthread_cond_wait(&m_cond, &m_mutex);
    pthread_mutex_unlock(&m_mutex);
    return ret;
}

int cond::signal()
{ return pthread_cond_signal(&m_cond);}