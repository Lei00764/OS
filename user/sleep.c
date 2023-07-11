/**
  *Copyright(C),2020 - 2021, Company Tech. Co., Ltd.
  *FileName:   sleep.c
  *Date:       2023-07-10 14:24:30
  *Author:     Xiang Lei
  *Version:    1.0
  *Path:       ~/Desktop/xv6-labs-2021/user
  *Description:
*/


#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(argc <= 1){
        printf("Error");
        exit(-1);
    }

    int s_time = atoi(argv[1]);  // atoi()函数将字符串转换成整数
    sleep(s_time);  

    exit(0);
}