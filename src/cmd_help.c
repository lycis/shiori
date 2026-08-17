#include <stdio.h>
#include "common.h"
#include "commands.h"

int command_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    size_t command_count = 0;
    const struct command_definition *commands = get_commands(&command_count);

    printf(
        "%s is a console scratchpad tool that helps you maintain "
        "thoughts, quick notes and todos in a quick fire-and-forget fashion.\n",
        APP_NAME
    );

    printf(
        "usage: %s [options] <command> [options] [subcommand] ...\n",
        APP_NAME
    );

    printf("\n");
    printf("Options:\n");
    printf(
        "  %-16s %s\n",
        "--debug",
        "Show debug and plumbing output."
    );

    printf("\n");
    printf("Available commands:\n");

    for(size_t i = 0; i < command_count; ++i) {
        printf("  %-16s %s\n", commands[i].name, commands[i].description
        );
    }

    return R_OK;
}