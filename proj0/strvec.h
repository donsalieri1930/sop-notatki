/*
    Dynamic array of char*. Allows to set value at any non-negative 
    index. Shrinking is not supported. Strings must be allocated on
    the heap, and should be freed by strvec_free.
*/

#ifndef STRVEC_H
#define STRVEC_H

#define INITIAL_CAPACITY 32

typedef struct {
    char **strings;
    int max_i;
    int capacity;
} strvec;

int strvec_init(strvec *v);
int strvec_set(strvec *v, int index, char *string);
void strvec_free(strvec *v);
int strvec_search(strvec *v, const char* string);
void strvec_print(strvec *v);

#endif
