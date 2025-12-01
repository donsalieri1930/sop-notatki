#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name) {
    fprintf(stderr, "USAGE: %s todo\n", name);
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sethandler_siginfo(void (*f)(int, siginfo_t*, void*), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_sigaction = f;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void nanosleep_complete(time_t sec, long nsec) {
    struct timespec req = {sec, nsec};
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) // nanosleep zostal przerwany, pozostaly czas zostal zapisany do rem
            req = rem;
        else
            ERR("nanosleep");
    }
}

volatile sig_atomic_t got_sigterm = 0;
volatile sig_atomic_t got_sigusr1 = 0;
volatile sig_atomic_t sigusr1_sender_pid = 0;

void handle_sig(int sig, siginfo_t *siginfo, void *ucontext) {
    if (sig == SIGTERM)
        got_sigterm = 1;
    if (sig == SIGUSR1) {
        got_sigusr1 = 1;
        sigusr1_sender_pid = siginfo->si_pid;
    }
}

volatile sig_atomic_t coughs = 0;

void handle_alarm_child(int sig) {
    _exit(coughs); // exit() nie jest signal-safe
}

void child_work(sigset_t *old_mask, int sick, int p, int k) {
    printf("child %d created sick: %d\n", getpid(), sick);
    sethandler_siginfo(handle_sig, SIGTERM);
    sethandler_siginfo(handle_sig, SIGUSR1);
    sethandler(handle_alarm_child, SIGALRM);
    if (sick == 1)
        alarm(k);
    srand(time(NULL) + getpid());
    while (sick == 0) {
        sigsuspend(old_mask);
        if (got_sigterm) {
            printf("child %d SIGTERM with coughs: %d\n", getpid(), coughs);
            exit(coughs);
        }
        if (got_sigusr1) {
            got_sigusr1 = 0;
            printf("child %d got SIGUSR1 from %d\n", getpid(), sigusr1_sender_pid);
            if (rand() % 101 < p) {
                printf("child %d got sick\n", getpid());
                sick = 1;
                alarm(k);
            }
        }
    }
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    while (1) {
        if (got_sigterm) {
            printf("child %d SIGTERM with coughs: %d\n", getpid(), coughs);
            exit(coughs);
        }
        int ms = rand() % 151 + 50;
        struct timespec req = {0, ms*1000000};
        nanosleep(&req, NULL); // SIGUSR1 zablokowany wiec tylko SIGTERM przerwie nanosleep 
        kill(0, SIGUSR1);
        coughs++;
        printf("child %d send SIGUSR1\n", getpid());
    }
}

void handle_alarm(int sig) { // handler rodzica
    if (sig == SIGALRM)
        kill(0, SIGTERM); // kill jest signal-safe
}

int main(int argc, char **argv) {
    if (argc != 5)
        usage(argv[0]);
    int t = atoi(argv[1]);
    int k = atoi(argv[2]);
    int n = atoi(argv[3]);
    int p = atoi(argv[4]);
    if (t < 1 || t > 100 || k < 1 || k > 100 || n < 1 || n > 30 || p < 1 || p > 100)
        usage(argv[0]);

    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);
    sethandler(handle_alarm, SIGALRM);
    alarm(t);

    for (int i=0; i<n; i++) {
        int pid = fork();
        switch (pid) {
            case -1:
                ERR("fork");
                break;
            case 0:
                // child
                child_work(&old_mask, i == 0, p, k);
                exit(EXIT_SUCCESS); // dla pewnosci
            default: 
                break;
        }
    }

    while (wait(NULL) > 0) {}
    return EXIT_SUCCESS;
}