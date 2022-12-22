#include <sys/ptrace.h>


#include "breakpoint.h"

void enable_breakpoint(breakpoint_t *bp) {
    uint64_t data = ptrace(PTRACE_PEEKDATA, bp->pid, bp->addr, NULL);
    bp->saved_data = (uint8_t)(data & 0xFF);

    uint64_t int3 = 0xCC;
    uint64_t data_with_int3 = ((bp->saved_data & ~(0xFF)) | int3);
    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, data_with_int3);

    bp->enabled = true;
}

void disable_breakpoint(breakpoint_t *bp) {
    uint64_t data = ptrace(PTRACE_PEEKDATA, bp->pid, bp->addr, NULL);
    uint64_t restored_data = ((data & ~(0xFF)) | bp->saved_data);

    ptrace(PTRACE_POKEDATA, bp->pid, bp->addr, restored_data);

    bp->enabled = false;
}