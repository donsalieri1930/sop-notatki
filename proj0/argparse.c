#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "strvec.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

int parse_args(const char *string, strvec *args) {
    size_t n = strlen(string);
    char *arg = malloc((n + 1)*sizeof(char));
    if (arg == NULL)
        return -1;
    size_t arg_len = 0;
    int ignore_space = 0;
    int status = 0;
    for (size_t i = 0; i < n; i++) {
        switch (string[i]) {
            case ' ':
                if (ignore_space)
                    arg[arg_len++] = string[i];

                else if (arg_len > 0) {
                    arg[arg_len] = 0;
                    if (strvec_add(args, arg) == -1) {
                        status = -1;
                        goto cleanup;
                    }
                    arg_len = 0;
                }
                break;

            case '"':
                ignore_space = !ignore_space;
                break;

            default:
                arg[arg_len++] = string[i];
                break;
        }
    }
    if (arg_len > 0) {
        arg[arg_len] = 0;
        status = strvec_add(args, arg);
    }

cleanup:
    free(arg);
    return status;
}