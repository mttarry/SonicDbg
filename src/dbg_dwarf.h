#ifndef DBG_DWARF_H
#define DBG_DWARF_H

#include <libdwarf-0/dwarf.h>
#include <libdwarf-0/libdwarf.h>

#include "debugger.h"

void dwarf_init(Dwarf_Debug *dbg, const char *program_name);
Dwarf_Addr get_func_addr(dbg_ctx *ctx, const char *symbol);
char* get_func_symbol_from_pc(dbg_ctx *ctx, uint64_t pc);
void print_lineno(dbg_ctx *ctx, uint64_t pc);
Dwarf_Unsigned get_pc_lineno(dbg_ctx *ctx, uint64_t pc);

#endif