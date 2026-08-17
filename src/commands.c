#include "common.h"
#include "commands.h"
#include "config.h"
#include "hooks.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int run_command(char* command, int argc, char* argv[]) {
    if(strcmp(command, "version") == 0) {
        return command_version(argc, argv);
    } else if(strcmp(command, "help") == 0) {
        printf("%s is a console scratchpad tool that helps you maintain thoughts, quick notes and todos in a quick fire-and-forget fashion.", APP_NAME);
        printf("usage: %s [options] <command> [options] [subcommand] ...\n", APP_NAME);
        printf("\n");
        printf("Options:\n");
        printf("  %-16s %s\n", "--debug", "Show debug and plumbing output.");
        printf("\n");
        printf("Available commands:\n");
        printf("  %-16s %s\n", "init",   "Initialize a new configuration");
        printf("  %-16s %s\n", "config", "Show or modify configuration");
        printf("  %-16s %s\n", "add",    "Add a new note or thought to the day");
        printf("  %-16s %s\n", "capture","Interactively capture notes and todos");
        printf("  %-16s %s\n", "topic",  "Browse notes by topic");
        printf("  %-16s %s\n", "tag",    "Find notes and todos by tag");
        printf("  %-16s %s\n", "todo",   "Manage your todos and tasks");
        printf("  %-16s %s\n", "today",  "Your overview for the current day");
        printf("  %-16s %s\n", "console","Start the interactive console");
        printf("  %-16s %s\n", "help",   "Show this help");
        printf("  %-16s %s\n", "version","Display current version information");
        return R_OK;
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
