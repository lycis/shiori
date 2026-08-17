#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cli.h" 
#include "color.h"
#include "platform.h"
#include "common.h"

void strip_leading_flags(int *argc, char ***argv)
{
    while (*argc > 0) {
        char *arg = (*argv)[0];

        if (arg[0] != '-' || arg[1] == '\0') {
            break;
        }

        (*argv)++;
        (*argc)--;
    }
}

bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands){
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

static int append_codepoint_utf8(
    char *buffer,
    size_t buffer_size,
    size_t *length,
    unsigned int codepoint
) {
    unsigned char encoded[4];
    size_t count;

    if(codepoint <= 0x7F) {
        encoded[0] = (unsigned char)codepoint;
        count = 1;
    }
    else if(codepoint <= 0x7FF) {
        encoded[0] = 0xC0 | (codepoint >> 6);
        encoded[1] = 0x80 | (codepoint & 0x3F);
        count = 2;
    }
    else if(codepoint <= 0xFFFF) {
        encoded[0] = 0xE0 | (codepoint >> 12);
        encoded[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        encoded[2] = 0x80 | (codepoint & 0x3F);
        count = 3;
    }
    else if(codepoint <= 0x10FFFF) {
        encoded[0] = 0xF0 | (codepoint >> 18);
        encoded[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        encoded[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        encoded[3] = 0x80 | (codepoint & 0x3F);
        count = 4;
    }
    else {
        return R_ERROR;
    }

    if(*length + count >= buffer_size) {
        return R_ERROR;
    }

    memcpy(buffer + *length, encoded, count);

    *length += count;
    buffer[*length] = '\0';

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

        while(j < prefix_length &&
              current[j] != '\0' &&
              first[j] == current[j]) {
            j++;
        }

        prefix_length = j;

        if(prefix_length == 0) {
            break;
        }
    }

    return prefix_length;
}

int read_interactive_line(
    const char *prompt,
    char *buffer,
    size_t buffer_size,
    completion_fn complete
) {
    if(prompt == NULL || buffer == NULL || buffer_size == 0) {
        return R_ERROR;
    }

    size_t length = 0;
    size_t cursor = 0;

    buffer[0] = '\0';

    if(terminal_enter_interactive_mode() != R_OK) {
        return R_ERROR;
    }

    while(true) {
        struct completion_result completions = {0};

        if(complete != NULL && length > 0) {
            completions = complete(buffer);
        }

        terminal_render_input(prompt, buffer, cursor, &completions);

        struct key_event event;

        if(terminal_read_key(&event) != R_OK) {
            terminal_leave_interactive_mode();
            return R_ERROR;
        }

        switch(event.type) {
            case KEY_CHARACTER:
                if(append_codepoint_utf8(
                    buffer,
                    buffer_size,
                    &length,
                    event.codepoint
                ) == R_OK) {
                    cursor = length;
                }
                break;

            case KEY_BACKSPACE:
                if(length > 0) {
                    /*
                     * Remove one complete UTF-8 codepoint.
                     */
                    length--;

                    while(length > 0 &&
                          ((unsigned char)buffer[length] & 0xC0) == 0x80) {
                        length--;
                    }

                    buffer[length] = '\0';
                    cursor = length;
                }
                break;

            case KEY_TAB:
                if(completions.count == 0) {
                    break;
                }

                if(completions.count == 1) {
                    const char *completion = completions.items[0];
                    size_t completion_length = strlen(completion);

                    if(completion_length < buffer_size) {
                        if(strcpy_s(
                            buffer,
                            buffer_size,
                            completion
                        ) == 0) {
                            length = completion_length;
                            cursor = length;
                        }
                    }

                    break;
                }

                /*
                * Multiple matches:
                * extend only as far as their common prefix.
                */
                size_t prefix_length =
                    completion_common_prefix_length(&completions);

                if(prefix_length > length &&
                prefix_length < buffer_size) {

                    memcpy(
                        buffer,
                        completions.items[0],
                        prefix_length
                    );

                    buffer[prefix_length] = '\0';

                    length = prefix_length;
                    cursor = length;
                }

                break;
                break;

            case KEY_ENTER:
                terminal_finish_input_line();
                terminal_leave_interactive_mode();
                return R_OK;

            case KEY_ESCAPE:
                buffer[0] = '\0';

                terminal_finish_input_line();
                terminal_leave_interactive_mode();

                return R_ERROR;

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
        if(strncmp(
            options[i],
            input,
            input_length
        ) == 0) {
            if(result.count >= MAX_COMPLETIONS) {
                break;
            }

            result.items[result.count++] = options[i];
        }
    }

    return result;
}