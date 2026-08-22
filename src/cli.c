#include "cli.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "color.h"
#include "common.h"
#include "platform.h"

void strip_leading_flags(int *argc, char ***argv) {
    while(*argc > 0) {
        char *arg = (*argv)[0];

        if(arg[0] != '-' || arg[1] == '\0') {
            break;
        }

        (*argv)++;
        (*argc)--;
    }
}

bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands) {
    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--") == 0) {
            break;
        }

        if(argv[i][0] != '-' || argv[i][1] == '\0') {
            if(allow_subcommands) {
                break;
            }

            continue;
        }

        if(strcmp(argv[i], sw) == 0) {
            return true;
        }
    }

    return false;
}

void print_divider(size_t width) {
    printf(COLOR_DIVIDER);

    for(size_t i = 0; i < width; ++i) {
        printf("─");
    }

    printf(ANSI_RESET "\n");
}

static int
insert_codepoint_utf8(char *buffer, size_t buffer_size, size_t *length, size_t *cursor, unsigned int codepoint) {
    unsigned char encoded[4];
    size_t count;

    if(codepoint <= 0x7F) {
        encoded[0] = (unsigned char)codepoint;
        count = 1;
    } else if(codepoint <= 0x7FF) {
        encoded[0] = 0xC0 | (codepoint >> 6);
        encoded[1] = 0x80 | (codepoint & 0x3F);
        count = 2;
    } else if(codepoint <= 0xFFFF) {
        encoded[0] = 0xE0 | (codepoint >> 12);
        encoded[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        encoded[2] = 0x80 | (codepoint & 0x3F);
        count = 3;
    } else if(codepoint <= 0x10FFFF) {
        encoded[0] = 0xF0 | (codepoint >> 18);
        encoded[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        encoded[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        encoded[3] = 0x80 | (codepoint & 0x3F);
        count = 4;
    } else {
        return R_ERROR;
    }

    if(*length + count >= buffer_size || *cursor > *length) {
        return R_ERROR;
    }

    memmove(buffer + *cursor + count, buffer + *cursor, *length - *cursor + 1);
    memcpy(buffer + *cursor, encoded, count);

    *length += count;
    *cursor += count;

    return R_OK;
}

static size_t completion_common_prefix_length(const struct completion_result *completions) {
    if(completions == NULL || completions->count == 0) {
        return 0;
    }

    const char *first = completions->items[0];
    size_t prefix_length = strlen(first);

    for(size_t i = 1; i < completions->count; ++i) {
        const char *current = completions->items[i];
        size_t j = 0;

        while(j < prefix_length && current[j] != '\0' && first[j] == current[j]) {
            j++;
        }

        prefix_length = j;

        if(prefix_length == 0) {
            break;
        }
    }

    return prefix_length;
}

static size_t current_token_start(const char *buffer) {
    if(buffer == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);
    size_t start = length;

    while(start > 0) {
        char c = buffer[start - 1];

        if(c == ' ' || c == '\t') {
            break;
        }

        start--;
    }

    return start;
}

static void add_history_item(struct command_history *history, const char *line) {
    if(history == NULL || line == NULL || line[0] == '\0') {
        return;
    }

    if(history->count > 0 && strcmp(history->items[history->count - 1], line) == 0) {
        return;
    }

    if(history->count == MAX_HISTORY_ITEMS) {
        memmove(history->items[0], history->items[1], (MAX_HISTORY_ITEMS - 1) * sizeof(history->items[0]));
        history->count--;
    }

    strcpy_s(history->items[history->count], sizeof(history->items[history->count]), line);
    history->count++;
}

static void recall_history_item(char *buffer, size_t buffer_size, size_t *length, size_t *cursor, const char *line) {
    if(strcpy_s(buffer, buffer_size, line) != 0) {
        return;
    }

    *length = strlen(buffer);
    *cursor = *length;
}

enum interactive_read_result read_interactive_line(
    const char *prompt,
    char *buffer,
    size_t buffer_size,
    completion_fn complete,
    struct command_history *history
) {
    if(prompt == NULL || buffer == NULL || buffer_size == 0) {
        return INTERACTIVE_READ_FAILED;
    }

    size_t length = 0;
    size_t cursor = 0;
    size_t history_position = history != NULL ? history->count : 0;
    char draft[DEFAULT_BUFFER_SIZE] = "";

    buffer[0] = '\0';

    while(true) {
        struct completion_result completions = {0};

        if(complete != NULL && length > 0) {
            completions = complete(buffer);
        }

        terminal_render_input(prompt, buffer, cursor, &completions);

        struct key_event event;

        if(terminal_read_key(&event) != R_OK) {
            terminal_cancel_input_line();
            return INTERACTIVE_READ_FAILED;
        }

        switch(event.type) {
        case KEY_CHARACTER:
            insert_codepoint_utf8(buffer, buffer_size, &length, &cursor, event.codepoint);
            break;

        case KEY_BACKSPACE:
            if(cursor > 0) {
                /*
                 * Remove the complete UTF-8 codepoint before the cursor.
                 */
                size_t character_start = cursor - 1;

                while(character_start > 0 && ((unsigned char)buffer[character_start] & 0xC0) == 0x80) {
                    character_start--;
                }

                size_t removed = cursor - character_start;

                memmove(buffer + character_start, buffer + cursor, length - cursor + 1);

                length -= removed;
                cursor = character_start;
            }
            break;

        case KEY_DELETE:
            if(cursor < length) {
                // remove the complete UTF-8 codepoint under the cursor.
                size_t character_end = cursor + 1;

                while(character_end < length && ((unsigned char)buffer[character_end] & 0xC0) == 0x80) {
                    character_end++;
                }

                size_t removed = character_end - cursor;

                memmove(buffer + cursor, buffer + character_end, length - character_end + 1);

                length -= removed;
            }
            break;

        case KEY_TAB: {
            if(completions.count == 0) {
                break;
            }

            size_t token_start = current_token_start(buffer);

            if(completions.count == 1) {
                const char *completion = completions.items[0];
                size_t completion_length = strlen(completion);

                size_t new_length = token_start + completion_length;

                if(new_length < buffer_size) {
                    memcpy(buffer + token_start, completion, completion_length);

                    buffer[new_length] = '\0';

                    length = new_length;
                    cursor = length;
                }

                break;
            }

            /*
             * Multiple matches:
             * extend current token only as far as the common prefix.
             */
            size_t prefix_length = completion_common_prefix_length(&completions);

            size_t current_token_length = length - token_start;

            if(prefix_length > current_token_length) {
                size_t new_length = token_start + prefix_length;

                if(new_length < buffer_size) {
                    memcpy(buffer + token_start, completions.items[0], prefix_length);

                    buffer[new_length] = '\0';

                    length = new_length;
                    cursor = length;
                }
            }

            break;
        }
        case KEY_ENTER:
            add_history_item(history, buffer);
            terminal_finish_input_line();
            return INTERACTIVE_READ_ACCEPTED;

        case KEY_ESCAPE:
            buffer[0] = '\0';

            terminal_cancel_input_line();

            return INTERACTIVE_READ_CANCELLED;

        case KEY_LEFT:
            if(cursor > 0) {
                cursor--;

                while(cursor > 0 && ((unsigned char)buffer[cursor] & 0xC0) == 0x80) {
                    cursor--;
                }
            }
            break;

        case KEY_RIGHT:
            if(cursor < length) {
                cursor++;

                while(cursor < length && ((unsigned char)buffer[cursor] & 0xC0) == 0x80) {
                    cursor++;
                }
            }
            break;

        case KEY_HOME:
            cursor = 0;
            break;

        case KEY_END:
            cursor = length;
            break;

        case KEY_UP:
            if(history != NULL && history_position > 0) {
                if(history_position == history->count) {
                    strcpy_s(draft, sizeof(draft), buffer);
                }

                history_position--;
                recall_history_item(buffer, buffer_size, &length, &cursor, history->items[history_position]);
            }
            break;

        case KEY_DOWN:
            if(history != NULL && history_position < history->count) {
                history_position++;

                recall_history_item(
                    buffer,
                    buffer_size,
                    &length,
                    &cursor,
                    history_position == history->count ? draft : history->items[history_position]
                );
            }
            break;

        default:
            break;
        }
    }
}

struct completion_result find_completions(const char *input, const char *options[], size_t option_count) {
    struct completion_result result = {0};

    if(input == NULL || input[0] == '\0') {
        return result;
    }

    size_t input_length = strlen(input);

    for(size_t i = 0; i < option_count; ++i) {
        if(strncmp(options[i], input, input_length) == 0) {
            if(result.count >= MAX_COMPLETIONS) {
                break;
            }

            result.items[result.count++] = options[i];
        }
    }

    return result;
}
