#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "cmd_shared.h"
#include "color.h"
#include "common.h"
#include "logging.h"
#include "note.h"
#include "todo.h"
#include "todo_list.h"

bool text_has_tag(const char *text, const char *tag) {
    if(text == NULL || tag == NULL || *tag == '\0') {
        return false;
    }

    char needle[DEFAULT_BUFFER_SIZE];

    int written = snprintf(needle, sizeof(needle), "#%s", tag);

    if(written < 0 || (size_t)written >= sizeof(needle)) {
        return false;
    }

    size_t needle_len = strlen(needle);
    const char *current = text;

    while((current = strstr(current, needle)) != NULL) {
        /*
         * A tag must start at the beginning of the text
         * or after whitespace.
         */
        bool valid_before = current == text || isspace((unsigned char)current[-1]);

        char after = current[needle_len];

        /*
         * Prevent #work from matching #workshop.
         */
        bool valid_after = after == '\0' || isspace((unsigned char)after) || ispunct((unsigned char)after);

        if(valid_before && valid_after) {
            return true;
        }

        current += needle_len;
    }

    return false;
}

bool note_has_tag(const struct note *item, const char *tag) {
    return text_has_tag(item->text, tag);
}

bool todo_has_tag(const struct todo *item, const char *tag) {
    return text_has_tag(item->text, tag);
}

struct tag_count {
    char name[DEFAULT_BUFFER_SIZE];
    size_t note_count;
    size_t todo_count;
    size_t last_item;
    bool last_was_todo;
    bool has_last_item;
};

static bool is_tag_trailing_punctuation(char value) {
    return value == '.' || value == ',' || value == ';' || value == ':' || value == '!' || value == '?' ||
           value == ')' || value == ']' || value == '}' || value == '\'' || value == '"';
}

static struct tag_count *find_tag_count(struct tag_count *tags, size_t count, const char *name) {
    for(size_t i = 0; i < count; ++i) {
        if(strcmp(tags[i].name, name) == 0) {
            return &tags[i];
        }
    }

    return NULL;
}

static int count_tags_in_text(
    const char *text,
    bool is_todo,
    size_t item_index,
    struct tag_count **tags,
    size_t *tag_count,
    size_t *tag_capacity
) {
    const char *current = text;

    while(*current != '\0') {
        while(isspace((unsigned char)*current)) {
            current++;
        }

        const char *token = current;

        while(*current != '\0' && !isspace((unsigned char)*current)) {
            current++;
        }

        size_t length = (size_t)(current - token);

        while(length > 1 && is_tag_trailing_punctuation(token[length - 1])) {
            length--;
        }

        if(length <= 1 || token[0] != '#' || (length >= 8 && strncmp(token, "#shiori/", 8) == 0)) {
            continue;
        }

        if(length > DEFAULT_BUFFER_SIZE) {
            log_error("Tag name is too long.\n");
            return R_ERROR;
        }

        char name[DEFAULT_BUFFER_SIZE];
        memcpy(name, token + 1, length - 1);
        name[length - 1] = '\0';

        struct tag_count *entry = find_tag_count(*tags, *tag_count, name);

        if(entry == NULL) {
            if(*tag_count == *tag_capacity) {
                size_t new_capacity = *tag_capacity == 0 ? 8 : *tag_capacity * 2;
                struct tag_count *new_tags = realloc(*tags, new_capacity * sizeof(struct tag_count));

                if(new_tags == NULL) {
                    log_error("Failed allocating tag list.\n");
                    return R_ERROR;
                }

                *tags = new_tags;
                *tag_capacity = new_capacity;
            }

            entry = &(*tags)[*tag_count];
            memset(entry, 0, sizeof(*entry));

            if(strcpy_s(entry->name, sizeof(entry->name), name) != 0) {
                log_error("Tag name is too long.\n");
                return R_ERROR;
            }

            (*tag_count)++;
        }

        if(entry->has_last_item && entry->last_was_todo == is_todo && entry->last_item == item_index) {
            continue;
        }

        if(is_todo) {
            entry->todo_count++;
        } else {
            entry->note_count++;
        }

        entry->last_item = item_index;
        entry->last_was_todo = is_todo;
        entry->has_last_item = true;
    }

    return R_OK;
}

static int compare_tag_counts_by_name(const void *left, const void *right) {
    const struct tag_count *left_tag = left;
    const struct tag_count *right_tag = right;

    return strcmp(left_tag->name, right_tag->name);
}

