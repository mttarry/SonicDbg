#include <sys/ptrace.h>

#include <stdio.h>
#include <stdlib.h>

#include "debugger.h"


void list_breakpoints(const dbg_ctx *ctx) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        printf("Breakpoint %d at 0x%lx\n", i + 1, ctx->breakpoints[i]->addr);
    }
}

void set_bp_at_addr(dbg_ctx *ctx, const char *addr) {
    if (ctx->active_breakpoints < MAX_BREAKPOINTS) {
        // Set breakpoint if limit has not been exceeded
        breakpoint_t *new_bp = new_breakpoint(ctx->pid, addr);
        enable_breakpoint(new_bp);
        ctx->breakpoints[ctx->active_breakpoints++] = new_bp;
        printf("Breakpoint at address %s\n", addr);
    }
    else {
        printf("Error: too many breakpoints active\n");
    }
}

int64_t read_memory(const pid_t pid, const uint64_t address) {
    int64_t val;
    if ((val = ptrace(PTRACE_PEEKDATA, pid, address, NULL)) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    return val;
}

void write_memory(const pid_t pid, const uint64_t address, const int64_t val) {
    if (ptrace(PTRACE_POKEDATA, pid, address, val) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}