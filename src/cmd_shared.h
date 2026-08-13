#ifndef _SHIORI_CMD_SHARED_H
#define _SHIORI_CMD_SHARED_H

#include <stdio.h>

int add_markdown_item(int argc, char *argv[], const char *filename, const char *prefix, const char *heading);
FILE* open_notes_file(const char* mode);
FILE* open_base_dir_file(const char *filename, const char* mode);

#endif