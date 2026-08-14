
#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "common.h"
#include "logging.h"
#include "config.h"

struct configuration g_config;

int read_config_file() {

    // clear the whole config
    memset(&g_config, 0, sizeof(g_config));

    char config_path[DEFAULT_BUFFER_SIZE];

    if(access(CONFIG_FILE_NAME, F_OK) == 0) {
        strcpy_s(config_path, sizeof(config_path), CONFIG_FILE_NAME);
    } else {
        char user_home[DEFAULT_BUFFER_SIZE];

        if(get_user_home(user_home, sizeof(user_home)) != R_OK) {
            log_error("Could not determine user home directory.\n");
            return R_ERROR;
        }

        snprintf(config_path, sizeof(config_path),"%s%s%s", user_home, get_path_separator(), CONFIG_FILE_NAME);

        if(access(config_path, F_OK) != 0) {
            log_error("%s config file not found. Please run `%s init` first.\n", CONFIG_FILE_NAME, APP_NAME);
            return R_ERROR;
        }
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, config_path, "r");
    if(err != 0 || config_file == NULL) {
        log_error("Error opening %s file\n", CONFIG_FILE_NAME);
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE];
    int lnr = 0;
    while(fgets(line, sizeof(line), config_file) != NULL) {
        lnr++;
        if(line[0] == '#') continue; // comment
        if(strlen(line) == 0 || line[0] == '\n') continue; // empty line

        if(strstr(line, ":") == NULL) {
            log_error("Invalid config entry at line %d\n", lnr);
            fclose(config_file);
            return R_ERROR;
        }

        char *colon = strchr(line, ':');

        if(colon == NULL) {
            log_error("Invalid config entry at line %d\n", lnr);
            return R_ERROR;
        }

        *colon = '\0';

        char *key = trim(line);
        char *value = trim(colon + 1);

        if(strcmp(key, "version") == 0) {
            g_config.version = atoi(value);
            if(g_config.version == 0) {
                log_error("Invalid config version at line %d\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        } else if(strcmp(key, "base_dir") == 0) {
            size_t len = strlen(value);
            if(len > 0) {
                char lc = value[strlen(value)-1];
                if(lc == '\\' || lc == '/') value[strlen(value)-1] = '\0';
                strcpy_s(g_config.base_dir, sizeof(g_config.base_dir), value);
            } else {
                log_error("invalid configuration (line %d): Empty base directory is not permitted.\n", lnr);
            }
        } else if(strcmp(key, "hook_after_command") == 0) {
            if(strlen(value) > 0) {
                if(strcpy_s(g_config.hooks.after_command, sizeof(g_config.hooks.after_command), value) != 0) {
                    log_error("invalid configuration (line %d): after_command_hook path is too long\n", lnr);
                    fclose(config_file);
                    return R_ERROR;
                }
            }
        }
    }

    fclose(config_file);
    
    return R_OK;
}
