#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

struct thread_context {
    pthread_t tid;
    struct {
        int n;
        int k;
        pthread_mutex_t *mut;
    } in;
};

int count = 0;
int running_threads;

void *thread_work(void *args) {
    struct thread_context *ctx = (struct thread_context*)args;
    while (1) {
        pthread_mutex_lock(ctx->in.mut);
        if (count < ctx->in.k) {
            count++;
            printf("Thread %lu: %d/%d\n", ctx->tid, count, ctx->in.k);
            pthread_mutex_unlock(ctx->in.mut);
        }
        else {
            running_threads--;
            pthread_mutex_unlock(ctx->in.mut);
            break;
        }
        sleep(1);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3)
        return EXIT_FAILURE;

    int n = atoi(argv[1]);
    int k = atoi(argv[2]);
    if (n <= 0 || k <= 0)
        return EXIT_FAILURE;

    struct thread_context *threads = malloc(n * sizeof(struct thread_context));
    if (threads == NULL)
        return EXIT_FAILURE;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_t mut;
    pthread_mutex_init(&mut, NULL);

    for (int i = 0; i < n; i++) {
        threads[i].in.n = n;
        threads[i].in.k = k;
        threads[i].in.mut = &mut;
        pthread_create(&threads[i].tid, &attr, thread_work, (void*)&threads[i]);
        running_threads++;
    }

    while (1) {
        pthread_mutex_lock(&mut);
        if (running_threads == 0) {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);
        sleep(1);
    }

    pthread_attr_destroy(&attr);
    free(threads);
    pthread_mutex_destroy(&mut);
}