#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#include "utils.h"


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

char** split(char *s) {
    // four arguments max, 32 characters each
    char **split = calloc(MAX_ARGS, sizeof(char*));
    int str_idx = 0;
    int char_idx = 0;

    split[0] = calloc(ARG_LEN, sizeof(char));

    for (size_t i = 0; i < strlen(s); ) {
        if (isspace(s[i])) {
            // consume whitespace between arguments
            while (isspace(s[i])) ++i;
            split[++str_idx] = calloc(ARG_LEN, sizeof(char));
            char_idx = 0;
        }
        else {
            split[str_idx][char_idx] = s[i++];
            char_idx++;
        }
    }

    return split;
}

int is_prefix(const char *prefix, const char *str) {
    if (strlen(prefix) > strlen(str)) 
        return 0;

    for (size_t i = 0; i < strlen(prefix); ++i) 
        if (prefix[i] != str[i]) 
            return 0;
    
    return 1;
}

uint64_t convert_val_radix(const char *val) {
    if (!val) 
        return 0;

    if (val[0] == '0' && val[1] == 'x') 
        return strtoul(val, NULL, 16);
        
    for (size_t i = 0; i < strlen(val); ++i) {
        if (isxdigit(val[i])) {
            return strtoul(val, NULL, 16);
        }
    }

    return strtoul(val, NULL, 10);
}