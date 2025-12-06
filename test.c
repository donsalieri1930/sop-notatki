#include <stdio.h>
#include <string.h>


int main() {
    char a[100] = "Hello"; // H e l l o \0 | w o r l d \0
    char b[] = "world";
    strcat(a, b);
    printf("%s\n", a);
}