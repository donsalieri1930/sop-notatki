#include <stdio.h>
#include <string.h>

int main() {
    char string[10];
    char user_password[] = "1234";
    gets(string);
    char password_guess[5];
    scanf("%4s", password_guess);
    if (strcmp(user_password, password_guess))
        printf("OK");
}