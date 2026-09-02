#ifndef SHIORI_CONFIG_H
#define SHIORI_CONFIG_H

#include "common.h"

#define MAX_CONFIG_ALIASES 32
#define MAX_ALIAS_NAME 64
#define MAX_ALIAS_EXPANSION DEFAULT_BUFFER_SIZE

struct config_hooks {
    char after_command[DEFAULT_BUFFER_SIZE];
};

struct config_alias {
    char name[MAX_ALIAS_NAME];
    char expansion[MAX_ALIAS_EXPANSION];
};

struct configuration {
    int version;
    char base_dir[4096];
    bool color;
    char todo_filename[2048];
    char notes_filename[2048];
    struct config_hooks hooks;
    struct config_alias aliases[MAX_CONFIG_ALIASES];
    size_t alias_count;
};

extern struct configuration g_config;

enum config_read_result {
    CONFIG_READ_OK,
    CONFIG_READ_NOT_FOUND,
    CONFIG_READ_ERROR,
};

[[nodiscard]] int read_config_file(void);
[[nodiscard]] enum config_read_result read_config_file_optional(void);
const struct config_alias *config_find_alias(const char *name);

#endif
