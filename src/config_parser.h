#ifndef SHIORI_CONFIG_PARSER_H
#define SHIORI_CONFIG_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum config_value_type {
    CONFIG_VALUE_STRING,
    CONFIG_VALUE_INTEGER,
    CONFIG_VALUE_BOOLEAN,
};

struct config_entry {
    char *path;
    enum config_value_type type;
    size_t line;
    size_t column;
    union {
        char *string_value;
        int64_t integer_value;
        bool boolean_value;
    } value;
};

struct config_document {
    struct config_entry *entries;
    size_t count;
    size_t capacity;
};

struct config_parse_error {
    size_t line;
    size_t column;
    char message[256];
};

[[nodiscard]] int config_parse_file(FILE *file, struct config_document *document, struct config_parse_error *error);
void config_document_destroy(struct config_document *document);
[[nodiscard]] const struct config_entry *config_get(const struct config_document *document, const char *path);

#endif
