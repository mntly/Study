#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

static void handler(int sig, siginfo_t *info, void *ptr)
{
    ucontext_t *uc = (ucontext_t *)ptr;

    unsigned long current_rsp;
    asm volatile (
        "mov %%rsp, %0"
        : "=r"(current_rsp)
    );

    unsigned long current_rbp;
    asm volatile (
        "mov %%rbp, %0"
        : "=r"(current_rbp)
    );

    printf("=== inside handler ===\n");
    printf("current RSP : 0x%lx\n", current_rsp);
    printf("current RBP : 0x%lx\n", current_rbp);

    printf("saved RSP   : 0x%llx\n", (unsigned long long) uc->uc_mcontext.gregs[REG_RSP]);
    printf("saved RIP   : 0x%llx\n", (unsigned long long) uc->uc_mcontext.gregs[REG_RIP]);
    printf("saved RAX   : 0x%llx\n", (unsigned long long) uc->uc_mcontext.gregs[REG_RAX]);
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;

    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    puts("before signal");

    raise(SIGUSR1);

    puts("after signal");

    return 0;
}