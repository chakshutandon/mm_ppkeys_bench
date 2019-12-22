#include "libppkey.h"

Node *head_registered_threads = NULL;

/* Return the value of the PKRU register. */
static unsigned int pkey_read (void) {
  unsigned int result;
  __asm__ volatile (".byte 0x0f, 0x01, 0xee"
                    : "=a" (result) : "c" (0) : "rdx");
  return result;
}

/* Overwrite the PKRU register with value. */
static void pkey_write (unsigned int value) {
  __asm__ volatile (".byte 0x0f, 0x01, 0xef"
                    : : "a" (value), "c" (0), "d" (0));
}

/* Return the memory address of ppkeys. */
static void *ppkru_get(void) {
    uint64_t res;
    // PPKRU confounded with R15 register
    __asm__ volatile("movq %%r15, %0" : "=r"(res));
    if (!res) {
        errno = ENOTSUP;
        perror("ppkru_get");
        exit(EXIT_FAILURE);
    }
    return (void *)res;
}

/* Update pkru with intersection of thread and process protection key. */
static void thread_ppkey_update_pkru(uint32_t ppkey) {
    unsigned int pkru = pkey_read();
    pkru = pkru | ppkey;
    pkey_write(pkru);
}

/* Handler to update thread PKRU register on SIGUSR1. */
static void sig_handler(int signal, siginfo_t *info, void *unused) {
    thread_ppkey_update_pkru((uint32_t)info->si_int);
}

/* Register thread with PKRU update signal handler and insert into list of active threads. */
int register_thread(pthread_t thread_id) {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_handler;
    
    sigaction(SIGUSR1, &sa, NULL);
    head_registered_threads = insert_node(head_registered_threads, thread_id);
}

/* Unregister thread from receiving PKRU update signal. */
int unregister_thread(pthread_t thread_id) {
    head_registered_threads = remove_node(head_registered_threads, thread_id);
}

/* Return the per-process protection of pkey. */
int ppkey_read(int key) {
    if (key <= 0 || key > 15) {
        errno = EINVAL;
        perror("ppkey_read");
        return -1;
    }
    struct ppkey_area *p = ppkru_get();
    return (p->ppkey >> (2 * key)) & 3;
}

/* Set the per-process protection of pkey. */
int ppkey_write(int key, unsigned int prot) {
    if (key <= 0 || key > 15 || prot < 0 || prot > 3) {
        errno = EINVAL;
        perror("ppkey_write");
        return -1;
    }
    struct ppkey_area *p = ppkru_get();
    
    unsigned int mask = 3 << (2 * key);
    prot = prot << (2 * key);

    p->ppkey = (p->ppkey & ~mask) | prot;
    
    union sigval value;
    value.sival_int = p->ppkey;

    Node *ptr;
    /* Simulate PPKRU support using Intel MPK (PKRU); signal all registered threads */
    for (ptr = head_registered_threads; ptr; ptr = ptr->next)
        pthread_sigqueue(ptr->thread_id, SIGUSR1, value);

    return 0;
}

/* Print value of ppkeys. */
void print_ppkeys() {
    int i, k;
    struct ppkey_area *p = ppkru_get();

    if (!p) {
        errno = ENOTSUP;
        perror("ppkru_get");
        exit(EXIT_FAILURE);
    }
    printf("Value of ppkey: %"PRIu32"\n", p->ppkey);
    for (i = 15; i >= 0; i--) {
        ppkey_read(i) >= 0 ? printf("%d ", k): printf("0 ");
    }
    printf("\n");
}
