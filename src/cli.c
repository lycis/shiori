#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "cli.h"

void strip_leading_flags(int *argc, char ***argv)
{
    while (*argc > 0) {
        char *arg = (*argv)[0];

        if (arg[0] != '-' || arg[1] == '\0') {
            break;
        }

        (*argv)++;
        (*argc)--;
    }
}

bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands){
    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--") == 0) {
            break;
        }

        if(argv[i][0] != '-' || argv[i][1] == '\0') {
            if(allow_subcommands) {
                break;
            }

            continue;
        }

        if(strcmp(argv[i], sw) == 0) {
            return true;
        }
    }

    return false;
}

void print_divider(size_t width) {
    printf(ANSI_FG_RGB(90, 105, 120));

    for(size_t i = 0; i < width; ++i) {
        printf("─");
    }

    printf(ANSI_RESET "\n");
}