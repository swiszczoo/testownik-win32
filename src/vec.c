#include <vec.h>

#include <memory.h>

#define INITIAL_CAPACITY            16

void vec_init(vec* instance, size_t element_size)
{
    instance->capacity = INITIAL_CAPACITY;
    instance->length = 0;
    instance->element_size = element_size;
    instance->data = malloc(element_size * INITIAL_CAPACITY);
}

void vec_destroy(vec* instance)
{
    free(instance->data);
}

void vec_clear(vec* instance)
{
    vec_destroy(instance);
    vec_init(instance, instance->element_size);
}

size_t vec_get_size(vec* instance)
{
    return instance->length;
}

static void* vec_get_unchecked(vec* instance, size_t index)
{
    return ((char*)instance->data) + instance->element_size * index;
}

void* vec_get(vec* instance, size_t index)
{
    if (index >= instance->length) {
        return NULL;
    }

    return vec_get_unchecked(instance, index);
}

static void vec_realloc(vec* instance)
{
    size_t new_capacity = instance->capacity * 2;
    instance->data = realloc(instance->data, instance->element_size * new_capacity);
    instance->capacity = new_capacity;
}

void vec_append(vec* instance, void* element)
{
    if (instance->length == instance->capacity) {
        vec_realloc(instance);
    }

    memcpy(vec_get_unchecked(instance, instance->length), element, instance->element_size);
    ++instance->length;
}

