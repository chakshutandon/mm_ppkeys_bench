#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>

#define PPKEY_DISABLE_ACCESS	0x1
#define PPKEY_DISABLE_WRITE	0x2

#define PAGE_SIZE 4096
#define MAX_PPKEY_THREADS 1024

struct ppkey_area {
    uint32_t ppkey;
    uint8_t padding[PAGE_SIZE - sizeof(uint32_t)];  
};

pthread_t registered_threads[MAX_PPKEY_THREADS];
int last_registered_thread;

/* Return the value of the PKRU register. */
static inline unsigned int pkey_read (void)
{
  unsigned int result;
  __asm__ volatile (".byte 0x0f, 0x01, 0xee"
                    : "=a" (result) : "c" (0) : "rdx");
  return result;
}

/* Overwrite the PKRU register with value. */
static inline void pkey_write (unsigned int value)
{
  __asm__ volatile (".byte 0x0f, 0x01, 0xef"
                    : : "a" (value), "c" (0), "d" (0));
}

static inline void libppkey_init() {
    last_registered_thread = 0;
}

/* Return the memory address of ppkeys. */
static inline void *ppkru_get(void) {
    uint64_t res;
    // PPKRU confounded with R15 register
    __asm__ volatile("movq %%r15, %0" : "=r"(res));
    return (void *)res;
}

/* Return the per-process protection of pkey. */
static inline int ppkey_read(int key) {
    if (key <= 0 || key > 15) {
        errno = EINVAL;
        return -1;
    }
    struct ppkey_area *p = ppkru_get();
    return (p->ppkey >> (2 * key)) & 3;
}

/* Update pkru with intersection of thread and process protection key. */
static inline void curr_thread_wrpkru(uint32_t ppkey) {
    unsigned int pkru = pkey_read();
    pkru = pkru | ppkey;
    pkey_write(pkru);
}

static inline void ppkey_register_thread(pthread_t tid) {
    registered_threads[last_registered_thread + 1] = tid;
    last_registered_thread++;
}

/* Set the per-process protection of pkey. */
static inline int ppkey_write(int key, unsigned int prot) {
    if (key <= 0 || key > 15 || prot < 0 || prot > 3) {
        errno = EINVAL;
        return -1;
    }
    struct ppkey_area *p = ppkru_get();
    unsigned int mask = 3 << (2 * key);
    prot = prot << (2 * key);

    p->ppkey = (p->ppkey & ~mask) | prot;

    // Simulate PPKRU support using Intel MPK (PKRU)
    // signal all registered threads to update pkru
    int i;
    for (i = 0; i < MAX_PPKEY_THREADS; i++) {
        if (registered_threads[i]) {
            // Threads that use ppkey_write must be registered with ppkey_register_thread
            // and install curr_thread_wrpkru handler for SIGUSR1 signal

            // union sigval {
            //     int   sival_int;
            //     void *sival_ptr;
            // };
            // See PTHREAD_SIGQUEUE(3).
            pthread_sigqueue(
                registered_threads[i], 
                SIGUSR1, 
                p->ppkey
            );
        }
    }
    return 0;
}

void print_ppkey_value(struct ppkey_area *p) {
    int i, k;

    printf("Value of ppkey: %"PRIu32"\n", p->ppkey);
    for (i = 15; i >= 0; i--) {
        k = ppkey_read(i);
        if (k >= 0)
            printf("%d ", k);
        else
        {
            printf("0 ", k);
        }
    }
    printf("\n");
}
