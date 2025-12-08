#define _GNU_SOURCE

#include "strvec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))

static int strvec_resize(strvec *v, int new_capacity) {
    char **new_buf = calloc(new_capacity, sizeof(char*));
    if (new_buf == NULL)
        return -1;
    memcpy(new_buf, v->strings, v->capacity*sizeof(char*));
    free(v->strings);
    v->strings = new_buf;
    v->capacity = new_capacity;
    return 0;
}

int strvec_init(strvec *v) {
    v->strings = calloc(INITIAL_CAPACITY, sizeof(char*));
    if (!v->strings)
        return -1;
    v->max_i = -1;
    v->capacity = INITIAL_CAPACITY;
    return 0;
}

int strvec_set(strvec *v, int i, const char *string) {
    if (v->capacity < i + 1) {
        int new_capacity = MAX(2*v->capacity, i + 1);
        if (strvec_resize(v, new_capacity) == -1)
            return -1;
    }
    char *string_copy = NULL;
    if (string != NULL) {
        string_copy = strdup(string);
        if (string_copy == NULL)
            return -1;
    }
    free(v->strings[i]);
    v->strings[i] = string_copy;
    if (string != NULL)
        v->max_i = MAX(v->max_i, i);
    return 0;
}

int strvec_add(strvec *v, const char *string) {
    int res = strvec_set(v, v->max_i + 1, string);
    return res;        
}

void strvec_free(strvec *v) {
    for (int i = 0; i <= v->max_i; i++)
        free(v->strings[i]);
    free(v->strings);
    v->max_i = -1;
    v->capacity = 0;
}

int strvec_search(strvec *v, const char* string) {
    for (int i=0; i<=v->max_i; i++) {
        if (v->strings[i] != NULL && strcmp(string, v->strings[i]) == 0)
            return i;
    }
    return -1;
}

void strvec_print(strvec *v) {
    printf("vector of capacity: %d\n", v->capacity);
    for (int i=0; i<=v->max_i; i++) {
        printf("\t%d: ", i);
        if (v->strings[i] != NULL)
            printf("%s", v->strings[i]);
        printf("\n");
    }
}
