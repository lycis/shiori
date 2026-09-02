#include "commands.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "hooks.h"
#include "logging.h"

enum command_index {
    COMMAND_INIT,
    COMMAND_CONFIG,
    COMMAND_ADD,
    COMMAND_CAPTURE,
    COMMAND_TOPIC,
    COMMAND_TAG,
    COMMAND_TODO,
    COMMAND_TODAY,
    COMMAND_CONSOLE,
    COMMAND_UTIL,
    COMMAND_HELP,
    COMMAND_VERSION,
    COMMAND_NOTE,
    COMMAND_COUNT
};

static struct command_definition commands[COMMAND_COUNT];
static bool commands_initialized = false;

static void init_commands() {
    size_t todo_commands_count = 0;
    const struct command_definition *todo_commands = get_todo_commands(&todo_commands_count);

    size_t util_commands_count = 0;
    const struct command_definition *util_commands = get_util_commands(&util_commands_count);

    size_t note_commands_count = 0;
    const struct command_definition *note_commands = get_note_commands(&note_commands_count);

    commands[COMMAND_INIT] =
        (struct command_definition){"init", "", "Initialize a new configuration", command_init, nullptr, 0, false};

    commands[COMMAND_CONFIG] = (struct command_definition){
        "config",
        "<command>",
        "Show or modify configuration",
        command_config,
        nullptr,
        0,
        true
    };

    commands[COMMAND_ADD] = (struct command_definition){
        "add",
        "[--topic <topic>] <note>",
        "Add a new note or thought to the day",
        command_add,
        nullptr,
        0,
        true
    };

    commands[COMMAND_CAPTURE] = (struct command_definition){
        "capture",
        "",
        "Interactively capture notes and todos",
        command_capture,
        nullptr,
        0,
        true
    };

    commands[COMMAND_TOPIC] = (struct command_definition){
        "topic",
        "<topic>",
        "Browse notes and todos by topic",
        command_topic,
        nullptr,
        0,
        true
    };

    commands[COMMAND_TAG] =
        (struct command_definition){"tag", "<tag>", "Find notes and todos by tag", command_tag, nullptr, 0, true};

    commands[COMMAND_TODO] = (struct command_definition){
        "todo",
        "<cmd>",
        "Manage your todos and tasks",
        command_todo,
        todo_commands,
        todo_commands_count,
        true
    };

    commands[COMMAND_TODAY] =
        (struct command_definition){"today", "", "Your overview for the current day", command_today, nullptr, 0, true};

    commands[COMMAND_CONSOLE] =
        (struct command_definition){"console", "", "Start the interactive console", command_console, nullptr, 0, true};

    commands[COMMAND_UTIL] = (struct command_definition){
        "util",
        "<cmd>",
        "Utility and integration commands",
        command_util,
        util_commands,
        util_commands_count,
        true
    };

    commands[COMMAND_HELP] = (struct command_definition){"help", "", "Show this help", command_help, nullptr, 0, false};

    commands[COMMAND_VERSION] = (struct command_definition){
        "version",
        "",
        "Display current version information",
        command_version,
        nullptr,
        0,
        false
    };

    commands[COMMAND_NOTE] = (struct command_definition){
        "note",
        "<cmd>",
        "Access and display details around your notes",
        command_note,
        note_commands,
        note_commands_count,
        true
    };

    commands_initialized = true;
}

const struct command_definition *get_commands(size_t *count) {
    if(!commands_initialized) {
        init_commands();
    }

    if(count != nullptr) {
        *count = COMMAND_COUNT;
    }

    return commands;
}

