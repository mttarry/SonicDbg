#include <sys/ptrace.h>
#include <stdlib.h>

#include "breakpoint.h"
#include <stdio.h>
void enable_breakpoint(breakpoint_t *bp) {
    bp->saved_data = ptrace(PTRACE_PEEKDATA, bp->pid, bp->addr, NULL);
    
    uint64_t trap = 0xD4200000;
    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, trap);

    printf("Enable BP: Replacing instruction %lu with %lu\n", bp->saved_data, trap);

    bp->enabled = true;
}

void disable_breakpoint(breakpoint_t *bp) {
    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, bp->saved_data);
    printf("Disable BP: Replacing trap with %lu\n", bp->saved_data);

    bp->enabled = false;
}

breakpoint_t *new_breakpoint(const pid_t pid, const char *addr) {
    breakpoint_t *new_bp = malloc(sizeof(breakpoint_t));

    new_bp->pid = pid;
    new_bp->addr = strtol(addr, NULL, 16);
    new_bp->enabled = false;
    new_bp->saved_data = 0;

    return new_bp;
}