#include "config_parser.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdckdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define CONFIG_LINE_MAX 4096
#define CONFIG_NESTING_MAX 32

struct config_level {
    size_t indentation;
    char *path;
};

static int set_error(struct config_parse_error *error, size_t line, size_t column, const char *format, ...) {
    error->line = line;
    error->column = column;

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(error->message, sizeof(error->message), format, arguments);
    va_end(arguments);
    return R_ERROR;
}

static char *copy_text(const char *text, size_t length) {
    size_t allocation_size;
    if(ckd_add(&allocation_size, length, 1)) {
        return nullptr;
    }

    char *copy = malloc(allocation_size);
    if(copy == nullptr) {
        return nullptr;
    }

    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

static bool valid_key(const char *key) {
    bool starts_with_letter = (key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= 'a' && key[0] <= 'z');
    if(!(starts_with_letter || key[0] == '_')) {
        return false;
    }

    for(size_t index = 1; key[index] != '\0'; ++index) {
        char character = key[index];
        bool letter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        bool digit = character >= '0' && character <= '9';
        if(!(letter || digit || character == '_' || character == '-')) {
            return false;
        }
    }
    return true;
}

static char *build_path(const char *parent, const char *key) {
    size_t parent_length = strlen(parent);
    size_t key_length = strlen(key);
    size_t length;
    size_t separator_length = parent_length > 0 ? 1 : 0;

    if(ckd_add(&length, parent_length, separator_length) || ckd_add(&length, length, key_length)) {
        return nullptr;
    }

    size_t allocation_size;
    if(ckd_add(&allocation_size, length, 1)) {
        return nullptr;
    }
    char *path = malloc(allocation_size);
    if(path == nullptr) {
        return nullptr;
    }

    size_t used = 0;
    if(parent_length > 0) {
        memcpy(path, parent, parent_length);
        used = parent_length;
        path[used++] = '.';
    }
    memcpy(path + used, key, key_length + 1);
    return path;
}

static char *trim_text(char *text) {
    while(isspace((unsigned char)*text)) {
        text++;
    }
    if(*text == '\0') {
        return text;
    }
    char *end = text + strlen(text) - 1;
    while(end > text && isspace((unsigned char)*end)) {
        end--;
    }
    end[1] = '\0';
    return text;
}

const struct config_entry *config_get(const struct config_document *document, const char *path) {
    for(size_t index = 0; index < document->count; ++index) {
        if(strcmp(document->entries[index].path, path) == 0) {
            return &document->entries[index];
        }
    }
    return nullptr;
}

static bool path_conflicts(const struct config_document *document, const char *path) {
    size_t path_length = strlen(path);
    for(size_t index = 0; index < document->count; ++index) {
        const char *existing = document->entries[index].path;
        size_t existing_length = strlen(existing);
        if(strcmp(existing, path) == 0) {
            return true;
        }
        if(existing_length < path_length && path[existing_length] == '.' &&
           strncmp(existing, path, existing_length) == 0) {
            return true;
        }
        if(path_length < existing_length && existing[path_length] == '.' && strncmp(existing, path, path_length) == 0) {
            return true;
        }
    }
    return false;
}

static int append_entry(struct config_document *document, struct config_entry entry) {
    if(document->count == document->capacity) {
        size_t new_capacity = document->capacity == 0 ? 8 : document->capacity * 2;
        size_t allocation_size;
        if(new_capacity < document->capacity || ckd_mul(&allocation_size, new_capacity, sizeof(*document->entries))) {
            return R_ERROR;
        }

        void *resized = realloc(document->entries, allocation_size);
        if(resized == nullptr) {
            return R_ERROR;
        }
        document->entries = resized;
        document->capacity = new_capacity;
    }

    document->entries[document->count++] = entry;
    return R_OK;
}

static int
parse_quoted_string(const char *input, char **output, struct config_parse_error *error, size_t line, size_t column) {
    char quote = input[0];
    size_t input_length = strlen(input);
    char *value = malloc(input_length + 1);
    if(value == nullptr) {
        return set_error(error, line, column, "out of memory");
    }

    size_t source = 1;
    size_t destination = 0;
    bool closed = false;
    while(input[source] != '\0') {
        if(input[source] == quote) {
            if(quote == '\'' && input[source + 1] == '\'') {
                value[destination++] = '\'';
                source += 2;
                continue;
            }
            closed = true;
            source++;
            break;
        }

        if(quote == '"' && input[source] == '\\') {
            source++;
            char escaped = input[source];
            switch(escaped) {
            case '"':
            case '\\':
                value[destination++] = escaped;
                break;
            case 'n':
                value[destination++] = '\n';
                break;
            case 'r':
                value[destination++] = '\r';
                break;
            case 't':
                value[destination++] = '\t';
                break;
            default:
                free(value);
                return set_error(error, line, column + source, "unsupported escape sequence");
            }
            source++;
            continue;
        }

        value[destination++] = input[source++];
    }

    if(!closed) {
        free(value);
        return set_error(error, line, column, "unterminated quoted string");
    }
    while(isspace((unsigned char)input[source])) {
        source++;
    }
    if(input[source] != '\0') {
        free(value);
        return set_error(error, line, column + source, "unexpected text after quoted string");
    }

    value[destination] = '\0';
    *output = value;
    return R_OK;
}

static int parse_scalar(char *text, struct config_entry *entry, struct config_parse_error *error) {
    if(text[0] == '\'' || text[0] == '"') {
        entry->type = CONFIG_VALUE_STRING;
        return parse_quoted_string(text, &entry->value.string_value, error, entry->line, entry->column);
    }

    if(strcmp(text, "true") == 0 || strcmp(text, "false") == 0) {
        entry->type = CONFIG_VALUE_BOOLEAN;
        entry->value.boolean_value = strcmp(text, "true") == 0;
        return R_OK;
    }

    if(strchr("[{}]|>&*!", text[0]) != nullptr) {
        return set_error(
            error,
            entry->line,
            entry->column,
            "unsupported YAML-style scalar syntax; quote the value to use it as text"
        );
    }

    bool looks_integer =
        isdigit((unsigned char)text[0]) || ((text[0] == '+' || text[0] == '-') && isdigit((unsigned char)text[1]));
    if(looks_integer) {
        errno = 0;
        char *end = nullptr;
        intmax_t parsed = strtoimax(text, &end, 10);
        if(*end == '\0') {
            if(errno == ERANGE || parsed < INT64_MIN || parsed > INT64_MAX) {
                return set_error(error, entry->line, entry->column, "integer is out of range");
            }
            entry->type = CONFIG_VALUE_INTEGER;
            entry->value.integer_value = (int64_t)parsed;
            return R_OK;
        }
    }

    entry->type = CONFIG_VALUE_STRING;
    entry->value.string_value = copy_text(text, strlen(text));
    if(entry->value.string_value == nullptr) {
        return set_error(error, entry->line, entry->column, "out of memory");
    }
    return R_OK;
}

void config_document_destroy(struct config_document *document) {
    if(document == nullptr) {
        return;
    }
    for(size_t index = 0; index < document->count; ++index) {
        free(document->entries[index].path);
        if(document->entries[index].type == CONFIG_VALUE_STRING) {
            free(document->entries[index].value.string_value);
        }
    }
    free(document->entries);
    memset(document, 0, sizeof(*document));
}

int config_parse_file(FILE *file, struct config_document *document, struct config_parse_error *error) {
    if(file == nullptr || document == nullptr || error == nullptr) {
        return R_ERROR;
    }

    memset(document, 0, sizeof(*document));
    memset(error, 0, sizeof(*error));
    struct config_level levels[CONFIG_NESTING_MAX] = {{.indentation = 0, .path = ""}};
    size_t depth = 0;
    bool pending_map = false;
    char *pending_path = nullptr;
    size_t pending_indentation = 0;
    size_t pending_line = 0;
    char line_buffer[CONFIG_LINE_MAX];
    size_t line_number = 0;

    while(fgets(line_buffer, sizeof(line_buffer), file) != nullptr) {
        line_number++;
        size_t length = strlen(line_buffer);
        if(length > 0 && line_buffer[length - 1] != '\n' && !feof(file)) {
            set_error(error, line_number, length + 1, "line exceeds %d bytes", CONFIG_LINE_MAX - 1);
            goto failure;
        }
        while(length > 0 && (line_buffer[length - 1] == '\n' || line_buffer[length - 1] == '\r')) {
            line_buffer[--length] = '\0';
        }

        size_t indentation = 0;
        while(line_buffer[indentation] == ' ') {
            indentation++;
        }
        if(line_buffer[indentation] == '\t') {
            set_error(error, line_number, indentation + 1, "tabs are not allowed in indentation");
            goto failure;
        }

        char *content = line_buffer + indentation;
        if(content[0] == '\0' || content[0] == '#') {
            continue;
        }

        if(pending_map) {
            if(indentation <= pending_indentation) {
                set_error(error, pending_line, pending_indentation + 1, "map must contain at least one indented entry");
                goto failure;
            }
            if(depth + 1 >= CONFIG_NESTING_MAX) {
                set_error(error, line_number, indentation + 1, "maximum nesting depth exceeded");
                goto failure;
            }
            depth++;
            levels[depth].indentation = indentation;
            levels[depth].path = pending_path;
            pending_path = nullptr;
            pending_map = false;
        } else {
            while(depth > 0 && indentation < levels[depth].indentation) {
                free(levels[depth].path);
                levels[depth].path = nullptr;
                depth--;
            }
            if(indentation != levels[depth].indentation) {
                set_error(error, line_number, indentation + 1, "indentation does not match an open map level");
                goto failure;
            }
        }

        char *colon = strchr(content, ':');
        if(colon == nullptr) {
            set_error(error, line_number, indentation + 1, "expected ':' after configuration key");
            goto failure;
        }
        *colon = '\0';
        char *key = trim_text(content);
        if(!valid_key(key)) {
            set_error(error, line_number, indentation + 1, "invalid configuration key '%s'", key);
            goto failure;
        }

        char *path = build_path(levels[depth].path, key);
        if(path == nullptr) {
            set_error(error, line_number, indentation + 1, "out of memory");
            goto failure;
        }
        if(path_conflicts(document, path)) {
            set_error(error, line_number, indentation + 1, "duplicate or conflicting configuration path '%s'", path);
            free(path);
            goto failure;
        }

        char *value = trim_text(colon + 1);
        if(value[0] == '\0') {
            pending_map = true;
            pending_path = path;
            pending_indentation = indentation;
            pending_line = line_number;
            continue;
        }

        struct config_entry entry = {
            .path = path,
            .line = line_number,
            .column = (size_t)(value - line_buffer) + 1,
        };
        if(parse_scalar(value, &entry, error) != R_OK) {
            free(path);
            goto failure;
        }
        if(append_entry(document, entry) != R_OK) {
            if(entry.type == CONFIG_VALUE_STRING) {
                free(entry.value.string_value);
            }
            free(path);
            set_error(error, line_number, indentation + 1, "out of memory");
            goto failure;
        }
    }

    if(ferror(file)) {
        set_error(error, line_number, 1, "failed reading configuration file");
        goto failure;
    }
    if(pending_map) {
        set_error(error, pending_line, pending_indentation + 1, "map must contain at least one indented entry");
        goto failure;
    }

    while(depth > 0) {
        free(levels[depth--].path);
    }
    return R_OK;

failure:
    free(pending_path);
    while(depth > 0) {
        free(levels[depth--].path);
    }
    config_document_destroy(document);
    return R_ERROR;
}
