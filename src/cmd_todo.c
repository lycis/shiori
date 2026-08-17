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
#include "todo.h"
#include "todo_list.h"
#include "commands.h"

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

    log_success("Initialized TODO front matter.\n");
    return R_OK;
}

int read_todo_metadata(char *filename, struct todo_metadata *md)
{
    FILE *file = open_base_dir_file(filename, "r");
    if(file == NULL) {
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
    item->due = 0;

    size_t used = 0;

    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--due") == 0 || strcmp(argv[i], "-d") == 0) {
            if(i == argc-1) {
                log_error("--due requires a due date.\n");
                return R_ERROR;
            }

            char* due_date = argv[i + 1];
            if(parse_date_arg(argv[i + 1], &item->due) != R_OK) {
                return R_ERROR;
            }

            log_debug("Due date detected: %s\n", due_date);

            i++;
            continue;
        }

        int written = snprintf(
            item->text + used,
            sizeof(item->text) - used,
            "%s%s",
            used > 0 ? " " : "",
            argv[i]
        );
        if(written < 0 || (size_t)written >= sizeof(item->text) - used) {
            log_error("Todo text is too long.\n");
            return R_ERROR;
        }

        used += (size_t)written;
    }

    struct todo_metadata md;
    if(read_todo_metadata(TODO_FILE, &md) != R_OK) {
        log_critical("Could not read TODO metadata.");
        return R_ERROR;
    }

    item->id = md.last_id++;
    item->created = time(NULL);
    item->status = OPEN;

    log_debug("Updating todo metadata with last_id change.");
    if(write_todo_metadata(TODO_FILE, &md) != R_OK) {
        return R_ERROR;
    }

    return R_OK;
}

static int write_todo_markdown(FILE *file, const struct todo *item) {
    char created_date[11];

    if(format_date(
        item->created,
        created_date,
        sizeof(created_date)
    ) != R_OK) {
        return R_ERROR;
    }

    if(fprintf(
        file,
        "* [%s] %s #%s/id/%llu #%s/created/%s",
        todo_status_mark(item->status),
        item->text,
        APP_NAME,
        item->id,
        APP_NAME,
        created_date
    ) < 0) {
        log_error(
            "Failed writing todo %llu.\n",
            item->id
        );
        return R_ERROR;
    }

    if(item->due != 0) {
        char due_date[11];

        if(format_date(
            item->due,
            due_date,
            sizeof(due_date)
        ) != R_OK) {
            return R_ERROR;
        }

        if(fprintf(
            file,
            " #%s/due/%s",
            APP_NAME,
            due_date
        ) < 0) {
            log_error(
                "Failed writing due date for todo %llu.\n",
                item->id
            );
            return R_ERROR;
        }
    }

    if(fputc('\n', file) == EOF) {
        log_error(
            "Failed finishing todo %llu.\n",
            item->id
        );
        return R_ERROR;
    }

    return R_OK;
}

int write_todo(char *filename, struct todo *item)
{
    FILE *file = NULL;

    errno_t err = fopen_s(
        &file,
        filename,
        "a"
    );

    if(err != 0 || file == NULL) {
        log_error(
            "Failed opening %s.\n",
            filename
        );
        return R_ERROR;
    }

    int result = write_todo_markdown(file, item);

    if(fclose(file) != 0) {
        log_error(
            "Failed closing %s.\n",
            filename
        );
        return R_ERROR;
    }

    return result;
}

static int command_todo_add(int argc, char* argv[]) {
    log_debug("Adding a new todo.\n");
    
    struct todo item;

    if(create_todo_from_args(argc, argv, &item) != R_OK) {
        return R_ERROR;
    }

    if(write_todo(TODO_FILE, &item) != R_OK) {
        return R_ERROR;
    }

    log_success("Task noted.\n");
    return R_OK;
}

struct todo_filter {
    bool show_open;
    bool show_in_progress;
    bool show_done;

    const char **tags;
    size_t tag_count;
};

