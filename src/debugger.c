#include <stdio.h>

#include "debugger.h"


void list_breakpoints(dbg_ctx *ctx) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        printf("Breakpoint %d at 0x%lx\n", i + 1, ctx->breakpoints[i]->addr);
    }
}

void set_bp_at_addr(dbg_ctx *ctx, char *addr) {
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