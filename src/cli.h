#ifndef _SHIORI_CLI_H
#define _SHIORI_CLI_H

#include <stdbool.h>

void strip_leading_flags(int *argc, char ***argv);
bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands);
void print_divider(size_t width);

#define ANSI_BOLD  "\x1b[1m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_FG_RGB(r, g, b) "\x1b[38;2;" #r ";" #g ";" #b "m"

#endif