#ifndef _SHIORI_CLI_H
#define _SHIORI_CLI_H

#include <stdbool.h>
#include "color.h"

void strip_leading_flags(int *argc, char ***argv);
bool has_switch(int argc, char *argv[], const char *sw, bool allow_subcommands);
void print_divider(size_t width);

#define MAX_COMPLETIONS 16

struct completion_result {
    const char *items[MAX_COMPLETIONS];
    size_t count;
};

typedef struct completion_result (*completion_fn)(const char *input);

int read_interactive_line(const char *prompt, char *buffer, size_t buffer_size, completion_fn complete);
struct completion_result find_completions(const char *input, const char *options[], size_t option_count);

#endif