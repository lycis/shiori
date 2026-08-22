#include "input_layout.h"

#include <string.h>

#include "utf8.h"

static size_t suffix_start_for_width(const char *text, size_t length, size_t maximum_width) {
    size_t start = length;
    size_t width = 0;

    while(start > 0) {
        size_t previous = utf8_previous_boundary(text, start);
        size_t character_width = utf8_range_cell_width(text, previous, start);
        if(width + character_width > maximum_width) {
            break;
        }
        start = previous;
        width += character_width;
    }
    return start;
}

struct input_layout calculate_input_layout(
    const char *prompt,
    const char *input,
    size_t cursor,
    size_t terminal_width,
    size_t preferred_input_start
) {
    struct input_layout layout = {0};
    if(prompt == NULL || input == NULL || terminal_width == 0) {
        return layout;
    }

    size_t prompt_length = strlen(prompt);
    size_t input_length = strlen(input);
    if(cursor > input_length) {
        cursor = input_length;
    }

    size_t maximum_prompt_width = terminal_width > 1 ? terminal_width - 1 : 0;
    layout.prompt_start = suffix_start_for_width(prompt, prompt_length, maximum_prompt_width);
    size_t prompt_width = utf8_range_cell_width(prompt, layout.prompt_start, prompt_length);
    size_t input_width = terminal_width > prompt_width ? terminal_width - prompt_width - 1 : 0;

    layout.input_start = preferred_input_start;
    if(layout.input_start > cursor || layout.input_start > input_length) {
        layout.input_start = cursor;
    }

    while(layout.input_start < cursor && utf8_range_cell_width(input, layout.input_start, cursor) > input_width) {
        layout.input_start = utf8_next_boundary(input, input_length, layout.input_start);
    }

    layout.input_end = layout.input_start;
    size_t visible_width = 0;
    while(layout.input_end < input_length) {
        size_t next = utf8_next_boundary(input, input_length, layout.input_end);
        size_t character_width = utf8_range_cell_width(input, layout.input_end, next);
        if(visible_width + character_width > input_width) {
            break;
        }
        visible_width += character_width;
        layout.input_end = next;
    }

    layout.cursor_column = prompt_width + utf8_range_cell_width(input, layout.input_start, cursor);
    return layout;
}
