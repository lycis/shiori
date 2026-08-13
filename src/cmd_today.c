#include <time.h>
#include <stdio.h>
#include "common.h"
#include "logging.h"
#include "cli.h"
#include "note.h"
#include "cmd_shared.h"
#include "todo.h"
#include "todo_list.h"

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

    printf("%s%s%s\n", ANSI_BOLD ANSI_BOLD ANSI_FG_RGB(255, 180, 80), heading, ANSI_RESET);
    print_divider(60);
    printf("\n");

    // Today's notes
    printf("  %s%s%s\n", ANSI_BOLD ANSI_BOLD ANSI_FG_RGB(110, 190, 255), "🗒️ Notes", ANSI_RESET);
    struct note_list note_list;
    note_list_init(&note_list);
    if(read_notes_for_date("NOTES.md", time(NULL), &note_list) != R_OK) {
        note_list_free(&note_list);
        return R_ERROR;
    }

    for(size_t i = 0; i < note_list.count; ++i) {
        printf("    • %s\n", note_list.items[i].text);
    }

    note_list_free(&note_list);

    // get all todos for today
    struct todo_list todo_list;
    todo_list_init(&todo_list);
    if(read_todos("TODOS.md", &todo_list) != R_OK) {
        todo_list_free(&todo_list);
        return R_ERROR;
    }

    // sort todos into in_progress and open buckets
    struct todo_list in_progress_todos;
    todo_list_init(&in_progress_todos);
    struct todo_list open_todos;
    todo_list_init(&open_todos);
    for(size_t i = 0; i < todo_list.count; ++i) {
        log_debug("Found todo (%llu) with status %s.\n", todo_list.items[i].id, todo_status_string(todo_list.items[i].status));
        if(todo_list.items[i].status == IN_PROGRESS) todo_list_add(&in_progress_todos, &todo_list.items[i]);
        if(todo_list.items[i].status == OPEN) todo_list_add(&open_todos, &todo_list.items[i]);
    }
    printf("\n");

    // print active todos
    printf("  %s%s%s\n", ANSI_BOLD ANSI_FG_RGB(255, 190, 80), "🚧 In Progress", ANSI_RESET);
    for(size_t i = 0; i < in_progress_todos.count; ++i) {
        struct todo *item = &in_progress_todos.items[i];

        printf("    %s› %4llu%s  %s\n", ANSI_FG_RGB(255, 190, 80), item->id, ANSI_RESET, item->text);
    }
    todo_list_free(&in_progress_todos);
    printf("\n");

    // print open todos
    printf("  %s%s%s\n", ANSI_BOLD ANSI_FG_RGB(110, 190, 255), "📌 Open", ANSI_RESET);
    for(size_t i = 0; i < open_todos.count; ++i) {
        struct todo *item = &open_todos.items[i];

        printf("    %s· %4llu%s  %s\n", ANSI_FG_RGB(110, 190, 255), item->id, ANSI_RESET, item->text);
    }
    todo_list_free(&open_todos);

    todo_list_free(&todo_list);

    printf("\n");
    print_divider(60);

    return R_OK;
}