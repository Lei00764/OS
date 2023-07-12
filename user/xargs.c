#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAX_SIZE 16

// 任务：echo hello too | xargs echo bye
// -> echo bye hello too
// hello too：前一个命令的标准化输出

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error\n");
        exit(-1);
    }

    // 处理 xargs 后面部分参数
    char *xargs_arr[MAXARG]; // 字符串输出
    int xargs_num = 0;
    for (int i = 1; i < argc; i++)
    {
        xargs_arr[xargs_num] = argv[i];
        xargs_num++;
    }

    char buffer[MAX_SIZE];
    int s_nums = 0;   // 记录buffer中读到的字符数量
    char *p = buffer; // 指向参数的首字母
    while (read(0, &buffer[s_nums], 1) == 1)
    {
        if (buffer[s_nums] == ' ')
        {
            buffer[s_nums] = 0;
            xargs_arr[xargs_num] = p;
            xargs_num++;
            p = &buffer[s_nums + 1];
        }
        else if (buffer[s_nums] == '\n')
        {
            buffer[s_nums] = 0;
            xargs_arr[xargs_num] = p;
            xargs_num++;
            p = &buffer[s_nums + 1];

            int pid = fork();
            if (pid == 0)
            {
                exec(xargs_arr[0], xargs_arr);
                exit(0);
            }
            else
            {
                wait(&pid);
                xargs_num = argc - 1;
            }
        }
        s_nums++;
    }

    exit(0);
}