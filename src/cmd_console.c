#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "cli.h"
#include "commands.h"


struct completion_result complete_command_definitions(const char *input, const struct command_definition *commands, size_t command_count) {
    struct completion_result result = {0};

    if(input == NULL || input[0] == '\0') {
        return result;
    }

    size_t input_length = strlen(input);

    for(size_t i = 0; i < command_count; ++i) {
        if(strncmp(
            commands[i].name,
            input,
            input_length
        ) == 0) {
            if(result.count >= MAX_COMPLETIONS) {
                break;
            }

            result.items[result.count++] = commands[i].name;
        }
    }

    return result;
}

static struct completion_result console_completion(const char *input) {
    size_t command_count = 0;
    const struct command_definition *commands =  get_commands(&command_count);
    struct completion_result result = complete_command_definitions(input, commands, command_count);

    static const char *console_commands[] = {
        "exit",
        "quit"
    };

    size_t input_length = strlen(input);

    for(size_t i = 0;  i < sizeof(console_commands) / sizeof(console_commands[0]); ++i) {
        if(strncmp(console_commands[i], input, input_length) == 0) {
            if(result.count >= MAX_COMPLETIONS) {
                break;
            }

            result.items[result.count++] = console_commands[i];
        }
    }

    return result;
}

int command_console(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) ||
       has_switch(argc, argv, "-h", false)) {
        printf("Starts an interactive console mode.\n");
        printf("\n");
        printf(
            "You can enter %s commands directly in the console. "
            "This helps as you do not have to run `%s <command>` all the time. "
            "Useful if you want to work continuously.\n",
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    log_info("Starting interactive console mode\n");
    printf("Type 'exit' or 'quit' to exit the console.\n");

    char prompt[DEFAULT_BUFFER_SIZE];
    snprintf(prompt, sizeof(prompt), "%s 🦊> ", APP_NAME);

    while(true) {
        char input[DEFAULT_BUFFER_SIZE];

        if(read_interactive_line(prompt, input, sizeof(input), console_completion) != R_OK) {
            log_error("Failed reading console input.\n");
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

    return R_OK;
}