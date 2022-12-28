#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dbg_dwarf.h"


void dwarf_init(Dwarf_Debug *dbg, const char *program_name) {
    int res;
    char trueoutpath[100];
    Dwarf_Error dw_error;

    res = dwarf_init_path(program_name, 
        trueoutpath, 
        sizeof(trueoutpath), 
        DW_GROUPNUMBER_ANY, 
        NULL, NULL,
        dbg,
        &dw_error);
    
    if (res == DW_DLV_ERROR) {
        printf("Error from libdwarf opening \"%s\":  %s\n",
            program_name,
            dwarf_errmsg(dw_error));
        return;
    }
    else if (res == DW_DLV_NO_ENTRY) {
        printf("There is no such file as \"%s\"\n",
            program_name);
        return;
    }
}

static bool pc_in_die(Dwarf_Die die, Dwarf_Addr pc) {
    int ret;
    Dwarf_Addr cu_lowpc, cu_highpc;
    enum Dwarf_Form_Class highpc_cls;
    Dwarf_Error err;

    ret = dwarf_lowpc(die, &cu_lowpc, &err);
    if (ret == DW_DLV_OK) {
        if (pc == cu_lowpc) {
            return true;
        }
        ret = dwarf_highpc_b(die, &cu_highpc, NULL, &highpc_cls, &err);
        if (ret == DW_DLV_OK) {
            if (highpc_cls == DW_FORM_CLASS_CONSTANT) {
                cu_highpc += cu_lowpc;
            }
            if (pc >= cu_lowpc && pc < cu_highpc) {
                return true;
            }
        }
    }
    return false;
}

static bool cmp_die_subprog_name(Dwarf_Die subprogram, const char *symbol) {
    char *subprogram_name;

    if (dwarf_diename(subprogram, &subprogram_name, NULL) == DW_DLV_OK) {
        if (strncmp(subprogram_name, symbol, strlen(subprogram_name)) == 0) {
           return true;
        }
    }

    return false;
}

// If symbol is NULL, get subprogram die based on PC
// Else, get subprogram die based on symbol
static Dwarf_Die find_subprog_die(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Bool is_info, const char *symbol, uint64_t pc) {
    static Dwarf_Die ret_die = NULL;
    Dwarf_Die child_die, sibling_die;
    Dwarf_Half tag;
    int ret;

    if (dwarf_tag(die, &tag, NULL) != DW_DLV_OK) {
        printf("Error in dwarf_tag\n");
        exit(1);
    }
    if (tag == DW_TAG_subprogram) {
        if (symbol != NULL && cmp_die_subprog_name(die, symbol)) {
            ret_die = die;
            return ret_die;
        }
        else if (pc_in_die(die, pc)) {
            ret_die = die;
            return ret_die;
        }
    } 

    ret = dwarf_child(die, &child_die, NULL);
    if (ret == DW_DLV_ERROR) {
        printf("Error in dwarf_child\n");
        exit(1);
    }
    else if (ret == DW_DLV_OK) {
        find_subprog_die(dbg, child_die, is_info, symbol, pc);
        dwarf_dealloc(dbg, child_die, DW_DLA_DIE);
    }

    ret = dwarf_siblingof_b(dbg, die, is_info, &sibling_die, NULL);
    if (ret == DW_DLV_ERROR) {
        printf("Error in dwarf_siblingof_b\n");
        exit(1);
    }
    else if (ret == DW_DLV_OK) {
        find_subprog_die(dbg, sibling_die, is_info, symbol, pc);
        dwarf_dealloc(dbg, sibling_die, DW_DLA_DIE);
    }
    
    return ret_die;
}


static Dwarf_Die get_subprog_die_cu(Dwarf_Debug dbg, const char *symbol, uint64_t pc) {
    int res;
    Dwarf_Bool is_info = 1;
    Dwarf_Unsigned cu_hdr_len = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Off abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error err = 0;
    
    Dwarf_Die subprog_die;

    while (1) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;

        res = dwarf_next_cu_header_d(dbg,
                is_info,
                &cu_hdr_len,
                &version_stamp,
                &abbrev_offset,
                &address_size,
                0,0,0,0,
                &next_cu_header,
                0,
                &err);
        
        if (res == DW_DLV_ERROR) {
            char *em = err ? dwarf_errmsg(err) : "unknown error";
            printf("Error in dwarf_next_cu_header_d: %s\n", em);
            exit(1);
        }
        else if (res == DW_DLV_NO_ENTRY) {
            break;
        }

        res = dwarf_siblingof_b(dbg, no_die, is_info, &cu_die, &err);
        if (res == DW_DLV_ERROR) {
            char *em = err ? dwarf_errmsg(err) : "unknown error";
            printf("Error in dwarf_siblingof_b (level 0): %s\n", em);
            exit(1);
        }
        else if (res == DW_DLV_NO_ENTRY) {
            printf("No DIE of Compilation Unit\n");
            exit(EXIT_FAILURE);
        }

        subprog_die = find_subprog_die(dbg, cu_die, is_info, symbol, pc);
        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }

    return subprog_die;
}


char* get_func_symbol_from_pc(Dwarf_Debug dbg, uint64_t pc) {
    Dwarf_Die subprog_die;
    char *subprog_name;
    
    subprog_die = get_subprog_die_cu(dbg, NULL, pc);
    if (dwarf_diename(subprog_die, &subprog_name, NULL) == DW_DLV_OK) {
        return subprog_name;
    }

    return NULL;
}

Dwarf_Addr get_func_addr(Dwarf_Debug dbg, const char *symbol) {
    Dwarf_Die subprog_die;
    char *subprog_name;
    Dwarf_Attribute *attrs;
    Dwarf_Addr brkpoint_addr;
    Dwarf_Signed attr_count, it;
    Dwarf_Error err;

    subprog_die = get_subprog_die_cu(dbg, symbol, 0);

    if (dwarf_diename(subprog_die, &subprog_name, &err) == DW_DLV_OK) {
        if (strncmp(subprog_name, symbol, strlen(subprog_name)) == 0) {
            if (dwarf_attrlist(subprog_die, &attrs, &attr_count, NULL) != DW_DLV_OK) {
                printf("Error in dwarf_attrlist\n");
                exit(EXIT_FAILURE);
            }
            for (it = 0; it < attr_count; ++it) {
                Dwarf_Half attr_code;
                if (dwarf_whatattr(attrs[it], &attr_code, NULL) != DW_DLV_OK) {
                    printf("Error in dwarf_whatattr\n");
                    exit(EXIT_FAILURE);
                }
                if (attr_code == DW_AT_low_pc) {
                    dwarf_formaddr(attrs[it], &brkpoint_addr, NULL);
                    return brkpoint_addr;
                }
            }
        }
    }

    return 0;
}