const struct command_definition *
find_command_definition(const struct command_definition *commands, size_t command_count, const char *name) {
    for(size_t i = 0; i < command_count; ++i) {
        if(strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }

    return nullptr;
}

const struct command_definition *find_subcommand(const struct command_definition *parent, const char *name) {
    if(parent == nullptr) {
        return nullptr;
    }

    return find_command_definition(parent->subcommands, parent->subcommand_count, name);
}

static int execute_command(const struct command_definition *command, int argc, char *argv[]) {
    if(command == nullptr) {
        return R_ERROR;
    }

    if(command->subcommand_count > 0 && command->subcommands != nullptr && argc > 0) {
        const struct command_definition *subcommand =
            find_command_definition(command->subcommands, command->subcommand_count, argv[0]);
        if(subcommand == nullptr) {
            log_error("No such subcommand '%s'.\n", argv[0]);
            return R_ERROR;
        }

        return execute_command(subcommand, argc - 1, &argv[1]);
    }

    // no subcommand
    if(command->handler == nullptr) {
        log_critical("No command handler registered for command '%s'\n", command->name);
        return R_ERROR;
    }

    return command->handler(argc, argv);
}

#define MAX_RESOLVED_ARGUMENTS 64
#define MAX_ALIAS_EXPANSIONS MAX_CONFIG_ALIASES

struct resolved_command {
    const struct command_definition *definition;
    char *name;
    int argc;
    char *arguments[MAX_RESOLVED_ARGUMENTS];
    char expansion_storage[MAX_ALIAS_EXPANSIONS][MAX_ALIAS_EXPANSION];
    bool config_loaded;
};

static int tokenize_alias(char *input, char *tokens[], int *token_count) {
    char *read = input;
    char *write = input;
    int count = 0;

    while(*read != '\0') {
        while(isspace((unsigned char)*read)) {
            ++read;
        }
        if(*read == '\0') {
            break;
        }
        if(count >= MAX_RESOLVED_ARGUMENTS) {
            *token_count = MAX_RESOLVED_ARGUMENTS + 1;
            return R_ERROR;
        }

        tokens[count++] = write;
        char quote = '\0';
        while(*read != '\0') {
            if(quote == '\0' && isspace((unsigned char)*read)) {
                break;
            }
            if(*read == '\'' || *read == '"') {
                if(quote == '\0') {
                    quote = *read++;
                    continue;
                }
                if(quote == *read) {
                    quote = '\0';
                    ++read;
                    continue;
                }
            }
            if(*read == '\\' && read[1] != '\0') {
                ++read;
            }
            *write++ = *read++;
        }
        if(quote != '\0') {
            return R_ERROR;
        }
        if(*read != '\0') {
            ++read;
        }
        *write++ = '\0';
    }

    *token_count = count;
    return R_OK;
}

static int resolve_command(char *command, int argc, char *argv[], struct resolved_command *resolved) {
    const char *visited[MAX_ALIAS_EXPANSIONS];
    int visited_count = 0;
    bool config_loaded = false;

    if(argc > MAX_RESOLVED_ARGUMENTS) {
        log_error("Too many command arguments; maximum is %d.\n", MAX_RESOLVED_ARGUMENTS);
        return R_ERROR;
    }
    memset(resolved, 0, sizeof(*resolved));
    memcpy(resolved->arguments, argv, (size_t)argc * sizeof(*argv));

    char *resolved_name = command;
    int resolved_argc = argc;
    while(true) {
        size_t command_count = 0;
        const struct command_definition *commands = get_commands(&command_count);
        const struct command_definition *definition = find_command_definition(commands, command_count, resolved_name);
        if(definition != nullptr) {
            resolved->definition = definition;
            resolved->name = resolved_name;
            resolved->argc = resolved_argc;
            resolved->config_loaded = config_loaded;
            return R_OK;
        }

        if(!config_loaded) {
            enum config_read_result config_result = read_config_file_optional();
            if(config_result == CONFIG_READ_ERROR) {
                return R_ERROR;
            }
            if(config_result == CONFIG_READ_NOT_FOUND) {
                log_error("Unknown command: %s\n", resolved_name);
                return R_ERROR;
            }
            config_loaded = true;
        }

        const struct config_alias *alias = config_find_alias(resolved_name);
        if(alias == nullptr) {
            log_error("Unknown command: %s\n", resolved_name);
            return R_ERROR;
        }
        for(int index = 0; index < visited_count; ++index) {
            if(strcmp(visited[index], alias->name) == 0) {
                log_error("Recursive command alias detected: ");
                for(int path_index = 0; path_index < visited_count; ++path_index) {
                    fprintf(stderr, "%s -> ", visited[path_index]);
                }
                fprintf(stderr, "%s\n", alias->name);
                return R_ERROR;
            }
        }
        if(visited_count >= MAX_ALIAS_EXPANSIONS) {
            log_error("Command alias expansion exceeds the maximum depth of %d.\n", MAX_ALIAS_EXPANSIONS);
            return R_ERROR;
        }
        visited[visited_count] = alias->name;
        if(strcpy_s(
               resolved->expansion_storage[visited_count],
               sizeof(resolved->expansion_storage[visited_count]),
               alias->expansion
           ) != 0) {
            return R_ERROR;
        }

        char *tokens[MAX_RESOLVED_ARGUMENTS];
        int token_count = 0;
        int token_result = tokenize_alias(resolved->expansion_storage[visited_count], tokens, &token_count);
        if(token_count > MAX_RESOLVED_ARGUMENTS) {
            log_error(
                "Command alias '%s' expands beyond the maximum of %d arguments.\n",
                alias->name,
                MAX_RESOLVED_ARGUMENTS
            );
            return R_ERROR;
        }
        if(token_result != R_OK || token_count == 0 || tokens[0][0] == '\0') {
            log_error("Invalid command alias expansion for '%s'.\n", alias->name);
            return R_ERROR;
        }
        ++visited_count;

        int prepended_count = token_count - 1;
        if(resolved_argc + prepended_count > MAX_RESOLVED_ARGUMENTS) {
            log_error(
                "Command alias '%s' expands beyond the maximum of %d arguments.\n",
                alias->name,
                MAX_RESOLVED_ARGUMENTS
            );
            return R_ERROR;
        }
        memmove(
            &resolved->arguments[prepended_count],
            resolved->arguments,
            (size_t)resolved_argc * sizeof(*resolved->arguments)
        );
        memcpy(resolved->arguments, &tokens[1], (size_t)prepended_count * sizeof(*tokens));
        resolved_argc += prepended_count;
        resolved_name = tokens[0];
    }
}

static int execute_resolved_command(const struct resolved_command *command) {
    if(command->definition == nullptr) {
        log_error("Unknown command: %s\n", command->name);
        return R_ERROR;
    }

    if(command->definition->requires_config && !command->config_loaded) {
        if(read_config_file() != R_OK) {
            return R_ERROR;
        }
    }

    int rc = execute_command(command->definition, command->argc, (char **)command->arguments);

    // call after command hook
    if(command->definition->requires_config && g_config.hooks.after_command[0] != '\0') {
        hook_after_command(command->name, command->argc, (char **)command->arguments);
    }

    return rc;
}

int run_command(char *command, int argc, char *argv[]) {
    struct resolved_command resolved;
    if(resolve_command(command, argc, argv, &resolved) != R_OK) {
        return R_ERROR;
    }
    return execute_resolved_command(&resolved);
}

int print_subcommand_help(
    const char *command_name,
    const char *description,
    const struct command_definition *commands,
    size_t command_count
) {
    if(command_name == nullptr || description == nullptr || commands == nullptr) {
        return R_ERROR;
    }

    printf("`%s %s` %s\n", APP_NAME, command_name, description);
    printf("\n");
    printf("Subcommands:\n");

    for(size_t i = 0; i < command_count; ++i) {
        char usage[DEFAULT_BUFFER_SIZE];

        if(commands[i].args != nullptr && commands[i].args[0] != '\0') {
            int written = snprintf(usage, sizeof(usage), "%s %s", commands[i].name, commands[i].args);

            if(written < 0 || (size_t)written >= sizeof(usage)) {
                return R_ERROR;
            }
        } else {
            int written = snprintf(usage, sizeof(usage), "%s", commands[i].name);

            if(written < 0 || (size_t)written >= sizeof(usage)) {
                return R_ERROR;
            }
        }

        printf("  %-24s %s\n", usage, commands[i].description);
    }

    return R_OK;
}
