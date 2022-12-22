#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    pid_t pid;
    intptr_t addr;
    bool enabled;
    uint8_t saved_data;
} breakpoint_t;


void enable_breakpoint(breakpoint_t *bp);
void disable_breakpoint(breakpoint_t *bp);


#endif
