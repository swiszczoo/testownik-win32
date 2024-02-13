#pragma once
#include <Windows.h>

typedef enum {
    TM_START_GAME = WM_USER + 10,
    TM_END_GAME,
} user_message;
