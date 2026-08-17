#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "cli.h"
#include "commands.h"

int command_console(int argc, char* argv[]) {

    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
        printf("Starts an interactive console mode.\n");
        printf("\n");
        printf("You can enter %s commands directly in the console. This helps as you do not have to run `%s <command>` all the time. Useful if you want to work continuously.", APP_NAME, APP_NAME);
        return R_OK;
    }

    log_info("Starting interactive console mode\n");
    printf("Type 'exit' or 'quit' to exit the console.\n");

    char input[DEFAULT_BUFFER_SIZE];
    printf("%s 🦊> ", APP_NAME);
    while(fgets(input, sizeof(input), stdin) != NULL) {
        char *command = trim(input);
        if(strlen(command) == 0) {
            continue;
        }

        if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            log_info("Exiting console mode\n");
            break;
        }

        // split command into arguments
        char *argv[DEFAULT_BUFFER_SIZE];
        int argc = 0;
        char *context = NULL;
        char *token = strtok_s(command, " ", &context);
        while(token != NULL && argc < DEFAULT_BUFFER_SIZE) {
            argv[argc++] = token;
            token = strtok_s(NULL, " ", &context);
        }

        run_command(argv[0], argc - 1, &argv[1]);
        printf("%s 🦊> ", APP_NAME);
    }

    return R_OK;
}