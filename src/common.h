#ifndef _SHIORI_COMMON_H
#define _SHIORI_COMMON_H

#define R_OK 0
#define R_ERROR 1

#define DEFAULT_BUFFER_SIZE 2048

#define SHIORI_EXIT_SUCCESS 0
#define SHIORI_EXIT_CONFIG_ERROR 1
#define SHIORI_EXIT_NO_COMMAND 2
#define SHIORI_EXIT_COMMAND_FAILED 3

#define CONFIG_FILE_NAME ".shiori"
#define CONFIG_VERSION 1
#define APP_NAME "shiori"
#define APP_VERSION "0.1.0"

#define TODO_FILE "TODOS.md"

#include <time.h>
#include <stdbool.h>

char *trim(char *str);
int create_file_if_not_exists(char* fname);
int get_base_dir_file_path(char *filename, char *buffer, size_t buffer_size);
int build_text_from_args(int argc, char *argv[], char *buffer, size_t buffer_size);
int build_daily_heading(char* buffer, size_t size, time_t date);
bool str_ends_with(const char *str, const char *suffix);
bool dates_equal(time_t a, time_t b);
int format_date(time_t date, char *buffer, size_t buffer_size);

#endif