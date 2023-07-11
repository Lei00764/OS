#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_SIZE 512
#define PIPE_WRITE 1
#define PIPE_READ 0

void pipe_filter(int pipe_fd[2])
{
    int new_pipe_fd[2];
    pipe(new_pipe_fd);

    int buffer;
    int first_num;

    if (read(pipe_fd[PIPE_READ], &buffer, sizeof(int)) != sizeof(int)) // 递归终止条件：不能从管道读出一个int
    {
        exit(0);
    }
    first_num = buffer;

    printf("prime %d\n", first_num);

    while (read(pipe_fd[PIPE_READ], &buffer, sizeof(int)))
    {
        if (buffer % first_num != 0) // 说明不是 first_num 的倍数
        {
            write(new_pipe_fd[PIPE_WRITE], &buffer, sizeof(int));
        }
    }
    close(pipe_fd[PIPE_READ]); // 端口用完后，及时关闭
    close(new_pipe_fd[PIPE_WRITE]);

    pipe_filter(new_pipe_fd);
}

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Error\n");
        exit(-1);
    }

    int pipe_fd[2];
    pipe(pipe_fd);

    // 将管道写入 2~35
    for (int i = 2; i <= 35; i++)
    {
        write(pipe_fd[PIPE_WRITE], &i, sizeof(int));
    }

    close(pipe_fd[PIPE_WRITE]); // 写完之后，关闭写端口

    pipe_filter(pipe_fd);
    exit(0);
}