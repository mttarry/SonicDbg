#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "dbg_dwarf.h"
#include "utils.h"

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
        if (strncmp(subprogram_name, symbol, strlen(symbol)) == 0) {
           return true;
        }
    }

    return false;
}

// If symbol is NULL, get subprogram die based on PC
// Else, get subprogram die based on symbol
static Dwarf_Die find_subprog_die(dbg_ctx *ctx, Dwarf_Die die, Dwarf_Bool is_info, const char *symbol, uint64_t pc) {
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
        find_subprog_die(ctx, child_die, is_info, symbol, pc);
        if (ret_die && ret_die != child_die)
            dwarf_dealloc(ctx->dwarf, child_die, DW_DLA_DIE);
    }

    ret = dwarf_siblingof_b(ctx->dwarf, die, is_info, &sibling_die, NULL);
    if (ret == DW_DLV_ERROR) {
        printf("Error in dwarf_siblingof_b\n");
        exit(1);
    }
    else if (ret == DW_DLV_OK) {
        find_subprog_die(ctx, sibling_die, is_info, symbol, pc);
        if (ret_die && ret_die != sibling_die)
            dwarf_dealloc(ctx->dwarf, sibling_die, DW_DLA_DIE);
    }
    
    return ret_die;
}


static Dwarf_Die get_subprog_die_cu(dbg_ctx *ctx, const char *symbol, uint64_t pc) {
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

        res = dwarf_next_cu_header_d(ctx->dwarf,
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

        res = dwarf_siblingof_b(ctx->dwarf, no_die, is_info, &cu_die, &err);
        if (res == DW_DLV_ERROR) {
            char *em = err ? dwarf_errmsg(err) : "unknown error";
            printf("Error in dwarf_siblingof_b (level 0): %s\n", em);
            exit(1);
        }
        else if (res == DW_DLV_NO_ENTRY) {
            printf("No DIE of Compilation Unit\n");
            exit(EXIT_FAILURE);
        }

        subprog_die = find_subprog_die(ctx, cu_die, is_info, symbol, pc);
        dwarf_dealloc(ctx->dwarf, cu_die, DW_DLA_DIE);
    }

    return subprog_die;
}


char* get_func_symbol_from_pc(dbg_ctx *ctx, uint64_t pc) {
    Dwarf_Die subprog_die;
    char *subprog_name;

    subprog_die = get_subprog_die_cu(ctx, NULL, pc);
    if (dwarf_diename(subprog_die, &subprog_name, NULL) == DW_DLV_OK) {
        return subprog_name;
    }

    return NULL;
}

static Dwarf_Die get_cu_die_by_pc(dbg_ctx *ctx, uint64_t pc) {
    int res;
    Dwarf_Bool is_info = 1;
    Dwarf_Unsigned cu_hdr_len = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Off abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Error err = 0;
    
    Dwarf_Die ret_die = 0;

    while (1) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;

        res = dwarf_next_cu_header_d(ctx->dwarf,
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

        res = dwarf_siblingof_b(ctx->dwarf, no_die, is_info, &cu_die, &err);
        if (res == DW_DLV_ERROR) {
            char *em = err ? dwarf_errmsg(err) : "unknown error";
            printf("Error in dwarf_siblingof_b (level 0): %s\n", em);
            exit(1);
        }
        else if (res == DW_DLV_NO_ENTRY) {
            printf("No DIE of Compilation Unit\n");
            exit(EXIT_FAILURE);
        }

        if (pc_in_die(cu_die, pc)) {
            ret_die = cu_die;
        }
        else {
            dwarf_dealloc(ctx->dwarf, cu_die, DW_DLA_DIE);
        }
    }

    return ret_die;
}

