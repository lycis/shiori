#include <time.h>
#include <stdio.h>
#include "common.h"
#include "logging.h"
#include "cli.h"
#include "note.h"
#include "cmd_shared.h"

static int build_dashboard_heading(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm local_time;
    if(localtime_s(&local_time, &now) != 0) {
        log_error("Failed to get local time.");
        return R_ERROR;
    }

    char date_str[50];
    if(strftime(date_str, sizeof(date_str), "%A, %Y-%m-%d", &local_time) == 0) {
        log_critical("Failed to format local date.");        
        return R_ERROR;
    }

    int written = snprintf(buffer, size, "📅 %s", date_str);
    if(written < 0 || written >= (int)size) {
        log_critical("Daily heading buffer too small.");
        return R_ERROR;
    }

    return R_OK;
}

int command_today(int argc, char* argv[]) {
    log_debug("Creating daily dashboard.\n");

    // Heading
    char heading[DEFAULT_BUFFER_SIZE];
    if(build_dashboard_heading(heading, sizeof(heading)) != R_OK) {
        return R_ERROR;
    }

    printf("%s%s%s\n\n", ANSI_BOLD ANSI_BOLD ANSI_FG_RGB(255, 180, 80), heading, ANSI_RESET);

    // Today's notes
    printf("  %s%s%s\n", ANSI_BOLD ANSI_BOLD ANSI_FG_RGB(110, 190, 255), "🗒️ Notes", ANSI_RESET);
    struct note_list list;
    note_list_init(&list);
    if(read_notes_for_date("NOTES.md", time(NULL), &list) != R_OK) {
        note_list_free(&list);
        return R_ERROR;
    }

    for(size_t i = 0; i < list.count; ++i) {
        printf("    • %s\n", list.items[i].text);
    }

    note_list_free(&list);

    // Active Todos

    return R_OK;
}