#ifndef SHIORI_COMMON_H
#define SHIORI_COMMON_H

#define R_OK 0
#define R_ERROR 1

#define DEFAULT_BUFFER_SIZE 2048

#define SHIORI_EXIT_SUCCESS 0
#define SHIORI_EXIT_CONFIG_ERROR 1
#define SHIORI_EXIT_NO_COMMAND 2
#define SHIORI_EXIT_COMMAND_FAILED 3
#define SHIORI_EXIT_UTF8_FAILED 4
#define SHIORI_EXIT_INTERRUPTED 130

#define CONFIG_FILE_NAME ".shiori"
#define CONFIG_VERSION 1
#define APP_NAME "shiori"
#define APP_VERSION "0.3.0"

#define TODO_FORMAT_VERSION 1
#define NOTES_FORMAT_VERSION 1

#define TODO_FILE "TODOS.md"
#define NOTES_FILE "NOTES.md"

#include <stddef.h>
#include <time.h>

char *trim(char *str);
[[nodiscard]] int create_file_if_not_exists(char *fname);
[[nodiscard]] int get_base_dir_file_path(const char *filename, char *buffer, size_t buffer_size);
[[nodiscard]] int build_text_from_args(int argc, char *argv[], char *buffer, size_t buffer_size);
[[nodiscard]] int build_daily_heading(char *buffer, size_t size, time_t date);
bool str_ends_with(const char *str, const char *suffix);
bool dates_equal(time_t a, time_t b);
[[nodiscard]] int format_date(time_t date, char *buffer, size_t buffer_size);
int compare_dates(time_t a, time_t b);
[[nodiscard]] int parse_int(const char *text, int *result);
[[nodiscard]] int join_array(int argc, char *argv[], char *buffer, size_t buffer_size);
[[nodiscard]] int
calculate_array_growth(size_t current_capacity, size_t element_size, size_t *new_capacity, size_t *allocation_size);
bool ends_with_whitespace(const char *text);

#endif
