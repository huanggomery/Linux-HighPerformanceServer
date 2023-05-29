/*
  获取和改变当前工作目录
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 100

int main()
{
    char buf[BUF_SIZE], path[BUF_SIZE] = "/home/huangchen/Documents";
    char *pwd;
    pwd = getcwd(buf, BUF_SIZE);
    if (!pwd)
    {
        printf("getcwd error\n");
        exit(-1);
    }
    printf("修改前:\n %s\n", pwd);

    if (chdir(path) == -1)
    {
        printf("chdir error\n");
        exit(-1);
    }

    pwd = getcwd(buf, BUF_SIZE);
    if (!pwd)
    {
        printf("getcwd error\n");
        exit(-1);
    }
    printf("修改后:\n %s\n", pwd);


    return 0;
}