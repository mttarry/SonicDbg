#include <sys/ptrace.h>
#include <sys/wait.h>

#include <stdio.h>

#include "debugger.h"
#include "commands.h"
#include "utils.h"




void handle_command(dbg_ctx *ctx, char *command) {
    trim_ends(&command);
    char **args = split(command);
    char *cmd = args[0];
    
    if (is_prefix(cmd, "continue")) {
        printf("Continuing...\n");
        ptrace(PTRACE_CONT, ctx->pid, NULL, NULL);
        int wait_status;
        int options = 0;
        waitpid(ctx->pid, &wait_status, options);
    }
    if (is_prefix(cmd, "breakpoint")) {
        char *addr = args[1];
        printf("Setting breakpoint at address %s\n", addr);
    }
}