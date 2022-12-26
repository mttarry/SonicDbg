#include <sys/ptrace.h>
#include <stdlib.h>

#include "breakpoint.h"
#include <stdio.h>

void enable_breakpoint(breakpoint_t *bp) {
    long saved_data = ptrace(PTRACE_PEEKDATA, bp->pid, bp->addr, NULL);
    // save lower half, the data that will be replaced by the trap instruction
    bp->saved_data = saved_data & 0xFFFFFFFF;
    long trap = 0xD4200000;

    // writing 64 bits, so writing both trap in lower half and next instruction in upper half
    long data_to_write = ((saved_data >> 32) << 32) | trap;
    
    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, data_to_write);

    bp->enabled = true;
}

void disable_breakpoint(breakpoint_t *bp) {
    // reading current data in case breakpoint has been set at next instruction
    long prev_data = ptrace(PTRACE_PEEKDATA, bp->pid, bp->addr, NULL);
    // data to restore is saved instruction to replace trap and current next instruction
    long data_to_restore = ((prev_data >> 32) << 32) | bp->saved_data;
    
    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, data_to_restore);

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