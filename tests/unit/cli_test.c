#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "platform.h"

static int enter_calls = 0;
static int leave_calls = 0;
static int finish_calls = 0;
static int cancel_calls = 0;
static size_t event_index = 0;
static const struct key_event *scripted_events = nullptr;
static size_t scripted_event_count = 0;
static size_t selection_render_count = 0;
static size_t selected_indices[16];
static char first_selected_buffer[DEFAULT_BUFFER_SIZE];
static bool selection_was_cleared_after_edit = false;
static size_t last_rendered_completion_count = 0;

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
    const struct completion_result *completions,
    bool has_completion_selection,
    size_t completion_selection
) {
    (void)prompt;
    (void)buffer;
    (void)cursor;
    (void)completions;
    last_rendered_completion_count = completions->count;

    if(has_completion_selection) {
        if(selection_render_count == 0) {
            strcpy_s(first_selected_buffer, sizeof(first_selected_buffer), buffer);
        }
        if(selection_render_count < sizeof(selected_indices) / sizeof(selected_indices[0])) {
            selected_indices[selection_render_count] = completion_selection;
        }
        selection_render_count++;
    } else if(selection_render_count > 0 && strcmp(buffer, "ca") == 0) {
        selection_was_cleared_after_edit = true;
    }
}

void terminal_finish_input_line(void) {
    finish_calls++;
}

void terminal_cancel_input_line(void) {
    cancel_calls++;
}