static int command_tag_list() {
    struct note_list notes;
    note_list_init(&notes);

    if(read_notes(NOTES_FILE, &notes) != R_OK) {
        note_list_free(&notes);
        return R_ERROR;
    }

    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(TODO_FILE, &todos) != R_OK) {
        note_list_free(&notes);
        todo_list_free(&todos);
        return R_ERROR;
    }

    struct tag_count *tags = NULL;
    size_t tag_count = 0;
    size_t tag_capacity = 0;

    for(size_t i = 0; i < notes.count; ++i) {
        if(count_tags_in_text(notes.items[i].text, false, i, &tags, &tag_count, &tag_capacity) != R_OK) {
            free(tags);
            note_list_free(&notes);
            todo_list_free(&todos);
            return R_ERROR;
        }
    }

    for(size_t i = 0; i < todos.count; ++i) {
        if(count_tags_in_text(todos.items[i].text, true, i, &tags, &tag_count, &tag_capacity) != R_OK) {
            free(tags);
            note_list_free(&notes);
            todo_list_free(&todos);
            return R_ERROR;
        }
    }

    note_list_free(&notes);
    todo_list_free(&todos);

    if(tag_count == 0) {
        log_info("No tags found.\n");
        free(tags);
        return R_OK;
    }

    qsort(tags, tag_count, sizeof(struct tag_count), compare_tag_counts_by_name);

    printf(
        "%s%s🏷️ Tags%s\n",
        color_style_sequence(COLOR_STYLE_TOPIC),
        color_style_sequence(COLOR_STYLE_BOLD),
        color_style_sequence(COLOR_STYLE_RESET)
    );
    print_divider(60);

    for(size_t i = 0; i < tag_count; ++i) {
        size_t total = tags[i].note_count + tags[i].todo_count;

        printf(
            "  %s#%-31s%s %3zu item%s  (%zu note%s, %zu todo%s)\n",
            color_style_sequence(COLOR_STYLE_TOPIC),
            tags[i].name,
            color_style_sequence(COLOR_STYLE_RESET),
            total,
            total == 1 ? "" : "s",
            tags[i].note_count,
            tags[i].note_count == 1 ? "" : "s",
            tags[i].todo_count,
            tags[i].todo_count == 1 ? "" : "s"
        );
    }

    free(tags);
    return R_OK;
}

int command_tag(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
        printf(
            "Usage:\n"
            "  %s tag <tag> [tag...]\n"
            "\n"
            "Shows all notes and todos containing all specified tags.\n"
            "\n"
            "Options:\n"
            "  %-22s List all tags and their usage counts\n"
            "  %-22s Show this help\n"
            "\n"
            "Examples:\n"
            "  %s tag decision\n"
            "  %s tag decision followup\n",
            APP_NAME,
            "-l, --list",
            "-h, --help",
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    if(has_switch(argc, argv, "--list", false) || has_switch(argc, argv, "-l", false)) {
        return command_tag_list();
    }

    if(argc < 1) {
        log_error("You need to specify at least one tag.");
        return R_ERROR;
    }

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(
        heading,
        "%s%s🏷️ Tag: %s",
        color_style_sequence(COLOR_STYLE_TOPIC),
        color_style_sequence(COLOR_STYLE_BOLD),
        argv[0]
    );
    printf("%s\n", heading);
    print_divider(60);
    printf("\n");

    printf(
        "  %s%s%s%s\n",
        color_style_sequence(COLOR_STYLE_BOLD),
        color_style_sequence(COLOR_STYLE_NOTES),
        "🗒️ Notes",
        color_style_sequence(COLOR_STYLE_RESET)
    );
    struct note_list list;
    note_list_init(&list);
    if(read_notes(NOTES_FILE, &list) != R_OK) {
        return R_ERROR;
    }

    if(list.count < 1) {
        log_info("No notes found for this tag.");
        note_list_free(&list);
        return R_OK;
    }

    time_t last_date = 0;
    bool have_last_date = false;
    for(size_t i = 0; i < list.count; ++i) {
        bool matches_all = true;

        for(int t = 0; t < argc; ++t) {
            if(!note_has_tag(&list.items[i], argv[t])) {
                matches_all = false;
                break;
            }
        }

        if(!matches_all) {
            continue;
        }

        if(!have_last_date || !dates_equal(last_date, list.items[i].created)) {
            char date_heading[32];

            if(build_daily_heading(date_heading, sizeof(date_heading), list.items[i].created) != R_OK) {
                note_list_free(&list);
                return R_ERROR;
            }

            printf("%s", color_style_sequence(COLOR_STYLE_BOLD));
            printf("  %s\n", date_heading);
            printf("%s", color_style_sequence(COLOR_STYLE_RESET));

            last_date = list.items[i].created;
            have_last_date = true;
        }

        printf(
            "%s    %s %s%s\n",
            color_style_sequence(COLOR_STYLE_NOTE_ID),
            list.items[i].id,
            color_style_sequence(COLOR_STYLE_RESET),
            list.items[i].text
        );
    }

    note_list_free(&list);

    printf("\n");
    print_divider(60);
    printf("\n");

    printf("  %s%s%s\n", color_style_sequence(COLOR_STYLE_TODOS), "📌 Todos", color_style_sequence(COLOR_STYLE_RESET));

    struct todo_list todos;
    todo_list_init(&todos);

    if(read_todos(TODO_FILE, &todos) != R_OK) {
        log_critical("Failed reading todo list.\n");
        todo_list_free(&todos);
        return R_ERROR;
    }

    for(size_t i = 0; i < todos.count; ++i) {
        bool matches_all = true;

        for(int t = 0; t < argc; ++t) {
            if(!todo_has_tag(&todos.items[i], argv[t])) {
                matches_all = false;
                break;
            }
        }

        if(!matches_all) {
            continue;
        }

        printf(
            "    %s %4llu  %s",
            todo_status_simple_icon(todos.items[i].status),
            todos.items[i].id,
            todos.items[i].text
        );

        if(todos.items[i].due != 0) {
            char due_date[11];

            if(format_date(todos.items[i].due, due_date, sizeof(due_date)) != R_OK) {
                todo_list_free(&todos);
                return R_ERROR;
            }

            printf(
                "  %s📅 %s%s",
                color_style_sequence(COLOR_STYLE_DUE_DATE),
                due_date,
                color_style_sequence(COLOR_STYLE_RESET)
            );
        }

        printf("\n");
    }

    todo_list_free(&todos);
    return R_OK;
}
