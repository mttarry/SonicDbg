#include <sys/wait.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debugger.h"
#include "commands.h"



int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please specify an executable file as input\n");
        exit(EXIT_FAILURE);
    }
    
    const char *path = argv[1];
    char *newargv[] = { NULL };
    char *newenviron[] = { NULL };
    
    pid_t child_pid = fork();
    
    if (child_pid == 0) {
        ptrace(PTRACE_TRACEME);
        printf("Executing tracee program...\n");
        execve(path, newargv, newenviron);
    }
    else {
        dbg_ctx ctx = { path, child_pid, 0, {}, NULL };

        dwarf_init(&ctx);

        wait_for_signal(ctx.pid);
        
        size_t buf_size = 512;
        char *buf = malloc(buf_size * sizeof(char));

        while (1) {
            printf("parpdbg> ");

            getline(&buf, &buf_size, stdin);

            handle_command(&ctx, buf);
        }
        
        free(buf);
    }
}