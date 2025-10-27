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

int main(int argc, char **argv) {
    int c;
    char *fname;
    int perms = -1;
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
        }
    }
    // walidacja argumentow
    if (fname == NULL || perms < 1 || size < 1)
        usage(argv[0]);
    // usuniecie pliku jesli istnieje
    errno = 0;
    if (unlink(fname) == -1 && errno != ENOENT)
        ERR("unlink");
    // utworzenie pliku i nadanie uprawnien
    FILE* fp = fopen(fname, "w+");
    if (fp == NULL)
        ERR("fopen");
    fclose(fp);
    if (chmod(fname, perms) == -1)
        ERR("chmod");

    fp = fopen(fname, "w");
    srand(time(NULL));
    for (int i=0; i<size; i++) {

        if (rand() % 10 == 0)
            fputc(ALPHABET[rand() % 26], fp);
        else fputc(0, fp);
    }
    fclose(fp);
    /* 
    Lepsze rozwiazanie to najpierw wypelnic plik 0 * size,
    a nastepnie zrobic 
        fseek(fp, rand() % size, SEEK_SET);
        fputc(ALPHABET[rand() % 26], fp);
    wtedy jest dokladnie 10% (w zaokragleniu do int) i
    size sie zgadza.
    */
}