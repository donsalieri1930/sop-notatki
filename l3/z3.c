#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

struct thread_in {
    int n;
    int m;
    int *track;
    pthread_mutex_t *mut;
};

void msleep(unsigned int milisec) {
    time_t sec = (int)(milisec / 1000);
    milisec = milisec - (sec * 1000);
    struct timespec req = {0, 0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    nanosleep(&req, &req);
}

void *thread_work(void *args) {
    struct thread_in *ctx = args;
    srand(time(NULL) + pthread_self());
    int pos = 0;
    pthread_mutex_lock(ctx->mut);
    ctx->track[0]++;
    pthread_mutex_unlock(ctx->mut);

    int direction = 1;
    
    while (1) {
        int old_pos = pos;
        int ms = rand() % 1321 + 200;
        msleep(ms);
        int step = rand() % 5 + 1;

        int max  = ctx->n - 1;
        int next = pos + direction * step;
        if (next > max) {
            pos = max;
            direction = -1;  
        } else if (next < 0) {
            pos = 0;
            direction = +1;
        } else {
            pos = next;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        pthread_mutex_lock(ctx->mut);
        if (pos != old_pos && ctx->track[pos] > 0 && pos != max) {
            pthread_mutex_unlock(ctx->mut);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            pos = old_pos;
            continue;
        }
        ctx->track[old_pos]--;
        ctx->track[pos]++;
        if (pos == max) {
            pthread_mutex_unlock(ctx->mut);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            break;
        }
        pthread_mutex_unlock(ctx->mut);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return NULL;
}

struct thread_sig_in {
    int *stop;
    pthread_mutex_t *mut;
};

void *thread_work_sig(void *args) {
    struct thread_sig_in *ctx = args;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    while (1) {
        int sig;
        if (sigwait(&mask, &sig) != 0)
            exit(EXIT_FAILURE);
        if (sig == SIGINT) {
            pthread_mutex_lock(ctx->mut);
            *(ctx->stop) = 1;
            pthread_mutex_unlock(ctx->mut);
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3)
        return EXIT_FAILURE;
    struct thread_in ctx;
    ctx.n = atoi(argv[1]);
    ctx.m = atoi(argv[2]);
    if (ctx.n <= 20 || ctx.m <= 2)
        return EXIT_FAILURE;
    ctx.track = malloc(ctx.n * sizeof(int));
    if (ctx.track == NULL)
        return EXIT_FAILURE;

    pthread_mutex_t mut;
    pthread_mutex_init(&mut, NULL);
    ctx.mut = &mut;

    pthread_t *threads = malloc(ctx.m * sizeof(pthread_t));
    if (threads == NULL)
        return EXIT_FAILURE;

    for (int i=0; i<ctx.n; i++) {
        ctx.track[i] = 0;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    for (int i=0; i<ctx.m; i++) {
        if (pthread_create(&threads[i], NULL, thread_work, &ctx) != 0)
            return EXIT_FAILURE;
    }

    int stop = 0;
    pthread_mutex_t mut_stop;
    pthread_mutex_init(&mut_stop, NULL);

    struct thread_sig_in sig_ctx =  {&stop, &mut_stop};

    pthread_t sig_thread;
    if (pthread_create(&sig_thread, NULL, thread_work_sig, &sig_ctx) != 0)
        return EXIT_FAILURE;

    while (1) {
        pthread_mutex_lock(&mut_stop);
        if (stop) {
            pthread_mutex_unlock(&mut_stop);
            for (int i=0; i<ctx.m; i++) {
                pthread_cancel(threads[i]);
            }
            break;
        }
        pthread_mutex_unlock(&mut_stop);
        pthread_mutex_lock(&mut);
        for (int i=0; i<ctx.n; i++)
            printf("%d ", ctx.track[i]);
        printf("\n");
        pthread_mutex_unlock(&mut);
        sleep(1);
    }

    for (int i=0; i<ctx.m; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_cancel(sig_thread);
    pthread_join(sig_thread, NULL);

    free(threads);
    free(ctx.track);
    pthread_mutex_destroy(ctx.mut);
    return EXIT_SUCCESS;
}