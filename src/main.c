#include <sys/wait.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "debugger.h"
#include "commands.h"
#include "dbg_dwarf.h"


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
        dbg_ctx ctx = {};
        ctx.program_name = path;
        ctx.pid = child_pid;

        wait_for_signal(ctx.pid);

        dwarf_init(&ctx.dwarf, ctx.program_name);

        if ((ctx.elf_fd = open(ctx.program_name, O_RDONLY, 0)) < 0) {
            printf(" opening \"%s\" failed\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        init_elf(&ctx);

        init_load_addr(&ctx);

        size_t buf_size = 512;
        char *buf = malloc(buf_size * sizeof(char));

        while (1) {
            printf("parpdbg> ");
            getline(&buf, &buf_size, stdin);
            handle_command(&ctx, buf);
        }
        
        
    }
}