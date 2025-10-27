#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void usage(char *pname)
{
  fprintf(stderr, "USAGE:%s name times>0\n", pname);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *pname = argv[0];
    if (argc != 3)
        usage(pname);
    char* name = argv[1];
    char* str_count = argv[2];
    errno = 0;
    long count = strtol(str_count, NULL, 0);
    if (errno || count < 0)
        usage(pname);
    for (int i=0; i<count; i++)
        printf("%s\n", name);
    return EXIT_SUCCESS;
}