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

int create_note_from_markdown(const char *markdown, time_t created, struct note *item) {
    if(markdown == NULL || item == NULL) {
        return R_ERROR;
    }

    const char *topic_tag = strstr(markdown, "#shiori/topic/");
    const char *id_tag = strstr(markdown, "<!-- shiori:id=");

    item->created = created;
    item->topic[0] = '\0';
    item->id[0] = '\0';

    const char *metadata_start = NULL;

    if(topic_tag != NULL) {
        metadata_start = topic_tag;
    }

    if(id_tag != NULL && (metadata_start == NULL || id_tag < metadata_start)) {
        metadata_start = id_tag;
    }

    size_t text_len = metadata_start != NULL
        ? (size_t)(metadata_start - markdown)
        : strlen(markdown);

    while(text_len > 0 &&
          (markdown[text_len - 1] == ' ' ||
           markdown[text_len - 1] == '\t')) {
        text_len--;
    }

    if(text_len >= sizeof(item->text)) {
        log_error("Note text is too long.\n");
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
            log_error("Topic name is too long.\n");
            return R_ERROR;
        }

        memcpy(item->topic, topic, topic_len);
        item->topic[topic_len] = '\0';
    }

    if(id_tag != NULL) {
        const char *id = id_tag + strlen("<!-- shiori:id=");
        size_t id_len = 0;

        while(id[id_len] != '\0' &&
              id[id_len] != ' ' &&
              id[id_len] != '\t' &&
              id[id_len] != '\r' &&
              id[id_len] != '\n' &&
              id[id_len] != '>') {
            id_len++;
        }

        if(id_len == 0) {
            log_warning("Note contains an empty id.\n");
        } else if(id_len >= sizeof(item->id)) {
            log_error("Note id is too long.\n");
            return R_ERROR;
        } else {
            memcpy(item->id, id, id_len);
            item->id[id_len] = '\0';
        }
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
    struct notes_metadata md;
    if(read_notes_metadata(filename, &md) != R_OK) {
        log_critical("Failed reading NOTES metadata.\n");
        return R_ERROR;
    }

    if(md.version < NOTES_FORMAT_VERSION) {
        log_warning("`%s` is an old version of the NOTES format. Please run `%s util migrate`.\n", filename, APP_NAME);
    }

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

int read_notes_metadata(const char *filename, struct notes_metadata* md) {
    FILE *file = open_base_dir_file(filename, "r");
    if(file == NULL) {
        log_error("Failed opening %s.\n", filename);
        return R_ERROR;
    }

    md->version = 0;

    char line[DEFAULT_BUFFER_SIZE];

    // Front matter must start with ---
    if(fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return R_OK;
    }

    if(strcmp(trim(line), "---") != 0) {
        fclose(file);
        return R_OK;
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
        } else {
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