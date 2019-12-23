#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>
#include <pthread.h>

#include "libppkey.h"

#define BUFFER_LENGTH 1024

// redefine
#define PKEY_DISABLE_ACCESS 0x1
#define PKEY_DISABLE_WRITE 0x2

struct arg_struct {
    int pkey;
    int *buffer;
};

/* Return the value of the PKRU register. */
static unsigned int pkey_read(void) {
  unsigned int result;
  __asm__ volatile (".byte 0x0f, 0x01, 0xee"
                    : "=a" (result) : "c" (0) : "rdx");
  return result;
}

int pkey_get(int key) {
    if (key < 0 || key > 15)
        return -1;
    unsigned int pkru = pkey_read();
    return (pkru >> (2 * key)) & 3;
}

void *ppkey_setter(void *argp) {
    register_thread();

    struct arg_struct *args = (struct arg_struct *)argp;
    int pkey = args->pkey;

    ppkey_write(pkey, PKEY_DISABLE_WRITE);
    printf("[ppkey_setter] ppkey_write(%d, PKEY_DISABLE_WRITE)\n", pkey);

    unregister_thread();
}

void *ppkey_getter(void *argp) {
    register_thread();

    struct arg_struct *args = (struct arg_struct *)argp;
    int *buffer = args->buffer;

    // Attempt to access buffer
    printf("[ppkey_getter] Accessing buffer...");
    *buffer = 1;
    printf("Done\n");

    printf("[ppkey_getter] PKRU: %d\n", pkey_read());

    // Start ppkey_setter thread to set PPKRU
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, ppkey_setter, argp);
    pthread_join(thread_id, NULL);

    printf("[ppkey_getter] PKRU: %d\n", pkey_read());

    // Attempt to write to buffer after PKEY_DISABLE_WRITE
    printf("[ppkey_getter] Accessing buffer... Expecting Segfault (See bug libppkey.c: sig_handler)\n");
    *buffer = 1;

    // Should never run
    printf("[ppkey_getter] Error: No Segfault (See bug libppkey.c: sig_handler)\n");
    exit(EXIT_FAILURE);

    unregister_thread();
}

int main() {
    // Create a buffer to protect
    int *buffer = mmap(NULL, BUFFER_LENGTH, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Allocate pkey from kernel, set protection to zero (Access Enable, Write Enable)
    int pkey = pkey_alloc(0, 0);
    if (pkey == -1) {
        perror("pkey_alloc");
        exit(EXIT_FAILURE);
    }
    
    // Protect buffer with pkey
    if (pkey_mprotect(buffer, 100, PROT_READ | PROT_WRITE, pkey)) {
        perror("pkey_mprotect");
        exit(EXIT_FAILURE);
    }

    pthread_t thread_id;
    // Setup args to pass to thread
    struct arg_struct args;
    args.pkey = pkey;
    args.buffer = buffer;
    // Create thread to read buffer
    pthread_create(&thread_id, NULL, ppkey_getter, &args);
    pthread_join(thread_id, NULL);
}