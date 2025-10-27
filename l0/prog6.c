#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
    #include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

extern char **environ;

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s [VARN_NAME VARN_VALUE] ... \n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *pname = argv[0];
    if (argc % 2 == 0)
        usage(pname);
    int var_count = (argc - 1) / 2;
    for (int i=0; i<var_count; i++) {
        errno = 0;
        setenv(argv[1+2*i], argv[1+2*i+1], 1); // pname KEY1 VAL1 KEY2 VAL2 KEY3 VAL3
        if (errno == EINVAL)
            ERR("setenv - variable name contains '='");
        if (errno)
            ERR("setenv");
    }
    int i=0;
    while (environ[i]){
        printf("%s\n", environ[i]);
        i++;
    }
    return EXIT_SUCCESS;
}