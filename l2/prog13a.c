#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

void child_work() {
    srand(time(NULL) + getpid());
    int delay = rand() % 6 + 5; // rand() % x przyjmuje wartosci od 0 to x-1
    sleep(delay);
    printf("%d\n", getpid());

}

int main(int argc, char **argv) {
    if (argc != 2)
        usage(argv[0]);
    int n = atoi(argv[1]);
    if (n == 0)
        usage(argv[0]);
    for (int i=0; i<n; i++) {
        int pid = fork();
        switch (pid) {
            case -1:
                ERR("fork");
                break;
            case 0:
                child_work();
                exit(EXIT_SUCCESS);
            default:
                break;
        }
    }
    
}