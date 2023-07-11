#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_SIZE 512


// 管道通信

// 要求：
// 父进程应该向子进程发送一个字节;子进程应该输出"<pid>: received ping"，
// 其中<pid>是它的进程ID，将管道上的字节写入父进程，然后退出;
// 父进程应该从子进程读取字节，打印"<pid>: received pong"，然后退出。


// 实现：
// 采用一个管道
// 父进程实现"ping"的写入和"pong"的读出
// 子进程实现"pong"的写入和"ping"的读出

/**
 * 在C语言中，pipe()函数是一个用于创建管道的系统调用，它接受一个大小为2的整数数组作为参数。
 * 当pipe()函数成功执行后，这个数组中的两个元素会被设置为新创建的管道的文件描述符。
 */

int main(int argc, char *argv[])
{
    if (argc != 1)  // 不接受参数
    {
        printf("Error");
        exit(-1);
    }

    int p1[2];  // p1[0] 为管道读出端、p1[1] 为管道写入端
    int p2[2];
    pipe(p1);
    pipe(p2);

    int pid = fork();  // 创建子进程
    if (pid == 0)  // pid = 0 说明是子进程
    {
        // 子进程
        char *buffer1 = malloc(sizeof(char) * MAX_SIZE);
        read(p1[0], buffer1, 4);
        printf("%d: received %s\n", getpid(), buffer1);
        
        write(p2[1], "pong", 4);
    }
    else
    {
        // 父进程
        write(p1[1], "ping", 4);

        char *buffer2 = malloc(sizeof(char) * MAX_SIZE);
        read(p2[0], buffer2, 4);
        printf("%d: received %s\n", getpid(), buffer2);
    }

    exit(0);
}