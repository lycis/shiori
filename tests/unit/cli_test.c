#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "platform.h"

static int enter_calls = 0;
static int leave_calls = 0;
static int finish_calls = 0;
static int cancel_calls = 0;
static size_t event_index = 0;
static const struct key_event *scripted_events = NULL;
static size_t scripted_event_count = 0;

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
    if(event == NULL) {
        return R_ERROR;
    }

    if(event_index >= scripted_event_count) {
        return R_ERROR;
    }

    *event = scripted_events[event_index++];
    return R_OK;
}

static void use_events(const struct key_event *events, size_t count) {
    scripted_events = events;
    scripted_event_count = count;
    event_index = 0;
}

int main(void) {
    static const struct key_event accepted_events[] = {
        {KEY_CHARACTER, 'h'},
        {KEY_CHARACTER, 'i'},
        {KEY_ENTER, 0},
    };
    char buffer[16];
    struct command_history history = {0};

    use_events(accepted_events, sizeof(accepted_events) / sizeof(accepted_events[0]));

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

    static const struct key_event escape_events[] = {{KEY_ESCAPE, 0}};
    use_events(escape_events, sizeof(escape_events) / sizeof(escape_events[0]));
    finish_calls = 0;
    cancel_calls = 0;

    result = read_interactive_line("> ", buffer, sizeof(buffer), NULL, &history);

    if(result != INTERACTIVE_READ_CANCELLED || finish_calls != 0 || cancel_calls != 1) {
        fprintf(stderr, "Escape did not cancel and clear the active line\n");
        return 1;
    }

    static const struct key_event limit_events[] = {
        {KEY_CHARACTER, 'a'}, {KEY_CHARACTER, 'b'}, {KEY_CHARACTER, 'c'}, {KEY_CHARACTER, 'd'},
        {KEY_CHARACTER, 'e'}, {KEY_CHARACTER, 'f'}, {KEY_CHARACTER, 'g'}, {KEY_CHARACTER, 'h'},
        {KEY_CHARACTER, 'i'}, {KEY_ENTER, 0},
    };
    char limited_buffer[8];
    use_events(limit_events, sizeof(limit_events) / sizeof(limit_events[0]));
    result = read_interactive_line("> ", limited_buffer, sizeof(limited_buffer), NULL, NULL);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(limited_buffer, "abcdefg") != 0) {
        fprintf(stderr, "input near the buffer limit was not safely bounded\n");
        return 1;
    }

    static const struct key_event unicode_events[] = {
        {KEY_CHARACTER, 0x732B},
        {KEY_CHARACTER, 0x1F600},
        {KEY_LEFT, 0},
        {KEY_BACKSPACE, 0},
        {KEY_RESIZE, 0},
        {KEY_ENTER, 0},
    };
    char unicode_buffer[8];
    use_events(unicode_events, sizeof(unicode_events) / sizeof(unicode_events[0]));
    result = read_interactive_line("> ", unicode_buffer, sizeof(unicode_buffer), NULL, NULL);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(unicode_buffer, "\xF0\x9F\x98\x80") != 0) {
        fprintf(stderr, "UTF-8 editing or resize handling corrupted the input\n");
        return 1;
    }

    return 0;
}
