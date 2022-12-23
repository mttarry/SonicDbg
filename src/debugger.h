#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <unistd.h>

#include "breakpoint.h"

#define MAX_BREAKPOINTS 32

typedef struct {
    const char *program_name;
    const pid_t pid;
    int active_breakpoints;
    breakpoint_t *breakpoints[MAX_BREAKPOINTS];
} dbg_ctx;


void list_breakpoints(dbg_ctx *ctx);
void set_bp_at_addr(dbg_ctx *ctx, char *addr);

#endif