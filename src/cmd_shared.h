#ifndef SHIORI_CMD_SHARED_H
#define SHIORI_CMD_SHARED_H

#include <stdio.h>

#include "todo.h"
#include "todo_list.h"

int add_markdown_item(int argc, char *argv[], const char *filename, const char *prefix, const char *heading);
FILE *open_notes_file(const char *mode);
FILE *open_base_dir_file(const char *filename, const char *mode);
int read_todos(const char *filename, struct todo_list *list);
int parse_date_arg(const char *value, time_t *result);

#endif
