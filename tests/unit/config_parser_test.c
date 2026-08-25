#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config_parser.h"

static int failures = 0;

#define ASSERT_TRUE(condition)                                                                                         \
    do {                                                                                                               \
        if(!(condition)) {                                                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                       \
            failures++;                                                                                                \
        }                                                                                                              \
    } while(false)

static int parse_text(const char *text, struct config_document *document, struct config_parse_error *error) {
    FILE *file = tmpfile();
    if(file == nullptr) {
        return R_ERROR;
    }
    fwrite(text, 1, strlen(text), file);
    rewind(file);
    int result = config_parse_file(file, document, error);
    fclose(file);
    return result;
}

static void test_flattened_paths_and_flexible_indentation(void) {
    const char *text = "version: 1\n"
                       "hooks:\n"
                       "    after_command: scripts\\after.cmd\n"
                       "aliases:\n"
                       " summarize: \"llm summarize\"\n"
                       " review: 'llm review'\n";
    struct config_document document;
    struct config_parse_error error;

    ASSERT_TRUE(parse_text(text, &document, &error) == R_OK);
    ASSERT_TRUE(document.count == 4);
    const struct config_entry *version = config_get(&document, "version");
    const struct config_entry *hook = config_get(&document, "hooks.after_command");
    const struct config_entry *alias = config_get(&document, "aliases.summarize");
    ASSERT_TRUE(version != nullptr && version->type == CONFIG_VALUE_INTEGER && version->value.integer_value == 1);
    ASSERT_TRUE(hook != nullptr && strcmp(hook->value.string_value, "scripts\\after.cmd") == 0);
    ASSERT_TRUE(alias != nullptr && strcmp(alias->value.string_value, "llm summarize") == 0);
    config_document_destroy(&document);
}

static void test_scalars_comments_utf8_and_escapes(void) {
    const char *text = "# comment\r\n"
                       "enabled: true\r\n"
                       "disabled: false\r\n"
                       "negative: -42\r\n"
                       "empty: \"\"\r\n"
                       "utf8: Grüße 世界\r\n"
                       "single: 'it''s literal'\r\n"
                       "escaped: \"a\\tb\\n\\\"c\\\"\"";
    struct config_document document;
    struct config_parse_error error;

    ASSERT_TRUE(parse_text(text, &document, &error) == R_OK);
    ASSERT_TRUE(config_get(&document, "enabled")->value.boolean_value);
    ASSERT_TRUE(!config_get(&document, "disabled")->value.boolean_value);
    ASSERT_TRUE(config_get(&document, "negative")->value.integer_value == -42);
    ASSERT_TRUE(strcmp(config_get(&document, "empty")->value.string_value, "") == 0);
    ASSERT_TRUE(strcmp(config_get(&document, "utf8")->value.string_value, "Grüße 世界") == 0);
    ASSERT_TRUE(strcmp(config_get(&document, "single")->value.string_value, "it's literal") == 0);
    ASSERT_TRUE(strcmp(config_get(&document, "escaped")->value.string_value, "a\tb\n\"c\"") == 0);
    config_document_destroy(&document);
}

static void assert_parse_error(const char *text, const char *message) {
    struct config_document document;
    struct config_parse_error error;
    ASSERT_TRUE(parse_text(text, &document, &error) == R_ERROR);
    ASSERT_TRUE(strstr(error.message, message) != nullptr);
}

static void test_malformed_input(void) {
    assert_parse_error("hooks:\nvalue: no child\n", "map must contain");
    assert_parse_error("hooks:\n  one: 1\n   two: 2\n", "indentation");
    assert_parse_error("hooks:\n\tone: 1\n", "tabs");
    assert_parse_error("missing colon\n", "expected ':'");
    assert_parse_error("bad.key: value\n", "invalid configuration key");
    assert_parse_error("value: 1\nvalue: 2\n", "duplicate or conflicting");
    assert_parse_error("value: \"unterminated\n", "unterminated");
    assert_parse_error("value: \"bad\\q\"\n", "unsupported escape");
    assert_parse_error("value: \"done\" trailing\n", "unexpected text");
    assert_parse_error("value: [one, two]\n", "unsupported YAML-style");
    assert_parse_error("value: |\n", "unsupported YAML-style");
}

int main(void) {
    test_flattened_paths_and_flexible_indentation();
    test_scalars_comments_utf8_and_escapes();
    test_malformed_input();

    if(failures > 0) {
        fprintf(stderr, "%d config parser test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("config parser tests passed\n");
    return EXIT_SUCCESS;
}
