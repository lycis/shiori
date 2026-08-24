#ifndef SHIORI_CONFIG_H
#define SHIORI_CONFIG_H

#include "common.h"
struct config_hooks {
    char after_command[DEFAULT_BUFFER_SIZE];
};

struct configuration {
    int version;
    char base_dir[4096];
    bool color;
    char todo_filename[2048];
    char notes_filename[2048];
    struct config_hooks hooks;
};

extern struct configuration g_config;

[[nodiscard]] int read_config_file(void);

#endif
