#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "cli.h"

extern bool g_debug_enabled;

#define COLOR_SUCCESS ANSI_FG_RGB(100, 210, 140)
#define COLOR_ERROR   ANSI_FG_RGB(255, 105, 120)
#define COLOR_WARNING ANSI_FG_RGB(255, 190, 80)
#define COLOR_INFO    ANSI_FG_RGB(110, 190, 255)

static void log_v(FILE *stream, const char *prefix, const char *fmt, va_list args)
{
    fputs(prefix, stream);
    vfprintf(stream, fmt , args);
    fputs(ANSI_RESET, stream);
}

void log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "❌ " COLOR_ERROR, fmt, args);
    va_end(args);
}

void log_critical(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "🤯 " COLOR_ERROR, fmt, args);
    va_end(args);
}

void log_warning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "⚠️ " COLOR_WARNING, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "ℹ️ " COLOR_INFO, fmt, args);
    va_end(args);
}

void log_success(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "✔️ " COLOR_SUCCESS, fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...)
{
    if (!g_debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    log_v(stdout, "[debug] ", fmt, args);
    va_end(args);
}
