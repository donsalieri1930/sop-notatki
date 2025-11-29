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

void child_work(int m, int n) {
    int sigusr2_count = 0;
    while (1) {
        for (int i=0; i<n-1; i++) {
            kill(getppid(), SIGUSR1);
            sleep_complete(0, m*10000);
        }
        kill(getppid(), SIGUSR2);
        sigusr2_count++;
        printf("child sending SIGUSR2 %d\n", sigusr2_count);
        sleep_complete(0, m*1000);
    }
}

// nie mozna zakladac zadnej korelacji!!!
volatile sig_atomic_t last_sig = 0; // koniecznie ten typ
volatile sig_atomic_t sigusr2_received = 0; // jesli sigsuspend otrzyma wiele sygnalow stracimy wszystkie przed ostatnim
// volatile sig_atomic_t sigusr2_count = 0;

void handle_sig(int sig) {
    last_sig = sig;
    if (sig == SIGUSR2) {
        sigusr2_received = 1;
        // sigusr2_count++;
    }
}

int main(int argc, char **argv) {
    if (argc != 3)  
        usage(argv[0]);
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    if (m <= 0 || n <= 0)
        usage(argv[0]);

    sigset_t new_mask, old_mask; // maska przed handlerami
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask); // blokujemy USR1, USR2

    set_handler(handle_sig, SIGUSR1); // handlery przed fork
    set_handler(handle_sig, SIGUSR2);

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
    int sigusr2_count = 1;
    // parent code
    while (1) {
        sigsuspend(&old_mask); // stara maska z odblokowanymi USR1, USR2
        // sigsuspend moze wpuscic wiecej sygnalow!!!
        if (sigusr2_received == 1) {
            printf("received SIGUSR2 #%d\n", sigusr2_count++);
            sigusr2_received = 0;
        }
    }
}

/*
tutaj implementuje schemat z maska i sigsuspend.
Handler ustawia globalny last_sig, reszta logiki w main
wazna jest kolejnosc
 - zablokowac sygnaly maska
 - ustawic handler
 - sigsuspend w petli ze stara maska!!!
 - sprawdzamy jaki sygnal dostalismy

jesli wystarczy jeden handler ustawiajacy last_sig to tak
treba robic. logika powinna byc w main
edit: tutaj logika w handlerze musi byc bardziej skomplikowana aby sygnaly sie nie skleily 


sigsuspend zawsze czeka na pierwszy otrzymany sygnal od czasu jego wywolania nie wczesniej
dlatego wazne jest blokowanie sygnalow
*/