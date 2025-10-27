#include <stdio.h>
#include <stdlib.h>

#define MAXL 20

int main(int argc, char **argv) {
    char *times_str = getenv("TIMES");
    int times = 1;
    if (times_str)
        times = atoi(times_str);
    char name[MAXL+2]; // MAXL + newline + terminator
    while (fgets(name, MAXL+2, stdin)) { // returns pointer to name or NULL if EOF
        for (int i=0; i<times; i++)
            printf("Hello, %s", name);
    }
    setenv("RESULT", "Done", 1);
    return EXIT_SUCCESS;
}