#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


struct thread_context {
    pthread_t tid;

    struct {
        long long start;
    } in;

    struct {
        long long result;
    } out;
};

void *work(void *args) {
    struct thread_context *ctx = (struct thread_context*)args;
    long long start = ctx->in.start;
    for (int i=0; i<5; i++) {
        start = start*start;
    }
    ctx->out.result = start;
    return NULL;
}


int main(int argc, char **argv) {
    if (argc != 2)
        return EXIT_FAILURE;
    int n = atoi(argv[1]);
    if (n == 0)
        return EXIT_FAILURE;

    struct thread_context *threads = malloc(n * sizeof(struct thread_context));
    if (threads == NULL)
        return EXIT_FAILURE;

    for (int i = 0; i < n; i++) {
        threads[i].in.start = i;
        pthread_create(&threads[i].tid, NULL, work, (void*)&threads[i]);
    }
    long long total = 0;
    for (int i=0; i<n; i++) {
        pthread_join(threads[i].tid, NULL);
        total += threads[i].out.result;
    }

    printf("Total: %lld\n", total);
    free(threads);
    return EXIT_SUCCESS;
}
