#pragma once
#include <stdint.h>

void random_init();
uint64_t random_next_number();
uint64_t random_next_range(uint64_t range);
void random_shuffle_int_array(int* ptr, size_t count);
