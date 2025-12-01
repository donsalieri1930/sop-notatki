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

sig_atomic_t alarm_received = 0;
sig_atomic_t sigusr1_received = 0;

void handle_sig_child(int sig) {
    if (sig == SIGALRM)
        alarm_received = 1;
    if (sig == SIGUSR1)
        sigusr1_received = 1;
}

void child_work(int n, sigset_t *old_mask) {
    sethandler(handle_sig_child, SIGALRM);
    sethandler(handle_sig_child, SIGUSR1);
    srand(time(NULL) + getpid());
    alarm(1);
    int s = rand() % 91 + 10;
    s *= 1024;
    char strpid[20];
    char strn[2];
    snprintf(strpid, 20, "%d.txt", getpid());
    snprintf(strn, 2, "%d", n);
    int fd = open(strpid, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd == -1)
        ERR("open");
    while (1) {
        sigsuspend(old_mask);
        if (alarm_received == 1) {
            if (close(fd) == -1)
                ERR("close");
            return;
        }
        if (sigusr1_received == 1) {
            sigusr1_received = 0;
            for (int i=0; i<s; i++) {
                write(fd, strn, 1);
            }
        }
    }
}

int main(int argc, char **argv) {
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);
    for (int i=1; i<argc; i++) {
        errno = 0;
        long n = strtol(argv[i], NULL, 10);
        if (errno == EINVAL || n < 0 || n > 9)
            usage(argv[0]);
        printf("int %d\n", n);
        int pid = fork();
        switch (pid) {
            case -1:
                ERR("fork");
            case 0:
                // child
                child_work(n, &old_mask);
                exit(EXIT_SUCCESS);
            default:
                break;
        }
    }
    // parent
    sethandler(handle_sig_child, SIGALRM);
    alarm(1);
    while (1) {
        if (alarm_received)
            break;
        nanosleep_complete(0, 10000000);
        kill(0, SIGUSR1);
    }
    while (wait(NULL) > 0) {}
    return EXIT_SUCCESS;
}