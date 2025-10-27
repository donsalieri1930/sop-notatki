#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#define BUFF_SIZE 1024
#define FD_LIMIT 100
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char* pname)
{
    fprintf(stderr, "USAGE:%s -p<0:any> path -r<0:1> -o<0:1> dest\n", pname);
    exit(EXIT_FAILURE);
}

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void scan_dir(char* path, FILE* stream, char* extension)
{
    DIR* dirp;
    struct dirent* dp;
    struct stat filestat;
    char* substr = NULL;
    if ((dirp = opendir(path)) == NULL)
        ERR("opendir");
    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            if (lstat(path, &filestat))
                ERR("lstat");
            if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
            {
                substr = strstr(dp->d_name, extension);
                if (substr != NULL && substr != dp->d_name)
                    fprintf(stream, "%s %ld\n", dp->d_name, filestat.st_size);
            }
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
}

int depth = 1;
char extension[BUFF_SIZE];
FILE* output_stream;
int count = 1;

int walk(const char *path, const struct stat *filestat, int flag, struct FTW *ftw_struct)
{
    if(ftw_struct->level>depth)
        return 0;

    
    char* substr = NULL;
    substr = strstr(path, extension);
    if (substr != NULL && substr != path)
    {
        const char* filename = strrchr(path, '/');
        if (filename != NULL)
            filename++;
        else{
            filename = path;
        }
        fprintf(output_stream, "%s %ld\n", filename, filestat->st_size);
    }
        
    return 0;
}

int main(int argc, char** argv)
{
    char* dest;
    char path[BUFF_SIZE];
    char cwd[BUFF_SIZE];
    cwd[0] = '\0';
    path[0] = '\0';
    extension[0] = '\0';
    int o = 0;
    output_stream = stderr;
    int c;
    while ((c = getopt(argc, argv, "p:e:d:o")) != -1)
        switch (c)
        {
            case 'p':
                strcat(path, optarg);
                strcat(path, " ");
                break;
            case 'e':
                strcat(extension, ".");
                strcat(extension, optarg);
                break;
            case 'd':
                errno = 0;
                depth = strtol(optarg, (char**)NULL, 10);
                if (errno != 0)
                    ERR("strtol");
                break;
            case 'o':
                if((dest=getenv("L1_OUTPUTFILE"))!=NULL)
                {
                    o++;
                    if (o > 1)
                        ERR("getopt");
                }
                break;
            case '?':
                usage(argv[0]);
                break;
            default:
                ERR("getopt");
        }

    if (getcwd(cwd, BUFF_SIZE) == NULL)
        ERR("cwd");

    if (o == 1)
    {
        if ((output_stream = fopen(dest, "w+")) == NULL)
            ERR("fopen");
    }

    char* token = strtok(path, " ");

    while (token != NULL)
    {
        fprintf(output_stream, "path: %s\n", token);
        if(depth > 1)
        {
            if(nftw(token,walk,FD_LIMIT,FTW_PHYS)!=0)
                ERR("nftw");
        }
        else 
        {
            scan_dir(token, output_stream, extension);   
        }
        fprintf(output_stream, "\n");
        token = strtok(NULL, " ");
    }

    if (o == 1 && fclose(output_stream))
        ERR("fclose");

    return EXIT_SUCCESS;
}