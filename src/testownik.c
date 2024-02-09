#include <testownik.h>

#include <encoding.h>
#include <random.h>
#include <vec.h>

#include <PathCch.h>
#include <Shlwapi.h>

#include <memory.h>
#include <string.h>
#include <wchar.h>

#define MAX_QUESTION_SIZE               (1 << 20)
#define MAX_ANSWERS_PER_QUESTION        TESTOWNIK_MAX_ANSWERS_PER_QUESTION

typedef struct {
    testownik_config config;
    TCHAR db_path[MAX_PATH + 16];
    vec questions;

    int* question_order;
    bool* questions_correct;
    bool* questions_active;

    bool is_game_in_progress;
    testownik_game_state game;
    testownik_question_info current_question;
} testownik;

typedef struct {
    WCHAR* text;
    WCHAR symbol;
    bool is_correct;
} testownik_answer;

typedef struct {
    int question_number;
    WCHAR* question_text;
    WCHAR* image_path;
    bool no_shuffle;
    int correct_answer_count;
    int total_answer_count;
    testownik_answer answers[MAX_ANSWERS_PER_QUESTION];
} testownik_question;

static testownik APP = { 0 };

void testownik_init(void)
{
    ZeroMemory(&APP, sizeof(APP));

    GetModuleFileName(GetModuleHandle(NULL), APP.db_path, MAX_PATH + 16);
    PathCchRemoveFileSpec(APP.db_path, MAX_PATH + 16);
    PathCchCombine(APP.db_path, MAX_PATH + 16, APP.db_path, L"qdb");

    vec_init(&APP.questions, sizeof(testownik_question));
}

static void testownik_free_question(testownik_question* q)
{
    if (q->question_text) {
        free(q->question_text);
        q->question_text = NULL;
    }
    if (q->image_path) {
        free(q->image_path);
        q->image_path = NULL;
    }

    for (int i = 0; i < MAX_ANSWERS_PER_QUESTION; ++i) {
        if (q->answers[i].text) {
            free(q->answers[i].text);
            q->answers[i].text = NULL;
        }
    }
}

static void testownik_parse_option(LPCWSTR text, testownik_question* out)
{
    if (wcsncmp(L";img=", text, 5) == 0) {
        out->image_path = malloc(sizeof(WCHAR) * (wcslen(text) - 4));

        if (out->image_path) {
            wcscpy(out->image_path, text + 5);
        }
    }
    else if (wcscmp(L";noshuffle", text) == 0) {
        out->no_shuffle = true;
    }
}

static int testownik_next_line(LPCWSTR input)
{
    int offset;

    for (offset = 0; input[offset]; ++offset) {
        if (input[offset] == '\n') {
            return offset + 1;
        }
    }

    return offset;
}

static int testownik_strip_whitespace_end(LPCWSTR text, int count)
{
    while (iswspace(text[count - 1]) && count > 0) {
        --count;
    }

    return count;
}

static void testownik_load_question_text(LPCWSTR text, int count, testownik_question* out)
{
    count = testownik_strip_whitespace_end(text, count);

    out->question_number = wcstol(text, NULL, 10);
    out->question_text = malloc(sizeof(WCHAR) * (count + 1));
    *out->question_text = L'\0';

    if (out->question_text) {
        for (int i = 0; text[i] && text[i + 1]; ++i) {
            if (text[i] == L'.' && text[i + 1] == L'\t') {
                wcsncat(out->question_text, text + i + 2, count - i - 2);
            }
        }
    }
}

static void testownik_load_answer(LPCWSTR text, int count, int answer_id, testownik_question* out)
{
    // Skip any whitespace characters
    while (iswspace(*text)) {
        ++text;
        --count;
    }

    count = testownik_strip_whitespace_end(text, count);

    out->answers[answer_id].text = malloc(sizeof(WCHAR) * (count + 1));
    *out->answers[answer_id].text = L'\0';
    wcsncat(out->answers[answer_id].text, text, count);

    if (count > 1) {
        out->answers[answer_id].symbol = text[1];
    }
    else {
        out->answers[answer_id].symbol = L'\0';
    }
}

