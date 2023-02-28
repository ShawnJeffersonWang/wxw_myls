#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>

#define PARAM_NONE 0
#define PARAM_a 1
#define PARAM_l 2
#define PARAM_R 4
#define PARAM_r 8
#define PARAM_t 16
#define PARMA_i 32
#define PARMA_s 64
#define PATH_MAX 4096

char PATH[PATH_MAX];
int flag;

void display_dir(char *path);
void display(char **filenames, int count);
void display_file(char *name);
void displayR(char *name);
void colorprint(char *name, mode_t st_mode);
void display_l(char *name);
void display_i(char *name);
void display_s(char *name);

int main(int argc, char *argv[])
{
    flag = PARAM_NONE;
    char *path[1];
    path[0] = (char *)malloc(sizeof(char) * PATH_MAX);
    char param[32] = {0};
    int k = 0;
    int num = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            for (int j = 1; j < strlen(argv[i]); j++)
            {
                param[k] = argv[i][j];
                k++;
            }
            num++; // bug1 计算'-'的数量,本身./myls2算1,加上'-'的数量num如果等于argc,说明未指定路径
        }
    }

    for (int i = 0; i < k; i++)
    {
        if (param[i] == 'a')
        {
            flag |= PARAM_a;
        }
        else if (param[i] == 'l')
        {
            flag |= PARAM_l;
        }
        else if (param[i] == 'R')
        {
            flag |= PARAM_R;
        }
        else if (param[i] == 'r')
        {
            flag |= PARAM_r;
        }
        else if (param[i] == 't')
        {
            flag |= PARAM_t;
        }
        else if (param[i] == 'i')
        {
            flag |= PARMA_i;
        }
        else if (param[i] == 's')
        {
            flag |= PARMA_s;
        }
        else
        {
            printf("myls:invalid options -%c\n", param[i]); // 显示输的啥是错的
            exit(0);
        }
    }

    if (1 + num == argc)
    {
        strcpy(path[0], ".");
        display_dir(path[0]);
        printf("\n"); // bug5 换行,这样下次可以从头开始输入
        return 0;
    }

    int i = 1;
    struct stat sbuf;
    do
    {
        if (argv[i][0] == '-')
        {
            i++;
            continue;
        }
        else
        {
            strcpy(path[0], argv[i]);
            if (stat(argv[i], &sbuf) == -1)
            {
                perror("stat error1");
                exit(1);
            }

            if (S_ISDIR(sbuf.st_mode))
            {
                display_dir(path[0]);
                i++;
            }
            else
            {
                display(path, 1); // 处理普通文件
                i++;
            }
        }
    } while (i < argc);

    printf("\n");

    return 0;
}

