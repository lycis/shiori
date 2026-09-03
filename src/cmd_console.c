#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "commands.h"
#include "common.h"
#include "config.h"
#include "logging.h"
#include "platform.h"

struct completion_result
complete_command_definitions(const char *input, const struct command_definition *commands, size_t command_count) {
    struct completion_result result = {};

    if(commands == nullptr) {
        return result;
    }

    size_t input_length = 0;

    if(input != nullptr) {
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

    if(input != nullptr) {
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

static bool completion_contains(const struct completion_result *result, const char *candidate) {
    for(size_t index = 0; index < result->count; ++index) {
        if(strcmp(result->items[index], candidate) == 0) {
            return true;
        }
    }
    return false;
}

static void add_alias_completions(struct completion_result *result, const char *input) {
    size_t input_length = input == nullptr ? 0 : strlen(input);
    size_t command_count = 0;
    const struct command_definition *commands = get_commands(&command_count);

    for(size_t index = 0; index < g_config.alias_count && result->count < MAX_COMPLETIONS; ++index) {
        const char *name = g_config.aliases[index].name;
        if((input_length == 0 || strncmp(name, input, input_length) == 0) &&
           find_command_definition(commands, command_count, name) == nullptr && !completion_contains(result, name)) {
            result->items[result->count++] = name;
        }
    }
}

static int
expand_completion_aliases(char *argv[], int *argc, char expansion_storage[MAX_CONFIG_ALIASES][MAX_ALIAS_EXPANSION]) {
    const char *visited[MAX_CONFIG_ALIASES];
    int visited_count = 0;

    while(*argc > 0) {
        size_t command_count = 0;
        const struct command_definition *commands = get_commands(&command_count);
        if(find_command_definition(commands, command_count, argv[0]) != nullptr) {
            return R_OK;
        }

        const struct config_alias *alias = config_find_alias(argv[0]);
        if(alias == nullptr || visited_count >= MAX_CONFIG_ALIASES) {
            return R_ERROR;
        }
        for(int index = 0; index < visited_count; ++index) {
            if(strcmp(visited[index], alias->name) == 0) {
                return R_ERROR;
            }
        }
        visited[visited_count] = alias->name;

        if(strcpy_s(expansion_storage[visited_count], MAX_ALIAS_EXPANSION, alias->expansion) != 0) {
            return R_ERROR;
        }
        char *tokens[MAX_COMMAND_ARGUMENTS];
        int token_count = 0;
        int token_result = tokenize_command_alias(expansion_storage[visited_count], tokens, &token_count);
        ++visited_count;

        if(token_result != R_OK || token_count == 0 || *argc - 1 + token_count > MAX_COMMAND_ARGUMENTS + 1) {
            return R_ERROR;
        }
        memmove(&argv[token_count], &argv[1], (size_t)(*argc - 1) * sizeof(*argv));
        memcpy(argv, tokens, (size_t)token_count * sizeof(*tokens));
        *argc = *argc - 1 + token_count;
    }

    return R_OK;
}

static struct completion_result console_completion(const char *input) {
    struct completion_result result = {};

    size_t command_count = 0;
    const struct command_definition *current_commands = get_commands(&command_count);

    if(input == nullptr) {
        return result;
    }

    char buffer[DEFAULT_BUFFER_SIZE];
    char expansion_storage[MAX_CONFIG_ALIASES][MAX_ALIAS_EXPANSION];

    if(strcpy_s(buffer, sizeof(buffer), input) != 0) {
        return result;
    }

    bool trailing_space = ends_with_whitespace(input);

    char *argv[MAX_COMMAND_ARGUMENTS + 1];
    int argc = 0;

    char *context = nullptr;
    char *token = strtok_s(buffer, " \t", &context);

    while(token != nullptr && argc < MAX_COMMAND_ARGUMENTS + 1) {
        argv[argc++] = token;
        token = strtok_s(nullptr, " \t", &context);
    }
    if(token != nullptr) {
        return result;
    }

    if(argc == 0) {
        result = complete_command_definitions("", current_commands, command_count);
        add_alias_completions(&result, "");
        add_console_special_completions(&result, "");
        return result;
    }

    if(trailing_space) {
        if(expand_completion_aliases(argv, &argc, expansion_storage) != R_OK) {
            return result;
        }
        for(int i = 0; i < argc; ++i) {
            const struct command_definition *definition =
                find_command_definition(current_commands, command_count, argv[i]);

            if(definition == nullptr) {
                return result;
            }

            current_commands = definition->subcommands;
            command_count = definition->subcommand_count;
        }

        return complete_command_definitions("", current_commands, command_count);
    }

    for(int i = 0; i < argc - 1; ++i) {
        if(i == 0) {
            if(expand_completion_aliases(argv, &argc, expansion_storage) != R_OK) {
                return result;
            }
        }
        const struct command_definition *definition = find_command_definition(current_commands, command_count, argv[i]);

        if(definition == nullptr || definition->subcommands == nullptr || definition->subcommand_count == 0) {
            return result;
        }

        current_commands = definition->subcommands;
        command_count = definition->subcommand_count;
    }

    result = complete_command_definitions(argv[argc - 1], current_commands, command_count);

    if(argc == 1) {
        add_alias_completions(&result, argv[0]);
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
    struct command_history history = {};
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

        char *command_argv[MAX_COMMAND_ARGUMENTS + 1];
        int command_argc = 0;

        char *context = nullptr;

        char *token = strtok_s(command, " \t", &context);

        while(token != nullptr && command_argc < MAX_COMMAND_ARGUMENTS + 1) {
            command_argv[command_argc++] = token;
            token = strtok_s(nullptr, " \t", &context);
        }

        if(token != nullptr) {
            log_error("Too many command arguments; maximum is %d.\n", MAX_COMMAND_ARGUMENTS);
            continue;
        }

        if(command_argc == 0) {
            continue;
        }

        run_command(command_argv[0], command_argc - 1, &command_argv[1]);
    }

    terminal_leave_interactive_mode();
    return result;
}
