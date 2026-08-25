
#include "config.h"

#include <stdio.h>
#include <string.h>

#include "color.h"
#include "common.h"
#include "config_parser.h"
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

static bool is_known_config_path(const char *path) {
    static const char *known_paths[] = {
        "version",
        "base_dir",
        "color",
        "notes_filename",
        "todo_filename",
        "hook_after_command",
        "hooks.after_command",
    };

    for(size_t index = 0; index < sizeof(known_paths) / sizeof(known_paths[0]); ++index) {
        if(strcmp(path, known_paths[index]) == 0) {
            return true;
        }
    }
    return false;
}

static int copy_config_string(
    const struct config_document *document,
    const char *path,
    char *destination,
    size_t destination_size,
    bool required,
    bool allow_empty
) {
    const struct config_entry *entry = config_get(document, path);
    if(entry == nullptr) {
        if(required) {
            log_error("invalid configuration: required key '%s' is missing.\n", path);
            return R_ERROR;
        }
        return R_OK;
    }
    if(entry->type != CONFIG_VALUE_STRING) {
        log_error("invalid configuration (line %zu): %s must be a string.\n", entry->line, path);
        return R_ERROR;
    }
    if(!allow_empty && entry->value.string_value[0] == '\0') {
        log_error("invalid configuration (line %zu): %s must not be empty.\n", entry->line, path);
        return R_ERROR;
    }
    if(strcpy_s(destination, destination_size, entry->value.string_value) != 0) {
        log_error("invalid configuration (line %zu): %s is too long.\n", entry->line, path);
        return R_ERROR;
    }
    return R_OK;
}

static int load_application_config(const struct config_document *document) {
    for(size_t index = 0; index < document->count; ++index) {
        const struct config_entry *entry = &document->entries[index];
        if(!is_known_config_path(entry->path)) {
            log_error("invalid configuration (line %zu): unknown key '%s'.\n", entry->line, entry->path);
            return R_ERROR;
        }
    }

    const struct config_entry *version = config_get(document, "version");
    if(version == nullptr) {
        log_error("invalid configuration: required key 'version' is missing.\n");
        return R_ERROR;
    }
    if(version->type != CONFIG_VALUE_INTEGER || version->value.integer_value != CONFIG_VERSION) {
        log_error("invalid configuration (line %zu): version must be %d.\n", version->line, CONFIG_VERSION);
        return R_ERROR;
    }
    g_config.version = (int)version->value.integer_value;

    if(copy_config_string(document, "base_dir", g_config.base_dir, sizeof(g_config.base_dir), true, false) != R_OK) {
        return R_ERROR;
    }
    size_t base_dir_length = strlen(g_config.base_dir);
    if(base_dir_length > 1 && is_path_separator(g_config.base_dir[base_dir_length - 1])) {
        g_config.base_dir[base_dir_length - 1] = '\0';
    }

    const struct config_entry *color = config_get(document, "color");
    if(color != nullptr) {
        if(color->type != CONFIG_VALUE_BOOLEAN) {
            log_error("invalid configuration (line %zu): color must be true or false.\n", color->line);
            return R_ERROR;
        }
        g_config.color = color->value.boolean_value;
    }

    if(copy_config_string(
           document,
           "notes_filename",
           g_config.notes_filename,
           sizeof(g_config.notes_filename),
           false,
           false
       ) != R_OK ||
       copy_config_string(
           document,
           "todo_filename",
           g_config.todo_filename,
           sizeof(g_config.todo_filename),
           false,
           false
       ) != R_OK) {
        return R_ERROR;
    }

    const struct config_entry *legacy_hook = config_get(document, "hook_after_command");
    const struct config_entry *nested_hook = config_get(document, "hooks.after_command");
    if(legacy_hook != nullptr && nested_hook != nullptr) {
        log_error("invalid configuration: hook_after_command and hooks.after_command cannot both be configured.\n");
        return R_ERROR;
    }
    const char *hook_path = nested_hook != nullptr ? "hooks.after_command" : "hook_after_command";
    if(copy_config_string(
           document,
           hook_path,
           g_config.hooks.after_command,
           sizeof(g_config.hooks.after_command),
           false,
           true
       ) != R_OK) {
        return R_ERROR;
    }

    return validate_storage_filenames();
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

    struct config_document document;
    struct config_parse_error parse_error;
    int parse_result = config_parse_file(config_file, &document, &parse_error);
    fclose(config_file);
    if(parse_result != R_OK) {
        log_error("%s:%zu:%zu: %s.\n", config_path, parse_error.line, parse_error.column, parse_error.message);
        return R_ERROR;
    }

    int load_result = load_application_config(&document);
    config_document_destroy(&document);
    if(load_result != R_OK) {
        return R_ERROR;
    }

    if(!g_config.color) {
        color_set_enabled(false);
    }

    return R_OK;
}
