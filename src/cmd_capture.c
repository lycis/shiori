#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "color.h"
#include "commands.h"
#include "common.h"
#include "logging.h"
#include "platform.h"

static int split_args(char *input, char *argv[], int max_args) {
    int argc = 0;
    char *context = NULL;

    char *token = strtok_s(input, " \t", &context);

    while(token != NULL && argc < max_args) {
        argv[argc++] = token;

        token = strtok_s(NULL, " \t", &context);
    }

    return argc;
}

static struct completion_result capture_completion(const char *input) {
    static const char *commands[] = {"/done", "/exit", "/quit"};

    return find_completions(input, commands, sizeof(commands) / sizeof(commands[0]));
}

int command_capture(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
        printf(
            "Usage:\n"
            "  %s capture [options]\n"
            "\n"
            "Starts an interactive capture session for notes and todos.\n"
            "Every entered line will be converted into a note or todo within "
            "the given context.\n"
            "\n"
            "Options:\n"
            "  %-22s Assign captured notes to a topic\n"
            "  %-22s Show this help\n"
            "\n"
            "Capture syntax:\n"
            "  %-22s Add a note\n"
            "  %-22s Add a todo\n"
            "  %-22s Add a todo with a due date\n"
            "  %-22s End the capture session\n"
            "\n"
            "Examples:\n"
            "  %s capture\n"
            "  %s capture -t someCoolTopic\n",
            APP_NAME,
            "-t, --topic <topic>",
            "-h, --help",
            "<text>",
            "! <text>",
            "! --due <date> <text>",
            "/done, /exit, /quit",
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    log_info("Entering long form capture mode.\n");
    const char *topic = NULL;

    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--topic") == 0 || strcmp(argv[i], "-t") == 0) {

            if(i + 1 >= argc) {
                log_error("%s requires a topic name.\n", argv[i]);
                return R_ERROR;
            }

            topic = argv[i + 1];
            i++;
        }
    }

    if(topic == NULL) {
        topic = "";
    } else {
        printf("✍️ Capturing topic: %s%s%s\n", COLOR_TOPIC, topic, ANSI_RESET);
        print_divider(60);
        printf("\n");
    }

    struct command_history history = {0};

    if(terminal_enter_interactive_mode() != R_OK) {
        return R_ERROR;
    }

    while(true) {
        char prompt[DEFAULT_BUFFER_SIZE];

        if(topic != NULL && topic[0] != '\0') {
            snprintf(prompt, sizeof(prompt), "~%s> ", topic);
        } else {
            snprintf(prompt, sizeof(prompt), "~> ");
        }

        char input[DEFAULT_BUFFER_SIZE];

        if(read_interactive_line(prompt, input, sizeof(input), capture_completion, &history) != R_OK) {
            log_critical("Failed to read interactive input.\n");
            break;
        }

        char *command = trim(input);
        if(strlen(command) == 0) {
            continue;
        }

        if(strcmp(command, "/exit") == 0 || strcmp(command, "/quit") == 0 || strcmp(command, "/done") == 0) {
            break;
        }

        if(command[0] == '!') {
            char *text = trim(command + 1);

            if(*text == '\0') {
                log_warning("Todo text cannot be empty.\n");
                continue;
            }

            char *todo_argv[32];
            int todo_argc = 0;

            todo_argc += split_args(text, &todo_argv[todo_argc], 32 - todo_argc);

            if(command_todo_add(todo_argc, todo_argv) != R_OK) {
                log_error("Failed capturing todo.\n");
            }
        } else {
            if(strlen(topic) > 0) {
                char *note_argv[] = {"--topic", (char *)topic, command};
                command_add(3, note_argv);
            } else {
                char *note_argv[] = {command};
                command_add(1, note_argv);
            }
        }
    }

    terminal_leave_interactive_mode();
    log_success("Capture mode ended.\n");

    return R_OK;
}
