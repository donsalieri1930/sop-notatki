#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int dirs = 0;
int files = 0;
int links = 0;
int other = 0;

int nftw_callback(const char *fpath, const struct stat *sb, int tflag, struct FTW* ftwbuf) {
    int mode = sb->st_mode;
    if (S_ISDIR(mode)) dirs++; // mozna uzyc tflag = FTW_D, FTW_DNR, ... (manpage)
    else if (S_ISREG(mode)) files++;
    else if (S_ISLNK(mode)) links++;
    else other++;
    return 0;
}

int main(int argc, char **argv) {
    for (int i=1; i<argc; i++) {
        if (nftw(argv[i], nftw_callback, 10, FTW_PHYS) != 0)
            ERR("nftw");
        printf("%s\tDirectories: %d, Files: %d, Links: %d, Other: %d\n", argv[i], dirs, files, links, other);
        dirs = 0;
        files = 0;
        links = 0;
        other = 0;
    }
    return EXIT_SUCCESS;
}