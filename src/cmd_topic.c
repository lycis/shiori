#include <stdio.h>
#include <time.h>
#include <string.h>
#include "common.h"
#include "note.h"
#include "logging.h"
#include "cli.h"

int command_topic(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) ||
       has_switch(argc, argv, "-h", false)) {

        printf(
            "Usage:\n"
            "  %s topic <name>\n"
            "\n"
            "Shows all notes assigned to a topic.\n",
            APP_NAME
        );

        return R_OK;
    }

    if(argc != 1) {
        log_error("You need to specify a topic.");
        return R_ERROR;
    }

    const char *topic = argv[0];

    char heading[DEFAULT_BUFFER_SIZE];
    sprintf(heading, ANSI_FG_RGB(180, 140, 255) ANSI_BOLD "🏷️ Topic: %s", topic);
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