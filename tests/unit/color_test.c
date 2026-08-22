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

    if(color_is_enabled() || strcmp(color_style_sequence(COLOR_STYLE_ERROR), "") != 0 ||
       strcmp(color_style_sequence(COLOR_STYLE_RESET), "") != 0) {
        fprintf(stderr, "disabled styles should be empty\n");
        return 1;
    }

    color_set_enabled(true);

    if(!color_is_enabled()) {
        fprintf(stderr, "color could not be re-enabled\n");
        return 1;
    }

    return 0;
}
