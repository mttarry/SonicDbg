#define _XOPEN_SOURCE 700

#include <sys/ptrace.h>
#include <sys/wait.h>

#include <libdwarf-0/libdwarf.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

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

void hit_bp_message(int bp_no, intptr_t addr, const char *func, Dwarf_Unsigned line_no, char *file) {
    printf("Breakpoint %d, " BLU "0x%lx " RESET "in " YEL "%s ()" RESET  " at line %llu of " GRN "%s\n" RESET, bp_no, addr, func, line_no, file);
}

void bp_info(dbg_ctx *ctx) {
    breakpoint_t *bp = at_breakpoint(ctx);
    uint64_t pc = get_pc(ctx->pid);

    if (bin_is_pie(ctx->elf))
        pc -= ctx->load_addr;

    char *func = get_func_symbol_from_pc(ctx, pc);
    struct src_info src_info = get_src_info(ctx, pc);
    hit_bp_message(bp->num, bp->addr, func, src_info.line_no, loc_last_dir(src_info.src_file_name));
    print_source(&src_info);
    free(src_info.src_file_name);
}

static void handle_sigtrap(dbg_ctx *ctx, siginfo_t info) {
    switch (info.si_code) {
        case TRAP_BRKPT:
        {
            bp_info(ctx);
            return;
        }
        case SI_USER:
            return;
        case TRAP_TRACE:
            return;
        default:
            printf("Unknown SIGTRAP: %s\n", strsignal(info.si_signo));
            return;
    }
}

static siginfo_t get_signal_info(pid_t pid) {
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, pid, NULL, &info);
    return info;
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
    Dwarf_Addr end_prologue_addr = 0;

    Dwarf_Line prologue_end_line = get_func_prologue_end_line(ctx, symbol);

    if (dwarf_lineaddr(prologue_end_line, &end_prologue_addr, NULL) != DW_DLV_OK) {
        printf("Error in dwarf_lineaddr\n");
        exit(EXIT_FAILURE);
    }

    if (bin_is_pie(ctx->elf))
        end_prologue_addr += ctx->load_addr;

    if (end_prologue_addr != 0) 
        set_bp_at_addr(ctx, end_prologue_addr);
    
    else 
        printf("Unable to set breakpoint at function %s\n", symbol);
    
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

void wait_for_signal(dbg_ctx *ctx) {
    int wait_status;
    int options = 0;
    waitpid(ctx->pid, &wait_status, options);

    siginfo_t siginfo = get_signal_info(ctx->pid);
    switch (siginfo.si_signo) {
        case SIGTRAP:
            handle_sigtrap(ctx, siginfo);
            break;
        case SIGSEGV:
            printf("Segfault: %d\n", siginfo.si_code);
            break;
        default:
            printf("Got signal: %s\n", strsignal(siginfo.si_signo));
            break;
    }

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

        wait_for_signal(ctx);
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
    wait_for_signal(ctx);
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

