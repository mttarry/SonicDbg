#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <unistd.h>

#include "breakpoint.h"

#define MAX_BREAKPOINTS 32

typedef struct {
    const char *program_name;
    const pid_t pid;
    breakpoint_t breakpoints[MAX_BREAKPOINTS];
} dbg_ctx;


#endif