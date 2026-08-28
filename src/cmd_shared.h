#ifndef SHIORI_CMD_SHARED_H
#define SHIORI_CMD_SHARED_H

#include <stdio.h>

#include "todo_list.h"

// clang-format off: long line is ok here... better than the format
[[nodiscard]] int add_markdown_item(int argc, char *argv[], const char *filename, const char *prefix, const char *heading);
// clang-format on
[[nodiscard]] FILE *open_notes_file(const char *mode);
[[nodiscard]] FILE *open_base_dir_file(const char *filename, const char *mode);
[[nodiscard]] int read_todos(const char *filename, struct todo_list *list);
[[nodiscard]] int parse_date_arg(const char *value, time_t *result);
void print_todo_topic_suffix(const struct todo *item);

#endif
