#include <stdio.h>
#include "common.h"

int command_help(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    
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
}