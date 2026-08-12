#include <corecrt.h>
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

    if(write_todo(TODO_FILE, &item) != R_OK) {
        return R_ERROR;
    }

    log_success("Task noted.\n");
    return R_OK;
}

static int create_todo_from_markdown(const char* markdown, struct todo *item) {
    if(strncmp(markdown, "* [", 3) != 0) {
        log_error("Invalid markdown line passed");
        return R_ERROR;
    }

    markdown += 3;

    if(markdown[0] == ' ') item->status = OPEN;
    else if(markdown[0] == 'X' || markdown[0] == 'x') item->status = DONE;
    else if(markdown[0] == '/') item->status = IN_PROGRESS;
    else {
        log_error("Invalid TODO status '%s'\n", markdown[0]);
        return R_ERROR;
    }

    if(markdown[1] != ']') {
        log_error("Malformed TODO checkbox.\n");
        return R_ERROR;
    }
    markdown += 2;

    if(markdown[0] == ' ') markdown++; // there may be a space after the closing bracket

    const char *id_tag = strstr(markdown, "#shiori/id/");
    const char *created_tag = strstr(markdown, "#shiori/created/");

    if(id_tag == NULL || created_tag == NULL) {
        log_error("Missing TODO metadata (id or creation date).\n");
        return R_ERROR;
    }

    if(created_tag < id_tag) {
        log_error("Invalid TODO metadata order (creation before id).\n");
        return R_ERROR;
    }

    // now let's extract the text
    size_t text_len = (size_t)(id_tag - markdown);

    // remove white spaces before the tags
    while(text_len > 0 && isspace((unsigned char)markdown[text_len - 1])) {
        text_len--;
    }

    if(text_len >= sizeof(item->text)) {
        log_error("TODO text is too long.\n");
        return R_ERROR;
    }

    memcpy(item->text, markdown, text_len);
    item->text[text_len] = '\0';

    // next extract the ID tag
    const char *id_value = id_tag + strlen("#shiori/id/");
    char *id_end = NULL;

    item->id = strtoull(id_value, &id_end, 10);

    if(id_end == id_value) {
        log_error("Invalid TODO ID.\n");
        return R_ERROR;
    }

    if(*id_end != '\0' && !isspace((unsigned char)*id_end)) {
        log_error("Invalid TODO ID.\n");
        return R_ERROR;
    }

    // now extract the creation date
    const char *created_value = created_tag + strlen("#shiori/created/");
    char date_str[11];

    if(strlen(created_value) < 10) {
        log_error("Invalid TODO creation date.\n");
        return R_ERROR;
    }

    memcpy(date_str, created_value, 10);
    date_str[10] = '\0';

    struct tm created = {0};

    if(sscanf_s(
        date_str,
        "%d-%d-%d",
        &created.tm_year,
        &created.tm_mon,
        &created.tm_mday
    ) != 3) {
        log_error("Invalid TODO creation date.\n");
        return R_ERROR;
    }

    created.tm_year -= 1900;
    created.tm_mon -= 1;
    created.tm_hour = 12;
    created.tm_isdst = -1;

    item->created = mktime(&created);

    if(item->created == (time_t)-1) {
        log_error("Failed converting TODO creation date.\n");
        return R_ERROR;
    }

    return R_OK;
}

static int read_todos(const char *filename, struct todo_list *list) {
    FILE *file = NULL;

    errno_t err = fopen_s(&file, filename, "r");
    if(err != 0 || file == NULL) {
        log_error("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE];
    unsigned int lnr = 0;
    bool in_metadata = false;
    bool metadata_skipped = false;
    while(fgets(line, sizeof(line), file) != NULL) {
        lnr++;

        // skip meta data section
        if(strcmp(trim(line), "---") == 0) {
            if(metadata_skipped) {
                log_critical("Corrupted TODOS.md (line %d): Invalid ---\n", lnr);
                fclose(file);
                return R_ERROR;
            }
            if(in_metadata) {
                in_metadata = false;
                metadata_skipped = true;
            } else {
                in_metadata = true;
            }

            continue;
        }

        if(in_metadata) continue;
        if(strlen(trim(line)) == 0) {
            continue;
        }

        // now we are in the real todo data :)
        if(strncmp(line, "* [", 3) != 0) {
            log_critical("Corrupted TODOS.md (line %d): Invalid TODO item format\n", lnr);
            fclose(file);
            return R_ERROR;
        }

        struct todo current_item;
        if(create_todo_from_markdown(line, &current_item) != R_OK) {
            fclose(file);
            log_critical("Corrupted TODOS.md (line %d): See above.\n", lnr);
            return R_ERROR;
        }

        if(todo_list_add(list, &current_item) != R_OK) {
            fclose(file);
            return R_ERROR;
        }
    }

    fclose(file);
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
    char file_path[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_file_path(TODO_FILE, file_path, sizeof(file_path)) != R_OK) {
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
            printf(
                "%s %-4llu %-40s 📅 %s\n",
                todo_status_icon(item->status),
                item->id,
                item->text,
                date
            );
        }
    }

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
        return R_OK;
    }

    char* command = argv[0];
    argc--;
    argv++;

    if(strcmp(command, "add") == 0) {
        log_debug("Running `todo add` command.\n");
        return command_todo_add(argc, argv);
    } else if(strcmp(command, "list") == 0) {
        return command_todo_list(argc, argv);
    } else {
        log_error("Invalid `todo` command. See --help for reference.");
        return R_ERROR;
    }

    return R_OK;
}
