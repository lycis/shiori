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
    case COLOR_STYLE_BOLD:
        return ANSI_BOLD;
    case COLOR_STYLE_TOPIC:
        return COLOR_TOPIC;
    case COLOR_STYLE_TODOS:
        return COLOR_TODOS;
    case COLOR_STYLE_NOTES:
        return COLOR_NOTES;
    case COLOR_STYLE_DUE_DATE:
        return COLOR_DUE_DATE;
    case COLOR_STYLE_HEADING:
        return COLOR_HEADING;
    case COLOR_STYLE_NOTE_ID:
        return COLOR_NOTE_ID;
    case COLOR_STYLE_METADATA:
        return COLOR_METADATA;
    case COLOR_STYLE_OPEN:
        return COLOR_OPEN;
    case COLOR_STYLE_COMPLETION_REMAINDER:
        return COLOR_COMPLETION_REMAINDER;
    case COLOR_STYLE_DIVIDER:
        return COLOR_DIVIDER;
    default:
        return "";
    }
}
