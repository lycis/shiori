#ifndef SHIORI_LOGGING_H
#define SHIORI_LOGGING_H

void log_error(const char *fmt, ...);
void log_critical(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_success(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif
