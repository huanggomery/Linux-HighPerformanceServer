// 使用信号量实现互斥锁，保护临界区
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <pthread.h>
#include <sys/sem.h>
#include "utils.h"

int sum = 0;
int sem_id;

void pv(int sem_id, int op)
{
    sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}

void *func(void *)
{
    for (int i = 0; i < 100000; ++i)
    {
        pv(sem_id, -1);
        ++sum;
        pv(sem_id, 1);
    }
}

int main()
{
    pthread_t tid1, tid2;
    sem_id = semget(IPC_PRIVATE, 1, 0666);
    semctl(sem_id, 0, SETVAL, 1);
    pthread_create(&tid1, nullptr, func, nullptr);
    pthread_create(&tid2, nullptr, func, nullptr);
    pthread_join(tid1, nullptr);
    pthread_join(tid2, nullptr);
    semctl(sem_id, 0, IPC_RMID, 0);
    printf("sum = %d\n", sum);

    return 0;
}