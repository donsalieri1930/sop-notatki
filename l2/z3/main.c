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
        if (errno == EINTR) // nanosleep zostal przerwany, pozostaly czas zapisany do rem
            req = rem;
        else
            ERR("nanosleep");
    }
}

ssize_t bulk_write(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;

    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}

volatile sig_atomic_t working = 0;
volatile sig_atomic_t got_sigint = 0;

void handle_sig_child(int sig) {
    if (sig == SIGUSR1)
        working = 1;
    if (sig == SIGUSR2)
        working = 0;
    if (sig == SIGINT)
        got_sigint = 1;
}

void child_work(int child_no) {
    printf("child %d: %d\n", child_no, getpid());
    srand(time(NULL) + getpid());
    int count = 0;
    while (1) {
        if (working) {
            int sm = rand() % 101 + 100;
            struct timespec req = {0, sm*1000000};
            nanosleep(&req, NULL);
            count++;
            printf("%d: %d\n", getpid(), count);
        }
        if (got_sigint) {
            char strpid[20];
            char strcount[20];
            snprintf(strpid, 20, "%d.txt", getpid());
            snprintf(strcount, 20, "%d", count);
            int fd = open(strpid, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            if (fd == -1)
                ERR("open");
            bulk_write(fd, strcount, strlen(strcount));
            if (close(fd) == -1)
                ERR("close");
            exit(EXIT_SUCCESS);
        }
    }
}

volatile sig_atomic_t got_sigusr1 = 0;

void handle_sig_parent(int sig) {
    if (sig == SIGUSR1)
        got_sigusr1 = 1;
    if (sig == SIGINT)
        got_sigint = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        usage(argv[0]);
    int n = atoi(argv[1]);
    if (n < 1)
        usage(argv[0]);
    // handlery beda dziedziczone przez dziecko
    // to eliminuje race conditioning
    printf("parent pid is %d\n", getpid());
    sethandler(handle_sig_child, SIGUSR1);
    sethandler(handle_sig_child, SIGUSR2);
    sethandler(handle_sig_child, SIGINT);

    int *pids = malloc(n*sizeof(int));
    if (pids == NULL)
        ERR("malloc");
    for (int i=0; i<n; i++) {
        int pid = fork();
        switch (pid) {
            case -1:
                ERR("fork");
                break;
            case 0:
                // child code
                child_work(i);
                exit(EXIT_SUCCESS);
                break;
            default:
                pids[i] = pid;
                break;
        }
    }
    // teoretycznie race conditioning ale nic sie nie da z tym zrobic
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);
    sethandler(handle_sig_parent, SIGUSR1);
    sethandler(handle_sig_parent, SIGINT);
    // parent code
    int working_child = 0;
    kill(pids[0], SIGUSR1);
    while (1) {
        sigsuspend(&old_mask);
        if (got_sigusr1) {
            got_sigusr1 = 0;
            kill(pids[working_child % n], SIGUSR2);
            working_child++;
            kill(pids[working_child % n], SIGUSR1);
        }
        if (got_sigint) {
            for (int i=0; i<n; i++)
                kill(pids[i], SIGINT);
            break;
        }
    }
    free(pids);
    while (wait(NULL) > 0) {}
    return EXIT_SUCCESS;
}

/*
jesli proces ma petle z jakims okreslonym tempem to nie mozna uzywac
sigsuspend + maska

dziecko dziedziczy handlery wiec mozna w rodzicu na chwile ustawic handlery
dzieci aby uniknac race conditioning

mozna uzywac tych samych handlerow i zmiennych w rodzicu i dzieciach 


*/
