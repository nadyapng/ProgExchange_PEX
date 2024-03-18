#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/signal.h>

void handle_sigusr(int sig, siginfo_t *info, void *ucontext);

void handle_sigchld(int sig, siginfo_t *info, void *ucontext);

int check_disconnect(struct TRADER** trader_arr, int num_traders, int *num_disconnected, long *exchange_fees);

#endif