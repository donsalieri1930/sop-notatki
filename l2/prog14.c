#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name) {
    fprintf(stderr, "USAGE: %s n>0 k>0 p>0 r>0\n", name);
    exit(EXIT_FAILURE);
}

void sleep_complete(time_t sec, long nsec) {
    struct timespec req = {sec, nsec};
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR)
            req = rem;
        else
            ERR("nanosleep");
    }
}

void set_handler(void (*f)(int), int sig) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sig, &act, NULL))
        ERR("sigaction");
}

volatile sig_atomic_t last_sig = 0;

void sig_handler(int sig) {
    last_sig = sig;
}

void child_work(int r) {
    printf("hello from %d\n", getpid());
    set_handler(sig_handler, SIGUSR1);
    set_handler(sig_handler, SIGUSR2);
    srand(time(NULL) + getpid());
    for (int i=0; i<r; i++) {
        sleep_complete(rand() % 6 + 5, 0); // [5, 10]
        switch (last_sig) {
            case SIGUSR1:
                printf("SUCCESS\n");
                break;
            case SIGUSR2:
                printf("FAILURE\n");
                break;
        }
    }
}


int main(int argc, char **argv) {
    if (argc != 5)  
        usage(argv[0]);
    int n = atoi(argv[1]);
    int k = atoi(argv[2]);
    int p = atoi(argv[3]);
    int r = atoi(argv[4]);
    if (n <= 0 || k <= 0 || p <= 0 || r <= 0)
        usage(argv[0]);
    for (int i=0; i<n; i++) {
        int pid = fork();
        switch (pid) {
            case -1:
                ERR("fork");
            case 0:
                // child code
                child_work(r);
                return EXIT_SUCCESS;
            default:
                // parent code
                break;
        }
    }
    // parent code
    set_handler(SIG_IGN, SIGUSR1);
    set_handler(SIG_IGN, SIGUSR2);

    int child_count = n;
    while (child_count > 0) {
        for (int i=0; i<n; i++) {
            int child_pid = waitpid(0, NULL, WNOHANG);
            if (child_pid == -1) {
                if (errno == EINTR)
                    break;
                ERR("waitpid");
            }
            else if (child_pid == 0)
                break;
            else if (child_pid > 0)
                child_count--;
        }
        sleep_complete(k, 0);
        kill(0, SIGUSR1);
        sleep_complete(p, 0);
        kill(0, SIGUSR2);
    }
}