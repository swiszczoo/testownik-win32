#include <encoding.h>

#include <Windows.h>

// Probably won't be used
static size_t encoding_get_grow_size_for_crlf_line_endings(const char* input)
{
    char prev_char = 0;
    size_t grow_size = 0;

    while (*input) {
        if ((*input) == '\n' && prev_char != '\r') {
            ++grow_size;
        }

        prev_char = *input;
        ++input;
    }

    return grow_size;
}

bool encoding_has_utf8_bom(const char* input)
{
    return input[0] == '\xef' && input[1] == '\xbb' && input[2] == '\xbf';
}

bool encoding_is_valid_utf8(const char* input)
{
    static const unsigned char LATER_MIN = 0b10000000;
    static const unsigned char LATER_MAX = 0b10111111;
                 
    static const unsigned char TWO_BYTE_MIN = 0b11000000;
    static const unsigned char TWO_BYTE_MAX = 0b11011111;
                 
    static const unsigned char THREE_BYTE_MIN = 0b11100000;
    static const unsigned char THREE_BYTE_MAX = 0b11101111;
                 
    static const unsigned char FOUR_BYTE_MIN = 0b11110000;
    static const unsigned char FOUR_BYTE_MAX = 0b11110111;

    int counter = 0;

    while (*input) {
        unsigned char c = (unsigned char)*input;

        if (counter > 0) {
            if (c < LATER_MIN || c > LATER_MAX) {
                return false;
            }
            --counter;
        }
        else if (c >= TWO_BYTE_MIN && c <= TWO_BYTE_MAX) {
            counter = 1;
        }
        else if (c >= THREE_BYTE_MIN && c <= THREE_BYTE_MAX) {
            counter = 2;
        }
        else if (c >= FOUR_BYTE_MIN && c <= FOUR_BYTE_MAX) {
            counter = 3;
        }
        else if (c >= 0x80) { // <- is not ASCII
            return false;
        }

        ++input;
    }

    return true;
}

size_t encoding_get_convert_out_size(const char* input)
{
    bool has_bom = encoding_has_utf8_bom(input);
    bool is_valid_utf8 = has_bom;

    if (!has_bom) {
        is_valid_utf8 = encoding_is_valid_utf8(input);
    }

    UINT input_codepage = is_valid_utf8 ? CP_UTF8 : 1250;
    int offset = has_bom ? 3 : 0;

    return MultiByteToWideChar(input_codepage, MB_PRECOMPOSED, input + offset, -1, NULL, 0) * sizeof(WCHAR);
}

bool encoding_convert_to_wide(wchar_t* output, size_t output_size, const char* input)
{
    bool has_bom = encoding_has_utf8_bom(input);
    bool is_valid_utf8 = has_bom;

    if (!has_bom) {
        is_valid_utf8 = encoding_is_valid_utf8(input);
    }

    UINT input_codepage = is_valid_utf8 ? CP_UTF8 : 1250;
    int offset = has_bom ? 3 : 0;

    size_t result = MultiByteToWideChar(
        input_codepage, MB_PRECOMPOSED, input + offset, -1, output, output_size / sizeof(WCHAR));

    return result > 0;
}
