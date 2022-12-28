#include <stdio.h>
#include <signal.h>

const char *password = "password";

void print(const char *str) {
    printf("%s\n", str);
}

int main(void) {
    print(password);
    print("Hello World!\n");
}