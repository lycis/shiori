#include "common.h"
#include "commands.h"
#include "config.h"
#include "hooks.h"
#include "logging.h"
#include <string.h>
#include <stdlib.h>

#define COMMAND_COUNT 12

static struct command_definition commands[COMMAND_COUNT];
static bool commands_initialized = false;

static void init_commands() {
    size_t todo_commands_count = 0;
    const struct command_definition* todo_commands = get_todo_commands(&todo_commands_count);

    size_t util_commands_count = 0;
    const struct command_definition* util_commands = get_util_commands(&util_commands_count);

    commands[0] = (struct command_definition) {
        "init",
        "Initialize a new configuration",
        command_init,
        NULL,
        0,
        false
    };

    commands[1] = (struct command_definition) {
        "config",
        "Show or modify configuration",
        command_config,
        NULL,
        0,
        true
    };

    commands[2] = (struct command_definition) {
        "add",
        "Add a new note or thought to the day",
        command_add,
        NULL,
        0,
        true
    };

    commands[3] = (struct command_definition) {
        "capture",
        "Interactively capture notes and todos",
        command_capture,
        NULL,
        0,
        true
    };

    commands[4] = (struct command_definition) {
        "topic",
        "Browse notes by topic",
        command_topic,
        NULL,
        0,
        true
    };

    commands[5] = (struct command_definition) {
        "tag",
        "Find notes and todos by tag",
        command_tag,
        NULL,
        0,
        true
    };

    commands[6] = (struct command_definition) {
        "todo",
        "Manage your todos and tasks",
        command_todo,
        todo_commands,
        todo_commands_count,
        true
    };

    commands[7] = (struct command_definition) {
        "today",
        "Your overview for the current day",
        command_today,
        NULL,
        0,
        true
    };

    commands[8] = (struct command_definition) {
        "console",
        "Start the interactive console",
        command_console,
        NULL,
        0,
        true
    };

    commands[9] = (struct command_definition) {
        "util",
        "Utility and integration commands",
        command_util,
        util_commands,
        util_commands_count,
        true
    };

    commands[10] = (struct command_definition) {
        "help",
        "Show this help",
        command_help,
        NULL,
        0,
        false
    };

    commands[11] = (struct command_definition) {
        "version",
        "Display current version information",
        command_version,
        NULL,
        0,
        false
    };

    commands_initialized = true;
}

const struct command_definition* get_commands(size_t *count) {
    if(!commands_initialized) {
        init_commands();
    }

    if(count != NULL) {
        *count = COMMAND_COUNT;
    }

    return commands;
}

const struct command_definition* find_command_definition(const struct command_definition *commands, size_t command_count, const char *name) {
    if(commands == NULL || name == NULL) {
        return NULL;
    }

    for(size_t i = 0; i < command_count; ++i) {
        if(strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }

    return NULL;
}

const struct command_definition *find_subcommand(const struct command_definition *parent, const char *name) {
    if(parent == NULL) {
        return NULL;
    }

    return find_command_definition(parent->subcommands,parent->subcommand_count, name);
}

int run_command(char* command, int argc, char* argv[]) {
    size_t command_count = 0;
    const struct command_definition* commands = get_commands(&command_count);
    const struct command_definition* current_command = find_command_definition(commands, command_count, command);

    if(current_command == NULL) {
        log_error("Unknown command: %s\n", command);
        return R_ERROR;
    }

    if(current_command->requires_config) {
        if(read_config_file() != R_OK) {
            return R_ERROR;
        }   
    }

    int rc = current_command->handler(argc, argv);
    
    // call after command hook
    if(current_command->requires_config && g_config.hooks.after_command[0] != '\0') {
        hook_after_command(command, argc, argv);
    }
    
    return rc;
} 
