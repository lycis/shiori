
#include "config.h"

#include <stdio.h>
#include <string.h>

#include "color.h"
#include "common.h"
#include "logging.h"
#include "platform.h"

struct configuration g_config;

static bool is_path_separator(char value) {
    return value == '/' || value == '\\';
}

static int validate_relative_filename(const char *key, char *filename) {
    if(filename[0] == '\0') {
        log_error("invalid configuration: %s must not be empty.\n", key);
        return R_ERROR;
    }

    if(is_path_separator(filename[0]) || strchr(filename, ':') != nullptr) {
        log_error("invalid configuration: %s must be relative to base_dir.\n", key);
        return R_ERROR;
    }

    size_t component_start = 0;
    size_t length = strlen(filename);

    for(size_t index = 0; index <= length; ++index) {
        if(index < length && !is_path_separator(filename[index])) {
            continue;
        }

        size_t component_length = index - component_start;
        bool dot_component = component_length == 1 && filename[component_start] == '.';
        bool parent_component =
            component_length == 2 && filename[component_start] == '.' && filename[component_start + 1] == '.';

        if(component_length == 0 || dot_component || parent_component) {
            log_error("invalid configuration: %s contains an invalid path component.\n", key);
            return R_ERROR;
        }

        if(index < length) {
            filename[index] = get_path_separator()[0];
            component_start = index + 1;
        }
    }

    char full_path[DEFAULT_BUFFER_SIZE];
    return get_base_dir_file_path(filename, full_path, sizeof(full_path));
}

static bool filenames_equal(const char *left, const char *right) {
#ifdef _WIN32
    return _stricmp(left, right) == 0;
#else
    return strcmp(left, right) == 0;
#endif
}

static bool filename_conflicts_with(const char *filename, const char *other) {
    char artifact[DEFAULT_BUFFER_SIZE];

    return filenames_equal(filename, other) ||
           (snprintf(artifact, sizeof(artifact), "%s.tmp", other) > 0 && filenames_equal(filename, artifact)) ||
           (snprintf(artifact, sizeof(artifact), "%s.bak", other) > 0 && filenames_equal(filename, artifact));
}

static int validate_storage_filenames(void) {
    if(validate_relative_filename("todo_filename", g_config.todo_filename) != R_OK ||
       validate_relative_filename("notes_filename", g_config.notes_filename) != R_OK) {
        return R_ERROR;
    }

    if(filename_conflicts_with(g_config.todo_filename, g_config.notes_filename) ||
       filename_conflicts_with(g_config.notes_filename, g_config.todo_filename)) {
        log_error("invalid configuration: todo and notes storage paths conflict.\n");
        return R_ERROR;
    }

    return R_OK;
}

int read_config_file(void) {
    log_debug("Reading config file.\n");

    // clear the whole config
    memset(&g_config, 0, sizeof(g_config));
    g_config.color = true;
    strcpy_s(g_config.todo_filename, sizeof(g_config.todo_filename), "TODOS.md");
    strcpy_s(g_config.notes_filename, sizeof(g_config.notes_filename), "NOTES.md");

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

    FILE *config_file = nullptr;
    int err = file_open_utf8(&config_file, config_path, "r");
    if(err != 0 || config_file == nullptr) {
        log_error("Error opening %s file\n", CONFIG_FILE_NAME);
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE];
    int lnr = 0;
    while(fgets(line, sizeof(line), config_file) != nullptr) {
        lnr++;
        if(line[0] == '#') {
            continue; // comment
        }
        if(strlen(line) == 0 || line[0] == '\n') {
            continue; // empty line
        }

        if(strstr(line, ":") == nullptr) {
            log_error("Invalid config entry at line %d\n", lnr);
            fclose(config_file);
            return R_ERROR;
        }

        char *colon = strchr(line, ':');

        if(colon == nullptr) {
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
        } else if(strcmp(key, "todo_filename") == 0) {
            if(strlen(value) == 0) {
                log_error("invalid configuration file (line %d): todo_filename must not be empty.\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
            if(strcpy_s(g_config.todo_filename, sizeof(g_config.todo_filename), value) != 0) {
                log_error("invalid configuration file (line %d): todo_filename is too long.\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        } else if(strcmp(key, "notes_filename") == 0) {
            if(strlen(value) == 0) {
                log_error("invalid configuration file (line %d): notes_filename must not be empty.\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
            if(strcpy_s(g_config.notes_filename, sizeof(g_config.notes_filename), value) != 0) {
                log_error("invalid configuration file (line %d): notes_filename is too long.\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        }
    }

    fclose(config_file);

    if(validate_storage_filenames() != R_OK) {
        return R_ERROR;
    }

    if(!g_config.color) {
        color_set_enabled(false);
    }

    return R_OK;
}
