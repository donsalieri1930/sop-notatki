#include <stdlib.h>
#include <string.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define INITIAL_CAPACITY 16

typedef struct {
    char **strings;
    int size;
    int capacity;
} strvec;

int strvec_init(strvec *v) {
    v->strings = calloc(INITIAL_CAPACITY, sizeof(char*));
    if (v->strings == NULL)
        return -1;
    v->size = 0;
    v->capacity = INITIAL_CAPACITY;
    return 0;
}

int strvec_resize(strvec *v, int new_capacity) {
    char **new_buf = calloc(new_capacity, sizeof(char*));
    if (new_buf == NULL)
        return -1;
    memcpy(new_buf, v->strings, v->size*sizeof(char*));
    free(v->strings);
    v->strings = new_buf;
    v->capacity = new_capacity;
    return 0;
}

int strvec_set(strvec *v, int index, char *string) {
    if (v->capacity < index + 1) {
        int new_capacity = MAX(2*v->capacity, index + 1);
        if (strvec_resize(v, new_capacity) == -1)
            return -1;
    }
    v->strings[index] = string;
    v->size = MAX(v->size, index + 1);
    return 0;
}

int strvec_add(strvec *v, char *string) {
    return strvec_set(v, v->size, string);
}

void strvec_free(strvec *v) {
    /* Use only if all elements are allocated on the heap. */
    for (int i=0; i<v->size; i++) 
        free(v->strings[i]);
    free(v->strings);
    v->strings = NULL;
    v->size = 0;
    v->capacity = 0;
}