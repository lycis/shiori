#ifndef SHIORI_COMMAND_H
#define SHIORI_COMMAND_H

#include <stdbool.h>

typedef int (*command_handler_fn)(int argc, char *argv[]);

struct command_definition {
    const char *name;
    const char *args;
    const char *description;
    command_handler_fn handler;

    const struct command_definition *subcommands;
    size_t subcommand_count;

    bool requires_config;
};

int command_init(int argc, char *argv[]);

int command_config(int argc, char *argv[]);

int command_todo(int argc, char *argv[]);
const struct command_definition *get_todo_commands(size_t *count);
int command_todo_add(int argc, char *argv[]); // used in cmd_capture

int command_add(int argc, char *argv[]);

int command_today(int argc, char *argv[]);

int command_topic(int argc, char *argv[]);

int command_capture(int argc, char *argv[]);

int command_tag(int argc, char *argv[]);

int command_console(int argc, char *argv[]);

int command_util(int argc, char *argv[]);
const struct command_definition *get_util_commands(size_t *count);

int command_version(int argc, char *argv[]);

int command_help(int argc, char *argv[]);

int command_note(int argc, char *argv[]);
const struct command_definition *get_note_commands(size_t *count);

const struct command_definition *get_commands(size_t *count);
const struct command_definition *
find_command_definition(const struct command_definition *commands, size_t command_count, const char *name);
const struct command_definition *find_subcommand(const struct command_definition *parent, const char *name);

int run_command(char *command, int argc, char *argv[]);

int print_subcommand_help(
    const char *command_name,
    const char *description,
    const struct command_definition *commands,
    size_t command_count
);

#endif
