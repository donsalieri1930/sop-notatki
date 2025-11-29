#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name) {
    fprintf(stderr, "USAGE: %s m>0 n>0\n", name);
    exit(EXIT_FAILURE);
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

void set_handler(void (*f)(int), int sig) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction)); // wypelnia bajty zerami
    act.sa_handler = f;
    if (-1 == sigaction(sig, &act, NULL))
        ERR("sigaction");
}

ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;

    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;

        if (c == 0)
            return len;  // EOF

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
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

void child_work(int m) {
    int sigusr1_send = 0;
    while (1) {
        if (kill(getppid(), SIGUSR1) == -1)
            ERR("kill");
        sigusr1_send++;
        printf("child sending SIGUSR1 #%d\n", sigusr1_send);
        nanosleep_complete(0, m*10000);
    }
}

// nie mozna zakladac zadnej korelacji!!!
volatile sig_atomic_t sigusr1_received = 0;
volatile sig_atomic_t sigusr1_count = 0;

void handle_sig(int sig) {
    if (sig == SIGUSR1) {
        sigusr1_received = 1;
        sigusr1_count++;
    }
}

int main(int argc, char **argv) {
    if (argc != 5)  
        usage(argv[0]);
    int delay_milisec = atoi(argv[1]);
    int block_count = atoi(argv[2]);
    int block_size = atoi(argv[3])*1024*1024;
    char *filename = argv[4];
    if (delay_milisec <= 0 || block_count <= 0 || block_size <= 0)
        usage(argv[0]);

    set_handler(handle_sig, SIGUSR1); // handlery przed fork

    int pid = fork();
    switch (pid) {
        case -1:
            ERR("fork");
        case 0:
            // child code
            child_work(delay_milisec);
            exit(EXIT_SUCCESS);
        default:
            break;
    }
    // parent code
    char *buf = malloc(block_size*sizeof(char));
    if (buf == NULL)
        ERR("malloc");

    int fd = TEMP_FAILURE_RETRY(open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666));
    int randfd = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY));
    if (fd == -1 || randfd == -1)
        ERR("open");

    for (int i=0; i<block_count; i++) {
        if (sigusr1_received == 1) {
            printf("parent received SIGUSR1 #%d\n", sigusr1_count);
            sigusr1_received = 0;
        }
        // copy block 
        if(bulk_read(randfd, buf, block_size) == -1)
            ERR("read");

        int real_size = bulk_write(fd, buf, block_size);
        if (real_size == -1)
            ERR("write");
        printf("copied %d bytes from /dev/urandom\n", real_size);
    }
    free(buf);
    if (kill(pid, SIGKILL) == -1)
        ERR("kill");
    if (TEMP_FAILURE_RETRY(close(fd)) == -1)
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(randfd)) == -1)
        ERR("close");
    waitpid(pid, NULL, 0);
}