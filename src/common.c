#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "logging.h"
#include "platform.h"
#include "common.h"
#include "config.h"

char *trim(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}

int create_file_if_not_exists(char* fname) {
    log_debug("Checking if file '%s' exists.\n", fname);

    if(access(fname, F_OK) == 0) return R_OK;

    log_debug("File does not exist. Creating it now.\n");
    FILE *f = NULL;
    log_debug("Opening daily note at: %s\n", fname);
    errno_t err = fopen_s(&f, fname, "w");
    if(err != 0  || f == NULL) {
        log_error("Failed creating file: %s\n", fname);
        return R_ERROR;
    }

    log_success("Created %s\n", fname);
    fclose(f);
    return R_OK;
}

int get_base_dir_file_path(char *filename, char *buffer, size_t buffer_size) {
    int written = snprintf(buffer, buffer_size, "%s%s%s", g_config.base_dir, get_path_separator(), filename);
    if(written < 0 || written >= buffer_size) {
        log_error("File path too long.\n");
        return R_ERROR;
    }
    return R_OK;
}

int build_text_from_args(int argc, char *argv[], char *buffer, size_t buffer_size) {
    buffer[0] = '\0';

    size_t used = 0;

    for(int i = 0; i < argc; ++i) {
        int written = snprintf(
            buffer + used,
            buffer_size - used,
            "%s%s",
            i > 0 ? " " : "",
            argv[i]
        );

        if(written < 0 ||
           (size_t)written >= buffer_size - used) {
            log_error("Text is too long.\n");
            return R_ERROR;
        }

        used += (size_t)written;
    }

    return R_OK;
}