static bool todo_has_tag(const struct todo *item, const char *tag) {
    char needle[DEFAULT_BUFFER_SIZE];

    snprintf(needle, sizeof(needle), "#%s", tag);

    log_debug(
        "Checking tag: text='%s' needle='%s'\n",
        item->text,
        needle
    );

    return strstr(item->text, needle) != NULL;
}

static bool todo_matches_filter(const struct todo *item, const struct todo_filter *filter) {
    bool status_matches = false;

    switch(item->status) {
        case OPEN:
            status_matches = filter->show_open;
            break;

        case IN_PROGRESS:
            status_matches = filter->show_in_progress;
            break;

        case DONE:
            status_matches = filter->show_done;
            break;
    }

    log_debug("Status filter: item %d (%d) -> %d\n", item->id, item->status, status_matches);

    if(!status_matches) {
        return false;
    }

    for(size_t i = 0; i < filter->tag_count; ++i) {
        if(!todo_has_tag(item, filter->tags[i])) {
            return false;
        }
    }

    return true;
}

static int command_todo_list(int argc, char* argv[]) {
    if(has_switch(argc, argv, "--help", true) || has_switch(argc, argv, "-h", true)) {
         printf(
            "Usage:\n"
            "  %s todo list [options]\n"
            "\n"
            "Lists todos from %s.\n"
            "\n"
            "By default, open and in-progress todos are shown.\n"
            "\n"
            "Options:\n"
            "  %-18s Show open todos\n"
            "  %-18s Show in-progress todos\n"
            "  %-18s Show completed todos\n"
            "  %-18s Show todos of all statuses\n"
            "  %-18s Filter by tag; may be specified multiple times\n"
            "  %-18s Show this help\n"
            "\n"
            "Examples:\n"
            "  %s todo list\n"
            "  %s todo list --done\n"
            "  %s todo list --all\n"
            "  %s todo list --tag work\n"
            "  %s todo list --tag work --tag urgent\n"
            "  %s todo list --done --tag work\n",
            APP_NAME,
            TODO_FILE,
            "--open",
            "--in-progress",
            "--done",
            "--all",
            "--tag <tag>",
            "-h, --help",
            APP_NAME,
            APP_NAME,
            APP_NAME,
            APP_NAME,
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    log_debug("Listing todos (c=%d)\n", argc);

    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(TODO_FILE, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    // default filter is open and in progress
    struct todo_filter filter = {
        .show_open = true,
        .show_in_progress = true,
        .show_done = false
    };

    bool has_status_filter =
        has_switch(argc, argv, "--open", false) ||
        has_switch(argc, argv, "--in-progress", false) ||
        has_switch(argc, argv, "--done", false) ||
        has_switch(argc, argv, "--all", false);

    if(has_status_filter) {
        filter.show_open = false;
        filter.show_in_progress = false;
        filter.show_done = false;
    }

    if(has_switch(argc, argv, "--open", false)) filter.show_open = true;
    if(has_switch(argc, argv, "--in-progress", false)) filter.show_in_progress = true;
    if(has_switch(argc, argv, "--done", false)) filter.show_done = true;
    if(has_switch(argc, argv, "--all", false)) {
        filter.show_open = true;
        filter.show_in_progress = true;
        filter.show_done = true;
    }

    // tag filters
    const char *tags[32];
    size_t tag_count = 0;

    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--tag") == 0) {
            if(i + 1 >= argc) {
                log_error("--tag requires a tag name.\n");
                todo_list_free(&todos);
                return R_ERROR;
            }

            if(tag_count >= 32) {
                log_error("Too many tag filters.\n");
                todo_list_free(&todos);
                return R_ERROR;
            }

            tags[tag_count++] = argv[i + 1];
            i++; // skip the value we just consumed
        }
    }

    filter.tags = tags;
    filter.tag_count = tag_count;

    for(size_t i = 0; i < filter.tag_count; ++i) {
        log_debug("Tag filter %zu: '%s'\n", i, filter.tags[i]);
    }

    for(size_t i = 0; i < todos.count; ++i) {
        struct todo *item = &todos.items[i];
        char date[11];

        if(format_todo_date(item->created, date, sizeof(date)) != R_OK) {
            strcpy_s(date, sizeof(date), "??????????");
        }

        if(todo_matches_filter(item, & filter)) {
            char due_buffer[DEFAULT_BUFFER_SIZE];
            if(item->due != 0) {
                char dbuffer[20];
                if(format_date(item->due, dbuffer, sizeof(dbuffer)) != R_OK) {
                    log_critical("Failed to format due date.\n");
                    todo_list_free(&todos);
                    return R_ERROR;
                }

                sprintf(due_buffer, " ⏰ %s", dbuffer);
            } else {
                sprintf(due_buffer, "");
            }

            printf(
                "%s %-4llu %-40s ➕ %s%s\n",
                todo_status_icon(item->status),
                item->id,
                item->text,
                date,
                due_buffer
            );
        }
    }

    todo_list_free(&todos);
    return R_OK;
}

static int parse_todo_id(const char *value, unsigned long long *id)
{
    char *end = NULL;

    unsigned long long parsed = strtoull(value, &end, 10);

    if(end == value || *end != '\0') {
        log_error("Invalid todo ID: %s\n", value);
        return R_ERROR;
    }

    *id = parsed;
    return R_OK;
}


int write_todo_list(
    const char *filename,
    const struct todo_list *list,
    const struct todo_metadata *md
) {
    char temp_path[DEFAULT_BUFFER_SIZE];
    char backup_path[DEFAULT_BUFFER_SIZE];

    int written = snprintf(
        temp_path,
        sizeof(temp_path),
        "%s.tmp",
        filename
    );

    if(written < 0 || (size_t)written >= sizeof(temp_path)) {
        log_error("Temporary file path too long.\n");
        return R_ERROR;
    }

    written = snprintf(
        backup_path,
        sizeof(backup_path),
        "%s.bak",
        filename
    );

    if(written < 0 || (size_t)written >= sizeof(backup_path)) {
        log_error("Backup file path too long.\n");
        return R_ERROR;
    }

    FILE *temp = NULL;

    errno_t err = fopen_s(
        &temp,
        temp_path,
        "w"
    );

    if(err != 0 || temp == NULL) {
        log_error("Failed opening temporary TODO file.\n");
        return R_ERROR;
    }

    /*
     * Write front matter.
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
        fclose(temp);
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Write all todos using the shared serializer.
     */
    for(size_t i = 0; i < list->count; ++i) {
        if(write_todo_markdown(
            temp,
            &list->items[i]
        ) != R_OK) {
            log_error(
                "Failed writing todo %llu.\n",
                list->items[i].id
            );

            fclose(temp);
            remove(temp_path);
            return R_ERROR;
        }
    }

    /*
     * Ensure everything reached the file successfully.
     */
    if(fclose(temp) != 0) {
        log_error("Failed closing temporary TODO file.\n");
        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Remove stale backup if one exists.
     */
    if(access(backup_path, F_OK) == 0) {
        if(remove(backup_path) != 0) {
            log_error(
                "Could not remove old TODO backup: %s\n",
                backup_path
            );

            remove(temp_path);
            return R_ERROR;
        }
    }

    /*
     * Back up current file.
     */
    if(rename(filename, backup_path) != 0) {
        log_error(
            "Failed backing up %s.\n",
            filename
        );

        remove(temp_path);
        return R_ERROR;
    }

    /*
     * Replace original with new file.
     */
    if(rename(temp_path, filename) != 0) {
        log_error(
            "Failed replacing %s.\n",
            filename
        );

        if(rename(backup_path, filename) != 0) {
            log_critical(
                "Failed restoring %s. Backup remains at %s.\n",
                filename,
                backup_path
            );
        }

        return R_ERROR;
    }

    /*
     * Cleanup backup.
     */
    if(remove(backup_path) != 0) {
        log_warning(
            "Could not remove TODO backup: %s\n",
            backup_path
        );
    }

    return R_OK;
}

static int set_todo_status(unsigned long long id, todo_status status) {
    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(TODO_FILE, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    struct todo *item = todo_list_find_by_id(&todos, id);
    if(item == NULL) {
        log_error("ID not found\n");
        todo_list_free(&todos);
        return R_ERROR;
    }

    item->status = status;
    log_debug("moved item status to in progess\n");

    // write todo list back to file
    struct todo_metadata md;

    if(read_todo_metadata(TODO_FILE, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    if(write_todo_list(TODO_FILE, &todos, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    log_success("Moved %d (%s) to %s.\n", id, item->text, todo_status_string(item->status));
    todo_list_free(&todos);
    return R_OK;
}

static int command_todo_start(int argc, char* argv[]) {
    log_debug("Moving item into progress\n");
    
    if(argc < 1) {
        log_error("You must specify a task id to start.\n");
        return R_ERROR;
    }

    unsigned long long id;
    if(parse_todo_id(argv[0], &id) != R_OK) {
        return R_ERROR;
    }

    set_todo_status(id, IN_PROGRESS);
    return R_OK;
}

static int command_todo_done(int argc, char* argv[]) {
    log_debug("Moving item into done\n");
    
    if(argc < 1) {
        log_error("You must specify a task id to mark done.\n");
        return R_ERROR;
    }

    unsigned long long id;
    if(parse_todo_id(argv[0], &id) != R_OK) {
        return R_ERROR;
    }

    set_todo_status(id, DONE);
    return R_OK;
}

static int command_todo_reopen(int argc, char* argv[]) {
    log_debug("Moving item into open\n");
    
    if(argc < 1) {
        log_error("You must specify a task id to reopen.\n");
        return R_ERROR;
    }

    unsigned long long id;
    if(parse_todo_id(argv[0], &id) != R_OK) {
        return R_ERROR;
    }

    set_todo_status(id, OPEN);
    return R_OK;
}

static int command_todo_rewrite(int argc, char *argv[])
{
    if(argc < 2) {
        log_error(
            "You must specify a task id and something to change.\n"
        );
        return R_ERROR;
    }

    unsigned long long id;

    if(parse_todo_id(argv[0], &id) != R_OK) {
        return R_ERROR;
    }

    /*
     * Parse arguments.
     *
     * Text is optional.
     * Due date is optional.
     */
    time_t new_due = 0;
    bool due_changed = false;

    char *text_argv[argc - 1];
    int text_argc = 0;

    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--due") == 0 ||
           strcmp(argv[i], "-d") == 0) {

            if(i + 1 >= argc) {
                log_error(
                    "%s requires a due date.\n",
                    argv[i]
                );
                return R_ERROR;
            }

            const char *due_arg = argv[i + 1];

            if(strcmp(due_arg, "none") == 0) {
                new_due = 0;
            }
            else if(parse_date_arg(
                due_arg,
                &new_due
            ) != R_OK) {
                return R_ERROR;
            }

            due_changed = true;

            log_debug(
                "New due date detected: %s\n",
                due_arg
            );

            i++; // skip value
            continue;
        }

        text_argv[text_argc++] = argv[i];
    }

    /*
     * ID alone does not constitute a rewrite.
     */
    if(text_argc == 0 && !due_changed) {
        log_error("Nothing to rewrite.\n");
        return R_ERROR;
    }

    log_debug("Rewriting todo %llu.\n", id);

    /*
     * Load todos.
     */
    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(TODO_FILE, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * Find target.
     */
    struct todo *item =
        todo_list_find_by_id(&todos, id);

    if(item == NULL) {
        log_error(
            "Todo %llu not found.\n",
            id
        );

        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * Preserve old values for logging.
     */
    char old_text[sizeof(item->text)];

    if(strcpy_s(
        old_text,
        sizeof(old_text),
        item->text
    ) != 0) {
        log_critical(
            "Failed to preserve old todo text.\n"
        );

        todo_list_free(&todos);
        return R_ERROR;
    }

    time_t old_due = item->due;

    /*
     * Update text, if provided.
     */
    if(text_argc > 0) {
        if(build_text_from_args(
            text_argc,
            text_argv,
            item->text,
            sizeof(item->text)
        ) != R_OK) {
            todo_list_free(&todos);
            return R_ERROR;
        }

        log_debug(
            "Rewriting todo %llu text: '%s' -> '%s'\n",
            id,
            old_text,
            item->text
        );
    }

    /*
     * Update due date, if requested.
     */
    if(due_changed) {
        item->due = new_due;

        if(new_due == 0) {
            log_debug(
                "Removing due date from todo %llu.\n",
                id
            );
        }
        else {
            char due_date[11];

            if(format_date(
                new_due,
                due_date,
                sizeof(due_date)
            ) != R_OK) {
                todo_list_free(&todos);
                return R_ERROR;
            }

            log_debug(
                "Setting due date of todo %llu to %s.\n",
                id,
                due_date
            );
        }
    }

    /*
     * Preserve file metadata.
     */
    struct todo_metadata md;

    if(read_todo_metadata(TODO_FILE, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * Save modified list.
     */
    if(write_todo_list(TODO_FILE, &todos, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * Success output depending on what changed.
     */
    if(text_argc > 0 && due_changed) {
        log_success(
            "Updated todo %llu text and due date.\n",
            id
        );
    }
    else if(text_argc > 0) {
        log_success(
            "Rewrote todo %llu: \"%s\" -> \"%s\"\n",
            id,
            old_text,
            item->text
        );
    }
    else if(new_due == 0) {
        log_success(
            "Removed due date from todo %llu.\n",
            id
        );
    }
    else {
        char due_date[11];

        if(format_date(
            new_due,
            due_date,
            sizeof(due_date)
        ) != R_OK) {
            todo_list_free(&todos);
            return R_ERROR;
        }

        log_success(
            "Set due date of todo %llu to %s.\n",
            id,
            due_date
        );
    }

    (void)old_due; // useful later if you want old -> new due logging

    todo_list_free(&todos);

    return R_OK;
}

static int command_todo_remove(int argc, char *argv[])
{
    if(argc < 1) {
        log_error("You must specify a todo ID to remove.\n");
        return R_ERROR;
    }

    unsigned long long id;

    if(parse_todo_id(argv[0], &id) != R_OK) {
        return R_ERROR;
    }

    char file_path[DEFAULT_BUFFER_SIZE];

    if(get_base_dir_file_path(
        TODO_FILE,
        file_path,
        sizeof(file_path)
    ) != R_OK) {
        return R_ERROR;
    }

    if(create_file_if_not_exists(file_path) != R_OK) {
        return R_ERROR;
    }

    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(file_path, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    struct todo *item = todo_list_find_by_id(&todos, id);

    if(item == NULL) {
        log_error("Todo %llu not found.\n", id);
        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * Preserve the text before removing the item from the array.
     */
    char removed_text[sizeof(item->text)];

    if(strcpy_s(
        removed_text,
        sizeof(removed_text),
        item->text
    ) != 0) {
        log_critical("Failed preserving todo text before removal.\n");
        todo_list_free(&todos);
        return R_ERROR;
    }

    if(todo_list_remove_by_id(&todos, id) != R_OK) {
        log_error("Failed removing todo %llu.\n", id);
        todo_list_free(&todos);
        return R_ERROR;
    }

    struct todo_metadata md;

    if(read_todo_metadata(file_path, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    if(write_todo_list(file_path, &todos, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    log_success(
        "Removed todo %llu: \"%s\"\n",
        id,
        removed_text
    );

    todo_list_free(&todos);

    return R_OK;
}

static int command_todo_prune(int argc, char *argv[]) {
    char file_path[DEFAULT_BUFFER_SIZE];

    if(get_base_dir_file_path(
        TODO_FILE,
        file_path,
        sizeof(file_path)
    ) != R_OK) {
        return R_ERROR;
    }

    if(create_file_if_not_exists(file_path) != R_OK) {
        return R_ERROR;
    }

    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(file_path, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    /*
     * First count how many completed todos would be removed.
     */
    size_t prune_count = 0;

    for(size_t i = 0; i < todos.count; ++i) {
        if(todos.items[i].status == DONE) {
            prune_count++;
        }
    }

    if(prune_count == 0) {
        log_info("No completed todos to prune.\n");
        todo_list_free(&todos);
        return R_OK;
    }

    /*
     * Do not modify anything unless explicitly confirmed.
     */
    if(!has_switch(argc, argv, "--force", false)) {
        log_warning(
            "Prune would permanently remove %zu completed todo%s.\n",
            prune_count,
            prune_count == 1 ? "" : "s"
        );

        log_info(
            "Run `%s todo prune --force` to continue.\n",
            APP_NAME
        );

        todo_list_free(&todos);
        return R_OK;
    }

    /*
     * Compact the array in place.
     *
     * read_index walks over all existing items.
     * write_index points to the next item we want to keep.
     */
    size_t write_index = 0;

    for(size_t read_index = 0;
        read_index < todos.count;
        ++read_index) {

        if(todos.items[read_index].status == DONE) {
            continue;
        }

        if(write_index != read_index) {
            todos.items[write_index] =
                todos.items[read_index];
        }

        write_index++;
    }

    todos.count = write_index;

    /*
     * Keep metadata, including last_id, unchanged.
     */
    struct todo_metadata md;

    if(read_todo_metadata(file_path, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    if(write_todo_list(file_path, &todos, &md) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    log_success(
        "Pruned %zu completed todo%s.\n",
        prune_count,
        prune_count == 1 ? "" : "s"
    );

    todo_list_free(&todos);

    return R_OK;
}

int command_todo(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("Please specify a todo command. Refer to --help if required.");
        return R_ERROR;
    }

    if(has_switch(argc, argv, "--help", true) || has_switch(argc, argv, "-h", true)) {
        printf("`%s todo` allows you to manage your personal todo list.\n", APP_NAME);
        printf("\n");
        printf("Subcommands:\n");
        printf("  %-16s %s\n", "add <text>",   "add a new todo item to the list");
        printf("  %-16s %s\n", "list",  "get a list of your todos that can be filtered");
        printf("  %-16s %s\n", "start <id>",  "move the item with the id in progress");
        printf("  %-16s %s\n", "done <id>",  "mark the item with the id as done");
        printf("  %-16s %s\n", "reopen <id>",  "move the item with the id back to open status");
        printf("  %-16s %s\n", "rewrite <id> <new_text>", "change the text of the todo with the given id");
        printf("  %-16s %s\n", "remove <id>", "permanently remove a todo item");
    printf("  %-16s %s\n", "prune", "remove all completed todo items");
        return R_OK;
    }
    
    return R_ERROR;
}

static const struct command_definition todo_commands[] = {
    {
        "add",
        "Add a new todo",
        command_todo_add,
        NULL,
        0,
        true
    },
    {
        "list",
        "List todos",
        command_todo_list,
        NULL,
        0,
        true
    },
    {
        "start",
        "Mark a todo as in progress",
        command_todo_start,
        NULL,
        0,
        true
    },
    {
        "done",
        "Mark a todo as completed",
        command_todo_done,
        NULL,
        0,
        true
    },
    {
        "reopen",
        "Reopen a todo",
        command_todo_reopen,
        NULL,
        0,
        true
    },
    {
        "rewrite",
        "Rewrite a todo",
        command_todo_rewrite,
        NULL,
        0,
        true
    },
    {
        "remove",
        "Remove a todo",
        command_todo_remove,
        NULL,
        0,
        true
    },
    {
        "prune",
        "Remove completed todos",
        command_todo_prune,
        NULL,
        0,
        true
    }
};

const struct command_definition* get_todo_commands(size_t* count) {
    if(count != NULL) {
        *count = sizeof(todo_commands) / sizeof(todo_commands[0]);
    }

    return todo_commands;
}