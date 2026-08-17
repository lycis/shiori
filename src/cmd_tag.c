#include <stdio.h>
#include "cmd_shared.h"
#include "common.h"
#include "note.h"
#include "logging.h"
#include "cli.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "todo.h"
#include "todo_list.h"

bool text_has_tag(const char *text, const char *tag)
{
    if(text == NULL || tag == NULL || *tag == '\0') {
        return false;
    }

    char needle[DEFAULT_BUFFER_SIZE];

    int written = snprintf(
        needle,
        sizeof(needle),
        "#%s",
        tag
    );

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
        bool valid_before =
            current == text ||
            isspace((unsigned char)current[-1]);

        char after = current[needle_len];

        /*
         * Prevent #work from matching #workshop.
         */
        bool valid_after =
            after == '\0' ||
            isspace((unsigned char)after) ||
            ispunct((unsigned char)after);

        if(valid_before && valid_after) {
            return true;
        }

        current += needle_len;
    }

    return false;
}

bool note_has_tag(const struct note *item, const char *tag)
{
    return text_has_tag(item->text, tag);
}

bool todo_has_tag(const struct todo *item, const char *tag)
{
    return text_has_tag(item->text, tag);
}

int command_tag(int argc, char* argv[]) {
    if(has_switch(argc, argv, "--help", false) ||
       has_switch(argc, argv, "-h", false)) {
        printf(
            "Usage:\n"
            "  %s tag <tag> [tag...]\n"
            "\n"
            "Shows all notes containing all specified tags.\n"
            "\n"
            "Options:\n"
            "  %-22s Show this help\n"
            "\n"
            "Examples:\n"
            "  %s tag decision\n"
            "  %s tag decision followup\n",
            APP_NAME,
            "-h, --help",
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    if(argc < 1) {
        log_error("You need to specify at least one tag.");
        return R_ERROR;
    }

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(heading, COLOR_TAG ANSI_BOLD "🏷️ Tag: %s", argv[0]);
    printf("%s\n", heading);
    print_divider(60);
    printf("\n");

    printf("  %s%s%s\n", ANSI_BOLD ANSI_BOLD COLOR_NOTES, "🗒️ Notes", ANSI_RESET);
    struct note_list list;
    note_list_init(&list);
    if(read_notes("NOTES.md", &list) != R_OK) {
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

            printf(ANSI_BOLD);
            printf("  %s\n", date_heading);
            printf(ANSI_RESET);

            last_date = list.items[i].created;
            have_last_date = true;
        }

        printf("    • %s\n", list.items[i].text);
    }

    note_list_free(&list);

    printf("\n");
    print_divider(60);
    printf("\n");

    printf("  %s%s%s\n", COLOR_TODOS, "📌 Todos", ANSI_RESET);
    
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

            if(format_date(
                todos.items[i].due,
                due_date,
                sizeof(due_date)
            ) != R_OK) {
                todo_list_free(&todos);
                return R_ERROR;
            }

            printf(
                "  %s📅 %s%s",
                COLOR_DUE_DATE,
                due_date,
                ANSI_RESET
            );
        }

        printf("\n");
    }

    todo_list_free(&todos);
    return R_OK;
}