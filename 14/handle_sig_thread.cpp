// 用一个线程处理该进程的所有信号
// 285页

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

void *handle_sig(void *args)
{
    printf("pid: %d\n", getpid());
    sigset_t sigs = *(sigset_t *)args;
    int sig;
    while (1)
    {
        if (sigwait(&sigs, &sig) != 0)
            return nullptr;
        printf("get signal: %d\n", sig);
    }
}

int main(int argc, char *argv[])
{
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCHLD);
    sigaddset(&sigs, SIGUSR1);
    sigaddset(&sigs, SIGQUIT);

    sigprocmask(SIG_SETMASK, &sigs, nullptr);
    
    pthread_t tid;
    pthread_create(&tid, NULL, handle_sig, &sigs);
    pthread_join(tid, nullptr);
}