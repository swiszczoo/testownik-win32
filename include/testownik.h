#pragma once
#include <Windows.h>

#include <stdbool.h>

typedef struct {
    TCHAR db_path[MAX_PATH+16];
} testownik;

void testownik_init();
bool testownik_try_load_database();
