#include <stdio.h>

#include "input_layout.h"
#include "utf8.h"

#define CHECK(condition, message)                                                                                       \
    do {                                                                                                                \
        if(!(condition)) {                                                                                              \
            fprintf(stderr, "%s\n", message);                                                                         \
            return 1;                                                                                                   \
        }                                                                                                               \
    } while(0)

int main(void) {
    CHECK(utf8_range_cell_width("hello", 0, 5) == 5, "ASCII width is incorrect");
    CHECK(utf8_range_cell_width("caf\xC3\xA9", 0, 5) == 4, "multibyte narrow width is incorrect");
    CHECK(utf8_range_cell_width("\xE7\x8C\xAB", 0, 3) == 2, "CJK width is incorrect");
    CHECK(utf8_range_cell_width("e\xCC\x81", 0, 3) == 1, "combining-mark width is incorrect");
    CHECK(utf8_range_cell_width("\xF0\x9F\x98\x80", 0, 4) == 2, "supplementary-plane width is incorrect");

    struct input_layout layout = calculate_input_layout("> ", "abcdefghij", 10, 8, 0);
    CHECK(layout.input_start == 5 && layout.input_end == 10, "long ASCII input did not scroll to the cursor");
    CHECK(layout.cursor_column == 7, "ASCII cursor column is incorrect");

    layout = calculate_input_layout("> ", "abcdefghij", 6, 8, 5);
    CHECK(layout.input_start == 5 && layout.input_end == 10, "visible cursor unexpectedly moved the viewport");

    layout = calculate_input_layout("> ", "abcdefghij", 0, 8, 5);
    CHECK(layout.input_start == 0 && layout.input_end == 5, "Home did not reveal the start of input");

    layout = calculate_input_layout("> ", "ab\xE7\x8C\xABz", 5, 6, 0);
    CHECK(layout.input_start == 1 && layout.cursor_column == 5, "wide character scrolling is incorrect");

    layout = calculate_input_layout("long> ", "x", 1, 4, 0);
    CHECK(layout.prompt_start > 0 && layout.cursor_column == 3, "narrow terminal did not truncate the prompt");

    return 0;
}
