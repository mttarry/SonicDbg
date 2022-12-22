#ifndef UTILS_H
#define UTILS_H


#define MAX_ARGS 4
#define ARG_LEN 32

void trim_ends(char **s);
int is_prefix(char *prefix, char *str);
char** split(char *s);


#endif