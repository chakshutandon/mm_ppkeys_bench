#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MILLION (unsigned long long) 1e6
#define BILLION (unsigned long long) 1e9

#define NR_THREADS_MAX 128

#define timeit(function, iter)                              \
    struct timespec start, end;                             \
    unsigned long long i;                                   \
                                                            \
    clock_gettime(CLOCK_REALTIME, &start);                  \
    for (i = 0; i < iter; i++) {                            \
        function;                                           \
    }                                                       \
    clock_gettime(CLOCK_REALTIME, &end);                    \

struct thread_args {
    uint32_t *ppkey;
    pthread_mutex_t lock;
};

long double delta_time(struct timespec start, struct timespec end) {
    return BILLION * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
}

void *do_thread_run(void *_args) {
    struct thread_args *args = (struct thread_args *) _args;

    timeit(
        pthread_mutex_lock(&args->lock);
        *args->ppkey = 1;
        pthread_mutex_unlock(&args->lock)
    , MILLION);

    long double *res = malloc(sizeof(long double));
    *res = delta_time(start, end) / MILLION;
    pthread_exit(res);
}

int main() {    
    uint32_t ppkey;

    struct thread_args *args = malloc(sizeof(struct thread_args));
    args->ppkey = &ppkey;
    
    if (pthread_mutex_init(&args->lock, NULL) != 0) { 
        return EXIT_FAILURE;
    }

    long double avg_lat;

    int nr_threads;
    for (nr_threads = 1; nr_threads <= NR_THREADS_MAX; nr_threads *= 2) {
        pthread_t tid[nr_threads];
        long double *latency[nr_threads];
        int i;
        for (i = 0; i < nr_threads; i++) {
            pthread_create(&tid[i], NULL, do_thread_run, args); 
        }
        for (i = 0; i < nr_threads; i++) {
            pthread_join(tid[i], (void **)&latency[i]);
        }
        avg_lat = 0;
        for (i = 0; i < nr_threads; i++) {
            avg_lat += *latency[i];
        }
        avg_lat = avg_lat / nr_threads;

        printf("Threads: %d, Average Latency: %Lf ns\n", nr_threads, avg_lat);
    }

    return EXIT_SUCCESS;
}