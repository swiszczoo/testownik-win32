#pragma once
#include <Windows.h>

#include <vec.h>

#include <stdbool.h>

typedef struct {
    bool shuffle_questions;
    bool shuffle_answers;
    bool always_multiselect;
    bool auto_accept;
} testownik_config;

typedef struct {
    int current_question; // <- this is the question number
    int current_question_real_idx; // <- this is the current index in order table
    int questions_active_count;
    int correct_count;
    int wrong_count;
} testownik_game_state;

void testownik_init();
bool testownik_try_load_database();
void testownik_get_db_path(LPTSTR out, size_t count);
void testownik_set_db_path(LPCTSTR new_db_path);
size_t testownik_get_question_count();
const testownik_config* testownik_get_configuration();
void testownik_start_game(testownik_config* game_config);
