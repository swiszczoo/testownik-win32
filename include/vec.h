#pragma once
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    size_t length;
    size_t capacity;
    size_t element_size;
    void* data;
} vec;

void vec_init(vec* instance, size_t element_size);
void vec_destroy(vec* instance);
void vec_clear(vec* instance);
size_t vec_get_size(vec* instance);
void* vec_get(vec* instance, size_t index);
void vec_append(vec* instance, void* element);

