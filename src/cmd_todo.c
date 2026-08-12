#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "logging.h"
#include "platform.h"
#include "cli.h"
#include "cmd_shared.h"

// Datatypes
struct todo {
    char text[DEFAULT_BUFFER_SIZE * 2];
    time_t created;
    unsigned long long id;
};

struct todo_metadata {
    int version;
    unsigned long long last_id;
};

// Prototypes
int write_todo_metadata(const char *filename, const struct todo_metadata *md);

int initialize_todo_front_matter(const char *filename) {
    struct todo_metadata md;
    md.last_id = 0;
    md.version = 1;

    log_debug("Creating TODO metadata front matter as it is missing.\n");
    if(write_todo_metadata(filename, &md) != R_OK) {
        log_critical("Failed to initialize TODO front matter.\n");
        return R_ERROR;
    }

    log_success("Initialized TODO front matter.");
    return R_OK;
}

int read_todo_metadata(char *filename, struct todo_metadata *md)
{
    FILE *file = NULL;

    errno_t err = fopen_s(&file, filename, "r");
    if(err != 0 || file == NULL) {
        log_error("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    md->version = 0;
    md->last_id = 0;

    char line[DEFAULT_BUFFER_SIZE];

    // Front matter must start with ---
    if(fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return R_OK;
    }

    if(strcmp(trim(line), "---") != 0) {
        log_warning("Missing TODO front matter in %s.\n", filename);
        fclose(file);

        log_debug("Trying to initialize missing TODO front matter.\n");
        if(initialize_todo_front_matter(filename) != R_OK) {
            return R_ERROR;
        }

        log_success("Initialized TODO front matter.");
        return read_todo_metadata(filename, md);
    }

    bool found_end = false;

    while(fgets(line, sizeof(line), file) != NULL) {
        char *current = trim(line);

        if(strcmp(current, "---") == 0) {
            found_end = true;
            break;
        }

        // Ignore empty lines
        if(*current == '\0') {
            continue;
        }

        char *colon = strchr(current, ':');

        if(colon == NULL) {
            log_error("Invalid TODO metadata entry: %s\n", current);
            fclose(file);
            return R_ERROR;
        }

        *colon = '\0';

        char *key = trim(current);
        char *value = trim(colon + 1);

        if(strcmp(key, "version") == 0) {
            md->version = atoi(value);
        }
        else if(strcmp(key, "last_id") == 0) {
            md->last_id = strtoull(value, NULL, 10);
        }
        else {
            log_debug("Ignoring unknown TODO metadata key: %s\n", key);
        }
    }

    fclose(file);

    if(!found_end) {
        log_error("Unterminated TODO front matter in %s.\n", filename);
        return R_ERROR;
    }

    return R_OK;
}

int write_todo_metadata(const char *filename, const struct todo_metadata *md)
{
    FILE *source = NULL;
    errno_t err = fopen_s(&source, filename, "r");

    if(err != 0 || source == NULL) {
        log_error("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    /*
     * Build temporary file path.
     */
    char temp_path[DEFAULT_BUFFER_SIZE];

    int written = snprintf(
        temp_path,
        sizeof(temp_path),
        "%s.tmp",
        filename
    );

    if(written < 0 || (size_t)written >= sizeof(temp_path)) {
        log_error("Temporary file path too long.\n");
        fclose(source);
        return R_ERROR;
    }

    /*
     * Open temporary file.
     */
    FILE *temp = NULL;

    err = fopen_s(&temp, temp_path, "w");

    if(err != 0 || temp == NULL) {
        log_error("Failed opening temporary file %s.\n", temp_path);
        fclose(source);
        return R_ERROR;
    }

    /*
     * Write updated front matter.
     */
    if(fprintf(
        temp,
        "---\n"
        "version: %d\n"
        "last_id: %llu\n"
        "---\n\n",
        md->version,
        md->last_id
    ) < 0) {
        log_error("Failed writing TODO metadata.\n");
        fclose(source);
        fclose(temp);
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Determine whether the existing file already has front matter.
     */
    char line[DEFAULT_BUFFER_SIZE];
    bool has_front_matter = false;

    if(fgets(line, sizeof(line), source) != NULL) {
        if(strcmp(trim(line), "---") == 0) {
            has_front_matter = true;
        }
    }

    /*
     * Existing front matter:
     *
     * Skip everything until its closing "---".
     */
    if(has_front_matter) {
        bool found_end = false;

        while(fgets(line, sizeof(line), source) != NULL) {
            if(strcmp(trim(line), "---") == 0) {
                found_end = true;
                break;
            }
        }

        if(!found_end) {
            log_error(
                "Unterminated TODO front matter in %s.\n",
                filename
            );

            fclose(source);
            fclose(temp);
            remove(temp_path);

            return R_ERROR;
        }

        /*
         * Our new front matter already ends with a blank line.
         *
         * If the old front matter also had a separating blank line,
         * skip it so we don't accumulate blank lines every time the
         * metadata is updated.
         */
        long position = ftell(source);

        if(fgets(line, sizeof(line), source) != NULL) {
            char *current = trim(line);

            if(*current != '\0') {
                /*
                 * Not an empty separator, so put the file pointer
                 * back before this line.
                 */
                if(fseek(source, position, SEEK_SET) != 0) {
                    log_error(
                        "Failed repositioning %s while updating metadata.\n",
                        filename
                    );

                    fclose(source);
                    fclose(temp);
                    remove(temp_path);

                    return R_ERROR;
                }
            }
        }
    }
    else {
        /*
         * No front matter existed.
         *
         * We already consumed the first line while checking for it,
         * so rewind and preserve the entire original file.
         */
        rewind(source);
    }

    /*
     * Copy the remaining Markdown content unchanged.
     */
    while(fgets(line, sizeof(line), source) != NULL) {
        if(fputs(line, temp) == EOF) {
            log_error("Failed writing temporary TODO file.\n");

            fclose(source);
            fclose(temp);
            remove(temp_path);

            return R_ERROR;
        }
    }

    /*
     * Make sure the reads/writes themselves did not fail.
     */
    if(ferror(source)) {
        log_error("Failed reading %s.\n", filename);

        fclose(source);
        fclose(temp);
        remove(temp_path);

        return R_ERROR;
    }

    if(fclose(source) != 0) {
        log_warning("Failed closing %s cleanly.\n", filename);
    }

    if(fclose(temp) != 0) {
        log_error("Failed closing temporary TODO file.\n");
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Build backup path.
     */
    char backup_path[DEFAULT_BUFFER_SIZE];

    written = snprintf(
        backup_path,
        sizeof(backup_path),
        "%s.bak",
        filename
    );

    if(written < 0 || (size_t)written >= sizeof(backup_path)) {
        log_error("Backup path too long.\n");
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Remove a stale backup from an earlier operation.
     */
    if(access(backup_path, F_OK) == 0) {
        log_debug("Removing existing TODO backup.\n");

        if(remove(backup_path) != 0) {
            log_error(
                "Could not remove old backup: %s\n",
                backup_path
            );

            remove(temp_path);
            return R_ERROR;
        }
    }

    /*
     * Move the current file out of the way.
     */
    log_debug("Backing up %s.\n", filename);

    if(rename(filename, backup_path) != 0) {
        log_error("Failed backing up %s.\n", filename);
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Install the new version.
     */
    log_debug("Replacing %s with updated metadata.\n", filename);

    if(rename(temp_path, filename) != 0) {
        log_error("Failed replacing %s.\n", filename);

        /*
         * Try to restore the original file.
         */
        log_debug("Restoring %s from backup.\n", filename);

        if(rename(backup_path, filename) != 0) {
            log_critical(
                "Failed restoring %s from backup. "
                "The original file remains at %s.\n",
                filename,
                backup_path
            );
        }

        return R_ERROR;
    }

    /*
     * New file is safely installed. Backup is no longer required.
     */
    if(remove(backup_path) != 0) {
        log_warning(
            "Could not remove backup: %s\n",
            backup_path
        );
    }

    return R_OK;
}

int create_todo_from_args(int argc, char *argv[], struct todo *item) {
    item->text[0] = '\0';

    size_t used = 0;

    for(int i = 0; i < argc; ++i) {
        int written = snprintf(
            item->text + used, 
            sizeof(item->text) - used, 
            "%s%s", i > 0 ? " " : "",
            argv[i]);
        if(written < 0 || (size_t)written >= sizeof(item->text) - used) {
            log_error("Todo text is too long.\n");
            return R_ERROR;
        }

        used += (size_t)written;
    }

    struct todo_metadata md;
    if(read_todo_metadata("TODOS.md", &md) != R_OK) {
        log_critical("Could not read TODO metadata.");
        return R_ERROR;
    }

    item->id = md.last_id++;
    item->created = time(NULL);

    log_debug("Updating todo metadata with last_id change.");
    if(write_todo_metadata("TODOS.md", &md) != R_OK) {
        return R_ERROR;
    }

    return R_OK;
}

int write_todo(char* filename, struct todo *item) {
    log_debug("Starting to write todo.\n");
    char tag_id[DEFAULT_BUFFER_SIZE], tag_created[DEFAULT_BUFFER_SIZE];

    // convert id to a tag
    log_debug("Converting id to tag.\n");
    int written = snprintf(tag_id, sizeof(tag_id), "#%s/id/%llu", APP_NAME, item->id);
    if(written < 0 || (size_t)written >= sizeof(tag_id)) {
        log_critical("Failed to build todo ID tag.");
        return R_ERROR;
    }

    // convert created date to a tag
    log_debug("Converting cration date to tag.\n");
    struct tm local_time;
    if(localtime_s(&local_time, &(item->created)) != 0) {
        log_error("Failed to get local time.");
        return R_ERROR;
    }
    
    char date_str[11];
    if(strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local_time) == 0) {
        log_error("Feild to format local date.");        
        return R_ERROR;
    }

    written = snprintf(tag_created, sizeof(tag_created), "#%s/created/%s", APP_NAME, date_str);
    if(written < 0 || (size_t)written >= sizeof(tag_created)) {
        log_critical("Failed to build todo created tag.");
        return R_ERROR;
    }


    char* argv[] = {item->text, tag_id, tag_created};

    return add_markdown_item(3, argv, filename, "* [ ] ", NULL);
}

int command_todo_add(int argc, char* argv[]) {
    log_debug("Adding a new todo.\n");
    
    struct todo item;

    if(create_todo_from_args(argc, argv, &item) != R_OK) {
        return R_ERROR;
    }

    if(write_todo("TODOS.md", &item) != R_OK) {
        return R_ERROR;
    }

    log_success("Task noted.\n");
    return R_OK;
}

int command_todo(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("Please specify a todo command. Refer to --help if required.");
        return R_ERROR;
    }

    if(has_switch(argc, argv, "--help") || has_switch(argc, argv, "-h")) {
        printf("`%s todo` allows you to manage your personal todo list.\n", APP_NAME);
        printf("\n");
        printf("Subcommands:\n");
        printf("  %-16s %s\n", "add",   "add a new todo item to the list");
        return R_OK;
    }

    char* command = argv[0];
    argc--;
    argv++;

    if(strcmp(command, "add") == 0) {
        log_debug("Running `todo add` command.\n");
        return command_todo_add(argc, argv);
    }

    return R_OK;
}
