#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "logging.h"
#include "platform.h"
#include "config.h"
#include "todo.h"
#include "todo_list.h"
#include "cmd_shared.h"


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

int get_base_dir_filepath(const char *filename, char* buffer, size_t bufferSize) {
    int written = snprintf(buffer, bufferSize, "%s%s%s", g_config.base_dir, get_path_separator(), filename);
    if(written < 0 || written >= (int)bufferSize) {
        log_error("File path too long.\n");
        return R_ERROR;
    }
    return R_OK;
}

FILE* open_base_dir_file(const char *filename, const char* mode) {
    char file_path[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_filepath(filename, file_path, sizeof(file_path)) != R_OK) {
        return NULL;
    }

    if(create_file_if_not_exists(file_path) != R_OK) {
        return NULL;
    }

    FILE *source = NULL;

    int err = file_open_utf8(&source, file_path, mode);
    if(err != 0 || source == NULL) {
        log_error("Failed opening %s.\n", filename);
        return NULL;
    }

    return source;
}

FILE* open_notes_file(const char* mode) {
    return open_base_dir_file(NOTES_FILE, mode);
}

int add_markdown_item(int argc, char *argv[], const char *filename, const char *prefix, const char *heading) {
    log_debug("Writing markdown item (c=%d)\n", argc);
    FILE* source = open_base_dir_file(filename, "r");
    if(source == NULL) {
        log_critical("failed to open base dir file %s\n", filename);
        return R_ERROR;
    }

    /*
     * No heading means that we don't need to insert anything
     * into the middle of the file. We can simply append.
     */
    if(heading == NULL) {
        fclose(source);
        FILE *file = open_base_dir_file(filename, "a");
        write_item_to_file(argc, argv, file, prefix);
        fclose(file);
        return R_OK;
    }

    /*
     * With a heading we need to potentially insert into an
     * existing block, so use the temp-file strategy.
     */  
    char file_path[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_filepath(filename, file_path, sizeof(file_path)) != R_OK) {
        fclose(source);
        return R_ERROR;
    }

    char temp_path[DEFAULT_BUFFER_SIZE];
    int written = snprintf(temp_path, sizeof(temp_path), "%s.tmp", filename);
    if(written < 0 || written >= (int)sizeof(temp_path)) {
        log_error("Temporary file path too long.\n");
        fclose(source);
        return R_ERROR;
    }

    FILE *temp = open_base_dir_file(temp_path, "w");
    if(temp == NULL) {
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

    if(file_access_utf8(backup_path, F_OK) == 0) {
        if(file_remove_utf8(backup_path) != 0) {
            log_error(
                "Could not remove previous backup: %s\n",
                backup_path
            );
            return R_ERROR;
        }
    }

    if(file_rename_utf8(file_path, backup_path) != 0) {
        log_error("Failed to create backup of %s.\n", filename);
        return R_ERROR;
    }

    char temp_path_base_dir[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_filepath(temp_path, temp_path_base_dir, sizeof(temp_path_base_dir)) != R_OK) {
        log_critical("Error when creating temp restore path.\n");
        return R_ERROR;
    }

    if(file_rename_utf8(temp_path_base_dir, file_path) != 0) {
        log_error("Failed replacing %s.\n", filename);

        if(file_rename_utf8(backup_path, file_path) != 0) {
            log_critical(
                "Failed restoring %s from backup.\n",
                filename
            );
        }

        return R_ERROR;
    }

    if(strcmp(filename, NOTES_FILE) != 0 && file_remove_utf8(backup_path) != 0) {
        log_warning("Failed to remove backup: %s\n", backup_path);
    }

    return R_OK;
}

static int create_todo_from_markdown(const char *markdown, struct todo *item) {
    if(strncmp(markdown, "* [", 3) != 0) {
        log_error("Invalid markdown line passed.\n");
        return R_ERROR;
    }

    markdown += 3;

    if(markdown[0] == ' ') {
        item->status = OPEN;
    }
    else if(markdown[0] == 'X' || markdown[0] == 'x') {
        item->status = DONE;
    }
    else if(markdown[0] == '/') {
        item->status = IN_PROGRESS;
    }
    else {
        log_error("Invalid TODO status '%c'.\n", markdown[0]);
        return R_ERROR;
    }

    if(markdown[1] != ']') {
        log_error("Malformed TODO checkbox.\n");
        return R_ERROR;
    }

    markdown += 2;

    if(markdown[0] == ' ') {
        markdown++;
    }

    const char *id_tag      = strstr(markdown, "#shiori/id/");
    const char *created_tag = strstr(markdown, "#shiori/created/");
    const char *due_tag     = strstr(markdown, "#shiori/due/");

    if(id_tag == NULL || created_tag == NULL) {
        log_error("Missing TODO metadata (id or creation date).\n");
        return R_ERROR;
    }

    if(created_tag < id_tag) {
        log_error("Invalid TODO metadata order (creation before id).\n");
        return R_ERROR;
    }

    /*
     * Extract visible todo text.
     */
    size_t text_len = (size_t)(id_tag - markdown);

    while(text_len > 0 &&
          isspace((unsigned char)markdown[text_len - 1])) {
        text_len--;
    }

    if(text_len >= sizeof(item->text)) {
        log_error("TODO text is too long.\n");
        return R_ERROR;
    }

    memcpy(item->text, markdown, text_len);
    item->text[text_len] = '\0';

    /*
     * ID
     */
    const char *id_value = id_tag + strlen("#shiori/id/");

    char *id_end = NULL;

    item->id = strtoull(id_value, &id_end, 10);

    if(id_end == id_value) {
        log_error("Invalid TODO ID.\n");
        return R_ERROR;
    }

    if(*id_end != '\0' &&
       !isspace((unsigned char)*id_end)) {
        log_error("Invalid TODO ID.\n");
        return R_ERROR;
    }

    /*
     * Creation date
     */
    const char *created_value = created_tag + strlen("#shiori/created/");

    char created_date[11];

    if(strlen(created_value) < 10) {
        log_error("Invalid TODO creation date.\n");
        return R_ERROR;
    }

    memcpy(created_date, created_value, 10);

    created_date[10] = '\0';

    if(parse_date_arg(created_date, &item->created) != R_OK) {
        log_error("Invalid TODO creation date '%s'.\n", created_date);
        return R_ERROR;
    }

    /*
     * Due date is optional.
     */
    item->due = 0;

    if(due_tag != NULL) {
        const char *due_value = due_tag + strlen("#shiori/due/");

        char due_date[11];

        if(strlen(due_value) < 10) {
            log_error("Invalid TODO due date.\n");
            return R_ERROR;
        }

        memcpy(due_date, due_value, 10);

        due_date[10] = '\0';

        if(parse_date_arg(due_date, &item->due) != R_OK) {
            log_error("Invalid TODO due date '%s'.\n", due_date);
            return R_ERROR;
        }
    }

    return R_OK;
}


int read_todos(const char *filename, struct todo_list *list) {
    FILE *file = open_base_dir_file(filename, "r");
    if(file == NULL) {
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

int parse_date_arg(const char *value, time_t *result) {
    time_t now = time(NULL);

    struct tm date;

    if(localtime_s(&date, &now) != 0) {
        log_error("Failed getting local date.\n");
        return R_ERROR;
    }

    // Normalize to noon so date arithmetic is less likely to stumble over DST boundaries.
    date.tm_hour = 12;
    date.tm_min = 0;
    date.tm_sec = 0;
    date.tm_isdst = -1;

    if(strcmp(value, "today") == 0) {
        *result = mktime(&date);
    }
    else if(strcmp(value, "yesterday") == 0) {
        date.tm_mday -= 1;
        *result = mktime(&date);
    }
    else if(strcmp(value, "tomorrow") == 0) {
        date.tm_mday += 1;
        *result = mktime(&date);
    }
    else {
        struct tm parsed = {0};

        if(sscanf_s(
            value,
            "%d-%d-%d",
            &parsed.tm_year,
            &parsed.tm_mon,
            &parsed.tm_mday
        ) != 3) {
            log_error(
                "Invalid date '%s'. Use YYYY-MM-DD, today, yesterday or tomorrow.\n",
                value
            );
            return R_ERROR;
        }

        parsed.tm_year -= 1900;
        parsed.tm_mon -= 1;
        parsed.tm_hour = 12;
        parsed.tm_isdst = -1;

        *result = mktime(&parsed);
    }

    if(*result == (time_t)-1) {
        log_error("Failed converting date '%s'.\n", value);
        return R_ERROR;
    }

    return R_OK;
}
