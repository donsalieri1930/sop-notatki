#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

struct thread_context {
    pthread_t tid;
    struct {
        int *numbers;
        int k;
        pthread_mutex_t *mut_numbers;
        pthread_mutex_t *mut_stop;
        sigset_t mask;
    } in;
};

int stop = 0;

void print_numbers(int *numbers, int k) {
    for (int i = 0; i < k; i++) {
        if (numbers[i] == 1)
            printf("%d ", i + 1);
    }
    printf("\n");
}

void remove_number(int *numbers, int k) {
    while (1) {
        int idx = rand() % k;
        if (numbers[idx] == 1) {
            numbers[idx] = 0;
            break;
        }
    }
}

void *thread_work(void *args) {

    struct thread_context *ctx = args;
    srand(time(NULL) + pthread_self());
    int count = ctx->in.k;
    while (1) {
        int sig;
        if (sigwait(&ctx->in.mask, &sig) != 0)
            exit(EXIT_FAILURE);
        printf("Received signal: %d\n", sig);
        switch (sig) {
            case SIGINT:
                if (count == 0)
                    break;
                pthread_mutex_lock(ctx->in.mut_numbers);
                remove_number(ctx->in.numbers, ctx->in.k);
                pthread_mutex_unlock(ctx->in.mut_numbers);
                count--;
                break;
            case SIGQUIT:
                pthread_mutex_lock(ctx->in.mut_stop);
                stop = 1;
                pthread_mutex_unlock(ctx->in.mut_stop);
                return NULL;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        return EXIT_FAILURE;
    int k = atoi(argv[1]);
    if (k <= 0)
        return EXIT_FAILURE;

    int *numbers = malloc(k * sizeof(int));
    if (numbers == NULL)    
        return EXIT_FAILURE;

    for (int i=0; i<k; i++)
        numbers[i] = 1;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    pthread_mutex_t mut_stop, mut_numbers;
    pthread_mutex_init(&mut_stop, NULL);
    pthread_mutex_init(&mut_numbers, NULL);

    struct thread_context ctx; // uwaga to jest na stosie w przyp watkow detachable to nie musi dzialac
    ctx.in.numbers = numbers;
    ctx.in.k = k;
    ctx.in.mut_numbers = &mut_numbers;
    ctx.in.mut_stop = &mut_stop;
    ctx.in.mask = mask;

    if (pthread_create(&ctx.tid, NULL, thread_work, (void*)&ctx) != 0)
        return EXIT_FAILURE;

    while (1) {
        pthread_mutex_lock(&mut_stop);
        if (stop == 1) {
            pthread_mutex_unlock(&mut_stop);
            break;
        }
        pthread_mutex_unlock(&mut_stop);

        pthread_mutex_lock(&mut_numbers);
        print_numbers(numbers, k);
        pthread_mutex_unlock(&mut_numbers);

        sleep(1);
    }

    if (pthread_join(ctx.tid, NULL) != 0)
        return EXIT_FAILURE;

    free(numbers);
    return EXIT_SUCCESS;
}