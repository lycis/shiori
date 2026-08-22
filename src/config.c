
#include "config.h"

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "logging.h"
#include "platform.h"

struct configuration g_config;

int read_config_file() {
    log_debug("Reading config file.\n");

    // clear the whole config
    memset(&g_config, 0, sizeof(g_config));
    g_config.color = true;

    char config_path[DEFAULT_BUFFER_SIZE];

    if(file_access_utf8(CONFIG_FILE_NAME, F_OK) == 0) {
        strcpy_s(config_path, sizeof(config_path), CONFIG_FILE_NAME);
    } else {
        char user_home[DEFAULT_BUFFER_SIZE];

        if(get_user_home(user_home, sizeof(user_home)) != R_OK) {
            log_error("Could not determine user home directory.\n");
            return R_ERROR;
        }

        snprintf(config_path, sizeof(config_path), "%s%s%s", user_home, get_path_separator(), CONFIG_FILE_NAME);

        if(file_access_utf8(config_path, F_OK) != 0) {
            log_error("%s config file not found. Please run `%s init` first.\n", CONFIG_FILE_NAME, APP_NAME);
            return R_ERROR;
        }
    }

    FILE *config_file = NULL;
    int err = file_open_utf8(&config_file, config_path, "r");
    if(err != 0 || config_file == NULL) {
        log_error("Error opening %s file\n", CONFIG_FILE_NAME);
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE];
    int lnr = 0;
    while(fgets(line, sizeof(line), config_file) != NULL) {
        lnr++;
        if(line[0] == '#') {
            continue; // comment
        }
        if(strlen(line) == 0 || line[0] == '\n') {
            continue; // empty line
        }

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
            if(parse_int(value, &g_config.version) != R_OK || g_config.version == 0) {
                log_error("Invalid config version at line %d\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        } else if(strcmp(key, "base_dir") == 0) {
            size_t len = strlen(value);
            if(len > 0) {
                char lc = value[strlen(value) - 1];
                if(lc == '\\' || lc == '/') {
                    value[strlen(value) - 1] = '\0';
                }
                strcpy_s(g_config.base_dir, sizeof(g_config.base_dir), value);
            } else {
                log_error("invalid configuration (line %d): Empty base directory is not permitted.\n", lnr);
            }
        } else if(strcmp(key, "color") == 0) {
            if(strcmp(value, "true") == 0) {
                g_config.color = true;
            } else if(strcmp(value, "false") == 0) {
                g_config.color = false;
            } else {
                log_error("invalid configuration (line %d): color must be true or false.\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        } else if(strcmp(key, "hook_after_command") == 0) {
            if(strlen(value) > 0) {
                if(strcpy_s(g_config.hooks.after_command, sizeof(g_config.hooks.after_command), value) != 0) {
                    log_error("invalid configuration (line %d): hook_after_command path is too long\n", lnr);
                    fclose(config_file);
                    return R_ERROR;
                }
            }
        }
    }

    fclose(config_file);

    return R_OK;
}
