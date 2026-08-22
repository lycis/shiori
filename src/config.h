#ifndef SHIORI_CONFIG_H
#define SHIORI_CONFIG_H

#include "common.h"
struct config_hooks {
    char after_command[DEFAULT_BUFFER_SIZE];
};

struct configuration {
    int version;
    char base_dir[4096];
    struct config_hooks hooks;
};

extern struct configuration g_config;

int read_config_file();

#endif