void display_dir(char *path) // 有且仅有该函数处理目录 排序必备
{
    DIR *dir;
    struct dirent *sdp;
    int count = 0;
    char temp[PATH_MAX];

    if ((flag & PARAM_R) != 0) // 作为一个强迫症，绝不允许格式上出问题
    {

        if (path[0] == '.' || path[0] == '/') //'.'表示传进来的是当前目录,'/'表示传进来的是自己写的目录
        {
            strcat(PATH, path);
        }
        else // 必须有,不然会段错误(核心已转储)
        {
            strcat(PATH, "/");
            strcat(PATH, path);
        }
        printf("%s:\n", PATH);
    }

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("opendir error");
        return;
    }
    while ((sdp = readdir(dir)) != NULL)
    {
        count++; // 计算目录打开后读取的文件数
    }
    closedir(dir);

    char **filenames = (char **)malloc(sizeof(char *) * count); // bug2 栈溢出 核心已转储
    for (int i = 0; i < count; i++)                             // 通过目录中文件数来动态的在堆上分配空间
    {
        filenames[i] = (char *)malloc(sizeof(char) * PATH_MAX); // 首先定义一个指针数组,然后依次让数组中每个指针指向分配好的空间,防止栈溢出
    }

    // char filenames[300][PATH_MAX+1],temp[PATH_MAX+1];  这里是被优化掉的代码，由于函数中定义的变量
    // 是在栈上分配空间，因此当多次调用的时候会十分消耗栈上的空间，最终导致栈溢出，在linux上的表现就是核心已转储
    // 并且有的目录中的文件数远远大于300

    dir = opendir(path);
    for (int i = 0; i < count; i++)
    {
        sdp = readdir(dir);
        if (sdp == NULL)
        {
            perror("readdir error");
            return;
        }
        strcpy(filenames[i], sdp->d_name); // 核心步骤,filenames装着path目录打开后获得的所有文件名
    }
    closedir(dir);

    struct stat sbuf1;
    struct stat sbuf2;
    if (flag & PARAM_t) // 用结构体的核心所在,要排序
    {
        for (int i = 0; i < count - 1; i++)
        {
            for (int j = 0; j < count - 1 - i; j++)
            {
                stat(filenames[j], &sbuf1);
                stat(filenames[j + 1], &sbuf2);
                if (sbuf1.st_mtime < sbuf2.st_mtime)
                {
                    strcpy(temp, filenames[j]);
                    strcpy(filenames[j], filenames[j + 1]);
                    strcpy(filenames[j + 1], temp);
                }
            }
        }
    }
    else if (flag & PARAM_r) //-r参数反向排序
    {
        for (int i = 0; i < count - 1; i++)
        {
            for (int j = 0; j < count - 1 - i; j++)
            {
                if (strcmp(filenames[j], filenames[j + 1]) < 0)
                {
                    strcpy(temp, filenames[j]);
                    strcpy(filenames[j], filenames[j + 1]);
                    strcpy(filenames[j + 1], temp);
                }
            }
        }
    }
    else // 正向排序
    {
        for (int i = 0; i < count - 1; i++)
        {
            for (int j = 0; j < count - 1 - i; j++)
            {
                if (strcmp(filenames[j], filenames[j + 1]) > 0)
                {
                    strcpy(temp, filenames[j]);
                    strcpy(filenames[j], filenames[j + 1]);
                    strcpy(filenames[j + 1], temp);
                }
            }
        }
    }

    if (chdir(path) < 0)
    {
        perror("chdir error"); // bug3 因为filenames里存的都是文件名而不是路径,所以要chdir到传进来的目录名,才能用stat
    }

    display(filenames, count); // 主要在display里操作,把处理好的文件名给他

    if ((flag & PARAM_l) == 0 && (flag & PARAM_R) != 0) // bug4 一般没有l的同时有R,就换行
    {
        printf("\n");
    }
}

void display(char **filenames, int count) // 这个函数相当于中转站,不同的人对应不同的操作
{
    switch (flag)
    {
    case PARAM_NONE:
    case PARAM_t:
    case PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.') // 排除'.'和'..'还有隐藏文件
            {
                display_file(filenames[i]);
            }
        }
        break;
    case PARAM_a:
    case PARAM_a + PARAM_t:
    case PARAM_a + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            display_file(filenames[i]);
        }
        break;
    case PARAM_a + PARAM_l:
    case PARAM_a + PARAM_l + PARAM_t:
    case PARAM_a + PARAM_l + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            display_l(filenames[i]);
        }
        break;
    case PARAM_l:
    case PARAM_l + PARAM_t:
    case PARAM_l + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                display_l(filenames[i]);
            }
        }
        break;
    case PARMA_i:
    case PARMA_i + PARAM_t:
    case PARMA_i + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                display_i(filenames[i]);
            }
        }
        break;
    case PARMA_s:
    case PARMA_s + PARAM_t:
    case PARMA_s + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                display_s(filenames[i]);
            }
        }
        break;
    case PARAM_R:
    case PARAM_R + PARAM_t:
    case PARAM_R + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                display_file(filenames[i]);
            }
        }
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                displayR(filenames[i]);
            }
        }
        break;
    case PARAM_R + PARAM_l:
    case PARAM_R + PARAM_l + PARAM_t:
    case PARAM_R + PARAM_l + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')

            {
                display_l(filenames[i]);
            }
        }
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                displayR(filenames[i]);
            }
        }
        break;
    case PARAM_R + PARAM_a:
    case PARAM_R + PARAM_a + PARAM_t:
    case PARAM_R + PARAM_a + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            display_file(filenames[i]);
        }
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                displayR(filenames[i]);
            }
        }
        break;
    case PARAM_R + PARAM_a + PARAM_l:
    case PARAM_R + PARAM_a + PARAM_l + PARAM_t:
    case PARAM_R + PARAM_a + PARAM_l + PARAM_r:
        for (int i = 0; i < count; i++)
        {
            display_l(filenames[i]);
        }
        for (int i = 0; i < count; i++)
        {
            if (filenames[i][0] != '.')
            {
                displayR(filenames[i]);
            }
        }
        break;
    default:
        break;
    }
}

