#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

extern bool g_debug_enabled;

static void log_v(FILE *stream, const char *prefix, const char *fmt, va_list args)
{
    fputs(prefix, stream);
    vfprintf(stream, fmt, args);
}

void log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "❌ ", fmt, args);
    va_end(args);
}

void log_critical(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "🤯 ", fmt, args);
    va_end(args);
}

void log_warning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "⚠️ ", fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "ℹ️ ", fmt, args);
    va_end(args);
}

void log_success(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "✔️ ", fmt, args);
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
