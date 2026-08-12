#ifndef _SHIORI_CONFIG_H
#define _SHIORI_CONFIG_H

struct configuration {
    int version;
    char base_dir[4096];
};

extern struct configuration g_config;

int read_config_file();

#endif