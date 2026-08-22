#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "commands.h"
#include "common.h"
#include "logging.h"
#include "platform.h"

struct completion_result
complete_command_definitions(const char *input, const struct command_definition *commands, size_t command_count) {
    struct completion_result result = {0};

    if(commands == NULL) {
        return result;
    }

    size_t input_length = 0;

    if(input != NULL) {
        input_length = strlen(input);
    }

    for(size_t i = 0; i < command_count; ++i) {
        if(input_length == 0 || strncmp(commands[i].name, input, input_length) == 0) {

            if(result.count >= MAX_COMPLETIONS) {
                break;
            }

            result.items[result.count++] = commands[i].name;
        }
    }

    return result;
}

static void add_console_special_completions(struct completion_result *result, const char *input) {
    static const char *console_commands[] = {"exit", "quit"};

    size_t input_length = 0;

    if(input != NULL) {
        input_length = strlen(input);
    }

    for(size_t i = 0; i < sizeof(console_commands) / sizeof(console_commands[0]); ++i) {
        if(input_length == 0 || strncmp(console_commands[i], input, input_length) == 0) {

            if(result->count >= MAX_COMPLETIONS) {
                return;
            }

            result->items[result->count++] = console_commands[i];
        }
    }
}

static struct completion_result console_completion(const char *input) {
    struct completion_result result = {0};

    size_t command_count = 0;
    const struct command_definition *current_commands = get_commands(&command_count);

    if(input == NULL) {
        return result;
    }

    char buffer[DEFAULT_BUFFER_SIZE];

    if(strcpy_s(buffer, sizeof(buffer), input) != 0) {
        return result;
    }

    bool trailing_space = ends_with_whitespace(input);

    char *argv[32];
    int argc = 0;

    char *context = NULL;
    char *token = strtok_s(buffer, " \t", &context);

    while(token != NULL && argc < 32) {
        argv[argc++] = token;
        token = strtok_s(NULL, " \t", &context);
    }

    /*
     * If there are no tokens yet, complete at the top level.
     * (May not be used right now if completion only starts
     * after at least one typed character, but it keeps the
     * function complete.)
     */
    if(argc == 0) {
        result = complete_command_definitions("", current_commands, command_count);
        add_console_special_completions(&result, "");
        return result;
    }

    /*
     * If input ends with whitespace:
     *
     * Example:
     *   "todo "
     *
     * Then "todo" is complete, and we want to suggest all of
     * its subcommands.
     */
    if(trailing_space) {
        for(int i = 0; i < argc; ++i) {
            const struct command_definition *definition =
                find_command_definition(current_commands, command_count, argv[i]);

            if(definition == NULL) {
                return result;
            }

            current_commands = definition->subcommands;
            command_count = definition->subcommand_count;
        }

        return complete_command_definitions("", current_commands, command_count);
    }

    /*
     * Otherwise, the last token is partial and should be completed.
     *
     * Example:
     *   "todo l"
     *
     * Resolve "todo", then complete "l" within its subcommands.
     */
    for(int i = 0; i < argc - 1; ++i) {
        const struct command_definition *definition = find_command_definition(current_commands, command_count, argv[i]);

        if(definition == NULL || definition->subcommands == NULL || definition->subcommand_count == 0) {
            return result;
        }

        current_commands = definition->subcommands;
        command_count = definition->subcommand_count;
    }

    result = complete_command_definitions(argv[argc - 1], current_commands, command_count);

    /*
     * Only add console-local commands at the top level.
     */
    if(argc == 1) {
        add_console_special_completions(&result, argv[0]);
    }

    return result;
}

int command_console(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
        printf("Starts an interactive console mode.\n");
        printf("\n");
        printf(
            "You can enter %s commands directly in the console. "
            "This helps as you do not have to run `%s <command>` all the time. "
            "Useful if you want to work continuously.\n"
            "\n"
            "Enter 'exit' or 'quit' to end the session, Escape to cancel it, "
            "or Ctrl+C to interrupt Shiori with exit status %d.\n",
            APP_NAME,
            APP_NAME,
            SHIORI_EXIT_INTERRUPTED
        );

        return R_OK;
    }

    log_info("Starting interactive console mode\n");
    printf("Type 'exit' or 'quit' to exit the console.\n");

    char prompt[DEFAULT_BUFFER_SIZE];
    snprintf(prompt, sizeof(prompt), "%s 🦊> ", APP_NAME);
    struct command_history history = {0};
    int result = R_OK;

    if(terminal_enter_interactive_mode() != R_OK) {
        return R_ERROR;
    }

    while(true) {
        char input[DEFAULT_BUFFER_SIZE];

        enum interactive_read_result read_result =
            read_interactive_line(prompt, input, sizeof(input), console_completion, &history);

        if(read_result == INTERACTIVE_READ_CANCELLED) {
            break;
        }

        if(read_result == INTERACTIVE_READ_FAILED) {
            log_error("Failed reading console input.\n");
            result = R_ERROR;
            break;
        }

        char *command = trim(input);

        if(*command == '\0') {
            continue;
        }

        if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            log_info("Exiting console mode\n");
            break;
        }

        /*
         * Split command into argv.
         */
        char *command_argv[64];
        int command_argc = 0;

        char *context = NULL;

        char *token = strtok_s(command, " \t", &context);

        while(token != NULL && command_argc < 64) {
            command_argv[command_argc++] = token;
            token = strtok_s(NULL, " \t", &context);
        }

        if(command_argc == 0) {
            continue;
        }

        run_command(command_argv[0], command_argc - 1, &command_argv[1]);
    }

    terminal_leave_interactive_mode();
    return result;
}
