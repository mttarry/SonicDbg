#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <libelf.h>

#define MAX_ARGS 4
#define ARG_LEN 32

#define GRN   "\x1B[32m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

void trim_ends(char **s);
int is_prefix(const char *prefix, const char *str);
char** split(char *s);
uint64_t convert_val_radix(const char *val);
void free_args(char **args);
bool is_symbol(const char *loc);
bool bin_is_pie(Elf *elf);
char *loc_last_dir(char *str);

#endif