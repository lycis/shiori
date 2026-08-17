#include "common.h"
#include "commands.h"
#include "config.h"
#include "hooks.h"
#include "logging.h"
#include <string.h>
#include <stdlib.h>

static const struct command_definition commands[] = {
    {
        "init",
        "Initialize a new configuration",
        command_init,
        NULL,
        0,
        false
    },
    {
        "config",
        "Show or modify configuration",
        command_config,
        NULL,
        0,
        true
    },
    {
        "add",
        "Add a new note or thought to the day",
        command_add,
        NULL,
        0,
        true
    },
    {
        "capture",
        "Interactively capture notes and todos",
        command_capture,
        NULL,
        0,
        true
    },
    {
        "topic",
        "Browse notes by topic",
        command_topic,
        NULL,
        0,
        true
    },
    {
        "tag",
        "Find notes and todos by tag",
        command_tag,
        NULL,
        0,
        true
    },
    {
        "todo",
        "Manage your todos and tasks",
        command_todo,
        NULL,
        0,
        true
    },
    {
        "today",
        "Your overview for the current day",
        command_today,
        NULL,
        0,
        true
    },
    {
        "console",
        "Start the interactive console",
        command_console,
        NULL,
        0,
        true
    },
    {
        "util",
        "Utility and integration commands",
        command_util,
        NULL,
        0,
        true
    },
    {
        "help",
        "Show this help",
        command_help,
        NULL,
        0,
        false
    },
    {
        "version",
        "Display current version information",
        command_version,
        NULL,
        0,
        false
    }
};

const struct command_definition* get_commands(size_t *count) {
    if(count != NULL) {
        *count = sizeof(commands) / sizeof(commands[0]);
    }

    return commands;
}

const struct command_definition* find_command(const struct command_definition *commands, size_t command_count, const char *name) {
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

int run_command(char* command, int argc, char* argv[]) {
    if(strcmp(command, "version") == 0) {
        return command_version(argc, argv);
    } else if(strcmp(command, "help") == 0) {
        return command_help(argc, argv);
    } else if(strcmp(command, "init") == 0) {
        return command_init(argc, argv);
    }

    if(read_config_file() != R_OK) {
        exit(SHIORI_EXIT_CONFIG_ERROR);
    }

    int rc = -1;
    if(strcmp(command, "config") == 0) {
        rc = command_config(argc, argv);
    } else if(strcmp(command, "console") == 0) {
        rc = command_console(argc, argv);
    } else if(strcmp(command, "add") == 0) {
        rc = command_add(argc, argv);
    } else if(strcmp(command, "todo") == 0) {
        rc = command_todo(argc, argv);
    } else if(strcmp(command, "today") == 0) {
        rc = command_today(argc, argv);
    } else if(strcmp(command, "topic") == 0) {
        rc = command_topic(argc, argv);
    } else if(strcmp(command, "capture") == 0) {
        rc = command_capture(argc, argv);
    } else if(strcmp(command, "tag") == 0) {
        rc = command_tag(argc, argv);
    } else if(strcmp(command, "util") == 0) {
        rc = command_util(argc, argv);
    } else {
        log_error("Unknown command: %s\n", command);
        return R_ERROR;
    }

    // call after command hook
    if(g_config.hooks.after_command[0] != '\0') {
        hook_after_command(command, argc, argv);
    }
    
    return rc;
} 
