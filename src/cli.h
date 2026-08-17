#ifndef _SHIORI_CLI_H
#define _SHIORI_CLI_H

#include <stdbool.h>
#include "color.h"

void strip_leading_flags(int *argc, char ***argv);
bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands);
void print_divider(size_t width);

typedef const char *(*completion_fn)(const char *input);

int read_interactive_line(const char *prompt, char *buffer, size_t buffer_size, completion_fn complete);

#endif