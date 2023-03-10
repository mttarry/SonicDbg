#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <unistd.h>

#include <libdwarf-0/dwarf.h>
#include <libdwarf-0/libdwarf.h>
#include <libelf.h>
#include <stdbool.h>

#include "breakpoint.h"

#define MAX_BREAKPOINTS 32

typedef struct {
    const char *program_name;
    pid_t pid;
    int active_breakpoints;
    breakpoint_t *breakpoints[MAX_BREAKPOINTS];
    Dwarf_Debug dwarf;
    Elf *elf;
    int elf_fd;
    intptr_t load_addr;
    char **args;
} dbg_ctx;

void init_elf(dbg_ctx *ctx);
void close_elf(dbg_ctx *ctx);
void free_debugger(dbg_ctx *ctx);
void init_load_addr(dbg_ctx *ctx);

bool continue_execution(dbg_ctx *ctx);

void list_breakpoints(const dbg_ctx *ctx);
void set_bp_at_addr(dbg_ctx *ctx, uint64_t addr);
void set_bp_at_func(dbg_ctx *ctx, const char *symbol);

long read_memory(const pid_t pid, const uint64_t address);
void write_memory(const pid_t pid, const uint64_t address, const long val);

uint64_t get_pc(const pid_t pid);
void set_pc(const pid_t pid, const uint64_t val);

void step_over_breakpoint(dbg_ctx *ctx);
breakpoint_t *at_breakpoint(dbg_ctx *ctx);

bool wait_for_signal(dbg_ctx *ctx);

void single_step(dbg_ctx *ctx);

#endif