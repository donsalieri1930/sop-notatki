#include <linux/limits.h>
#include <string.h>

#include "strvec.h"

strvec *argparse(strvec*args, char *string) {
    strvec_init(args);
    char *start = string;
    char *end = string;
    char arg[PATH_MAX + 1];
    int quotes = 0;
    for (int i=0; i<strlen(string); i++) {
        // End of argument
        if (string[i] == ' ' && !quotes) {
            strncpy(arg, start, end - start);
            arg[end - start] = 0;
            strvec_add(args, arg);
            start = end + 2;
        }
        if (string[i] == '"' && !quotes) {
            quotes = 1;
            start++;
        }
        if (string[i] == '"' && quotes) {
            strncpy(arg, start, end - 1 - start);
            arg[end - 1 - start] = 0;
            strvec_add(args, arg);
            start = end + 1;
        }
        end++;
    }
    strncpy(arg, start, end - start);
    arg[end - start] = 0;
    strvec_add(args, arg);
    return args;
}

int main() {
    strvec args;
    char string[] = "hello world this is a test";
    argparse(&args, string);
    strvec_print(&args);
    strvec_free(&args);
    return 0;
}