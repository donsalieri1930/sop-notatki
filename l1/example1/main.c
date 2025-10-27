#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define ERR_NOEXIT(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s -p dir1 -p dir2 ... -o file\n", pname);
    exit(EXIT_FAILURE);
}

void print_dir(char *dirname, FILE *out) {
    DIR *dirp = opendir(dirname);
    if (dirp == NULL) {
        ERR_NOEXIT("opendir");
        return;
    }
    struct dirent *dp = NULL;
    struct stat filestat;
    fprintf(out, "SCIEZKA:\n%s\nLISTA PLIKOW:\n", dirname);
    while (1) {
        dp = readdir(dirp);
        if (dp == NULL)
            break;
        if (fstatat(dirfd(dirp), dp->d_name, &filestat, AT_SYMLINK_NOFOLLOW) == -1)
            ERR("fstatat");
        fprintf(out, "%s %ld\n", dp->d_name, filestat.st_size);
    }
    if (closedir(dirp) == -1)
        ERR("closedir");
}

int main(int argc, char *argv[]) {
    int c;
    char **dirs = malloc(argc * sizeof(char*));
    if (dirs == NULL)
        ERR("malloc");
    int dircount = 0;
    char *outfile = NULL;
    while ((c = getopt(argc, argv, "p:o:")) != -1) {
        switch (c) {
            case 'p':
                dirs[dircount] = optarg; // pointer do argv (pamiec zwolniona po zakonczeniu main)
                dircount++;
                break;
            case 'o':
                if (outfile != NULL)
                    usage(argv[0]);
                outfile = optarg;
                break;
            case '?':
            default:
                usage(argv[0]);
                break;
        }
    }
    if (dircount == 0 || optind < argc) // dodatkowe argumenty pozycyjne?
        usage(argv[0]);
    FILE *out;
    if (outfile == NULL)
        out = stdout;
    else
        if ((out = fopen(outfile, "w+")) == NULL)
            ERR("fopen");

    for (int i=0; i<dircount; i++)
        print_dir(dirs[i], out);

    if (outfile != NULL && fclose(out) != 0) // blad zamkniecia (nie zamykamy stdout!!)
        ERR("fclose");

    free(dirs); // zwalniam tylko tablice pointerow, one wskazuja na argv ktore sie zwolni automatycznie
    return EXIT_SUCCESS;
}