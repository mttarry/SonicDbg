#include <sys/ptrace.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "debugger.h"
#include "registers.h"


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

long read_memory(const pid_t pid, const uint64_t address) {
    long val;
    val = ptrace(PTRACE_PEEKDATA, pid, address, NULL);
    if (errno != 0) {
        perror("PTRACE_PEEKDATA error: ");
        printf("Errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    return val;
}

void write_memory(const pid_t pid, const uint64_t address, const long val) {
    if (ptrace(PTRACE_POKEDATA, pid, address, val) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}

uint64_t get_pc(const pid_t pid) {
    return get_register_value(pid, AARCH64_PC_REGNUM);
}

void set_pc(const pid_t pid, const uint64_t val) {
    set_register_value(pid, AARCH64_PC_REGNUM, val);
}

breakpoint_t *get_bp_at_address(dbg_ctx *ctx, uint64_t addr) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        if (ctx->breakpoints[i]->addr == (intptr_t)addr)
            return ctx->breakpoints[i];
    }

    return NULL;
}

void wait_for_signal(const pid_t pid) {
    int wait_status;
    int options = 0;
    waitpid(pid, &wait_status, options);
}

void step_over_breakpoint(dbg_ctx *ctx) {
    uint64_t possible_bp_loc = get_pc(ctx->pid);

    breakpoint_t *bp = get_bp_at_address(ctx, possible_bp_loc);
    if (bp && bp->enabled) {
        disable_breakpoint(bp);

        if (ptrace(PTRACE_SINGLESTEP, ctx->pid, NULL, NULL) < 0) {
            perror("Error: ");
            exit(EXIT_FAILURE);
        }

        wait_for_signal(ctx->pid);
        enable_breakpoint(bp);
    }
}