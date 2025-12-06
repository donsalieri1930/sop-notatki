#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <ftw.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>

#define MAXFD 20
#define COPY_BUF_SIZE 4096

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do {
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

ssize_t bulk_write(int fd, const char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void try_copy(int src_fd, int dest_fd) {
    char buf[COPY_BUF_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = bulk_read(src_fd, buf, sizeof(buf))) > 0) {
        if (bulk_write(dest_fd, buf, bytes_read) == -1)
            ERR("bulk_write");
    }
    if (bytes_read == -1)
        ERR("bulk_read");
}

char *join_paths(const char *path1, const char *path2) {
    /* Join any two paths. Returned pointer must be freed. */
    int path1_len = strlen(path1);
    int path2_len = strlen(path2);
    char *path12 = malloc((path1_len + 1 + path2_len + 1)*sizeof(char));
    if (path12 == NULL)
        ERR("malloc");
    if (path1_len == 0) {
        memcpy(path12, path2, path2_len + 1);
        return path12;
    }
    if (path2_len == 0) {
        memcpy(path12, path1, path1_len + 1);
        return path12;
    }
    if (path1[path1_len - 1] == '/')
        path1_len--;
    memcpy(path12, path1, path1_len);
    path12[path1_len] = '/';
    int path2_slash = 0;
    if (path2[0] == '/') {
        path2_len--;
        path2_slash = 1;
    }
    memcpy(path12 + path1_len + 1, path2 + path2_slash, path2_len + 1);
    return path12;
}

int nftw_src_len, src_absolute_len;
char src_absolute[PATH_MAX + 1];
char dest_absolute[PATH_MAX + 1];

int walk(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    /* 
        Path to source directory inside fpath is not guaranteed to be 
        equal to path passed into nftw.
    */
    if (ftwbuf->level == 0)
        nftw_src_len = strlen(fpath);

    const char *src_suffix = fpath + nftw_src_len;
    char *dest_fpath = join_paths(dest_absolute, src_suffix);
    int src_fd, dest_fd;
    char link_to[PATH_MAX + 1];
    ssize_t link_to_len;
    char *new_link_target;
    switch (tflag) {
        case FTW_D:
            if (mkdir(dest_fpath, sb->st_mode) == -1)
                ERR("mkdir");
            printf("%s %s %s\n", fpath, src_suffix, dest_fpath);
            break;

        case FTW_F:
            src_fd = open(fpath, O_RDONLY);
            dest_fd = open(dest_fpath, O_CREAT | O_WRONLY, sb->st_mode);
            if (src_fd == -1 || dest_fd == -1)
                ERR("open");
            try_copy(src_fd, dest_fd);
            if (close(src_fd) == -1 || close(dest_fd) == -1)
                ERR("close");
            break;

        case FTW_SL:
            link_to_len = readlink(fpath, link_to, sizeof(link_to) - 1);
            if (link_to_len == -1) 
                ERR("readlink");
            link_to[link_to_len] = 0;
            /* 
                If link target is an absolute path, and inside source directory, 
                create a symlink to a copy. Otherwise create symlink with the same path.
            */
            if (strncmp(src_absolute, link_to, src_absolute_len) == 0 &&
                (link_to[src_absolute_len] == '/' || link_to[src_absolute_len] == 0)) {
                src_suffix = link_to + src_absolute_len;
                new_link_target = join_paths(dest_absolute, src_suffix);
                if (symlink(new_link_target, dest_fpath) == -1)
                    ERR("symlink");
                free(new_link_target);
            } else {
                if (symlink(link_to, dest_fpath) == -1)
                        ERR("symlink");
            }
            break;
        default:
            break;
    }
    free(dest_fpath);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2)
        exit(EXIT_FAILURE);
    char *src_path = argv[1];
    realpath(src_path, src_absolute);
    realpath("backup", dest_absolute);
    src_absolute_len = strlen(src_absolute);
    nftw(src_path, walk, MAXFD, FTW_PHYS);
}