#include <sys/user.h>
#include <sys/uio.h>
#include <sys/ptrace.h>

#include <linux/elf.h>

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "registers.h"

/* The required core 'R' registers.  */
static const char *const aarch64_r_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_X0_REGNUM! */
  "x0", "x1", "x2", "x3",
  "x4", "x5", "x6", "x7",
  "x8", "x9", "x10", "x11",
  "x12", "x13", "x14", "x15",
  "x16", "x17", "x18", "x19",
  "x20", "x21", "x22", "x23",
  "x24", "x25", "x26", "x27",
  "x28", "x29", "x30", "sp",
  "pc", "cpsr"
};


uint64_t get_register_value(pid_t pid, enum aarch64_regnum regnum)
{
    struct iovec iovec;
    elf_gregset_t regs;

    iovec.iov_base = &regs;
    iovec.iov_len = sizeof(regs);

    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iovec) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
    
    return ((uint64_t *)iovec.iov_base)[regnum];
}

void set_register_value(pid_t pid, enum aarch64_regnum regnum, uint64_t val) {
    struct iovec iovec;
    elf_gregset_t regs;

    iovec.iov_base = &regs;
    iovec.iov_len = sizeof(regs);

    if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iovec) < 0) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    ((uint64_t *)iovec.iov_base)[regnum] = val;

    if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iovec)) {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }
}

const char *get_register_name(enum aarch64_regnum regnum)
{
    return aarch64_r_register_names[regnum];
}

enum aarch64_regnum get_register_from_name(char *name)
{
    for (int i = 0; i < 34; ++i)
    {
        if (strcmp(name, aarch64_r_register_names[i]) == 0)
            return i;
    }
    return -1;
}
