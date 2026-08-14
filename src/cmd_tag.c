#include <stdio.h>
#include "common.h"
#include "note.h"
#include "logging.h"
#include "cli.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

bool note_has_tag(const struct note *note, const char *tag) {
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

    const char *current = note->text;

    while((current = strstr(current, needle)) != NULL) {
        char before = current == note->text
            ? '\0'
            : current[-1];

        char after = current[strlen(needle)];

        bool valid_before =
            current == note->text ||
            isspace((unsigned char)before);

        bool valid_after =
            after == '\0' ||
            isspace((unsigned char)after) ||
            ispunct((unsigned char)after);

        if(valid_before && valid_after) {
            return true;
        }

        current += strlen(needle);
    }

    return false;
}

int command_tag(int argc, char* argv[]) {
    if(argc != 1) {
        log_error("You need to specify at least one tag.");
        return R_ERROR;
    }

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(heading, ANSI_FG_RGB(180, 140, 255) ANSI_BOLD "🏷️ Tag: %s", argv[0]);
    printf("%s\n", heading);
    print_divider(60);
    printf("\n");

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
        if(!note_has_tag(&list.items[i], argv[0])) continue; // not a note of this topic

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