static bool testownik_parse_question(LPWSTR content, testownik_question* out)
{
    if (content[0] != L'Q' || content[1] != L'Q') {
        return false;
    }

    int current_offset = 2;
    int num_answers = 0;
    int correct_answer_count = 0;
    bool correct_answers[MAX_ANSWERS_PER_QUESTION];
    ZeroMemory(correct_answers, sizeof(correct_answers));

    // Read expected answer count and correct answers
    while (true) {
        WCHAR wc = content[current_offset];
        if (wc == L'0') {
            ++num_answers;
        }
        else if (wc == L'1') {
            correct_answers[num_answers] = true;
            ++correct_answer_count;
            ++num_answers;
        }
        else if (!wc) {
            return false;
        }
        else if (wc == L'\r') {
            ++current_offset;
            continue;
        }
        else if (wc == ';' || wc == '\n') {
            break;
        }

        ++current_offset;
    }

    if (correct_answers == 0 || num_answers > MAX_ANSWERS_PER_QUESTION) {
        return false;
    }

    // Parse additional options if any
    while (true) {
        WCHAR wc = content[current_offset];
        int previous_offset = 0;
        WCHAR previous_char = '\0';
        if (wc != ';') {
            break; // no more options
        }

        // Find an enclosing options delimiter (either ;, \r or \n) and replace
        // temporarily with \0
        for (int offset = current_offset + 1; content[offset]; ++offset)
        {
            WCHAR tmp = content[offset];
            if (tmp == L';' || tmp == '\r' || tmp == '\n') {
                previous_offset = offset;
                previous_char = tmp;
                content[offset] = '\0';
                break;
            }
        }

        if (previous_offset > 0) {
            testownik_parse_option(content + current_offset, out);
            content[previous_offset] = previous_char;

            current_offset = previous_offset;
        }
        else {
            break;
        }
    }

    // Skip all whitespace
    while (iswspace(content[current_offset])) {
        ++current_offset;
    }

    // Load question text
    int question_length = testownik_next_line(content + current_offset);
    testownik_load_question_text(content + current_offset, question_length, out);
    current_offset += question_length;

    // Load answers
    int num_answers_lines = 0;
    while (content[current_offset]) {
        int answer_length = testownik_next_line(content + current_offset);
        testownik_load_answer(content + current_offset,
            answer_length, num_answers_lines, out);

        ++num_answers_lines;
        current_offset += answer_length;
    }

    if (num_answers_lines != num_answers) {
        return false;
    }

    // Finish everything
    out->correct_answer_count = correct_answer_count;
    out->total_answer_count = num_answers;
    for (int i = 0; i < out->total_answer_count; ++i) {
        out->answers[i].is_correct = correct_answers[i];
    }

    return true;
}

static bool testownik_load_question(LPCWSTR path, testownik_question* out)
{
#ifdef _DEBUG
    OutputDebugString(L"Loading: ");
    OutputDebugString(path);
    OutputDebugString(L"\r\n");
#endif

    HANDLE file = INVALID_HANDLE_VALUE;
    char* content_buffer = NULL;
    WCHAR* wide_content = NULL;

    file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        goto error;
    }

    LARGE_INTEGER file_size;
    GetFileSizeEx(file, &file_size);
    if (file_size.QuadPart > MAX_QUESTION_SIZE) {
        goto error;
    }

    content_buffer = malloc(file_size.QuadPart + 1);
    if (!content_buffer) {
        goto error;
    }

    content_buffer[file_size.QuadPart] = '\0';

    DWORD bytes_read;
    DWORD total_size = file_size.LowPart;

    if (!ReadFile(file, content_buffer, total_size, &bytes_read, NULL)) {
        goto error;
    }

    if (bytes_read != total_size) {
        goto error;
    }

    size_t wide_content_size = encoding_get_convert_out_size(content_buffer);
    wide_content = malloc(wide_content_size);
    if (!wide_content) {
        goto error;
    }

    if (!encoding_convert_to_wide(wide_content, wide_content_size, content_buffer)) {
        goto error;
    }

    if (!testownik_parse_question(wide_content, out)) {
        goto error;
    }

    free(wide_content);
    free(content_buffer);
    CloseHandle(file);

    return true;

error:
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }
    if (content_buffer) {
        free(content_buffer);
    }
    if (wide_content) {
        free(wide_content);
    }

    return false;
}

static void testownik_clear_database(void)
{
    size_t count = vec_get_size(&APP.questions);
    while (count--) {
        testownik_free_question((testownik_question*)vec_get(&APP.questions, count));
    }

    vec_clear(&APP.questions);

    if (APP.question_order) {
        free(APP.question_order);
        APP.question_order = NULL;
    }

    if (APP.questions_correct) {
        free(APP.questions_correct);
        APP.questions_correct = NULL;
    }

    if (APP.questions_active) {
        free(APP.questions_active);
        APP.questions_active = NULL;
    }
}

static int testownik_sort_questions_comparator(const void* elem1, const void* elem2)
{
    const testownik_question* left = (const testownik_question*)elem1;
    const testownik_question* right = (const testownik_question*)elem2;

    return left->question_number - right->question_number;
}

static void testownik_sort_questions(void)
{
    if (vec_get_size(&APP.questions) == 0) {
        return;
    }

    qsort(vec_get(&APP.questions, 0), vec_get_size(&APP.questions),
        sizeof(testownik_question), testownik_sort_questions_comparator);
}

