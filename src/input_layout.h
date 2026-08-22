#ifndef SHIORI_INPUT_LAYOUT_H
#define SHIORI_INPUT_LAYOUT_H

#include <stddef.h>

struct input_layout {
    size_t prompt_start;
    size_t input_start;
    size_t input_end;
    size_t cursor_column;
};

struct input_layout calculate_input_layout(
    const char *prompt,
    const char *input,
    size_t cursor,
    size_t terminal_width,
    size_t preferred_input_start
);

#endif
