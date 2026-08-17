#ifndef _SHIORI_COMMAND_H
#define _SHIORI_COMMAND_H

#include <stdbool.h>

typedef int (*command_handler_fn)(int argc, char *argv[]);

struct command_definition {
    const char *name;
    const char *description;
    command_handler_fn handler;

    const struct command_definition *subcommands;
    size_t subcommand_count;

    bool requires_config;
};

int command_init(int argc, char* argv[]);
int command_config(int argc, char* argv[]);
int command_todo(int argc, char* argv[]);
int command_add(int argc, char* argv[]);
int command_today(int argc, char* argv[]);
int command_topic(int argc, char *argv[]);
int command_capture(int argc, char *argv[]);
int command_tag(int argc, char *argv[]);
int command_console(int argc, char*argv[]);
int command_util(int argc, char*argv[]);
int command_version(int argc, char * argv[]);
int command_help(int argc, char* argv[]);

const struct command_definition *get_commands(size_t *count);
const struct command_definition *find_command(const struct command_definition *commands, size_t command_count, const char *name);

int run_command(char* command, int argc, char* argv[]);

#endif