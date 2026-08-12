#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "logging.h"
#include "platform.h"
#include "config.h"


bool is_heading(const char *line) {
    return line[0] == '#' && line[1] == ' ';
}

void write_item_to_file(int argc, char *argv[], FILE *file, const char *prefix) {
    fprintf(file, "%s", prefix);

    for(int i = 0; i < argc; ++i) {
        fprintf(file, "%s", argv[i]);

        if(i != argc - 1) {
            fprintf(file, " ");
        }
    }

    fprintf(file, "\n");
}

bool heading_matches(const char *line, const char *heading){
    size_t len = strlen(heading);

    return strncmp(line, heading, len) == 0 &&
           (line[len] == '\n' ||
            line[len] == '\r' ||
            line[len] == '\0');
}

int add_markdown_item(int argc, char *argv[], const char *filename, const char *prefix, const char *heading) {
    log_debug("Writing markdown item (c=%d)\n", argc);
    char file_path[DEFAULT_BUFFER_SIZE];

    int written = snprintf(file_path, sizeof(file_path), "%s%s%s", g_config.base_dir, get_path_separator(), filename);
    if(written < 0 || written >= (int)sizeof(file_path)) {
        log_error("File path too long.\n");
        return R_ERROR;
    }

    if(create_file_if_not_exists(file_path) != R_OK) {
        return R_ERROR;
    }

    /*
     * No heading means that we don't need to insert anything
     * into the middle of the file. We can simply append.
     */
    if(heading == NULL) {
        FILE *file = NULL;

        errno_t err = fopen_s(&file, file_path, "a");
        if(err != 0 || file == NULL) {
            log_error("Failed opening %s.\n", filename);
            return R_ERROR;
        }

        write_item_to_file(argc, argv, file, prefix);

        fclose(file);
        return R_OK;
    }

    /*
     * With a heading we need to potentially insert into an
     * existing block, so use the temp-file strategy.
     */

    FILE *source = NULL;

    errno_t err = fopen_s(&source, file_path, "r");
    if(err != 0 || source == NULL) {
        log_error("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    char temp_path[DEFAULT_BUFFER_SIZE];

    written = snprintf(temp_path, sizeof(temp_path), "%s.tmp", file_path);
    if(written < 0 || written >= (int)sizeof(temp_path)) {
        log_error("Temporary file path too long.\n");
        fclose(source);
        return R_ERROR;
    }

    FILE *temp = NULL;

    err = fopen_s(&temp, temp_path, "w");
    if(err != 0 || temp == NULL) {
        log_error("Failed opening temporary file.\n");
        fclose(source);
        return R_ERROR;
    }

    bool found_heading = false;
    bool item_written = false;

    char line[DEFAULT_BUFFER_SIZE];

    while(fgets(line, sizeof(line), source) != NULL) {
        if(!found_heading) {
            if(heading_matches(line, heading)) {
                log_debug("Found heading: %s\n", heading);
                found_heading = true;
            }
        }
        else if(!item_written && is_heading(line)) {
            write_item_to_file(argc, argv, temp, prefix);
            item_written = true;
        }

        fputs(line, temp);
    }

    if(found_heading && !item_written) {
        write_item_to_file(argc, argv, temp, prefix);
    }

    if(!found_heading) {
        fprintf(temp, "\n%s\n", heading);
        write_item_to_file(argc, argv, temp, prefix);
    }

    fclose(source);
    fclose(temp);

    /*
     * Replace the original safely.
     */

    char backup_path[DEFAULT_BUFFER_SIZE];

    written = snprintf(
        backup_path,
        sizeof(backup_path),
        "%s.bak",
        file_path
    );

    if(written < 0 || written >= (int)sizeof(backup_path)) {
        log_error("Backup file path too long.\n");
        return R_ERROR;
    }

    if(access(backup_path, F_OK) == 0) {
        if(remove(backup_path) != 0) {
            log_error(
                "Could not remove previous backup: %s\n",
                backup_path
            );
            return R_ERROR;
        }
    }

    if(rename(file_path, backup_path) != 0) {
        log_error("Failed to create backup of %s.\n", filename);
        return R_ERROR;
    }

    if(rename(temp_path, file_path) != 0) {
        log_error("Failed replacing %s.\n", filename);

        if(rename(backup_path, file_path) != 0) {
            log_critical(
                "Failed restoring %s from backup.\n",
                filename
            );
        }

        return R_ERROR;
    }

    if(remove(backup_path) != 0) {
        log_warning("Failed to remove backup: %s\n", backup_path
        );
    }

    return R_OK;
}