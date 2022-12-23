#include <sys/ptrace.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>

#include "debugger.h"
#include "commands.h"
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
        char *addr = args[1];
        // If no address is specified, list currently active breakpoints
        if (!addr)
            list_breakpoints(ctx);
        else 
            set_bp_at_addr(ctx, addr);
    }
    else {
        printf("Unknown command!\n");
        goto error;
    }

error:
    free_args(args);
}