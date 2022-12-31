#include <sys/ptrace.h>
#include <sys/wait.h>

#include <libdwarf-0/libdwarf.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "debugger.h"
#include "registers.h"
#include "dbg_dwarf.h"
#include "utils.h"

void free_debugger(dbg_ctx *ctx) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        free(ctx->breakpoints[i]);
    }
    dwarf_finish(ctx->dwarf);
    close_elf(ctx);
}

breakpoint_t *at_breakpoint(dbg_ctx *ctx) {
    uint64_t pc = get_pc(ctx->pid);
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        if (ctx->breakpoints[i]->addr == (intptr_t)pc) {
            return ctx->breakpoints[i];
        }
    }

    return NULL;
}

void list_breakpoints(const dbg_ctx *ctx) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        printf("Breakpoint %d at 0x%lx\n", i + 1, ctx->breakpoints[i]->addr);
    }
}

void set_bp_at_addr(dbg_ctx *ctx, uint64_t addr) {
    // Set breakpoint if limit has not been exceeded
    if (ctx->active_breakpoints < MAX_BREAKPOINTS) {
        if (bin_is_pie(ctx->elf)) {
            addr += ctx->load_addr;
        }
        breakpoint_t *new_bp = new_breakpoint(ctx->pid, ctx->active_breakpoints + 1, addr);
        enable_breakpoint(new_bp);
        ctx->breakpoints[ctx->active_breakpoints++] = new_bp;
        printf("Breakpoint %d at 0x%lx\n", ctx->active_breakpoints, addr);
    }
    else {
        printf("Error: too many breakpoints active\n");
    }
}

void set_bp_at_func(dbg_ctx *ctx, const char *symbol) {
    Dwarf_Addr addr = get_func_addr(ctx, symbol);
    if (addr != 0) {
        set_bp_at_addr(ctx, addr);
    }
    else {
        printf("Unable to set breakpoint at function %s\n", symbol);
    }
}

breakpoint_t *get_bp_at_address(dbg_ctx *ctx, uint64_t addr) {
    for (int i = 0; i < ctx->active_breakpoints; ++i) {
        if (ctx->breakpoints[i]->addr == (intptr_t)addr)
            return ctx->breakpoints[i];
    }

    return NULL;
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

void single_step(dbg_ctx *ctx) {
    breakpoint_t *bp = at_breakpoint(ctx);
    if (bp && bp->enabled) {
        step_over_breakpoint(ctx);
    }

    if (ptrace(PTRACE_SINGLESTEP, ctx->pid, NULL, NULL) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}

void continue_execution(dbg_ctx *ctx) {
    step_over_breakpoint(ctx);
    if (ptrace(PTRACE_CONT, ctx->pid, NULL, NULL) < 0)
    {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
    wait_for_signal(ctx->pid);

    breakpoint_t *bp = at_breakpoint(ctx);
    if (bp) {
        uint64_t pc = get_pc(ctx->pid);

        if (bin_is_pie(ctx->elf))
            pc -= ctx->load_addr;

        char *func = get_func_symbol_from_pc(ctx, pc);
        struct src_info src_info = get_src_info(ctx, pc);
        printf("Hit: Breakpoint %d at 0x%lx in %s at line %llu of %s\n", bp->num, bp->addr, func, src_info.line_no, loc_last_dir(src_info.src_file_name));
        free(src_info.src_file_name);
    }
}


void init_elf(dbg_ctx *ctx) {
    if (elf_version(EV_CURRENT) == EV_NONE) {
        printf("ELF library initialization failed: %s\n", elf_errmsg(elf_errno()));
        exit(EXIT_FAILURE);
    }
    if ((ctx->elf = elf_begin(ctx->elf_fd, ELF_C_READ, NULL)) == NULL) {
        printf(" elf_begin () failed: %s\n", elf_errmsg(elf_errno()));
        exit(EXIT_FAILURE);
    }
    if (elf_kind(ctx->elf) != ELF_K_ELF) {
        printf("Error: file is not an ELF object\n");
        exit(EXIT_FAILURE);
    }
}

void close_elf(dbg_ctx *ctx) {
    elf_end(ctx->elf);
    close(ctx->elf_fd);
}

void init_load_addr(dbg_ctx *ctx) {
    FILE *file;
    char *linebuf, *filebuf;
    size_t buf_size = 100;
 
    if (bin_is_pie(ctx->elf)) {
        filebuf = malloc(buf_size * sizeof(char));

        sprintf(filebuf, "/proc/%d/maps", ctx->pid);

        if ((file = fopen(filebuf, "r")) == NULL) {
            printf("Error opening %s\n", filebuf);
            perror("Error");
            exit(EXIT_FAILURE);
        }

        linebuf = malloc(buf_size * sizeof(char));
        getline(&linebuf, &buf_size, file);

        char *addr = strtok(linebuf, "-");

        ctx->load_addr = strtol(addr, NULL, 16);

        free(linebuf);
        free(filebuf);
        fclose(file);
    }
}