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

char *trim(char *str);
int create_file_if_not_exists(char* fname);
int get_base_dir_file_path(char *filename, char *buffer, size_t buffer_size);

#endif