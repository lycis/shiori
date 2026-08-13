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

static int create_note_from_markdown(const char *markdown, time_t created,struct note *item) {
    const char *topic_tag = strstr(markdown, "#shiori/topic/");

    item->created = created;
    item->topic[0] = '\0';

    size_t text_len = topic_tag != NULL
        ? (size_t)(topic_tag - markdown)
        : strlen(markdown);

    while(text_len > 0 &&
          (markdown[text_len - 1] == ' ' ||
           markdown[text_len - 1] == '\t')) {
        text_len--;
    }

    if(text_len >= sizeof(item->text)) {
        log_error("Note text is too long.");
        return R_ERROR;
    }

    memcpy(item->text, markdown, text_len);
    item->text[text_len] = '\0';

    if(topic_tag != NULL) {
        const char *topic = topic_tag + strlen("#shiori/topic/");

        size_t topic_len = 0;

        while(topic[topic_len] != '\0' &&
              topic[topic_len] != ' ' &&
              topic[topic_len] != '\t' &&
              topic[topic_len] != '\r' &&
              topic[topic_len] != '\n') {
            topic_len++;
        }

        if(topic_len >= sizeof(item->topic)) {
            log_error("Topic name is too long.");
            return R_ERROR;
        }

        memcpy(item->topic, topic, topic_len);
        item->topic[topic_len] = '\0';
    } else {
        memset(item->topic, 0, 1); // set topic to nothing
    }

    return R_OK;
}

static int parse_daily_heading(const char *heading, time_t *date) {
    if(heading == NULL || date == NULL) {
        return R_ERROR;
    }

    int year;
    int month;
    int day;

    if(sscanf_s(heading, "# %d-%d-%d", &year, &month, &day) != 3) {
        return R_ERROR;
    }

    struct tm parsed = {0};

    parsed.tm_year = year - 1900;
    parsed.tm_mon = month - 1;
    parsed.tm_mday = day;
    parsed.tm_hour = 12;
    parsed.tm_isdst = -1;

    time_t result = mktime(&parsed);

    if(result == (time_t)-1) {
        return R_ERROR;
    }

    *date = result;
    return R_OK;
}
 
int read_notes(const char *filename, struct note_list *list) {
    FILE *file = open_base_dir_file(filename, "r");
    if(file == NULL) {
        log_critical("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE * 2];
    unsigned int line_number = 0;

    time_t current_date = 0;
    bool have_date = false;

    while(fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        char *current = trim(line);

        if(*current == '\0') {
            continue;
        }

         // A heading starts a new daily section.
        if(strncmp(current, "# ", 2) == 0) {
            if(parse_daily_heading(current, &current_date) != R_OK) {
                log_warning(
                    "Ignoring invalid note heading on line %u.\n",
                    line_number
                );

                have_date = false;
                continue;
            }

            have_date = true;
            continue;
        }

        // Ignore content outside a valid daily section.
        if(!have_date) {
            continue;
        }

        // Notes are Markdown bullet list items.
        if(strncmp(current, "* ", 2) != 0) {
            log_warning(
                "Ignoring unexpected line %u in note section.\n",
                line_number
            );
            continue;
        }

        current += 2;

        struct note item = {0};

        if(create_note_from_markdown(
            current,
            current_date,
            &item
        ) != R_OK) {
            log_error(
                "Failed parsing note on line %u.\n",
                line_number
            );

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

    log_debug(
        "Loaded %zu note%s.\n",
        list->count,
        list->count == 1 ? "" : "s"
    );

    return R_OK;
}

int read_notes_for_date(const char *filename, const time_t date, struct note_list *list) {
    struct note_list all_notes;
    note_list_init(&all_notes);

    if(read_notes(filename, &all_notes) != R_OK) {
        note_list_free(&all_notes);
        return R_ERROR;
    }

    for(size_t i = 0; i < all_notes.count; ++i) {
        if(!dates_equal(all_notes.items[i].created, date)) {
            continue;
        }

        if(note_list_add(list, &all_notes.items[i]) != R_OK) {
            note_list_free(&all_notes);
            return R_ERROR;
        }
    }

    note_list_free(&all_notes);

    log_debug("Loaded %zu note%s for requested date.\n", list->count, list->count == 1 ? "" : "s");

    return R_OK;
}