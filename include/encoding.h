#pragma once
#include <stdbool.h>
#include <stddef.h>

bool encoding_has_utf8_bom(const char* input);
bool encoding_is_valid_utf8(const char* input);
size_t encoding_get_convert_out_size(const char* input);
bool encoding_convert_to_wide(wchar_t* output, size_t output_size, const char* input);
