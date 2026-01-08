#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

struct thread_in {
    int *students;
    pthread_mutex_t *mut;
};

struct thread_context {
    pthread_t tid;
    struct thread_in *in;
    int where;
    int alive;
};

void thread_cleanup(void *args) {
    struct thread_context *ctx = args;
    pthread_mutex_unlock(ctx->in->mut);
}

void *thread_work(void *args) {
    struct thread_context *ctx = args;
    for (int i=0; i<4; i++) {
        pthread_mutex_lock(ctx->in->mut);
        pthread_cleanup_push(thread_cleanup, ctx);
        ctx->in->students[i]--;
        if (i < 3) {
            ctx->in->students[i+1]++;
            ctx->where++;
        }
        pthread_testcancel();
        pthread_cleanup_pop(1);
        sleep(1);
    }
    pthread_mutex_lock(ctx->in->mut);
    pthread_cleanup_push(thread_cleanup, ctx);
    ctx->in->students[ctx->where]--;
    pthread_testcancel();
    pthread_cleanup_pop(1);
    printf("thread %lu finished\n", ctx->tid);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2)
        return EXIT_FAILURE;
    int n = atoi(argv[1]);
    if (n <= 0)
        return EXIT_FAILURE;

    pthread_mutex_t mut;
    pthread_mutex_init(&mut, NULL);

    int students[4] = {0, 0, 0, 0};

    struct thread_in in = {students, &mut};

    struct thread_context *threads = malloc(n * sizeof(struct thread_context));

    if (threads == NULL)
        return EXIT_FAILURE;

    for (int i=0; i<n; i++) {
        threads[i].in = &in;
        threads[i].where = 0;
        threads[i].alive = 1;
        if (pthread_create(&threads[i].tid, NULL, thread_work, &threads[i]) != 0)
            return EXIT_FAILURE;
    }

    double elapsed_ms = 0;
    while (1) {
        // usun losowego (zywego) studenta
        while (1) {
            int idx = rand() % n;
            if (threads[idx].alive) {
                pthread_cancel(threads[idx].tid);
                threads[idx].alive = 0;
                printf("thread %lu canceled\n", threads[idx].tid);
                break;
            }
        }
        pthread_mutex_lock(&mut);
        printf("Students: [%d, %d, %d, %d]\n", students[0], students[1], students[2], students[3]);
        pthread_mutex_unlock(&mut);
        struct timespec req = {0, 0};
        int ms = rand() % 201 + 100;
        req.tv_nsec = ms * 1000000;
        if (elapsed_ms + ms > 4000)
            break;
        nanosleep(&req, NULL);
        elapsed_ms += ms;
    }
    for (int i=0; i<n; i++) {
        pthread_join(threads[i].tid, NULL);
    }

    free(threads);
    pthread_mutex_destroy(&mut);
    return EXIT_SUCCESS;
}