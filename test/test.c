#include <stdio.h>
#include <signal.h>

const char *password = "password";

extern int add(int a, int b);

void print(const char *str) {
    printf("%s\n", str);
}

int main(void) {
    print(password);
    print("Hello World!");
    printf("%d\n", add(1, 2));
}