static Dwarf_Line get_pc_line(Dwarf_Die cu_die, uint64_t pc) {
    Dwarf_Unsigned version_out = 0;
    Dwarf_Small is_single_table = 0;
    Dwarf_Line_Context context_out = 0;
    Dwarf_Error err = 0;
    Dwarf_Line *linebuf = 0;
    Dwarf_Signed linecount = 0;
    Dwarf_Addr lineaddr = 0;

    if (dwarf_srclines_b(cu_die, &version_out, &is_single_table, &context_out, &err) == DW_DLV_OK) {
        if (dwarf_srclines_from_linecontext(context_out, &linebuf, &linecount, &err) == DW_DLV_OK) {
            for (int i = 0; i < linecount; ++i) {
                if (dwarf_lineaddr(linebuf[i], &lineaddr, &err) == DW_DLV_OK) {
                    if (pc == lineaddr) return linebuf[i];
                } 
            }
        }
    }

    return 0;
}

struct src_info get_src_info(dbg_ctx *ctx, uint64_t pc) {
    Dwarf_Unsigned ret_lineno = 0, ret_fileno = 0;
    Dwarf_Signed filecount = 0;
    Dwarf_Error err = 0;
    char *srcfile, *ret_str;
    char **src_files; 

    struct src_info src_info = {};

    Dwarf_Die cu_die = get_cu_die_by_pc(ctx, pc);
    Dwarf_Line pc_line = get_pc_line(cu_die, pc);

    if (cu_die == NULL) {
        printf("Error: Couldn't find CU DIE corresponding to PC\n");
        exit(EXIT_FAILURE);
    }
    if (pc_line == NULL) {
        printf("Error: Couldn't find Dwarf_Line corresponding to PC\n");
        exit(EXIT_FAILURE);
    }
    if (dwarf_lineno(pc_line, &ret_lineno, &err) != DW_DLV_OK) {
        printf("Error in dwarf_lineno\n");
        exit(EXIT_FAILURE);
    }

    src_info.src_line = pc_line;
    src_info.line_no = ret_lineno;

    if (dwarf_srcfiles(cu_die, &src_files, &filecount, &err) == DW_DLV_OK) {
        if (dwarf_line_srcfileno(pc_line, &ret_fileno, &err) == DW_DLV_OK) {
            srcfile = src_files[ret_fileno - 1];
            ret_str = calloc(strlen(srcfile) + 1, sizeof(char));
            memcpy(ret_str, srcfile, strlen(srcfile) + 1);
            src_info.src_file_name = ret_str;
        }
        else {
            printf("Error: Couldn't find src file number corresponding to Dwarf_Line\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Error: Couldn't get src file list from CU DIE\n");
        exit(EXIT_FAILURE);
    }
    
    return src_info;
}

void print_source(struct src_info *src_info) {
    FILE *f;
    if ((f = fopen(src_info->src_file_name, "r")) == NULL) {
        printf("Failure to open %s\n", src_info->src_file_name);
        exit(EXIT_FAILURE);
    }

    char line[256];
    Dwarf_Unsigned count = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (count == src_info->line_no) {
            printf("%llu\t%s", src_info->line_no, line);
            break;
        }
        
        count++;
    }

    fclose(f);
} 

Dwarf_Addr get_func_addr(dbg_ctx *ctx, const char *symbol) {
    Dwarf_Die subprog_die;
    char *subprog_name;
    Dwarf_Attribute *attrs = 0;
    Dwarf_Addr brkpoint_addr = 0;
    Dwarf_Signed attr_count = 0, it = 0;
    Dwarf_Error err = 0;

    subprog_die = get_subprog_die_cu(ctx, symbol, 0);

    if (dwarf_diename(subprog_die, &subprog_name, &err) == DW_DLV_OK) {
        if (strncmp(subprog_name, symbol, strlen(subprog_name)) == 0) {
            if (dwarf_attrlist(subprog_die, &attrs, &attr_count, &err) != DW_DLV_OK) {
                printf("Error in dwarf_attrlist\n");
                exit(EXIT_FAILURE);
            }
            for (it = 0; it < attr_count; ++it) {
                Dwarf_Half attr_code;
                if (dwarf_whatattr(attrs[it], &attr_code, &err) != DW_DLV_OK) {
                    printf("Error in dwarf_whatattr\n");
                    exit(EXIT_FAILURE);
                }
                if (attr_code == DW_AT_low_pc) {
                    dwarf_formaddr(attrs[it], &brkpoint_addr, &err);
                    return brkpoint_addr;
                }
            }
        }
    }

    return 0;
}