static void testownik_prepare_initial_state(void)
{
    int total_question_count = vec_get_size(&APP.questions);

    APP.question_order = malloc(sizeof(int) * total_question_count);
    APP.questions_correct = malloc(sizeof(bool) * total_question_count);
    APP.questions_active = malloc(sizeof(bool) * total_question_count);

    for (int i = 0; i < total_question_count; ++i) {
        APP.question_order[i] = i;
        APP.questions_correct[i] = false;
        APP.questions_active[i] = true;
    }

    APP.game.current_question = 0;
    APP.game.current_question_real_idx = 0;
    APP.game.questions_active_count = total_question_count;
    APP.game.correct_count = 0;
    APP.game.wrong_count = 0;
}

bool testownik_try_load_database(void)
{
    if (!PathIsDirectory(APP.db_path)) {
        return false;
    }

    testownik_clear_database();

    WIN32_FIND_DATA ffd;
    HANDLE search_handle;
    TCHAR filter[MAX_PATH + 16];

    PathCchCombine(filter, MAX_PATH + 16, APP.db_path, L"*.txt");

    search_handle = FindFirstFile(filter, &ffd);
    if (search_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        if (!(ffd.dwFileAttributes & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE))) {
            continue;
        }

        TCHAR question_path[MAX_PATH + 16];
        testownik_question q;
        ZeroMemory(&q, sizeof(testownik_question));

        PathCchCombine(question_path, MAX_PATH + 16, APP.db_path, ffd.cFileName);

        if (testownik_load_question(question_path, &q)) {
            vec_append(&APP.questions, &q);
        }
        else {
            testownik_free_question(&q);
        }

    } while (FindNextFile(search_handle, &ffd));

    DWORD findError = GetLastError();
    FindClose(search_handle);

    testownik_sort_questions();
    testownik_prepare_initial_state();

    return findError == ERROR_NO_MORE_FILES && vec_get_size(&APP.questions) > 0;
}

void testownik_get_db_path(LPTSTR out, size_t count)
{
    wcscpy(out, APP.db_path);
}

void testownik_set_db_path(LPCTSTR new_db_path)
{
    wcscpy(APP.db_path, new_db_path);
}

size_t testownik_get_question_count(void)
{
    return vec_get_size(&APP.questions);
}

const testownik_config* testownik_get_configuration(void)
{
    return &APP.config;
}

void testownik_start_game(testownik_config* game_config)
{
    if (APP.is_game_in_progress) {
        return;
    }

    memcpy(&APP.config, game_config, sizeof(testownik_config));

    int total_questions_count = vec_get_size(&APP.questions);
    int active_questions = 0;
    for (int i = 0; i < total_questions_count; ++i) {
        APP.questions_correct[i] = false;
        if (APP.questions_active[i]) {
            ++active_questions;
        }
    }

    APP.game.current_question = 0;
    APP.game.current_question_real_idx = -1;
    APP.game.questions_active_count = active_questions;
    APP.game.correct_count = 0;
    APP.game.wrong_count = 0;

    if (game_config->shuffle_questions) {
        random_shuffle_int_array(APP.question_order, total_questions_count);
    }

    APP.is_game_in_progress = true;
}

void testownik_get_game_state(testownik_game_state* out)
{
    memcpy(out, &APP.game, sizeof(testownik_game_state));
}

static void testownik_update_current_question_state()
{
    int question_id = APP.question_order[APP.game.current_question_real_idx];
    testownik_question* question = (testownik_question*)vec_get(&APP.questions, question_id);

    memset(&APP.current_question, 0, sizeof(testownik_question_info));

    APP.current_question.question_number = question->question_number;
    APP.current_question.question_text = question->question_text;
    APP.current_question.question_type = MULTI_CHOICE;

    if (question->correct_answer_count == 1 && !APP.config.always_multiselect) {
        APP.current_question.question_type = SINGLE_CHOICE;
    }

    APP.current_question.answer_count = question->total_answer_count;
    for (int i = 0; i < question->total_answer_count; ++i) {
        APP.current_question.answer_id[i] = i;
    }

    if (APP.config.shuffle_answers) {
        random_shuffle_int_array(
            APP.current_question.answer_id, question->total_answer_count);
    }

    for (int i = 0; i < question->total_answer_count; ++i) {
        int answer_id = APP.current_question.answer_id[i];
        APP.current_question.answer_symbol[i] = question->answers[answer_id].symbol;
        APP.current_question.answer_text[i] = question->answers[answer_id].text;
    }
}

bool testownik_move_to_next_question(void)
{
    if (!APP.is_game_in_progress) {
        return false;
    }

    ++APP.game.current_question_real_idx;
    ++APP.game.current_question;

    int total_questions = vec_get_size(&APP.questions);

    while (APP.game.current_question_real_idx < total_questions) {
        int question_id = APP.question_order[APP.game.current_question_real_idx];
        if (APP.questions_active[question_id]) {
            testownik_update_current_question_state();
            return true;
        }
    }

    // There are no more questions
    APP.is_game_in_progress = false;
    return false;
}

bool testownik_get_question_info(testownik_question_info* info)
{
    if (APP.is_game_in_progress) {
        memcpy(info, &APP.current_question, sizeof(testownik_question_info));
        return true;
    }

    return false;
}

