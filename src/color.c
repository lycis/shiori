#include "color.h"

static bool g_color_enabled = true;

void color_set_enabled(bool enabled) {
    g_color_enabled = enabled;
}

bool color_is_enabled(void) {
    return g_color_enabled;
}

const char *color_style_sequence(enum color_style style) {
    if(!g_color_enabled) {
        return "";
    }

    switch(style) {
    case COLOR_STYLE_RESET:
        return ANSI_RESET;
    case COLOR_STYLE_SUCCESS:
        return COLOR_SUCCESS;
    case COLOR_STYLE_ERROR:
        return COLOR_ERROR;
    case COLOR_STYLE_WARNING:
        return COLOR_WARNING;
    case COLOR_STYLE_INFO:
        return COLOR_INFO;
    default:
        return "";
    }
}
