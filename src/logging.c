#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "color.h"

extern bool g_debug_enabled;

static void log_v(FILE *stream, const char *prefix, enum color_style style, const char *fmt, va_list args) {
    fputs(prefix, stream);
    fputs(color_style_sequence(style), stream);
    vfprintf(stream, fmt, args);
    fputs(color_style_sequence(COLOR_STYLE_RESET), stream);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "❌ ", COLOR_STYLE_ERROR, fmt, args);
    va_end(args);
}

void log_critical(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "🤯 ", COLOR_STYLE_ERROR, fmt, args);
    va_end(args);
}

void log_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "⚠️ ", COLOR_STYLE_WARNING, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "ℹ️ ", COLOR_STYLE_INFO, fmt, args);
    va_end(args);
}

void log_success(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "✔️ ", COLOR_STYLE_SUCCESS, fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...) {
    if(!g_debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    log_v(stdout, "[debug] ", COLOR_STYLE_RESET, fmt, args);
    va_end(args);
}
