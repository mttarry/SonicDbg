#include <libdwarf-0/libdwarf.h>
#include <libdwarf-0/dwarf.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dwarf_init(Dwarf_Debug *dbg, char *prog) {
    int res;
    char trueoutpath[100];
    Dwarf_Error dw_error;

    res = dwarf_init_path(prog, 
        trueoutpath, 
        sizeof(trueoutpath), 
        DW_GROUPNUMBER_ANY, 
        NULL, NULL,
        dbg, 
        &dw_error);
    
    if (res == DW_DLV_ERROR) {
        printf("Error from libdwarf opening \"%s\":  %s\n",
            prog,
            dwarf_errmsg(dw_error));
        return;
    }
    if (res == DW_DLV_NO_ENTRY) {
        printf("There is no such file as \"%s\"\n",
            prog);
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

static Dwarf_Addr get_subprog_addr(Dwarf_Debug dbg, Dwarf_Die subprogram, char *symbol) {
    char *subprogram_name;
    Dwarf_Attribute *attrs;
    Dwarf_Addr brkpoint_addr;
    Dwarf_Signed attr_count, it;

    if (dwarf_diename(subprogram, &subprogram_name, NULL) == DW_DLV_OK) {
        if (strncmp(subprogram_name, symbol, strlen(subprogram_name)) == 0) {
            if (dwarf_attrlist(subprogram, &attrs, &attr_count, NULL) != DW_DLV_OK) {
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


static Dwarf_Addr get_subprog_die(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Bool is_info, char *symbol) {
    Dwarf_Addr brkpoint_addr = 0;
    Dwarf_Die child_die, sibling_die;
    Dwarf_Half tag;
    int ret;

    if (dwarf_tag(die, &tag, NULL) != DW_DLV_OK) {
        printf("Error in dwarf_tag\n");
        exit(1);
    }
    if (tag == DW_TAG_subprogram) {
        brkpoint_addr = get_subprog_addr(dbg, die, symbol);
        if (brkpoint_addr != 0) {
            return brkpoint_addr;
        }
    }

    ret = dwarf_child(die, &child_die, NULL);
    if (ret == DW_DLV_ERROR) {
        printf("Error in dwarf_child\n");
        exit(1);
    }
    else if (ret == DW_DLV_OK) {
        brkpoint_addr = get_subprog_die(dbg, child_die, is_info, symbol);
        dwarf_dealloc(dbg, child_die, DW_DLA_DIE);
        if (brkpoint_addr != 0) {
            return brkpoint_addr;
        }
    }

    ret = dwarf_siblingof_b(dbg, die, is_info, &sibling_die, NULL);
    if (ret == DW_DLV_ERROR) {
        printf("Error in dwarf_siblingof_b\n");
        exit(1);
    }
    else if (ret == DW_DLV_OK) {
        brkpoint_addr = get_subprog_die(dbg, sibling_die, is_info, symbol);
        dwarf_dealloc(dbg, sibling_die, DW_DLA_DIE);
        if (brkpoint_addr != 0) {
            return brkpoint_addr;
        }
    }

    return 0;
}


Dwarf_Addr get_func_addr(Dwarf_Debug dbg, char *symbol) {
    int res;
    Dwarf_Bool is_info = 1;
    Dwarf_Unsigned cu_hdr_len = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Off abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error err = 0;
    
    Dwarf_Addr func_addr;

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

        func_addr = get_subprog_die(dbg, cu_die, is_info, symbol);
        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }

    return func_addr;
}



int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: .dwarf prog");
        exit(1);
    }
    char *prog_name = argv[1];

    Dwarf_Debug dbg;

    dwarf_init(&dbg, prog_name);

    Dwarf_Addr main_addr = get_func_addr(dbg, "main");

    printf("%llu\n", main_addr);

}