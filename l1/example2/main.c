#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <getopt.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define ERR_NOEXIT(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))
#define FREE(ptr) (free(ptr), ptr = NULL)
#define MAXFD 20 // fd_limit to ilosc deskryptorow a nie max glebokosc (GLEBOKOSC > fd_limit ale nie rowna sie)

char *ext = NULL;
int depth = 1;
FILE *out = NULL;

void usage(char *pname) {
    fprintf(stderr, "USAGE:%s -p [dir1] -p [dir2] ... -d [depth] -e [extention] -o\n", pname);
    exit(EXIT_FAILURE);
}

char *file_extension(const char *fname) { // pointer z fname -> optarg -> argv
    char *dot = strrchr(fname, '.'); // strRchr - pierwsze wystapienie od konca
    if (dot == NULL)
        return NULL;
    return dot + 1; // od pierwszego znaka za kropka
}

int walk(const char *name, const struct stat *s, int type, struct FTW *f) {
    if (ext != NULL && strcmp(file_extension(name), ext) != 0)
        return 0;
    if (type != FTW_F || f->level > depth)
        return 0;
    fprintf(out, "%s %ld\n", name, s->st_size);
    return 0;
}

int main(int argc, char **argv) {
    int c;
    char **paths = malloc(argc*sizeof(char*));
    if (paths == NULL)
        ERR("malloc");
    int pathc = 0;
    char *env = NULL;
    out = stdout;
    while ((c = getopt(argc, argv, "p:d:e:o")) != -1) {
        switch (c) {
            case 'p':
                paths[pathc] = optarg;
                pathc++;
                break;
            case 'd':
                depth = atoi(optarg);
                if (depth < 1)
                    usage(argv[0]);
                break;
            case 'e':
                ext = optarg;
                break;
            case 'o':
                if ((env = getenv("L1_OUTPUTFILE")) == NULL)
                    ERR("getenv");
                out = fopen(env, "w+");
                if (out == NULL)
                    ERR("fopen");
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }
    for (int i=0; i<pathc; i++) {
        fprintf(out, "path: %s\n", paths[i]);
        if(nftw(paths[i], walk, MAXFD, FTW_PHYS) != 0) // FTW_PHYS - nie podozaj za linkami
            ERR_NOEXIT("nftw");
    }
    FREE(paths);
    paths = NULL;
    if (out != stdout && fclose(out) != 0) // nie zamykamy stdout
        ERR("fclose");
    return EXIT_SUCCESS;
}