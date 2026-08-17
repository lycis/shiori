#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cli.h" 
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
    printf(ANSI_FG_RGB(90, 105, 120));

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
        const char *suggestion = NULL;

        if(complete != NULL && length > 0) {
            suggestion = complete(buffer);
        }

        terminal_render_input(
            prompt,
            buffer,
            cursor,
            suggestion
        );

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
                if(suggestion != NULL) {
                    size_t suggestion_length = strlen(suggestion);

                    if(suggestion_length < buffer_size) {
                        if(strcpy_s(
                            buffer,
                            buffer_size,
                            suggestion
                        ) == 0) {
                            length = suggestion_length;
                            cursor = length;
                        }
                    }
                }
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

const char *find_completion(const char *input, const char *options[], size_t option_count) {
    if(input == NULL || input[0] == '\0') {
        return NULL;
    }

    size_t input_length = strlen(input);

    for(size_t i = 0; i < option_count; ++i) {
        if(strncmp(
            options[i],
            input,
            input_length
        ) == 0) {
            return options[i];
        }
    }

    return NULL;
}