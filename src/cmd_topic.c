#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli.h"
#include "cmd_shared.h"
#include "color.h"
#include "common.h"
#include "config.h"
#include "logging.h"
#include "note.h"
#include "todo.h"
#include "todo_list.h"

struct topic_count {
    char name[DEFAULT_BUFFER_SIZE];
    size_t note_count;
    size_t todo_count;
};

static int register_topic(
    const char *topic,
    bool is_todo,
    struct topic_count **topics,
    size_t *topic_count,
    size_t *topic_capacity
) {
    if(topic[0] == '\0') {
        return R_OK;
    }

    for(size_t i = 0; i < *topic_count; ++i) {
        if(strcmp((*topics)[i].name, topic) == 0) {
            if(is_todo) {
                (*topics)[i].todo_count++;
            } else {
                (*topics)[i].note_count++;
            }
            return R_OK;
        }
    }

    if(*topic_count == *topic_capacity) {
        size_t new_capacity;
        size_t allocation_size;
        if(calculate_array_growth(*topic_capacity, sizeof(**topics), &new_capacity, &allocation_size) != R_OK) {
            log_critical("Topic list is too large.\n");
            return R_ERROR;
        }

        struct topic_count *new_topics = realloc(*topics, allocation_size);
        if(new_topics == nullptr) {
            log_error("Failed allocating topic list.\n");
            return R_ERROR;
        }

        *topics = new_topics;
        *topic_capacity = new_capacity;
    }

    struct topic_count *entry = &(*topics)[*topic_count];
    memset(entry, 0, sizeof(*entry));
    if(strcpy_s(entry->name, sizeof(entry->name), topic) != 0) {
        log_error("Topic name is too long.\n");
        return R_ERROR;
    }

    if(is_todo) {
        entry->todo_count = 1;
    } else {
        entry->note_count = 1;
    }
    (*topic_count)++;
    return R_OK;
}

static int compare_topic_counts_by_name(const void *left, const void *right) {
    const struct topic_count *left_topic = left;
    const struct topic_count *right_topic = right;
    return strcmp(left_topic->name, right_topic->name);
}

static int command_topic_list() {
    struct note_list notes;
    note_list_init(&notes);
    if(read_notes(g_config.notes_filename, &notes) != R_OK) {
        note_list_free(&notes);
        return R_ERROR;
    }

    struct todo_list todos;
    todo_list_init(&todos);
    if(read_todos(g_config.todo_filename, &todos) != R_OK) {
        note_list_free(&notes);
        todo_list_free(&todos);
        return R_ERROR;
    }

    struct topic_count *topics = nullptr;
    size_t topic_count = 0;
    size_t topic_capacity = 0;

    for(size_t i = 0; i < notes.count; ++i) {
        if(register_topic(notes.items[i].topic, false, &topics, &topic_count, &topic_capacity) != R_OK) {
            free(topics);
            note_list_free(&notes);
            todo_list_free(&todos);
            return R_ERROR;
        }
    }

    for(size_t i = 0; i < todos.count; ++i) {
        if(register_topic(todos.items[i].topic, true, &topics, &topic_count, &topic_capacity) != R_OK) {
            free(topics);
            note_list_free(&notes);
            todo_list_free(&todos);
            return R_ERROR;
        }
    }

    note_list_free(&notes);
    todo_list_free(&todos);

    if(topic_capacity == 0) {
        log_info("No topics found.\n");
        free(topics);
        return R_OK;
    }

    qsort(topics, topic_count, sizeof(*topics), compare_topic_counts_by_name);

    printf(
        "%s%s🏷️ Topics%s\n",
        color_style_sequence(COLOR_STYLE_TOPIC),
        color_style_sequence(COLOR_STYLE_BOLD),
        color_style_sequence(COLOR_STYLE_RESET)
    );
    print_divider(60);

    for(size_t i = 0; i < topic_count; ++i) {
        size_t total = topics[i].note_count + topics[i].todo_count;
        printf(
            "  %-31s %3zu item%s  (%zu note%s, %zu todo%s)\n",
            topics[i].name,
            total,
            total == 1 ? "" : "s",
            topics[i].note_count,
            topics[i].note_count == 1 ? "" : "s",
            topics[i].todo_count,
            topics[i].todo_count == 1 ? "" : "s"
        );
    }

    free(topics);
    return R_OK;
}

int command_topic(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {

        printf(
            "Usage:\n"
            "  %s topic <name>\n"
            "\n"
            "Shows all notes and todos assigned to a topic.\n"
            "\n"
            "Options:\n"
            "  %-20s List all topics and their stats\n"
            "  %-20s Show this help\n"
            "\n",
            APP_NAME,
            "-l, --list",
            "-h, --help"
        );

        return R_OK;
    }

    bool list_topics = has_switch(argc, argv, "--list", false) || has_switch(argc, argv, "-l", false);

    if(list_topics) {
        return command_topic_list();
    }

    if(argc != 1) {
        log_error("You need to specify a topic.");
        return R_ERROR;
    }

    const char *topic = argv[0];

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(
        heading,
        "%s%s🏷️ Topic: %s",
        color_style_sequence(COLOR_STYLE_TOPIC),
        color_style_sequence(COLOR_STYLE_BOLD),
        topic
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
    if(read_notes(g_config.notes_filename, &list) != R_OK) {
        return R_ERROR;
    }

    time_t last_date = 0;
    bool have_last_date = false;
    for(size_t i = 0; i < list.count; ++i) {
        if(strcmp(list.items[i].topic, topic) != 0) {
            continue; // not a note of this topic
        }

        if(!have_last_date || !dates_equal(last_date, list.items[i].created)) {
            char date_heading[32];

            if(build_daily_heading(date_heading, sizeof(date_heading), list.items[i].created) != R_OK) {
                note_list_free(&list);
                return R_ERROR;
            }

            printf("%s", color_style_sequence(COLOR_STYLE_BOLD));
            printf("%s\n", date_heading);
            printf("%s", color_style_sequence(COLOR_STYLE_RESET));

            last_date = list.items[i].created;
            have_last_date = true;
        }

        printf(
            "%s   %s %s %s\n",
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
    if(read_todos(g_config.todo_filename, &todos) != R_OK) {
        todo_list_free(&todos);
        return R_ERROR;
    }

    for(size_t i = 0; i < todos.count; ++i) {
        if(strcmp(todos.items[i].topic, topic) != 0) {
            continue;
        }

        printf(
            "    %s %4llu  %s",
            todo_status_simple_icon(todos.items[i].status),
            todos.items[i].id,
            todos.items[i].text
        );

        print_todo_priority_suffix(&todos.items[i]);

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
