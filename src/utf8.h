#ifndef SHIORI_UTF8_H
#define SHIORI_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

bool utf8_decode(const char *text, size_t length, size_t offset, unsigned int *codepoint, size_t *next_offset);
size_t utf8_next_boundary(const char *text, size_t length, size_t offset);
size_t utf8_previous_boundary(const char *text, size_t offset);
size_t utf8_codepoint_cell_width(unsigned int codepoint);
size_t utf8_range_cell_width(const char *text, size_t start, size_t end);

char **convert_wargv_to_utf8(int argc, wchar_t *wargv[]);
void free_utf8_argv(int argc, char *argv[]);
#endif
