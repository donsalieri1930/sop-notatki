/*
    Dynamic array of strings. Allows to set value, including NULL, 
    at any non-negative index. Shrinking is not supported. Array
    makes its own copies of the strings.
*/

#ifndef STRVEC_H
#define STRVEC_H

#define INITIAL_CAPACITY 2

typedef struct {
    char **strings;
    int max_i;
    int capacity;
} strvec;

int strvec_init(strvec *v);
int strvec_set(strvec *v, int index, const char *string);
void strvec_free(strvec *v);
int strvec_search(strvec *v, const char* string);
void strvec_print(strvec *v);
int strvec_add(strvec *v, const char *string);

#endif
