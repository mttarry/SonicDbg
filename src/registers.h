#ifndef REGISTERS_H
#define REGISTERS_H

#include <unistd.h>
#include <stdint.h>

#if defined(__aarch64__) || defined(__arm__)
#include <asm/ptrace.h>
typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[(sizeof (struct user_pt_regs) / sizeof(elf_greg_t))];
#endif

#define NUM_GP_REG  34

enum aarch64_regnum
{
  AARCH64_X0_REGNUM,		/* First integer register.  */
  AARCH64_FP_REGNUM = AARCH64_X0_REGNUM + 29,	/* Frame register, if used.  */
  AARCH64_LR_REGNUM = AARCH64_X0_REGNUM + 30,	/* Return address.  */
  AARCH64_SP_REGNUM,		/* Stack pointer.  */
  AARCH64_PC_REGNUM,		/* Program counter.  */
  AARCH64_CPSR_REGNUM,		/* Current Program Status Register.  */
  AARCH64_V0_REGNUM,		/* First fp/vec register.  */
  AARCH64_V31_REGNUM = AARCH64_V0_REGNUM + 31,	/* Last fp/vec register.  */
  AARCH64_SVE_Z0_REGNUM = AARCH64_V0_REGNUM,	/* First SVE Z register.  */
  AARCH64_SVE_Z31_REGNUM = AARCH64_V31_REGNUM,  /* Last SVE Z register.  */
  AARCH64_FPSR_REGNUM,		/* Floating Point Status Register.  */
  AARCH64_FPCR_REGNUM,		/* Floating Point Control Register.  */
  AARCH64_SVE_P0_REGNUM,	/* First SVE predicate register.  */
  AARCH64_SVE_P15_REGNUM = AARCH64_SVE_P0_REGNUM + 15,	/* Last SVE predicate
							   register.  */
  AARCH64_SVE_FFR_REGNUM,	/* SVE First Fault Register.  */
  AARCH64_SVE_VG_REGNUM,	/* SVE Vector Granule.  */

  /* Other useful registers.  */
  AARCH64_LAST_X_ARG_REGNUM = AARCH64_X0_REGNUM + 7,
  AARCH64_STRUCT_RETURN_REGNUM = AARCH64_X0_REGNUM + 8,
  AARCH64_LAST_V_ARG_REGNUM = AARCH64_V0_REGNUM + 7
};



uint64_t get_register_value(pid_t pid, enum aarch64_regnum regnum);
void set_register_value(pid_t pid, enum aarch64_regnum regnum, uint64_t val);
void dump_registers(pid_t pid);

const char *get_register_name(enum aarch64_regnum regnum);
enum aarch64_regnum get_register_from_name(const char *name);


#endif