#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "logging.h"
#include "platform.h"

char *trim(char *str) {
    while(isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;

    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}

int create_file_if_not_exists(char *fname) {
    log_debug("Checking if file '%s' exists.\n", fname);

    if(file_access_utf8(fname, F_OK) == 0) {
        return R_OK;
    }

    log_debug("File does not exist. Creating it now.\n");
    FILE *f = NULL;
    log_debug("Opening daily note at: %s\n", fname);
    int err = file_open_utf8(&f, fname, "w");
    if(err != 0 || f == NULL) {
        log_error("Failed creating file: %s\n", fname);
        return R_ERROR;
    }

    if(!str_ends_with(fname, ".tmp")) {
        log_success("Created %s\n", fname);
    }
    fclose(f);
    return R_OK;
}

int get_base_dir_file_path(const char *filename, char *buffer, size_t buffer_size) {
    int written = snprintf(buffer, buffer_size, "%s%s%s", g_config.base_dir, get_path_separator(), filename);
    if(written < 0 || (size_t)written >= buffer_size) {
        log_error("File path too long.\n");
        return R_ERROR;
    }
    return R_OK;
}

int build_text_from_args(int argc, char *argv[], char *buffer, size_t buffer_size) {
    buffer[0] = '\0';

    size_t used = 0;

    for(int i = 0; i < argc; ++i) {
        int written = snprintf(buffer + used, buffer_size - used, "%s%s", i > 0 ? " " : "", argv[i]);

        if(written < 0 || (size_t)written >= buffer_size - used) {
            log_error("Text is too long.\n");
            return R_ERROR;
        }

        used += (size_t)written;
    }

    return R_OK;
}

int build_daily_heading(char *buffer, size_t size, time_t date) {
    // convert the date to the block date in NOTES.md which is YYYY-mm-dd
    struct tm local_time;

    if(localtime_s(&local_time, &date) != 0) {
        log_critical("Failed converting requested note date.\n");
        return R_ERROR;
    }

    // format the date
    char date_str[11];

    if(strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local_time) == 0) {
        log_critical("Failed formatting requested note date.\n");
        return R_ERROR;
    }

    // write it in heading format
    int written = snprintf(buffer, size, "# %s", date_str);
    if(written < 0 || written >= (int)size) {
        log_critical("Daily heading buffer too small.");
        return R_ERROR;
    }

    return R_OK;
}

bool str_ends_with(const char *str, const char *suffix) {
    if(str == NULL || suffix == NULL) {
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if(suffix_len > str_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool dates_equal(time_t a, time_t b) {
    struct tm date_a;
    struct tm date_b;

    if(localtime_s(&date_a, &a) != 0 || localtime_s(&date_b, &b) != 0) {
        return false;
    }

    return date_a.tm_year == date_b.tm_year && date_a.tm_mon == date_b.tm_mon && date_a.tm_mday == date_b.tm_mday;
}

int format_date(time_t date, char *buffer, size_t buffer_size) {
    if(buffer == NULL || buffer_size == 0) {
        return R_ERROR;
    }

    struct tm local_time;

    if(localtime_s(&local_time, &date) != 0) {
        log_error("Failed to convert date to local time.\n");
        return R_ERROR;
    }

    if(strftime(buffer, buffer_size, "%Y-%m-%d", &local_time) == 0) {
        log_error("Failed to format date.\n");
        return R_ERROR;
    }

    return R_OK;
}

int compare_dates(time_t a, time_t b) {
    struct tm date_a;
    struct tm date_b;

    if(localtime_s(&date_a, &a) != 0 || localtime_s(&date_b, &b) != 0) {
        return 0;
    }

    if(date_a.tm_year != date_b.tm_year) {
        return date_a.tm_year < date_b.tm_year ? -1 : 1;
    }

    if(date_a.tm_mon != date_b.tm_mon) {
        return date_a.tm_mon < date_b.tm_mon ? -1 : 1;
    }

    if(date_a.tm_mday != date_b.tm_mday) {
        return date_a.tm_mday < date_b.tm_mday ? -1 : 1;
    }

    return 0;
}

int parse_int(const char *text, int *result) {
    if(text == NULL || result == NULL || *text == '\0') {
        return R_ERROR;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(text, &end, 10);

    if(errno == ERANGE || value < INT_MIN || value > INT_MAX || *end != '\0') {
        return R_ERROR;
    }

    *result = (int)value;
    return R_OK;
}

int join_array(int argc, char *argv[], char *buffer, size_t buffer_size) {
    if(buffer == NULL || buffer_size == 0) {
        return R_ERROR;
    }

    buffer[0] = '\0';

    if(argc <= 0) {
        return R_OK;
    }

    if(argv == NULL) {
        return R_ERROR;
    }

    size_t used = 0;

    for(int i = 0; i < argc; ++i) {
        int written = snprintf(buffer + used, buffer_size - used, "%s%s", i > 0 ? " " : "", argv[i]);
        if(written < 0 || (size_t)written >= buffer_size - used) {
            log_error("Joined argument string is too long.\n");
            return R_ERROR;
        }

        used += (size_t)written;
    }

    return R_OK;
}

bool ends_with_whitespace(const char *text) {
    size_t len;

    if(text == NULL) {
        return false;
    }

    len = strlen(text);

    if(len == 0) {
        return false;
    }

    return isspace((unsigned char)text[len - 1]) != 0;
}
