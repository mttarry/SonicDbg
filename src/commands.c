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

void continue_execution(dbg_ctx *ctx) {
    printf("Continuing...\n");
    ptrace(PTRACE_CONT, ctx->pid, NULL, NULL);
    int wait_status;
    int options = 0;
    waitpid(ctx->pid, &wait_status, options);
}

void handle_breakpoint_command(dbg_ctx *ctx, char *addr) {
    // If no address is specified, list currently active breakpoints
    if (!addr)
        list_breakpoints(ctx);
    else 
        set_bp_at_addr(ctx, addr);
}


void handle_register_command(dbg_ctx *ctx, const char *action, const char *reg_name, const char *val) {
    if (action == NULL) {
        dump_registers(ctx->pid);
        return;
    }
    else if (reg_name == NULL) {
        printf("Please specify a register name\n");
        return;
    }

    const int regnum = get_register_from_name(reg_name);
    if (regnum == -1) {
        printf("Error: Unknown register\n");
        return;
    }
    if (is_prefix(action, "read")) {
        uint64_t val = get_register_value(ctx->pid, regnum);
        printf("%lu\n", val);
    }
    else if (is_prefix(action, "write")) {
        set_register_value(ctx->pid, regnum, strtoul(val, NULL, 10));
        printf("%s = %s\n", reg_name, val);
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
    else if (is_prefix(cmd, "register")) {
        handle_register_command(ctx, args[1], args[2], args[3]);
    }
    else {
        printf("Unknown command!\n");
        goto error;
    }

error:
    free_args(args);
}