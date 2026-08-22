#include <stdio.h>
#include <string.h>

#include "color.h"

int main(void) {
    if(!color_is_enabled()) {
        fprintf(stderr, "color should be enabled by default\n");
        return 1;
    }

    if(strcmp(color_style_sequence(COLOR_STYLE_SUCCESS), COLOR_SUCCESS) != 0) {
        fprintf(stderr, "enabled success style does not match existing output\n");
        return 1;
    }

    color_set_enabled(false);

    for(int style = COLOR_STYLE_RESET; style <= COLOR_STYLE_DIVIDER; ++style) {
        if(strcmp(color_style_sequence((enum color_style)style), "") != 0) {
            fprintf(stderr, "disabled style %d should be empty\n", style);
            return 1;
        }
    }

    if(color_is_enabled()) {
        fprintf(stderr, "color should report disabled\n");
        return 1;
    }

    color_set_enabled(true);

    if(!color_is_enabled()) {
        fprintf(stderr, "color could not be re-enabled\n");
        return 1;
    }

    return 0;
}
