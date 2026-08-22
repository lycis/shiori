#ifndef SHIORI_PLATFORM_H
#define SHIORI_PLATFORM_H

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>

#define F_OK 0
#endif

#ifdef __linux__
#include <langinfo.h>
#include <locale.h>
#include <unistd.h>
#endif

#include <stdio.h>

#include "cli.h"

char *get_path_separator();
int file_access_utf8(const char *path, int mode);
int file_open_utf8(FILE **file, const char *path, const char *mode);
int file_remove_utf8(const char *path);
int file_rename_utf8(const char *old_path, const char *new_path);
int terminal_enable_utf8(void);
int get_user_home(char *buffer, size_t size);
char *get_current_path(char *buffer, size_t size);
int set_environment_variable(const char *name, const char *value);
bool environment_variable_nonempty(const char *name);
bool stream_is_terminal(FILE *stream);
int run_script_basedir(const char *path);

enum key_type {
    KEY_CHARACTER,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_TAB,
    KEY_ESCAPE,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_UP,
    KEY_DOWN
};
struct key_event {
    enum key_type type;
    unsigned int codepoint;
};

int terminal_enter_interactive_mode(void);
void terminal_leave_interactive_mode(void);
void terminal_render_input(
    const char *prompt,
    const char *buffer,
    size_t cursor,
    const struct completion_result *completions
);
void terminal_finish_input_line(void);
void terminal_cancel_input_line(void);
int terminal_read_key(struct key_event *event);

#endif
