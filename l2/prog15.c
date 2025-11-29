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
    fprintf(stderr, "USAGE: %s m>0 n>0\n", name);
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

void child_work(int m, int n) {
    int sigusr2_count = 0;
    while (1) {
        for (int i=0; i<n-1; i++) {
            kill(getppid(), SIGUSR1);
            sleep_complete(0, m*1000);
        }
        kill(getppid(), SIGUSR2);
        sigusr2_count++;
        printf("child sending SIGUSR2 %d\n", sigusr2_count);
        sleep_complete(0, m*1000);
    }
}

volatile sig_atomic_t sigusr2_count = 0;

void handle_sigusr2(int sig) {
    sigusr2_count++;
}

int main(int argc, char **argv) {
    if (argc != 3)  
        usage(argv[0]);
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    if (m <= 0 || n <= 0)
        usage(argv[0]);

    int pid = fork();
    switch (pid) {
        case -1:
            ERR("fork");
        case 0:
            // child code
            child_work(m, n);
            exit(EXIT_SUCCESS);
        default:
            break;
    }
    // parent code
    set_handler(handle_sigusr2, SIGUSR2);
    set_handler(SIG_IGN, SIGUSR1);
    while (1) {
        printf("%d SIGUSR2 signals received\n", sigusr2_count);
        sleep_complete(2, 0);
    }
}

/*
ten program dziala bo w handlerze asynchronicznie zwiekszamy licznik
a w main nie czekamy na otrzymanie sygnalu, niezaleznie wypisujemy co 3 sekundy

ogolnie, zawsze trzeba
 - zablokowac sygnaly maska
 - sigsuspend w petli 
*/