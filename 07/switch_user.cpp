/*
以root身份启动，以普通用户身份执行
书117页
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

bool switch_to_user(uid_t user_id, gid_t gp_id)
{
    uid_t uid = getuid();
    gid_t gid = getuid();
    if (uid == user_id && gid == gp_id)
        return true;
    if (setgid(gp_id) < 0 || setuid(user_id) < 0)
        return false;
    return true;
}

int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    gid_t gid = getgid();
    printf("uid: %d,  euid: %d,  gid: %d\n", uid, euid, gid);
    if (!switch_to_user(1000,1000))
    {
        printf("set error\n");
        exit(-1);
    }
    
    uid = getuid();
    euid = geteuid();
    gid = getgid();
    printf("uid: %d,  euid: %d,  gid: %d\n", uid, euid, gid);
    
    return 0;
}