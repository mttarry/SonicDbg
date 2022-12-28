#ifndef DBG_DWARF_H
#define DBG_DWARF_H

#include <libdwarf-0/dwarf.h>
#include <libdwarf-0/libdwarf.h>

void dwarf_init(Dwarf_Debug *dbg, const char *program_name);
Dwarf_Addr get_func_addr(Dwarf_Debug dbg, const char *symbol);

#endif