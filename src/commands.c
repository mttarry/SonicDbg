#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

#include <stdio.h>
#include <stdlib.h>

#include "debugger.h"
#include "commands.h"
#include "registers.h"
#include "utils.h"

static void free_args(char **args)
{
    char **tmp = args;
    while (*tmp)
        free(*tmp++);
    free(args);
}

static void continue_execution(dbg_ctx *ctx)
{
    printf("Continuing...\n");
    step_over_breakpoint(ctx);
    if (ptrace(PTRACE_CONT, ctx->pid, NULL, NULL) < 0)
    {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
    wait_for_signal(ctx->pid);

    breakpoint_t *bp = check_breakpoint_hit(ctx);
    if (bp) {
        printf("Hit: Breakpoint %d at 0x%lx\n", bp->num, bp->addr);
    }
}

static void handle_breakpoint_command(dbg_ctx *ctx, const char *addr)
{
    // If no address is specified, list currently active breakpoints
    if (!addr)
        list_breakpoints(ctx);
    else
        set_bp_at_addr(ctx, addr);
}

static void handle_register_command(const pid_t pid,
                                    const char *action,
                                    const char *reg_name,
                                    const char *val)
{
    if (action == NULL)
    {
        printf("Please specify an action (read/write/dump)\n");
        return;
    }

    if (is_prefix(action, "dump"))
    {
        dump_registers(pid);
        return;
    }

    if (reg_name == NULL)
    {
        printf("Register name needed for read/write operation\n");
        return;
    }

    const int regnum = get_register_from_name(reg_name);
    if (regnum == -1)
    {
        printf("Error: Unknown register\n");
        return;
    }
    if (is_prefix(action, "read"))
    {
        uint64_t reg_val = get_register_value(pid, regnum);
        printf("$%d = %lu\n", regnum, reg_val);
    }
    else if (is_prefix(action, "write"))
    {
        if (val == NULL)
        {
            printf("Register value needed for write operation\n");
            return;
        }
        set_register_value(pid, regnum, convert_val_radix(val));
        printf("$%d = %s\n", regnum, val);
    }
}

void handle_memory_command(const pid_t pid,
                           const char *action,
                           const char *address,
                           const char *val)
{

    if (action == NULL || address == NULL)
    {
        printf("Please specify an action (read/write) and an address\n");
        return;
    }

    uint64_t addr = strtoul(address, NULL, 16);
    if (is_prefix(action, "read"))
    {
        long mem_val = read_memory(pid, addr);
        printf("%ld\n", mem_val);
    }
    else if (is_prefix(action, "write"))
    {
        if (val == NULL)
        {
            printf("Value needed for write operation\n");
            return;
        }
        uint64_t write_val = convert_val_radix(val);
        write_memory(pid, addr, write_val);
        printf("*%s = %s\n", address, val);
    }
}

void handle_command(dbg_ctx *ctx, char *command)
{
    // remove leading and trailing whitespace
    trim_ends(&command);

    // split command into separate arguments
    char **args = split(command);
    char *cmd = args[0];

    if (is_prefix(cmd, "continue"))
    {
        continue_execution(ctx);
    }
    else if (is_prefix(cmd, "breakpoint"))
    {
        handle_breakpoint_command(ctx, args[1]);
    }
    else if (is_prefix(cmd, "register"))
    {
        handle_register_command(ctx->pid, args[1], args[2], args[3]);
    }
    else if (is_prefix(cmd, "memory"))
    {
        handle_memory_command(ctx->pid, args[1], args[2], args[3]);
    }
    else
    {
        printf("Unknown command!\n");
        goto error;
    }

error:
    free_args(args);
}