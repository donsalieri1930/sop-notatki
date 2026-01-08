#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>

struct thread_context {
    pthread_t tid;

    struct {
        int *counter;
        int *stop;
        int *checked;
        pthread_mutex_t *mut_l;
        pthread_mutex_t *mut_stop;
        pthread_mutex_t *mut_checked;
    } in;

};

void *thread_work(void *args) {
    struct thread_context *ctx = (struct thread_context*)args;
    (void)ctx;
    srand(time(NULL) + ctx->tid);
    int m = rand() % 99 + 2;
    printf("Thread %lu: checking divisibility by %d\n", ctx->tid, m);

    while (1) {
        pthread_mutex_lock(ctx->in.mut_stop);
        if (*(ctx->in.stop) == 1) {
            pthread_mutex_unlock(ctx->in.mut_stop);
            break;
        }
        pthread_mutex_unlock(ctx->in.mut_stop);
        pthread_mutex_lock(ctx->in.mut_l);
        if (*(ctx->in.counter) % m == 0) {
            printf("thread %lu: %d is divisible by %d\n", ctx->tid, *(ctx->in.counter), m);
        }
        pthread_mutex_unlock(ctx->in.mut_l);

        pthread_mutex_lock(ctx->in.mut_checked);
        (*(ctx->in.checked))++;
        pthread_mutex_unlock(ctx->in.mut_checked);
        printf("thread %lu: checked %d\n", ctx->tid, *(ctx->in.checked));
    }
    return NULL;
}

struct sig_thread_in {
    pthread_mutex_t *mut_stop;
    int *stop;
};

void *thread_work_sig(void *args) {
    struct sig_thread_in *ctx = args;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    while (1) {
        int sig;
        if (sigwait(&mask, &sig) != 0)
            exit(EXIT_FAILURE);
        if (sig == SIGINT) {
            pthread_mutex_lock(ctx->mut_stop);
            *(ctx->stop) = 1;
            pthread_mutex_unlock(ctx->mut_stop);
            break;
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2)
        return EXIT_FAILURE;
    int n = atoi(argv[1]);
    if (n <= 0)
        return EXIT_FAILURE;

    struct thread_context *threads = malloc(n * sizeof(struct thread_context));
    if (threads == NULL)
        return EXIT_FAILURE;

    int l = 1;

    pthread_mutex_t mut_l, mut_stop, mut_checked;
    pthread_mutex_init(&mut_l, NULL);
    pthread_mutex_init(&mut_stop, NULL);
    pthread_mutex_init(&mut_checked, NULL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    int stop = 0;
    int checked = 0;


    for (int i=0; i<n; i++) {
        threads[i].in.counter = &l;
        threads[i].in.mut_l = &mut_l;
        threads[i].in.stop = &stop;
        threads[i].in.mut_stop = &mut_stop;
        threads[i].in.checked = &checked;
        threads[i].in.mut_checked = &mut_checked;
        if (pthread_create(&threads[i].tid, NULL, thread_work, (void*)&threads[i]) != 0)
            return EXIT_FAILURE;
    }
    struct sig_thread_in sig_ctx = {&mut_stop, &stop};
    pthread_t sig_thread;

    if (pthread_create(&sig_thread, NULL, thread_work_sig, (void*)&sig_ctx) != 0)
        return EXIT_FAILURE;

    struct timespec ts  = {0, 100000000}; // 0.1s

    while (1) {
        pthread_mutex_lock(&mut_stop);
        if (stop == 1) {
            pthread_mutex_unlock(&mut_stop);
            break;
        }
        pthread_mutex_unlock(&mut_stop);

        nanosleep(&ts, NULL);

        pthread_mutex_lock(&mut_l);
        l++;
        pthread_mutex_unlock(&mut_l);
        printf("%d\n", l);

        checked = 0;
        while (1) {
            pthread_mutex_lock(&mut_stop);
            if (stop == 1) {
                pthread_mutex_unlock(&mut_stop);
                break;
            }
            pthread_mutex_unlock(&mut_stop);

            pthread_mutex_lock(&mut_checked);
            if (checked == n) {
                pthread_mutex_unlock(&mut_checked);
                break;
            }
            pthread_mutex_unlock(&mut_checked);
        }
    }

    for (int i=0; i<n; i++) {
        pthread_join(threads[i].tid, NULL);
    }

    pthread_join(sig_thread, NULL);

    free(threads);

}
