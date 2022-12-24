#ifndef UTILS_H
#define UTILS_H


#define MAX_ARGS 4
#define ARG_LEN 32

void trim_ends(char **s);
int is_prefix(const char *prefix, const char *str);
char** split(char *s);


#endif