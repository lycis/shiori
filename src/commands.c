#include "common.h"
#include "commands.h"
#include "config.h"
#include "hooks.h"
#include "logging.h"
#include <string.h>
#include <stdlib.h>

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
