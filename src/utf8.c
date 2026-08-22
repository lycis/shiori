#include "utf8.h"

#include <stdlib.h>
#include <wchar.h>

bool utf8_decode(const char *text, size_t length, size_t offset, unsigned int *codepoint, size_t *next_offset) {
    if(text == NULL || codepoint == NULL || next_offset == NULL || offset >= length) {
        return false;
    }

    const unsigned char *bytes = (const unsigned char *)text;
    unsigned char first = bytes[offset];
    size_t count = 0;
    unsigned int value = 0;

    if(first <= 0x7F) {
        count = 1;
        value = first;
    } else if(first >= 0xC2 && first <= 0xDF) {
        count = 2;
        value = first & 0x1F;
    } else if(first >= 0xE0 && first <= 0xEF) {
        count = 3;
        value = first & 0x0F;
    } else if(first >= 0xF0 && first <= 0xF4) {
        count = 4;
        value = first & 0x07;
    } else {
        return false;
    }

    if(offset + count > length) {
        return false;
    }

    for(size_t i = 1; i < count; ++i) {
        unsigned char continuation = bytes[offset + i];
        if((continuation & 0xC0) != 0x80) {
            return false;
        }
        value = (value << 6) | (continuation & 0x3F);
    }

    if((count == 3 && value < 0x800) || (count == 4 && value < 0x10000) || (value >= 0xD800 && value <= 0xDFFF) ||
       value > 0x10FFFF) {
        return false;
    }

    *codepoint = value;
    *next_offset = offset + count;
    return true;
}

size_t utf8_next_boundary(const char *text, size_t length, size_t offset) {
    if(text == NULL || offset >= length) {
        return length;
    }

    unsigned int codepoint;
    size_t next;
    if(utf8_decode(text, length, offset, &codepoint, &next)) {
        return next;
    }

    return offset + 1;
}

size_t utf8_previous_boundary(const char *text, size_t offset) {
    if(text == NULL || offset == 0) {
        return 0;
    }

    size_t previous = offset - 1;
    while(previous > 0 && ((unsigned char)text[previous] & 0xC0) == 0x80) {
        previous--;
    }
    return previous;
}

static bool codepoint_in_ranges(unsigned int codepoint, const unsigned int ranges[][2], size_t count) {
    for(size_t i = 0; i < count; ++i) {
        if(codepoint >= ranges[i][0] && codepoint <= ranges[i][1]) {
            return true;
        }
    }
    return false;
}

size_t utf8_codepoint_cell_width(unsigned int codepoint) {
    static const unsigned int combining[][2] = {
        {0x0300, 0x036F},
        {0x1AB0, 0x1AFF},
        {0x1DC0, 0x1DFF},
        {0x20D0, 0x20FF},
        {0xFE00, 0xFE0F},
        {0xFE20, 0xFE2F},
        {0xE0100, 0xE01EF},
    };
    static const unsigned int wide[][2] = {
        {0x1100, 0x115F},
        {0x2329, 0x232A},
        {0x2E80, 0x303E},
        {0x3040, 0xA4CF},
        {0xAC00, 0xD7A3},
        {0xF900, 0xFAFF},
        {0xFE10, 0xFE19},
        {0xFE30, 0xFE6F},
        {0xFF00, 0xFF60},
        {0xFFE0, 0xFFE6},
        {0x1F300, 0x1FAFF},
        {0x20000, 0x3FFFD},
    };

    if(codepoint == 0 || codepoint < 0x20 || (codepoint >= 0x7F && codepoint < 0xA0) ||
       codepoint_in_ranges(codepoint, combining, sizeof(combining) / sizeof(combining[0]))) {
        return 0;
    }
    if(codepoint_in_ranges(codepoint, wide, sizeof(wide) / sizeof(wide[0]))) {
        return 2;
    }
    return 1;
}

size_t utf8_range_cell_width(const char *text, size_t start, size_t end) {
    if(text == NULL || start >= end) {
        return 0;
    }

    size_t width = 0;
    size_t offset = start;
    while(offset < end) {
        unsigned int codepoint;
        size_t next;
        if(!utf8_decode(text, end, offset, &codepoint, &next)) {
            width++;
            offset++;
            continue;
        }
        width += utf8_codepoint_cell_width(codepoint);
        offset = next;
    }
    return width;
}

enum utf16_decode_result
utf16_decode_code_unit(struct utf16_decoder *decoder, unsigned int code_unit, unsigned int *codepoint) {
    if(decoder == NULL || codepoint == NULL || code_unit > 0xFFFF) {
        return UTF16_DECODE_INVALID;
    }

    if(code_unit >= 0xD800 && code_unit <= 0xDBFF) {
        decoder->high_surrogate = code_unit;
        return UTF16_DECODE_PENDING;
    }

    if(code_unit >= 0xDC00 && code_unit <= 0xDFFF) {
        if(decoder->high_surrogate == 0) {
            return UTF16_DECODE_INVALID;
        }

        *codepoint = 0x10000 + ((decoder->high_surrogate - 0xD800) << 10) + (code_unit - 0xDC00);
        decoder->high_surrogate = 0;
        return UTF16_DECODE_CODEPOINT;
    }

    decoder->high_surrogate = 0;
    *codepoint = code_unit;
    return UTF16_DECODE_CODEPOINT;
}

#ifdef _WIN32
#include <windows.h>

char **convert_wargv_to_utf8(int argc, wchar_t *wargv[]) {
    if(argc <= 0 || wargv == NULL) {
        return NULL;
    }

    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    if(argv == NULL) {
        return NULL;
    }

    for(int i = 0; i < argc; ++i) {
        int required = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);

        if(required <= 0) {
            free_utf8_argv(argc, argv);
            return NULL;
        }

        argv[i] = malloc((size_t)required);
        if(argv[i] == NULL) {
            free_utf8_argv(argc, argv);
            return NULL;
        }

        int written = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], required, NULL, NULL);

        if(written <= 0) {
            free_utf8_argv(argc, argv);
            return NULL;
        }
    }

    argv[argc] = NULL;

    return argv;
}

void free_utf8_argv(int argc, char *argv[]) {
    if(argv == NULL) {
        return;
    }

    for(int i = 0; i < argc; ++i) {
        free(argv[i]);
    }

    free(argv);
}

#endif
