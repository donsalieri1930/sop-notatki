#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <ftw.h>

#define MAXFD 20

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

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

char *dest_base = "backup";
int walk_src_base_len;

int walk(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    /* Path to source directory inside fpath is not guaranteed to be
       the same as user provided path. */
    if (ftwbuf->level == 0)
        walk_src_base_len = strlen(fpath);

    const char *src_suffix = fpath + walk_src_base_len;
    char *dest_fpath = join_paths(dest_base, src_suffix);
    switch (tflag) {
        case FTW_D:
            mkdir(dest_fpath, sb->st_mode);
            printf("%s %s %s\n", fpath, src_suffix, dest_fpath);
            break;
        case FTW_F:
            break;
        case FTW_SL:
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
    char *src_base = argv[1];
    nftw(src_base, walk, MAXFD, 0);
}