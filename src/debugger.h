#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <unistd.h>

#include <libdwarf-0/dwarf.h>
#include <libdwarf-0/libdwarf.h>


#include "breakpoint.h"

#define MAX_BREAKPOINTS 32

typedef struct {
    const char *program_name;
    pid_t pid;
    int active_breakpoints;
    breakpoint_t *breakpoints[MAX_BREAKPOINTS];
    Dwarf_Debug dwarf;
} dbg_ctx;

void dwarf_init(dbg_ctx *ctx);

void list_breakpoints(const dbg_ctx *ctx);
void set_bp_at_addr(dbg_ctx *ctx, const char *addr);

long read_memory(const pid_t pid, const uint64_t address);
void write_memory(const pid_t pid, const uint64_t address, const long val);

uint64_t get_pc(const pid_t pid);
void set_pc(const pid_t pid, const uint64_t val);

void step_over_breakpoint(dbg_ctx *ctx);
breakpoint_t *check_breakpoint_hit(dbg_ctx *ctx);

void wait_for_signal(const pid_t pid);


#endif