#pragma once
#include <Windows.h>

#include <vec.h>

#include <stdbool.h>

#define TESTOWNIK_MAX_ANSWERS_PER_QUESTION        32

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

typedef enum {
    NO_QUESTION,
    SINGLE_CHOICE,
    MULTI_CHOICE
} testownik_question_type;

typedef enum {
    NO_STATE,
    SELECTED_AND_OK,
    SELECTED_AND_WRONG,
    NOT_SELECTED_AND_OK,
    SHOULD_BE_SELECTED,
} testownik_answer_state;

typedef enum {
    QUESTION,
    CORRECT,
    WRONG,
    PARTIALLY_CORRECT
} testownik_question_result;

typedef struct {
    int question_number;
    LPCTSTR question_text;
    WCHAR question_image_path[1024];
    testownik_question_type question_type;
    int answer_count;
    int answer_id[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    WCHAR answer_symbol[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    LPCTSTR answer_text[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    testownik_answer_state answer_state[TESTOWNIK_MAX_ANSWERS_PER_QUESTION];
    testownik_question_result result;
} testownik_question_info;

void testownik_init(void);
bool testownik_try_load_database(void);
void testownik_get_db_path(LPTSTR out, size_t count);
void testownik_set_db_path(LPCTSTR new_db_path);
size_t testownik_get_question_count(void);
const testownik_config* testownik_get_configuration(void);
void testownik_start_game(testownik_config* game_config);
void testownik_get_game_state(testownik_game_state* out);

bool testownik_is_game_in_progress(void);
bool testownik_move_to_next_question(void);
bool testownik_get_question_info(testownik_question_info* info);
int testownik_get_game_seconds_elapsed(bool after_game);
bool testownik_check_answer(bool checked[TESTOWNIK_MAX_ANSWERS_PER_QUESTION]);
void testownik_restart_all_questions(void);
void testownik_restart_wrong_answers(void);
