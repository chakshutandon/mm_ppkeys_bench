#define _GNU_SOURCE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <inttypes.h>
#include <ucontext.h>

#include <signal.h>

#include <error.h>
#include <errno.h>

#include "list.h"

#define PAGE_SIZE 4096

struct ppkey_area {
    uint32_t ppkey;
    uint8_t padding[PAGE_SIZE - sizeof(uint32_t)];  
};

static unsigned int pkey_read(void);
static void pkey_write(unsigned int value);

static void *ppkru_get(void);
static void thread_ppkey_update_pkru(uint32_t ppkey);
static void sig_handler(int signal, siginfo_t *info, void *ucontext);

int register_thread(void);
int unregister_thread(void);

int ppkey_read(int key);
int ppkey_write(int key, unsigned int prot);
void print_ppkeys(void);
