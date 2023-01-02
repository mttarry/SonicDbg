#ifndef DBG_DWARF_H
#define DBG_DWARF_H

#include <libdwarf-0/dwarf.h>
#include <libdwarf-0/libdwarf.h>

#include "debugger.h"

struct src_info {
    char *src_file_name;
    Dwarf_Line src_line;
    Dwarf_Unsigned line_no;
};

void dwarf_init(Dwarf_Debug *dbg, const char *program_name);
Dwarf_Addr get_func_addr(dbg_ctx *ctx, const char *symbol);
char* get_func_symbol_from_pc(dbg_ctx *ctx, uint64_t pc);
struct src_info get_src_info(dbg_ctx *ctx, uint64_t pc);
void print_source(struct src_info *src_info);
Dwarf_Line get_func_prologue_end_line(dbg_ctx *ctx, const char *symbol);
#endif