#pragma once
#include <Windows.h>

void image_decoder_init(void);
void image_decoder_destroy(void);
HBITMAP image_decoder_file_to_hbitmap(LPCWSTR path);
