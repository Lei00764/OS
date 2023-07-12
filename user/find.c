#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 任务：find .b
// 在 . 目录下找 b

/**
 * @brief 从完全路径名中提取出文件或目录的名字
 *
 * @param path
 * @return char*
 */
char *
get_name_from_path(char *path)
{

    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    return p;
}

void find(char *path, char *file_name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) // 文件是否打开成功
    {
        fprintf(2, "findA: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) // 获取文件描述符fd对应的文件或目录的状态信息
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
        // printf("%s\n", get_name_from_path(path));
        // printf("%s\n", file_name);
        if (strcmp(get_name_from_path(path), file_name) == 0)
        {
            printf("%s\n", path); // 唯一输出
        }
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }

        strcpy(buf, path); // 将 path 复制到 buf
        p = buf + strlen(buf);
        *p++ = '/'; // 尾部添加

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            // 必须剔除 . 和 ..，否则无限递归
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) // 目录项是空
                continue;

            memmove(p, de.name, DIRSIZ); // 用 de.name 替换 p
            p[DIRSIZ] = 0;

            // printf("buf: %s\n", buf);  // 测试
            // printf("递归调用：%s, %s\n", buf, file_name); // 测试
            find(buf, file_name);
        }
        break;
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("Error\n");
        exit(-1);
    }

    find(argv[1], argv[2]);
    exit(0);
}