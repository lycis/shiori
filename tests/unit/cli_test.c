#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "platform.h"

static int enter_calls = 0;
static int leave_calls = 0;
static int finish_calls = 0;
static int cancel_calls = 0;
static size_t event_index = 0;
static bool send_escape = false;

int terminal_enter_interactive_mode(void) {
    enter_calls++;
    return R_OK;
}

void terminal_leave_interactive_mode(void) {
    leave_calls++;
}

void terminal_render_input(
    const char *prompt,
    const char *buffer,
    size_t cursor,
    const struct completion_result *completions
) {
    (void)prompt;
    (void)buffer;
    (void)cursor;
    (void)completions;
}

void terminal_finish_input_line(void) {
    finish_calls++;
}

void terminal_cancel_input_line(void) {
    cancel_calls++;
}

int terminal_read_key(struct key_event *event) {
    static const struct key_event events[] = {
        {KEY_CHARACTER, 'h'},
        {KEY_CHARACTER, 'i'},
        {KEY_ENTER, 0},
    };

    if(event == NULL) {
        return R_ERROR;
    }

    if(send_escape) {
        if(event_index > 0) {
            return R_ERROR;
        }

        *event = (struct key_event){KEY_ESCAPE, 0};
        event_index++;
        return R_OK;
    }

    if(event_index >= sizeof(events) / sizeof(events[0])) {
        return R_ERROR;
    }

    *event = events[event_index++];
    return R_OK;
}

int main(void) {
    char buffer[16];
    struct command_history history = {0};

    enum interactive_read_result result = read_interactive_line("> ", buffer, sizeof(buffer), NULL, &history);

    if(result != INTERACTIVE_READ_ACCEPTED) {
        fprintf(stderr, "read_interactive_line returned %d\n", result);
        return 1;
    }

    if(strcmp(buffer, "hi") != 0) {
        fprintf(stderr, "expected submitted line 'hi', got '%s'\n", buffer);
        return 1;
    }

    if(enter_calls != 0 || leave_calls != 0) {
        fprintf(stderr, "line reader changed terminal mode (%d enters, %d leaves)\n", enter_calls, leave_calls);
        return 1;
    }

    if(finish_calls != 1) {
        fprintf(stderr, "expected one finished line, got %d\n", finish_calls);
        return 1;
    }

    if(cancel_calls != 0) {
        fprintf(stderr, "accepted input unexpectedly cancelled the line\n");
        return 1;
    }

    if(history.count != 1 || strcmp(history.items[0], "hi") != 0) {
        fprintf(stderr, "submitted line was not added to history\n");
        return 1;
    }

    event_index = 0;
    send_escape = true;
    finish_calls = 0;
    cancel_calls = 0;

    result = read_interactive_line("> ", buffer, sizeof(buffer), NULL, &history);

    if(result != INTERACTIVE_READ_CANCELLED || finish_calls != 0 || cancel_calls != 1) {
        fprintf(stderr, "Escape did not cancel and clear the active line\n");
        return 1;
    }

    return 0;
}
