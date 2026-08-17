#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "color.h"
#include "common.h"
#include "note.h"
#include "logging.h"
#include "cli.h"

struct topic_count {
    char name[DEFAULT_BUFFER_SIZE];
    size_t count;
};

static int command_topic_list() {
    struct note_list notes;
    note_list_init(&notes);
    if(read_notes("NOTES.md", &notes) != R_OK) {
        note_list_free(&notes);
        return R_ERROR;
    }

    struct topic_count *topics = NULL;
    size_t topic_count = 0;
    size_t topic_capacity = 0;

    for(size_t i = 0; i < notes.count; ++i) {
        const char *topic = notes.items[i].topic;
        if(topic[0] == '\0') continue; // ignore notes witout topic for now

        bool found = false;
        for(size_t j = 0; j < topic_count; ++j) {
            if(strcmp(topics[j].name, topic) == 0) {
                topics[j].count++;
                found = true;
                break;
            }
        }

        if(found) {
            continue;
        }

        // new topic to register
        if(topic_count == topic_capacity) {
            size_t new_capacity =
                topic_capacity == 0
                    ? 8
                    : topic_capacity * 2;

            struct topic_count *new_topics = realloc(
                topics,
                new_capacity * sizeof(struct topic_count)
            );

            if(new_topics == NULL) {
                log_error("Failed allocating topic list.\n");
                free(topics);
                note_list_free(&notes);
                return R_ERROR;
            }

            topics = new_topics;
            topic_capacity = new_capacity;
        }

        struct topic_count *entry = &topics[topic_count];

        if(strcpy_s(
            entry->name,
            sizeof(entry->name),
            topic
        ) != 0) {
            log_error("Topic name is too long.\n");
            free(topics);
            note_list_free(&notes);
            return R_ERROR;
        }

        entry->count = 1;
        topic_count++;
    }

    if(topic_capacity == 0) {
        log_info("No topics found.\n");
        free(topics);
        note_list_free(&notes);
        return R_OK;
    }

     printf("%s%s🏷️ Topics%s\n", COLOR_TOPIC, ANSI_BOLD,ANSI_RESET);
    print_divider(60);

    for(size_t i = 0; i < topic_count; ++i) {
        printf("  %-45s %zu note%s\n", topics[i].name, topics[i].count,topics[i].count == 1 ? "" : "s");
    }

    free(topics);
    note_list_free(&notes);
    return R_OK;
}

int command_topic(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) ||
       has_switch(argc, argv, "-h", false)) {

        printf(
            "Usage:\n"
            "  %s topic <name>\n"
            "\n"
            "Shows all notes assigned to a topic.\n"
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

    bool list_topics =
    has_switch(argc, argv, "--list", false) ||
    has_switch(argc, argv, "-l", false);

    if(list_topics) {
        return command_topic_list();
    }

    if(argc != 1) {
        log_error("You need to specify a topic.");
        return R_ERROR;
    }

    const char *topic = argv[0];

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(heading, COLOR_TOPIC ANSI_BOLD "🏷️ Topic: %s", topic);
    printf("%s\n", heading);
    print_divider(60);
    printf("\n");

    struct note_list list;
    note_list_init(&list);
    if(read_notes("NOTES.md", &list) != R_OK) {
        return R_ERROR;
    }

    if(list.count < 1) {
        log_info("No notes found for this topic.");
        note_list_free(&list);
        return R_OK;
    }

    time_t last_date = 0;
    bool have_last_date = false;
    for(size_t i = 0; i < list.count; ++i) {
        if(strcmp(list.items[i].topic, topic) != 0) continue; // not a note of this topic

        if(!have_last_date || !dates_equal(last_date, list.items[i].created)) {
            char date_heading[32];

            if(build_daily_heading(date_heading, sizeof(date_heading), list.items[i].created) != R_OK) {
                note_list_free(&list);
                return R_ERROR;
            }

            printf(ANSI_BOLD);
            printf("%s\n", date_heading);
            printf(ANSI_RESET);

            last_date = list.items[i].created;
            have_last_date = true;
        }

        printf("  • %s\n", list.items[i].text);
    }

    printf("\n");
    print_divider(60);

    note_list_free(&list);
    return R_OK;
}