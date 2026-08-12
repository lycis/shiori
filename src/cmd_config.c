#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "common.h"
#include "cli.h"
#include "config.h"

int command_config(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("No config command provided. Please provide a config command.\n");
        return R_ERROR;
    }

    if(has_switch(argc, argv, "--help", true) || has_switch(argc, argv, "-h", true)) {
        printf("Allows you to view and manage the config file.\n");
        printf("\n");
        printf("Available config commands:\n");
        printf("  show: Show the current config version\n");
        return R_OK;
    }

    if(strcmp(argv[0], "show") == 0) {
        printf("version: %d\n", g_config.version);
        printf("base_dir: %s\n", g_config.base_dir);
    } else {
        log_error("Unknown config command. See --help\n", argv[0]);
        return R_ERROR;
    }

    return R_OK;
}