int terminal_read_key(struct key_event *event) {
    if(event == nullptr) {
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

static void reset_render_observations(void) {
    selection_render_count = 0;
    first_selected_buffer[0] = '\0';
    selection_was_cleared_after_edit = false;
    last_rendered_completion_count = 0;
}

static struct completion_result test_completion(const char *input) {
    static const char *options[] = {"capture", "config"};
    return find_completions(input, options, sizeof(options) / sizeof(options[0]));
}

static struct completion_result long_completion(const char *input) {
    static const char *options[] = {"abcdefghij"};
    return find_completions(input, options, sizeof(options) / sizeof(options[0]));
}

static struct completion_result common_prefix_completion(const char *input) {
    static const char *options[] = {"capture", "catalog"};
    return find_completions(input, options, sizeof(options) / sizeof(options[0]));
}

int main(void) {
    static const struct key_event accepted_events[] = {
        {KEY_CHARACTER, 'h'},
        {KEY_CHARACTER, 'i'},
        {KEY_ENTER, 0},
    };
    char buffer[16];
    struct command_history history = {};

    use_events(accepted_events, sizeof(accepted_events) / sizeof(accepted_events[0]));

    enum interactive_read_result result = read_interactive_line("> ", buffer, sizeof(buffer), nullptr, &history);

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

    result = read_interactive_line("> ", buffer, sizeof(buffer), nullptr, &history);

    if(result != INTERACTIVE_READ_CANCELLED || finish_calls != 0 || cancel_calls != 1) {
        fprintf(stderr, "Escape did not cancel and clear the active line\n");
        return 1;
    }

    static const struct key_event limit_events[] = {
        {KEY_CHARACTER, 'a'},
        {KEY_CHARACTER, 'b'},
        {KEY_CHARACTER, 'c'},
        {KEY_CHARACTER, 'd'},
        {KEY_CHARACTER, 'e'},
        {KEY_CHARACTER, 'f'},
        {KEY_CHARACTER, 'g'},
        {KEY_CHARACTER, 'h'},
        {KEY_CHARACTER, 'i'},
        {KEY_ENTER, 0},
    };
    char limited_buffer[8];
    use_events(limit_events, sizeof(limit_events) / sizeof(limit_events[0]));
    result = read_interactive_line("> ", limited_buffer, sizeof(limited_buffer), nullptr, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(limited_buffer, "abcdefg") != 0) {
        fprintf(stderr, "input near the buffer limit was not safely bounded\n");
        return 1;
    }

    struct key_event *stress_events = calloc(DEFAULT_BUFFER_SIZE + 1, sizeof(*stress_events));
    if(stress_events == nullptr) {
        fprintf(stderr, "failed allocating buffer-limit stress events\n");
        return 1;
    }
    for(size_t i = 0; i < DEFAULT_BUFFER_SIZE; ++i) {
        stress_events[i] = (struct key_event){KEY_CHARACTER, 'x'};
    }
    stress_events[DEFAULT_BUFFER_SIZE] = (struct key_event){KEY_ENTER, 0};

    char stress_buffer[DEFAULT_BUFFER_SIZE];
    use_events(stress_events, DEFAULT_BUFFER_SIZE + 1);
    result = read_interactive_line("> ", stress_buffer, sizeof(stress_buffer), nullptr, nullptr);
    free(stress_events);
    if(result != INTERACTIVE_READ_ACCEPTED || strlen(stress_buffer) != DEFAULT_BUFFER_SIZE - 1) {
        fprintf(stderr, "configured input buffer limit was not preserved\n");
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
    result = read_interactive_line("> ", unicode_buffer, sizeof(unicode_buffer), nullptr, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(unicode_buffer, "\xF0\x9F\x98\x80") != 0) {
        fprintf(stderr, "UTF-8 editing or resize handling corrupted the input\n");
        return 1;
    }

    static const struct key_event down_enter_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_DOWN, 0},
        {KEY_ENTER, 0},
        {KEY_ENTER, 0},
    };
    reset_render_observations();
    use_events(down_enter_events, sizeof(down_enter_events) / sizeof(down_enter_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0 || selection_render_count != 1 ||
       strcmp(first_selected_buffer, "c") != 0 || selected_indices[0] != 0) {
        fprintf(stderr, "Down/Enter did not select without editing and then accept the first completion\n");
        return 1;
    }

    static const struct key_event up_tab_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_UP, 0},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    reset_render_observations();
    use_events(up_tab_events, sizeof(up_tab_events) / sizeof(up_tab_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "config") != 0 || selection_render_count != 1 ||
       selected_indices[0] != 1) {
        fprintf(stderr, "Up/Tab did not select and accept the last completion\n");
        return 1;
    }

    static const struct key_event wrap_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_DOWN, 0},
        {KEY_DOWN, 0},
        {KEY_DOWN, 0},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    reset_render_observations();
    use_events(wrap_events, sizeof(wrap_events) / sizeof(wrap_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0 || selection_render_count != 3 ||
       selected_indices[0] != 0 || selected_indices[1] != 1 || selected_indices[2] != 0) {
        fprintf(stderr, "completion selection did not wrap with repeated Down keys\n");
        return 1;
    }

    static const struct key_event edit_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_DOWN, 0},
        {KEY_CHARACTER, 'a'},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    reset_render_observations();
    use_events(edit_events, sizeof(edit_events) / sizeof(edit_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0 || !selection_was_cleared_after_edit) {
        fprintf(stderr, "editing did not clear completion selection\n");
        return 1;
    }

    static const struct key_event oversized_events[] = {
        {KEY_CHARACTER, 'a'},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    char tiny_buffer[5];
    use_events(oversized_events, sizeof(oversized_events) / sizeof(oversized_events[0]));
    result = read_interactive_line("> ", tiny_buffer, sizeof(tiny_buffer), long_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(tiny_buffer, "a") != 0) {
        fprintf(stderr, "oversized completion partially modified the input\n");
        return 1;
    }

    static const struct key_event prefix_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    use_events(prefix_events, sizeof(prefix_events) / sizeof(prefix_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), common_prefix_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "ca") != 0) {
        fprintf(stderr, "Tab no longer extends multiple matches to their common prefix\n");
        return 1;
    }

    static const struct key_event exact_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_CHARACTER, 'a'},
        {KEY_CHARACTER, 'p'},
        {KEY_CHARACTER, 't'},
        {KEY_CHARACTER, 'u'},
        {KEY_CHARACTER, 'r'},
        {KEY_CHARACTER, 'e'},
        {KEY_UP, 0},
        {KEY_ENTER, 0},
    };
    reset_render_observations();
    use_events(exact_events, sizeof(exact_events) / sizeof(exact_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, nullptr);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0 || last_rendered_completion_count != 0) {
        fprintf(stderr, "exact completion remained visible or navigable\n");
        return 1;
    }

    struct command_history completion_history = {};
    strcpy_s(completion_history.items[0], sizeof(completion_history.items[0]), "previous");
    completion_history.count = 1;
    static const struct key_event completion_history_events[] = {
        {KEY_CHARACTER, 'c'},
        {KEY_DOWN, 0},
        {KEY_TAB, 0},
        {KEY_ENTER, 0},
    };
    use_events(completion_history_events, sizeof(completion_history_events) / sizeof(completion_history_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, &completion_history);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0 || completion_history.count != 2 ||
       strcmp(completion_history.items[0], "previous") != 0) {
        fprintf(stderr, "completion navigation replaced history or the edited draft\n");
        return 1;
    }

    static const struct key_event history_events[] = {{KEY_UP, 0}, {KEY_ENTER, 0}};
    use_events(history_events, sizeof(history_events) / sizeof(history_events[0]));
    result = read_interactive_line("> ", buffer, sizeof(buffer), test_completion, &completion_history);
    if(result != INTERACTIVE_READ_ACCEPTED || strcmp(buffer, "capture") != 0) {
        fprintf(stderr, "Up no longer recalls history when completion suggestions are absent\n");
        return 1;
    }

    return 0;
}
