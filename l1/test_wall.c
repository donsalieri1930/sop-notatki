#include <stdio.h>

int f() { } // brak return

int add(int a, int b) {
    int result;
    if (a == b)             // przypisanie w if
        printf("eq\n");
    return result;         // użycie niezainicjalizowanej
}

char *get_string() {
    char *string = "Hello world";
    return string;
}

int main(void) {
    int unused = 5;        // nieużyta
    return add(1, 2);
}
