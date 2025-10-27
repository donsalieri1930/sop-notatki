#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s -n Name -p OCTAL -s SIZE\n", pname);
    exit(EXIT_FAILURE);
}

void try_remove(char *fname) {
    errno = 0;
    if (unlink(fname) == -1 && errno != ENOENT)
        ERR("unlink");
}

void create_with_perms(char *fname, int perms) {
    FILE *fp = fopen(fname, "w+");
    if (fp == NULL)
        ERR("fopen");
    if (chmod(fname, perms) == -1) // plik nie musi byc otwarty zeby zrobic chmod
        ERR("chmod");
    if (fclose(fp) != 0)
        ERR("fclose");
}

void write_zeros(FILE *fp, int size) {
    for (int i=0; i<size; i++)
        fputc(0, fp);
}

void random_letters(FILE *fp, int size, int count) {
    for (int i=0; i<count; i++) {
        if (fseek(fp, rand() % size, SEEK_SET) == -1)
            ERR("fseek");
        fputc(ALPHABET[rand() % 26], fp);
    }
}

int main(int argc, char **argv) {
    int c;
    char *fname;
    int perms = 0;
    int size = 0;
    while((c = getopt(argc, argv, "n:p:s:")) != -1) {
        switch (c) {
            case 'n':
                fname = optarg;
                break;
            case 'p':
                perms = strtol(optarg, NULL, 8);
                break;
            case 's':
                size = atoi(optarg);
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }
    if (fname == NULL || perms < 1 || size < 1)
        usage(argv[0]);

    try_remove(fname);
    create_with_perms(fname, perms);
    FILE *fp = fopen(fname, "w");
    srand(time(NULL));
    write_zeros(fp, size);
    random_letters(fp, size, size / 10);
    if (fclose(fp) == EOF)
        ERR("fclose");
    return EXIT_SUCCESS;
}