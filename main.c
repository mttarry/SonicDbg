#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


size_t CMD_SZ = 32U;


typedef struct {
    const char *program_name;
    const pid_t pid;
} dbg_ctx;


void trim_ends(char **s) {
    char *beg = *s;
    char *end = *s + strlen(*s) - 1;

    while (isspace(*beg)) {
        beg++;
    }
    while (isspace(*end)) {
        *end = '\0';
        end--;
    }
    *s = beg;
}

int is_prefix(char *prefix, char *str) {
    trim_ends(&prefix);

    if (strlen(prefix) > strlen(str)) 
        return 0;

    for (int i = 0; i < strlen(prefix); ++i) 
        if (prefix[i] != str[i]) 
            return 0;
    
    return 1;
}

void handle_command(dbg_ctx *ctx, char *command) {
    if (is_prefix(command, "continue")) {
        printf("Continuing...\n");
        ptrace(PTRACE_CONT, ctx->pid, NULL, NULL);
        int wait_status;
        int options = 0;
        waitpid(ctx->pid, &wait_status, options);
    }
}

int main(int argc, char **argv) {
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
        dbg_ctx ctx = { path, child_pid };
        int wait_status;
        int options = 0;
        waitpid(child_pid, &wait_status, options);  

        while (1) {
            printf("parpdbg> ");
            char *buf = malloc(sizeof(char) * CMD_SZ);
            getline(&buf, &CMD_SZ, stdin);
            handle_command(&ctx, buf);

            free(buf);
        }

    }
}