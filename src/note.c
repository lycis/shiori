#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "note.h"
#include "common.h"
#include "logging.h"
#include "cmd_shared.h"

void note_list_init(struct note_list *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void note_list_free(struct note_list *list)
{
    free(list->items);

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int note_list_add(
    struct note_list *list,
    const struct note *item
) {
    if(list->count == list->capacity) {
        size_t new_capacity =
            list->capacity == 0
            ? 8
            : list->capacity * 2;

        struct note *new_items = realloc(
            list->items,
            new_capacity * sizeof(struct note)
        );

        if(new_items == NULL) {
            log_error("Failed allocating note list.\n");
            return R_ERROR;
        }

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *item;
    list->count++;

    return R_OK;
}


int read_notes_for_date(const char *filename, const time_t date, struct note_list *list) {
    FILE *file = open_base_dir_file(filename, "r");
    if(file == NULL) {
        log_critical("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    // build the heading a
    char target_heading[32];
    if(build_daily_heading(target_heading, sizeof(target_heading), date) != R_OK) {
        fclose(file);
        return R_ERROR;
    }

    log_debug(
        "Looking for notes under heading '%s'.\n",
        target_heading
    );

    char line[DEFAULT_BUFFER_SIZE * 2];

    bool in_target_section = false;
    unsigned int line_number = 0;

    while(fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        char *current = trim(line);

        if(*current == '\0') {
            continue; // empty line
        }

        // heading starts a new section
        if(strncmp(current, "# ", 2) == 0) {
            if(strcmp(current, target_heading) == 0) {
                log_debug("Found note section for on line %u.\n", line_number);
                in_target_section = true;
                continue;
            }

           // If we were already reading the requested section, 
           // another heading means we're finished.
            if(in_target_section) {
                break;
            }

            continue;
        }

        // we did not find the requested date
        if(!in_target_section) {
            continue;
        }

        // Our notes are always markdown bullet lists with a asterisk leading
        if(strncmp(current, "* ", 2) != 0) {
            log_warning("Ignoring unexpected line %u in note section.\n", line_number);
            continue;
        }

        // skip over the leading bullet point preamble '* '
        current += 2;

        struct note item = {0};

        // notes in the block share the same creation date
        item.created = date;

        if(strcpy_s(item.text, sizeof(item.text), current) != 0) {
            log_error("Note on line %u is too long.\n", line_number);
            fclose(file);
            return R_ERROR;
        }

        if(note_list_add(list, &item) != R_OK) {
            fclose(file);
            return R_ERROR;
        }
    }

    if(ferror(file)) {
        log_error("Failed reading %s.\n", filename);
        fclose(file);
        return R_ERROR;
    }

    fclose(file);
    log_debug("Loaded %zu note%s for requested date.\n", list->count, list->count == 1 ? "" : "s");
    return R_OK;
}