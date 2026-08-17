#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "cli.h"
#include "commands.h"

static const char *console_completion(const char *input) {
    static const char *commands[] = {
        "add",
        "capture",
        "config",
        "console",
        "tag",
        "today",
        "todo",
        "topic",
        "version",
        "exit",
        "quit"
    };

    return find_completion(input, commands, sizeof(commands) / sizeof(commands[0]));
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