void display_file(char *name) // 打印文件
{
    struct stat sbuf; // 由于只是文件名,并不是完整路径,不能用stat函数,所以会stat error2和stat error3
    if (stat(name, &sbuf) == -1)
    {
        perror("stat error2");
        return;
    }
    colorprint(name, sbuf.st_mode);
}

void displayR(char *name) // 只处理目录,只有display_file具有打印功能,结束只能是displayR()一个都打不出来的时候
{
    struct stat sbuf;
    if (stat(name, &sbuf) == -1)
    {
        perror("stat error3");
        return;
    }
    if (S_ISDIR(sbuf.st_mode)) // 不是目录的话,displayR()不会做任何事情
    {
        printf("\n"); // 注意细节!
        // 这里会切换目录chdir(path),所以后面要换回来
        display_dir(name); // 遇到文件夹都用display_dir这个函数 *递归体现在display()中的for循环,
        free(name);        // 打印目录下所有文件和这里的display_dir(),遇到目录继续for循环
        //  将之前filenames[i]中的空间释放
        char *p = PATH;
        while (*++p)        // 必须有,不然目录会叠加,目录多的话段错误
            ;               // display_dir()已经执行完,说明目录里文件已经打印完毕,可以换PATH并且返回上一层目录
        while (*--p != '/') // 从后往前到'/'停下来,然后用'\0'断开,例如:../01C语言学习: ,../02cpp_practice:
            ;
        *p = '\0';

        chdir(".."); // 返回上层目录
        return;
    }
}

void colorprint(char *name, mode_t st_mode)
{
    if (S_ISDIR(st_mode)) // 目录
    {
        printf("\033[1;34m%-s  \033[0m", name);
    }
    else if (st_mode & S_IXUSR || st_mode & S_IXGRP || st_mode & S_IXOTH) // 可执行文件
    {
        printf("\033[1;32m%-s  \033[0m", name);
    }
    else if (S_ISLNK(st_mode)) // 链接
    {
        printf("\033[1;36m%-s  \033[0m", name);
    }
    else
    {
        printf("%s  ", name);
    }
}

void display_l(char *name)
{
    struct stat sbuf;
    struct passwd *pw;
    struct group *gr;
    char time_buf[32] = {0};
    if (stat(name, &sbuf) == -1)
    {
        perror("stat error4");
        return;
    }

    if (S_ISREG(sbuf.st_mode))
    {
        printf("-");
    }
    else if (S_ISDIR(sbuf.st_mode))
    {
        printf("d");
    }
    else if (S_ISLNK(sbuf.st_mode))
    {
        printf("l");
    }
    else if (S_ISBLK(sbuf.st_mode))
    {
        printf("b");
    }
    else if (S_ISCHR(sbuf.st_mode))
    {
        printf("c");
    }
    else if (S_ISFIFO(sbuf.st_mode))
    {
        printf("p");
    }
    else if (S_ISSOCK(sbuf.st_mode))
    {
        printf("s");
    }

    if (sbuf.st_mode & S_IRUSR)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IWUSR)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IXUSR)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }

    if (sbuf.st_mode & S_IRGRP)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IWGRP)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IXGRP)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }

    if (sbuf.st_mode & S_IROTH)
    {
        printf("r");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IWOTH)
    {
        printf("w");
    }
    else
    {
        printf("-");
    }
    if (sbuf.st_mode & S_IXOTH)
    {
        printf("x");
    }
    else
    {
        printf("-");
    }

    printf(" ");

    printf("%ld ", sbuf.st_nlink);
    // 跟据uid和gid获取用户名和组名
    pw = getpwuid(sbuf.st_uid);
    gr = getgrgid(sbuf.st_gid);
    printf("%-s ", pw->pw_name);
    printf("%-s ", gr->gr_name);

    printf("%6ld", sbuf.st_size); // 加6,方便对齐

    strcpy(time_buf, ctime(&sbuf.st_mtime));
    time_buf[strlen(time_buf) - 1] = '\0';
    printf(" %s ", time_buf);

    colorprint(name, sbuf.st_mode);

    printf("\n"); // 换行
}

void display_i(char *name)
{
    struct stat sbuf;
    if (stat(name, &sbuf) == -1)
    {
        perror("stat error 6");
        return;
    }
    printf("%ld ", sbuf.st_ino);

    colorprint(name, sbuf.st_mode);

    printf("\n");
}

void display_s(char *name)
{
    struct stat sbuf;
    if (stat(name, &sbuf) == -1)
    {
        perror("stat error 7");
        return;
    }
    printf("%ld ", sbuf.st_size);

    colorprint(name, sbuf.st_mode);

    printf("\n");
}